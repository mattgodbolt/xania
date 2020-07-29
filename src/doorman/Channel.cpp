#include "Channel.hpp"

#include "Doorman.hpp"
#include "Misc.hpp"
#include "Xania.hpp"

#include <arpa/inet.h>
#include <arpa/telnet.h>
#include <csignal>
#include <cstring>
#include <netdb.h>

using namespace std::literals;

static constexpr auto MaxIncomingDataBufferSize = 2048u;

static void send_com(const Fd &fd, byte a, byte b) {
    byte buf[] = {IAC, a, b};
    fd.write(buf);
}

static void send_opt(const Fd &fd, byte a) {
    byte buf[] = {IAC, SB, a, TELQUAL_SEND, IAC, SE};
    fd.write(buf);
}

void Channel::async_lookup_process(sockaddr_in address, Fd reply_fd) {
    // Firstly, and quite importantly, close all the descriptors we don't need, so we don't keep open sockets past their
    // useful life.  This was a bug which meant ppl with firewalled auth ports could keep other ppls connections open
    // (nonresponding).
    doorman_.for_each_channel([](Channel &channel) { channel.close_silently(); });
    mud_.invalidate_from_lookup_process();

    // Bail out on a PIPE - this means our parent has crashed
    signal(SIGPIPE, [](int) {
        log_out("hostname lookup process exiting on sigpipe");
        exit(1);
    });

    auto *ent = gethostbyaddr((char *)&address.sin_addr, sizeof(address.sin_addr), AF_INET);

    std::string hostName = ent ? ent->h_name : inet_ntoa(address.sin_addr);
    size_t nBytes = hostName.size() + 1;
    try {
        reply_fd.write(nBytes);
        reply_fd.write(hostName.c_str(), nBytes);
    } catch (const std::runtime_error &re) {
        log_out("ID: Unable to write to doorman - perhaps it crashed: %s", re.what());
        return;
    }
    // Log the source hostname but mask it for privacy.
    log_out("ID: Looked up %s", get_masked_hostname(hostName).c_str());
}

static bool supports_ansi(const char *src) {
    static const char *terms[] = {"xterm",      "xterm-colour", "xterm-color", "ansi",
                                  "xterm-ansi", "vt100",        "vt102",       "vt220"};
    size_t i;
    for (i = 0; i < (sizeof(terms) / sizeof(terms[0])); ++i)
        if (!strcasecmp(src, terms[i]))
            return true;
    return false;
}

void Channel::set_echo(bool echo) {
    send_com(fd_, echo ? WONT : WILL, TELOPT_ECHO);
    echoing_ = echo;
}

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
    log_out("[%d] Closing connection to %s", id_, get_masked_hostname(hostname_).c_str());
    mud_.send_close_msg(*this);
    close_silently();
}

void Channel::close_silently() {
    fd_.close();
    host_lookup_fd_.close();
    if (host_lookup_pid_) {
        if (::kill(host_lookup_pid_, 9) < 0)
            log_out("Couldn't kill child process (lookup process has already exited)");
        host_lookup_pid_ = 0;
    }
    doorman_.schedule_remove(*this);
}

Channel::Channel(Doorman &doorman, Xania &xania, int32_t id, Fd fd, const sockaddr_in &address)
    : doorman_(doorman), mud_(xania), id_(id), fd_(std::move(fd)), hostname_(inet_ntoa(address.sin_addr)),
      port_(ntohs(address.sin_port)), netaddr_(ntohl(address.sin_addr.s_addr)) {
    log_out("[%d] Incoming connection from %s on fd %d", id_, get_masked_hostname(hostname_).c_str(), fd_.number());

    // TODO: consider what happens if this throws :/

    // Send all options out
    send_telopts();

    // Tell them if the mud is down
    if (!mud_.connected()) {
        send_to_client("Xania is down at the moment - you will be connected as soon as it is up again.\n\r"sv);
    }

    send_connect_packet();

    // Start the async hostname lookup process.
    int pipe_fds[2];
    if (pipe(pipe_fds) == -1) {
        log_out("Pipe failed!");
        return;
    }
    Fd parent_fd(pipe_fds[0]);
    Fd child_fd(pipe_fds[1]);
    log_out("Pipes %d<->%d", pipe_fds[0], pipe_fds[1]);
    auto forkRet = fork();
    switch (forkRet) {
    case 0: {
        // Child process - go and do the lookup
        log_out("ID: doing lookup");
        async_lookup_process(address, std::move(child_fd));
        log_out("ID: exiting");
        exit(0);
    } break;
    case -1:
        // Error - unable to fork()
        log_out("Unable to fork() a lookup process");
        break;
    default:
        host_lookup_fd_ = std::move(parent_fd);
        host_lookup_pid_ = forkRet;
        log_out("Forked hostname process has PID %d on %d", forkRet, host_lookup_fd_.number());
        break;
    }
}

