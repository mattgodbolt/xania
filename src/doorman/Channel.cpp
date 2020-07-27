#include "Channel.hpp"

#include "Xania.hpp"
#include "doorman.hpp"

#include <arpa/inet.h>
#include <arpa/telnet.h>
#include <csignal>
#include <cstring>
#include <netdb.h>

using namespace std::literals;

std::array<Channel, MaxChannels> channels;

extern Xania mud; // TODO
static constexpr auto MaxIncomingDataBufferSize = 2048u;

static void SendCom(const Fd &fd, byte a, byte b) {
    byte buf[] = {IAC, a, b};
    fd.write(buf);
}

static void SendOpt(const Fd &fd, byte a) {
    byte buf[] = {IAC, SB, a, TELQUAL_SEND, IAC, SE};
    fd.write(buf);
}

static void AsyncHostnameLookup(sockaddr_in address, int outFd) {
    /*
     * Firstly, and quite importantly, close all the descriptors we
     * don't need, so we don't keep open sockets past their useful
     * life.  This was a bug which meant ppl with firewalled auth
     * ports could keep other ppls connections open (nonresponding)
     */
    for (auto &channel : channels)
        channel.close();

    // Bail out on a PIPE - this means our parent has crashed
    signal(SIGPIPE, [](int) {
        log_out("hostname lookup process exiting on sigpipe");
        exit(1);
    });

    auto *ent = gethostbyaddr((char *)&address.sin_addr, sizeof(address.sin_addr), AF_INET);

    std::string hostName = ent ? ent->h_name : inet_ntoa(address.sin_addr);
    auto nBytes = hostName.size() + 1;
    auto header_written = write(outFd, &nBytes, sizeof(nBytes));
    if (header_written != sizeof(nBytes) || write(outFd, hostName.c_str(), nBytes) != static_cast<ssize_t>(nBytes)) {
        log_out("ID: Unable to write to doorman - perhaps it crashed (%d, %s)", errno, strerror(errno));
    } else {
        // Log the source hostname but mask it for privacy.
        log_out("ID: Looked up %s", get_masked_hostname(hostName).c_str());
    }
}

static bool SupportsAnsi(const char *src) {
    static const char *terms[] = {"xterm",      "xterm-colour", "xterm-color", "ansi",
                                  "xterm-ansi", "vt100",        "vt102",       "vt220"};
    size_t i;
    for (i = 0; i < (sizeof(terms) / sizeof(terms[0])); ++i)
        if (!strcasecmp(src, terms[i]))
            return true;
    return false;
}

void Channel::closeConnection() {
    // Log the source IP but masked for privacy.
    log_out("[%d] Closing connection to %s", id_, get_masked_hostname(hostname_).c_str());
    close();
}

void Channel::set_echo(bool echo) {
    SendCom(fd_, echo ? WONT : WILL, TELOPT_ECHO);
    echoing_ = echo;
}

void Channel::on_reconnect_attempt() {
    if (!fd_.is_open())
        return;
    if (!sent_reconnect_message_) {
        send_to_client("Attempting to reconnect to Xania"sv);
        sent_reconnect_message_ = true;
    }
    send_to_client(".", 1);
}

void Channel::close() {
    fd_.close();
    if (host_lookup_fds_[0]) {
        ::close(host_lookup_fds_[0]);
    }
    if (host_lookup_fds_[1]) {
        ::close(host_lookup_fds_[1]);
    }
    if (host_lookup_pid_) {
        if (::kill(host_lookup_pid_, 9) < 0) {
            log_out("Couldn't kill child process (ident process has already exitted)");
        }
        host_lookup_pid_ = 0;
    }
}

