#include "Xania.hpp"

#include "Channel.hpp"
#include "Doorman.hpp"
#include "Misc.hpp"

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

void Xania::send_to_mud(const Packet &p, const void *payload) {
    if (!connected())
        return;
    if (p.nExtra > PACKET_MAX_PAYLOAD_SIZE) {
        log_.error("Dropping MUD packet of type {} as its payload is too long ({} > {})", p.type, p.nExtra,
                   PACKET_MAX_PAYLOAD_SIZE);
        return;
    }
    const iovec vec[2] = {{iovec_cast(&p), sizeof(Packet)}, {iovec_cast(payload), p.nExtra}};
    // TODO writev? rephrase this whole thing?
    if (::writev(fd_.number(), vec, 2) != static_cast<ssize_t>(sizeof(Packet) + p.nExtra)) {
        perror("send_to_mud");
        exit(1);
    }
}

void Xania::try_connect() {
    last_connect_attempt_ = std::chrono::system_clock::now();

    fd_ = Fd::socket(PF_UNIX, SOCK_STREAM, 0);
    log_.debug("Descriptor {} is Xania", fd_.number());

    try {
        sockaddr_un xaniaAddr{};
        xaniaAddr.sun_family = PF_UNIX;
        snprintf(xaniaAddr.sun_path, sizeof(xaniaAddr.sun_path), XANIA_FILE, doorman_.port(),
                 getenv("USER") ? getenv("USER") : "unknown");
        fd_.connect(xaniaAddr);
    } catch (const std::runtime_error &re) {
        log_.warn("Connection attempt to MUD failed: {}", re.what());
        doorman_.for_each_channel([&](Channel &chan) {
            try {
                chan.on_reconnect_attempt();
            } catch (const std::runtime_error &re) {
                log_.info("[{}]: Unable to send reconnect message: %{}", chan.id(), re.what());
            }
        });
        log_.debug("Closing descriptor {} (mud)", fd_.number());
        fd_.close();
        return;
    }

    log_.info("Connected to Xania; awaiting boot message");
    state_ = State::ConnectedAwaitingInit;
    doorman_.broadcast("\n\rReconnected to Xania - Xania is still booting...\n\r");
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
    fd_.close();
    state_ = State::Disconnected;

    doorman_.broadcast("\n\rDoorman has lost connection to Xania.\n\r");
    doorman_.for_each_channel([](Channel &chan) { chan.mark_disconnected(); });
}

void Xania::process_mud_message() {
    try {
        auto p = fd_.read_all<Packet>();
        std::string payload;
        if (p.nExtra) {
            if (p.nExtra > PACKET_MAX_PAYLOAD_SIZE) {
                log_.error("MUD payload too big: {}!", p.nExtra);
                close();
                return;
            }
            payload.resize(p.nExtra);
            fd_.read_all(payload.data(), p.nExtra);
        }

        // Only the init packet has no channel; so we don't try and look it up.
        if (p.type == PACKET_INIT) {
            // Xania has initialised
            if (state_ != State::ConnectedAwaitingInit) {
                log_.error("Got init packet when not expecting it!");
                close();
                return;
            }
            log_.info("Xania has booted");
            state_ = State::Connected;
            // Now try to connect everyone who has been waiting
            doorman_.for_each_channel([](Channel &chan) { chan.send_connect_packet(); });
            return;
        }

        // Any other message must be associated with a channel.
        auto channelPtr = doorman_.find_channel_by_id(p.channel);
        if (!channelPtr) {
            log_.error("Couldn't find channel ID {} when receiving MUD message!", p.channel);
            return;
        }
        auto &channel = *channelPtr;

        switch (p.type) {
        case PACKET_MESSAGE:
            /* A message from the MUD */
            try {
                channel.send_to_client(payload);
            } catch (const std::runtime_error &re) {
                log_.info("[{}] Received error '{}' on write - closing connection", p.channel, re.what());
                send_close_msg(channel);
                channel.close();
            }
            break;
        case PACKET_AUTHORIZED:
            /* A character has successfully logged in */
            channel.on_auth(payload);
            break;
        case PACKET_DISCONNECT:
            /* Kill off a channel */
            channel.close();
            break;
        case PACKET_ECHO_ON:
            /* Xania wants text to be echoed by the client */
            channel.set_echo(true);
            break;
        case PACKET_ECHO_OFF:
            /* Xania wants text to not be echoed by the client */
            channel.set_echo(false);
            break;
        case PACKET_INIT:
        default: break;
        }
    } catch (const std::runtime_error &re) {
        log_.warn("Connection to MUD lost: {}", re.what());
        close();
    }
}

void Xania::send_close_msg(const Channel &channel) { send_to_mud({PACKET_DISCONNECT, 0, channel.id(), {}}); }

void Xania::on_client_message(const Channel &channel, std::string_view message) const {
    // We drop messages here if there's nothing upstream. Probably for the best.
    if (!connected())
        return;
    Packet p{PACKET_MESSAGE, static_cast<uint32_t>(message.size() + 2), channel.id(), {}};
    static const auto crlf = "\n\r"sv;
    iovec vec[3] = {{iovec_cast(&p), sizeof(p)},
                    {iovec_cast(message.data()), message.size()},
                    {iovec_cast(crlf.data()), crlf.size()}};
    // rephrase?
    if (::writev(fd_.number(), vec, 3) != static_cast<ssize_t>(sizeof(p) + message.size() + crlf.size())) {
        perror("send_to_mud");
        exit(1);
    }
}

void Xania::invalidate_from_lookup_process() {
    fd_.close();
    state_ = State::Disconnected;
}

Xania::Xania(Doorman &doorman) : log_(logger_for("Xania")), doorman_(doorman) {}