void Channel::on_data(gsl::span<const byte> incoming_data) {
    /* Check for buffer overflow */
    if ((incoming_data.size() + buffer_.size()) > MaxIncomingDataBufferSize) {
        fd_.write(">>> Too much incoming data at once - PUT A LID ON IT!!\n\r"sv);
        buffer_.clear();
        close();
        return;
    }
    /* Add the data into the buffer */
    buffer_.insert(buffer_.end(), incoming_data.begin(), incoming_data.end());

    /* Scan through and remove telnet commands */
    // TODO gsl::span ftw
    size_t nRealBytes = 0;
    size_t nLeft = buffer_.size();
    for (byte *ptr = buffer_.data(); nLeft;) {
        /* telnet command? */
        if (*ptr == IAC) {
            if (nLeft < 3) /* Is it too small to fit a whole command in? */
                break;
            switch (*(ptr + 1)) {
            case WILL:
                switch (*(ptr + 2)) {
                case TELOPT_TTYPE:
                    // Check to see if we've already got the info
                    // Whoiseye's bloody DEC-TERM spams us multiply
                    // otherwise...
                    if (!got_term_) {
                        got_term_ = true;
                        send_opt(fd_, *(ptr + 2));
                    }
                    break;
                case TELOPT_NAWS:
                    // Do nothing for NAWS
                    break;
                default: send_com(fd_, DONT, *(ptr + 2)); break;
                }
                memmove(ptr, ptr + 3, nLeft - 3);
                nLeft -= 3;
                break;
            case WONT:
                send_com(fd_, DONT, *(ptr + 2));
                memmove(ptr, ptr + 3, nLeft - 3);
                nLeft -= 3;
                break;
            case DO:
                if (ptr[2] == TELOPT_ECHO && echoing_ == false) {
                    send_com(fd_, WILL, TELOPT_ECHO);
                    memmove(ptr, ptr + 3, nLeft - 3);
                    nLeft -= 3;
                } else {
                    send_com(fd_, WONT, *(ptr + 2));
                    memmove(ptr, ptr + 3, nLeft - 3);
                    nLeft -= 3;
                }
                break;
            case DONT:
                send_com(fd_, WONT, *(ptr + 2));
                memmove(ptr, ptr + 3, nLeft - 3);
                nLeft -= 3;
                break;
            case SB: {
                char sbType;
                byte *eob, *p;
                /* Enough room for an SB command? */
                if (nLeft < 6) {
                    nLeft = 0;
                    break;
                }
                sbType = *(ptr + 2);
                //          sbWhat = *(ptr + 3);
                /* Now read up to the IAC SE */
                eob = ptr + nLeft - 1;
                for (p = ptr + 4; p < eob; ++p)
                    if (*p == IAC && *(p + 1) == SE)
                        break;
                /* Found IAC SE? */
                if (p == eob) {
                    /* No: skip it all */
                    nLeft = 0;
                    break;
                }
                *p = '\0';
                /* Now to decide what to do with this new data */
                switch (sbType) {
                case TELOPT_TTYPE:
                    log_out("[%d] TTYPE = %s", id_, ptr + 4);
                    if (supports_ansi((const char *)ptr + 4))
                        ansi_ = true;
                    break;
                case TELOPT_NAWS:
                    width_ = ptr[4];
                    height_ = ptr[6];
                    log_out("[%d] NAWS = %dx%d", id_, width_, height_);
                    break;
                }

                /* Remember eob is 1 byte behind the end of buffer */
                memmove(ptr, p + 2, nLeft - ((p + 2) - ptr));
                nLeft -= ((p + 2) - ptr);

                break;
            }

            default:
                memmove(ptr, ptr + 2, nLeft - 2);
                nLeft -= 2;
                break;
            }

        } else {
            ptr++;
            nRealBytes++;
            nLeft--;
        }
    }
    /* Now to update the buffer stats */
    buffer_.resize(nRealBytes);

    /* Second pass - look for whole lines of text */
    nLeft = buffer_.size();
    byte *ptr;
    for (ptr = buffer_.data(); nLeft; ++ptr, --nLeft) {
        char c;
        switch (*ptr) {
        case '\n':
        case '\r':
            // TODO this is terribly ghettish. don't need to memmove, and all that. ugh. But write tests first...
            // then change...
            c = *ptr; // legacy<-
            /* Send it to the MUD */
            mud_.on_client_message(
                *this, std::string_view(reinterpret_cast<const char *>(buffer_.data()), ptr - buffer_.data()));
            /* Check for \n\r or \r\n */
            if (nLeft > 1 && (*(ptr + 1) == '\r' || *(ptr + 1) == '\n') && *(ptr + 1) != c) {
                memmove(buffer_.data(), ptr + 2, nLeft - 2);
                nLeft--;
            } else if (nLeft) {
                memmove(buffer_.data(), ptr + 1, nLeft - 1);
            } // else nLeft is zero, so we might as well avoid calling memmove(x,y,
            // 0)

            /* The +1 at the top of the loop resets ptr to buffer */
            ptr = buffer_.data() - 1;
            break;
        }
    }
    buffer_.resize(ptr - buffer_.data());
}