void Channel::newConnection(Fd fd, sockaddr_in address) {
    // One day I will be a constructor...
    this->fd_ = std::move(fd);
    hostname_ = inet_ntoa(address.sin_addr);
    port_ = ntohs(address.sin_port);
    netaddr_ = ntohl(address.sin_addr.s_addr);
    buffer_.clear();
    width_ = 80;
    height_ = 24;
    ansi_ = got_term_ = false;
    echoing_ = true;
    sent_reconnect_message_ = false;
    connected_ = false;
    auth_char_name_.clear();

    log_out("[%d] Incoming connection from %s on fd %d", id_, get_masked_hostname(hostname_).c_str(), fd_.number());

    /* Send all options out */
    sendTelopts();

    /* Fire off the query to the ident server */
    pid_t forkRet = -1;
    if (pipe(host_lookup_fds_) == -1) {
        log_out("Pipe failed - next message (unable to fork) is due to this");
        host_lookup_fds_[0] = host_lookup_fds_[1] = 0;
    } else {
        log_out("Pipes %d<->%d", host_lookup_fds_[0], host_lookup_fds_[1]);
        forkRet = fork();
    }
    switch (forkRet) {
    case 0: {
        // Child process - go and do the lookup
        // First close the child's copy of the parent's fd
        ::close(host_lookup_fds_[0]);
        log_out("ID: doing lookup");
        // TODO this is a terrible mess. the child wants to close all FDs...
        // we probably don't need to store the child's FD at all.
        auto child_fd = host_lookup_fds_[1];
        host_lookup_fds_[1] = 0;
        AsyncHostnameLookup(address, child_fd);
        ::close(child_fd);
        log_out("ID: exiting");
        exit(0);
    } break;
    case -1:
        // Error - unable to fork()
        log_out("Unable to fork() an IdentD lookup process");
        // Close up the pipe
        if (host_lookup_fds_[0])
            ::close(host_lookup_fds_[0]);
        if (host_lookup_fds_[1])
            ::close(host_lookup_fds_[1]);
        host_lookup_fds_[0] = host_lookup_fds_[1] = 0;
        break;
    default:
        // First close the parent's copy of the child's fd
        ::close(host_lookup_fds_[1]);
        host_lookup_fds_[1] = 0;
        host_lookup_pid_ = forkRet;
        log_out("Forked hostname process has PID %d on %d", forkRet, host_lookup_fds_[0]);
        break;
    }

    // Tell them if the mud is down
    if (!mud.connected()) {
        auto msg = "Xania is down at the moment - you will be connected as soon "
                   "as it is up again.\n\r"sv;
        send_to_client(msg.data(), msg.size());
    }

    sendConnectPacket();
}

bool Channel::incomingData(const byte *incoming_data, int nBytes) {
    /* Check for buffer overflow */
    if ((nBytes + buffer_.size()) > MaxIncomingDataBufferSize) {
        fd_.write(">>> Too much incoming data at once - PUT A LID ON IT!!\n\r"sv);
        buffer_.clear();
        return false;
    }
    /* Add the data into the buffer */
    buffer_.insert(buffer_.end(), incoming_data, incoming_data + nBytes);

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
                        SendOpt(fd_, *(ptr + 2));
                    }
                    break;
                case TELOPT_NAWS:
                    // Do nothing for NAWS
                    break;
                default: SendCom(fd_, DONT, *(ptr + 2)); break;
                }
                memmove(ptr, ptr + 3, nLeft - 3);
                nLeft -= 3;
                break;
            case WONT:
                SendCom(fd_, DONT, *(ptr + 2));
                memmove(ptr, ptr + 3, nLeft - 3);
                nLeft -= 3;
                break;
            case DO:
                if (ptr[2] == TELOPT_ECHO && echoing_ == false) {
                    SendCom(fd_, WILL, TELOPT_ECHO);
                    memmove(ptr, ptr + 3, nLeft - 3);
                    nLeft -= 3;
                } else {
                    SendCom(fd_, WONT, *(ptr + 2));
                    memmove(ptr, ptr + 3, nLeft - 3);
                    nLeft -= 3;
                }
                break;
            case DONT:
                SendCom(fd_, WONT, *(ptr + 2));
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
                    if (SupportsAnsi((const char *)ptr + 4))
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
            mud.on_client_message(
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
    return true;
}

