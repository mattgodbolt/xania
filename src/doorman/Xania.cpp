#include "Xania.hpp"

#include "Channel.hpp"
#include "doorman.hpp"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <unistd.h>

using namespace std::literals;

static constexpr auto time_between_retries = 10s;

namespace {
void *iovec_cast(const void *thing) { return const_cast<void *>(thing); }
}

void Xania::SendToMUD(const Packet &p, const void *payload) {
    if (!connected())
        return;
    const iovec vec[2] = {{iovec_cast(&p), sizeof(Packet)}, {iovec_cast(payload), p.nExtra}};
    if (::writev(fd_, vec, 2) != static_cast<ssize_t>(sizeof(Packet) + p.nExtra)) {
        perror("SendToMUD");
        exit(1);
    }
}

void Xania::try_connect() {
    last_connect_attempt_ = std::chrono::system_clock::now();

    sockaddr_un xaniaAddr{};
    xaniaAddr.sun_family = PF_UNIX;
    extern int port; // TODO
    snprintf(xaniaAddr.sun_path, sizeof(xaniaAddr.sun_path), XANIA_FILE, port,
             getenv("USER") ? getenv("USER") : "unknown");

    /*
     * Create a socket for connection to Xania itself
     */
    fd_ = socket(PF_UNIX, SOCK_STREAM, 0);
    if (fd_ < 0) {
        log_out("Unable to create a socket to connect to Xania with!");
        exit(1);
    }
    log_out("Descriptor %d is Xania", fd_);

    if (connect(fd_, (sockaddr *)&xaniaAddr, sizeof(xaniaAddr)) >= 0) {
        log_out("Connected to Xania; awaiting boot message");
        state_ = State::ConnectedAwaitingInit;
        wall("\n\rReconnected to Xania - Xania is still booting...\n\r");
    } else {
        log_out("Connection attempt to MUD failed.");
        for (auto &chan : channels)
            chan.on_reconnect_attempt();
        log_out("Closing descriptor %d (mud)", fd_);
        ::close(fd_);
        fd_ = -1;
    }
}

void Xania::poll() {
    switch (state_) {
    case State::Connected:
    case State::ConnectedAwaitingInit: break;
    case State::Disconnected:
        if (std::chrono::system_clock::now() - last_connect_attempt_ > time_between_retries) {
            try_connect();
        }
        break;
    }
}

void Xania::close() {
    ::close(fd_);
    fd_ = -1;
    state_ = State::Disconnected;

    wall("\n\rDoorman has lost connection to Xania.\n\r");

    for (auto &chan : channels)
        chan.mark_disconnected();
}

void Xania::process_mud_message() {
    Packet p;
    int nBytes;

    nBytes = read(fd_, (char *)&p, sizeof(Packet));

    if (nBytes != sizeof(Packet)) {
        log_out("doorman::Connection to MUD lost");
        close();
        return;
    }
    char payload[16384];
    if (p.nExtra) {
        if (p.nExtra > sizeof(payload)) {
            log_out("payload too big: %d!", p.nExtra);
            close();
            return;
        }
        int num_read = read(fd_, payload, p.nExtra);
        if (num_read != (int)p.nExtra) {
            log_out("doorman::Connection to MUD lost");
            close();
            return;
        }
    }
    auto &channel = channels[p.channel];

    switch (p.type) {
    case PACKET_MESSAGE:
        /* A message from the MUD */
        if (!channel.send_to_client(payload, p.nExtra)) {
            log_out("[%d] Received error %d (%s) on write - closing connection", p.channel, errno, strerror(errno));
            send_close_msg(channel);
            channel.closeConnection();
        }
        break;
    case PACKET_AUTHORIZED:
        /* A character has successfully logged in */
        channel.on_auth(std::string_view(payload, p.nExtra));
        log_out("[%d] Successfully authorized %s", p.channel, channels[p.channel].authed_name().c_str());
        break;
    case PACKET_DISCONNECT:
        /* Kill off a channel */
        channel.closeConnection();
        break;
    case PACKET_INIT:
        /* Xania has initialised */
        if (state_ != State::ConnectedAwaitingInit) {
            log_out("Got init packet when not expecting it!");
            close();
            return;
        }
        log_out("Xania has booted");
        state_ = State::Connected;
        // Now try to connect everyone who has been waiting
        for (auto &chan : channels)
            chan.sendConnectPacket();
        break;
    case PACKET_ECHO_ON:
        /* Xania wants text to be echoed by the client */
        channel.set_echo(true);
        break;
    case PACKET_ECHO_OFF:
        /* Xania wants text to not be echoed by the client */
        channel.set_echo(false);
        break;
    default: break;
    }
}

void Xania::send_close_msg(const Channel &channel) { SendToMUD({PACKET_DISCONNECT, 0, channel.id(), {}}); }

void Xania::on_client_message(const Channel &channel, std::string_view message) const {
    // There was a TODO to store messages in the original code if we weren't connected...probably unwise?
    if (!connected())
        return;
    Packet p{PACKET_MESSAGE, static_cast<uint32_t>(message.size() + 2), channel.id(), {}};
    static const auto crlf = "\n\r"sv;
    iovec vec[3] = {{iovec_cast(&p), sizeof(p)},
                    {iovec_cast(message.data()), message.size()},
                    {iovec_cast(crlf.data()), crlf.size()}};
    if (::writev(fd_, vec, 3) != static_cast<ssize_t>(sizeof(p) + message.size() + crlf.size())) {
        perror("SendToMUD");
        exit(1);
    }
}