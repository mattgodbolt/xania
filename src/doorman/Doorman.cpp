#include "Doorman.hpp"
#include "Channel.hpp"
#include "SeasocksToLogger.hpp"
#include "Xania.hpp"
#include "doorman_protocol.h"

#include <algorithm>
#include <string_view>
#include <utility>
#include <vector>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <netinet/in.h>

using namespace std::literals;
using namespace fmt::literals;

namespace {

class WebSocketChannel : public ChannelBase {
    Logger log_;
    IdAllocator::Reservation id_;
    seasocks::WebSocket &socket_;
    Xania &mud_;
    bool connected_ = false;
    std::string hostname_;
    std::string auth_char_name_;

    void send_info_packet() {
        // TODO duplication
        size_t payload_size = sizeof(InfoData) + hostname_.size() + 1;
        if (payload_size > PACKET_MAX_PAYLOAD_SIZE) {
            log_.error("Dropping MUD info message as payload size was too big ({} > {})", payload_size,
                       PACKET_MAX_PAYLOAD_SIZE);
            return;
        }
        char buffer[PACKET_MAX_PAYLOAD_SIZE];
        auto data = new (buffer) InfoData;
        Packet p{PACKET_INFO, static_cast<uint32_t>(payload_size), id_->id(), {}};

        data->port = htons(socket_.getRemoteAddress().sin_port);
        data->netaddr = htonl(socket_.getRemoteAddress().sin_addr.s_addr);
        data->ansi = true;
        strcpy(data->data, hostname_.c_str());
        mud_.send_to_mud(p, buffer);
    }

public:
    WebSocketChannel(IdAllocator::Reservation id, seasocks::WebSocket *socket, Xania &mud)
        : log_(logger_for("ws.{}"_format(id->id()))), id_(std::move(id)), socket_(*socket), mud_(mud) {

        // TODO so much duplication.
        if (!mud_.connected()) {
            send_to_client("Xania is down at the moment - you will be connected as soon as it is up again.\n\r"sv);
        }

        send_connect_packet();
        hostname_ = "TODO"; // lookups!
    }

    [[nodiscard]] const IdAllocator::Reservation &reservation() const override { return id_; }
    void send_to_client(std::string_view message) override { socket_.send(std::string(message)); }
    void set_echo(bool echo) override { (void)echo; }
    void close() override { socket_.close(); }
    void on_auth(std::string_view name) override { auth_char_name_ = name; }
    void mark_disconnected() noexcept override { connected_ = false; }
    void on_reconnect_attempt() override {}
    void send_connect_packet() override {
        if (connected_) {
            log_.warn("Attempt to send connect packet for already-connected channel");
            return;
        }
        if (!mud_.connected())
            return;

        // Have we already been authenticated?
        if (!auth_char_name_.empty()) {
            connected_ = true;
            mud_.send_to_mud({PACKET_RECONNECT, static_cast<uint32_t>(auth_char_name_.size() + 1), id_->id(), {}},
                             auth_char_name_.c_str());
            send_info_packet();
            log_.debug("Sent reconnect packet to MUD for {}", auth_char_name_);
        } else {
            connected_ = true;
            mud_.send_connect(*this);
            send_info_packet();
            log_.debug("Sent connect packet to MUD");
        }
        //        sent_reconnect_message_ = false;
    }
    void on_data(std::string_view data) { mud_.on_client_message(*this, data); }
};

}

class Doorman::MudWebsocketHandler : public seasocks::WebSocket::Handler {
    Doorman &doorman_;

    std::unordered_map<seasocks::WebSocket *, WebSocketChannel *> socket_to_channel_;

public:
    explicit MudWebsocketHandler(Doorman &doorman) : doorman_(doorman) {}

    void onConnect(seasocks::WebSocket *connection) override {
        auto id = doorman_.id_allocator_.reserve();
        // TODO: check we're not over the number of channels....
        auto insertRec =
            doorman_.channels_.emplace(id->id(), std::make_unique<WebSocketChannel>(id, connection, doorman_.mud_));
        socket_to_channel_.emplace(connection, static_cast<WebSocketChannel *>(insertRec.first->second.get()));
    }
    void onData(seasocks::WebSocket *connection, const char *string) override {
        auto &ws_channel = *socket_to_channel_.at(connection);
        ws_channel.on_data(string);
    }
    void onDisconnect(seasocks::WebSocket *connection) override {
        // TODO prove this gets called even if close() is called by the MUD
        auto &ws_channel = *socket_to_channel_.at(connection);
        socket_to_channel_.erase(connection);
        doorman_.schedule_remove(ws_channel);
    }
};