void Channel::incomingHostnameInfo() {
    log_out("[%d on %d] Incoming IdentD info", id_, host_lookup_fds_[0]);
    // A response from the ident server
    size_t numBytes;
    auto ret = read(host_lookup_fds_[0], &numBytes, sizeof(numBytes));
    if (ret <= 0) {
        log_out("[%d] IdentD pipe died on read (read returned %ld)", id_, ret);
        ::close(host_lookup_fds_[0]);
        host_lookup_fds_[0] = host_lookup_fds_[1] = 0;
        host_lookup_pid_ = 0;
        return;
    }
    if (numBytes >= 1024) {
        log_out("Ident responded with >1k - possible spam attack");
        numBytes = 1023;
    }
    char host_buffer[1024];
    auto numRead = read(host_lookup_fds_[0], host_buffer, numBytes);
    ::close(host_lookup_fds_[0]);
    host_lookup_fds_[0] = host_lookup_fds_[1] = 0;
    host_lookup_pid_ = 0;

    if (numRead != static_cast<ssize_t>(numBytes)) {
        log_out("[%d] Partial read %ld!=%ld", id_, numRead, numBytes);
        return;
    }
    hostname_ = std::string_view(host_buffer, numBytes);
    // Log the source IP but mask it for privacy.
    log_out("[%d] %s", id_, get_masked_hostname(hostname_).c_str());
    sendInfoPacket();
}

void Channel::sendInfoPacket() const {
    char buffer[4096];
    auto data = new (buffer) InfoData;
    Packet p{PACKET_INFO, static_cast<uint32_t>(sizeof(InfoData) + hostname_.size() + 1), id_, {}};

    data->port = port_;
    data->netaddr = netaddr_;
    data->ansi = ansi_;
    strcpy(data->data, hostname_.c_str());
    mud.SendToMUD(p, buffer);
}

void Channel::sendConnectPacket() {
    if (!fd_.is_open())
        return;
    if (connected_) {
        log_out("[%d] Attempt to send connect packet for already-connected channel", id_);
        return;
    }
    if (!mud.connected())
        return;

    // Have we already been authenticated?
    if (!auth_char_name_.empty()) {
        connected_ = true;
        mud.SendToMUD({PACKET_RECONNECT, static_cast<uint32_t>(auth_char_name_.size() + 1), id_, {}},
                      auth_char_name_.c_str());
        sendInfoPacket();
        log_out("[%d] Sent reconnect packet to MUD for %s", id_, auth_char_name_.c_str());
    } else {
        connected_ = true;
        mud.SendToMUD({PACKET_CONNECT, 0, id_, {}});
        sendInfoPacket();
        log_out("[%d] Sent connect packet to MUD", id_);
    }
    sent_reconnect_message_ = false;
}

void Channel::sendTelopts() const {
    SendCom(fd_, DO, TELOPT_TTYPE);
    SendCom(fd_, DO, TELOPT_NAWS);
    SendCom(fd_, WONT, TELOPT_ECHO);
}

void Channel::on_lookup_died(int status) {
    // Did the process exit abnormally?
    if (!(WIFEXITED(status))) {
        log_out("Zombie reaper: process on channel %d died abnormally", id_);
        ::close(host_lookup_fds_[0]);
        ::close(host_lookup_fds_[1]);
        host_lookup_pid_ = 0;
        host_lookup_fds_[0] = host_lookup_fds_[1] = 0;
    } else {
        if (host_lookup_fds_[0] || host_lookup_fds_[1]) {
            log_out("Zombie reaper: A process died, and didn't necessarily "
                    "close up after itself");
        }
    }
}
void Channel::set_id(int32_t id) { id_ = id; }