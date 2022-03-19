#include "Channel.hpp"

#include "Doorman.hpp"
#include "Xania.hpp"
#include "common/mask_hostname.hpp"

#include <arpa/inet.h>
#include <cstring>
#include <netdb.h>

using namespace std::literals;

static constexpr auto MaxIncomingDataBufferSize = 2048u;

std::string Channel::lookup(const sockaddr_in &address) const {
    char lookup_buffer[8192];
    hostent ent{};
    hostent *res = &ent;
    int host_errno{};
    auto as_ip = std::string(inet_ntoa(address.sin_addr));
    auto gh_errno = gethostbyaddr_r(&address.sin_addr, sizeof(address.sin_addr), AF_INET, &ent, lookup_buffer,
                                    sizeof(lookup_buffer), &res, &host_errno);
    if (gh_errno != 0) {
        char error_buffer[1024];
        log_.error("Error looking up address {} : {}", as_ip, strerror_r(gh_errno, error_buffer, sizeof(error_buffer)));
        return as_ip;
    }
    if (host_errno != 0) {
        log_.warn("DNS resolution failure looking up address {} : {}", as_ip, hstrerror(host_errno));
        return as_ip;
    }
    return res->h_name;
}

void Channel::set_echo(bool echo) { telnet_.set_echo(echo); }

void Channel::on_reconnect_attempt() {
    if (!fd_.is_open())
        return;
    if (!sent_reconnect_message_) {
        send_to_client("Attempting to reconnect to Xania"sv);
        sent_reconnect_message_ = true;
    }
    send_to_client("."sv);
}

void Channel::close() {
    // Log the source IP but masked for privacy.
    log_.info("Closing connection to {}", get_masked_hostname(hostname_));
    if (connected_)
        mud_.send_close_msg(*this);
    fd_.close();
    doorman_.schedule_remove(*this);
}

Channel::Channel(Doorman &doorman, Xania &xania, IdAllocator::Reservation id, Fd fd, const sockaddr_in &address)
    : log_(logger_for(fmt::format("Channel.{}", id->id()))), doorman_(doorman), mud_(xania), id_(std::move(id)),
      fd_(std::move(fd)), port_(ntohs(address.sin_port)), netaddr_(ntohl(address.sin_addr.s_addr)) {
    hostname_ = lookup(address);
    log_.info("Incoming connection from {} on fd {}", get_masked_hostname(hostname_), fd_.number());

    // Send all options out
    telnet_.send_telopts();

    // Tell them if the mud is down
    if (!mud_.connected()) {
        send_to_client("Xania is down at the moment - you will be connected as soon as it is up again.\n\r"sv);
    }

    send_connect_packet();
}

void Channel::on_data(gsl::span<const byte> incoming_data) {
    auto new_buf_size = incoming_data.size() + telnet_.buffered_data_size();
    if (new_buf_size >= MaxIncomingDataBufferSize) {
        log_.warn("Client sent too much data ({}>{})", new_buf_size, MaxIncomingDataBufferSize);
        try {
            fd_.write(">>> Too much incoming data at once - PUT A LID ON IT!!\n\r"sv);
        } catch (std::runtime_error &re) {
            log_.info("Ignoring error telling client to shut up");
        }
        close();
        return;
    }
    telnet_.add_data(incoming_data);
}

void Channel::send_info_packet() const {
    size_t payload_size = sizeof(InfoData) + hostname_.size() + 1;
    if (payload_size > PacketMaxPayloadSize) {
        log_.error("Dropping MUD info message as payload size was too big ({} > {})", payload_size,
                   PacketMaxPayloadSize);
        return;
    }
    char buffer[PacketMaxPayloadSize];
    auto data = new (buffer) InfoData;
    Packet p{PACKET_INFO, static_cast<uint32_t>(payload_size), id_->id(), {}};

    data->port = port_;
    data->netaddr = netaddr_;
    data->ansi = telnet_.supports_ansi();
    strcpy(data->data, hostname_.c_str());
    mud_.send_to_mud(p, buffer);
}

void Channel::send_connect_packet() {
    if (!fd_.is_open())
        return;
    if (connected_) {
        log_.warn("Attempt to send connect packet for already-connected channel");
        return;
    }
    if (!mud_.connected())
        return;

    // Have we already been authenticated? Note that the packet size == char name length so it's not null terminated.
    if (!auth_char_name_.empty()) {
        connected_ = true;
        mud_.send_to_mud({PACKET_RECONNECT, static_cast<uint32_t>(auth_char_name_.size()), id_->id(), {}},
                         auth_char_name_.c_str());
        send_info_packet();
        log_.debug("Sent reconnect packet to MUD for {}", auth_char_name_);
    } else {
        connected_ = true;
        mud_.send_connect(*this);
        send_info_packet();
        log_.debug("Sent connect packet to MUD");
    }
    sent_reconnect_message_ = false;
}

int Channel::set_fds(fd_set &input_fds, fd_set &exception_fds) noexcept {
    if (!fd_.is_open())
        return 0;
    FD_SET(fd_.number(), &exception_fds);
    FD_SET(fd_.number(), &input_fds);
    return fd_.number();
}

void Channel::on_data_available() {
    // This constant doesn't control how large the data sent to the MUD is; it's more a "fairness"
    // indicator: per poll() we won't read more than from each socket. This prevents one chatty client
    // from starving out the others. If a client gets particularly far ahead, then their TCP window
    // will close up, and we want to provide them with that backpressure.
    constexpr auto PerSocketReadSize = 1024;
    byte buf[PerSocketReadSize];
    try {
        auto num_read = fd_.try_read_some(gsl::span<byte>(buf));
        if (num_read == 0) {
            log_.info("Remote end closed connection");
            close();
        } else {
            on_data(gsl::span(buf, num_read));
        }
    } catch (const std::runtime_error &re) {
        log_.info("Error reading from connection, closing: {}", re.what());
        close();
    }
}

void Channel::send_bytes(gsl::span<const byte> data) { fd_.write(data); }

void Channel::on_line(std::string_view line) { mud_.on_client_message(*this, line); }

void Channel::on_terminal_size(unsigned int width, unsigned int height) {
    // TODO(#71): we lie and say we wordwrap at this length... perhaps we should?
    log_.debug("NAWS = {}x{}", width, height);
}

void Channel::on_terminal_type(std::string_view type, bool ansi_supported) {
    log_.debug("Terminal {}, supports ansi {}", type, ansi_supported);
    send_info_packet();
}

void Channel::on_auth(std::string_view name) {
    auth_char_name_ = name;
    log_.info("Successfully authorized {}", auth_char_name_);
}

void Channel::check_fds(const fd_set &input_fds, const fd_set &exception_fds) {
    if (!fd_.is_open())
        return;
    if (FD_ISSET(fd_.number(), &exception_fds))
        close();
    else if (FD_ISSET(fd_.number(), &input_fds))
        on_data_available();
}