void Channel::on_host_info() {
    log_out("[%d on %d] Incoming lookup info", id_, host_lookup_fd_.number());
    // Move out the fd, so we know however we exit from this point it will be closed.
    auto response_fd = std::move(host_lookup_fd_);
    try {
        auto numBytes = response_fd.read_all<size_t>();
        char host_buffer[1024];
        if (numBytes > sizeof(host_buffer)) {
            log_out("Lookup responded with > %lu bytes - possible spam attack", sizeof(host_buffer));
            numBytes = sizeof(host_buffer);
        }
        response_fd.read_all(host_buffer, numBytes);
        hostname_ = std::string_view(host_buffer, numBytes);

    } catch (const std::runtime_error &re) {
        log_out("[%d] Lookup pipe died on read: %s", id_, re.what());
        host_lookup_pid_ = 0;
        return;
    }
    // Log the source IP but mask it for privacy.
    log_out("[%d] %s", id_, get_masked_hostname(hostname_).c_str());
    send_info_packet();
}

void Channel::send_info_packet() const {
    char buffer[4096];
    auto data = new (buffer) InfoData;
    Packet p{PACKET_INFO, static_cast<uint32_t>(sizeof(InfoData) + hostname_.size() + 1), id_, {}};

    data->port = port_;
    data->netaddr = netaddr_;
    data->ansi = ansi_;
    strcpy(data->data, hostname_.c_str());
    mud_.send_to_mud(p, buffer);
}

void Channel::send_connect_packet() {
    if (!fd_.is_open())
        return;
    if (connected_) {
        log_out("[%d] Attempt to send connect packet for already-connected channel", id_);
        return;
    }
    if (!mud_.connected())
        return;

    // Have we already been authenticated?
    if (!auth_char_name_.empty()) {
        connected_ = true;
        mud_.send_to_mud({PACKET_RECONNECT, static_cast<uint32_t>(auth_char_name_.size() + 1), id_, {}},
                         auth_char_name_.c_str());
        send_info_packet();
        log_out("[%d] Sent reconnect packet to MUD for %s", id_, auth_char_name_.c_str());
    } else {
        connected_ = true;
        mud_.send_to_mud({PACKET_CONNECT, 0, id_, {}});
        send_info_packet();
        log_out("[%d] Sent connect packet to MUD", id_);
    }
    sent_reconnect_message_ = false;
}

void Channel::send_telopts() const {
    send_com(fd_, DO, TELOPT_TTYPE);
    send_com(fd_, DO, TELOPT_NAWS);
    send_com(fd_, WONT, TELOPT_ECHO);
}

void Channel::on_lookup_died(int status) {
    // Did the process exit abnormally?
    if (!(WIFEXITED(status))) {
        log_out("Zombie reaper: process on channel %d died abnormally", id_);
    } else {
        if (host_lookup_fd_.is_open()) {
            log_out("Zombie reaper: A process died, and didn't necessarily "
                    "close up after itself");
        }
    }
    host_lookup_fd_.close();
    host_lookup_pid_ = 0;
}

int Channel::set_fds(fd_set &input_fds, fd_set &exception_fds) noexcept {
    if (!fd_.is_open())
        return 0;
    FD_SET(fd_.number(), &exception_fds);
    FD_SET(fd_.number(), &input_fds);
    if (host_lookup_fd_.is_open()) {
        FD_SET(host_lookup_fd_.number(), &input_fds);
        return std::max(host_lookup_fd_.number(), fd_.number());
    }
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
        auto num_read = fd_.try_read_some(buf);
        if (num_read == 0) {
            log_out("[%d] Remote end closed connection", id_);
            close();
        } else {
            on_data(gsl::span(buf, num_read));
        }
    } catch (const std::runtime_error &re) {
        log_out("[%d] Error reading from connection, closing: %s", id_, re.what());
        close();
    }
}