Doorman::Doorman(int port)
    : log_(logger_for("Doorman")), port_(port), mud_(*this), web_server_(std::make_shared<SeasocksToLogger>()) {
    log_.info("Attempting to bind to port {}", port);
    listenSock_ = Fd::socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

    sockaddr_in sin{};
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = PF_INET;
    sin.sin_port = htons(port);
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    listenSock_.setsockopt(SOL_SOCKET, SO_REUSEADDR, static_cast<int>(1))
        .setsockopt(SOL_SOCKET, SO_LINGER, linger{true, 2})
        .bind(sin)
        .listen(4);

    log_.info("Doorman is ready to accept telnet connections on port {}", port);

    auto web_port = port + 1;
    web_server_.setStaticPath("../html");
    web_server_.addWebSocketHandler("/mud", std::make_shared<MudWebsocketHandler>(*this));
    web_server_.startListening(web_port);
    log_.info("Doorman is ready to accept web connections on port {}", web_port);
}

void Doorman::poll() {
    mud_.poll();
    socket_poll();
    remove_dead_channels();
}

void Doorman::remove_dead_channels() {
    // As channels can get deleted while we're iterating over the master list of channels, we protect ourselves from the
    // "delete this" problem by deferring their actual removal until we know that no calls to the Channel are on the
    // call stack.
    for (auto id : channels_to_remove_) {
        log_.debug("Removed channel {} from master channel list", id);
        channels_.erase(id);
    }
    channels_to_remove_.clear();
}

void Doorman::socket_poll() {
    fd_set input_fds, exception_fds;
    FD_ZERO(&input_fds);
    FD_ZERO(&exception_fds);
    FD_SET(listenSock_.number(), &input_fds);
    FD_SET(web_server_.fd(), &input_fds);
    int max_fd = std::max(listenSock_.number(), web_server_.fd());
    if (mud_.fd_ok()) {
        FD_SET(mud_.fd().number(), &input_fds);
        FD_SET(mud_.fd().number(), &exception_fds);
        max_fd = std::max(max_fd, mud_.fd().number());
    }
    for_each_channel([&](ChannelBase &channel) {
        // OK HEINOUS HACK ALERT FOR NOW TODO NOT THIS AT ALL
        auto *ch = dynamic_cast<Channel *>(&channel);
        if (ch)
            max_fd = std::max(ch->set_fds(input_fds, exception_fds), max_fd);
    });

    timeval timeOut = {1, 0}; // Wakes up once a second to do housekeeping
    int nFDs = select(max_fd + 1, &input_fds, nullptr, &exception_fds, &timeOut);
    if (nFDs == -1 && errno != EINTR) {
        log_.error("Unable to select()!");
        perror("select");
        exit(1);
    }
    if (nFDs <= 0)
        return;

    if (mud_.fd_ok()) {
        // Mud handling.
        if (FD_ISSET(mud_.fd().number(), &exception_fds)) {
            log_.warn("Got exception from MUD file descriptor");
            mud_.close();
        } else if (FD_ISSET(mud_.fd().number(), &input_fds))
            mud_.process_mud_message();
    }

    if (FD_ISSET(listenSock_.number(), &input_fds))
        accept_new_connection();

    if (FD_ISSET(web_server_.fd(), &input_fds))
        web_server_.poll(0);

    for_each_channel([&](ChannelBase &channel) {
        // OK HEINOUS HACK ALERT FOR NOW TODO NOT THIS AT ALL
        auto *ch = dynamic_cast<Channel *>(&channel);
        if (ch)
            ch->check_fds(input_fds, exception_fds);
    });
}

void Doorman::accept_new_connection() {
    sockaddr_in incoming{};
    socklen_t len = sizeof(incoming);
    try {
        auto newFd = listenSock_.accept(reinterpret_cast<sockaddr *>(&incoming), &len);

        if (channels_.size() == MaxChannels) {
            newFd.write("Xania is out of channels!\n\rTry again soon\n\r"sv);
            log_.warn("Rejected connection - out of channels");
            return;
        }

        auto id = id_allocator_.reserve();
        auto insert_rec =
            channels_.emplace(id->id(), std::make_unique<Channel>(*this, mud_, id, std::move(newFd), incoming));
        if (!insert_rec.second) {
            // This should be impossible! If this happens we'll silently close the connection.
            log_.error("Reused channel ID {}!", id->id());
        }
    } catch (const std::runtime_error &re) {
        log_.warn("Unable to accept new connection: {}", re.what());
    }
}

ChannelBase *Doorman::find_channel_by_id(int32_t channel_id) {
    if (auto it = channels_.find(channel_id); it != channels_.end())
        return it->second.get();
    return nullptr;
}

void Doorman::broadcast(std::string_view message) {
    for_each_channel([&](ChannelBase &chan) {
        try {
            chan.send_to_client(message);
        } catch (const std::runtime_error &re) {
            log_.debug("Error while broadcasting to {}: {}", chan.reservation()->id(), re.what());
            chan.close();
        }
    });
}
