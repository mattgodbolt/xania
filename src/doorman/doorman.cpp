/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                    */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  doorman.c                                                            */
/*                                                                       */
/*************************************************************************/

/*
 * Doorman.c
 * Stand-alone 'doorman' for the MUD
 * Sits on port 9000, and relays connections through a named pipe
 * to the MUD itself, thus acting as a link between the MUD
 * and the player independant of the MUD state.
 * If the MUD crashes, or is rebooted, it should be able to maintain
 * connections, and reconnect automatically
 *
 * Terminal issues are resolved here now, which means ANSI-compliant terminals
 * should automatically enable colour, and word-wrapping is done by doorman
 * instead of the MUD, so if you have a wide display you can take advantage
 * of it.
 *
 */

#include "doorman.h"

#include <fmt/format.h>

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

#include <arpa/inet.h>
#include <arpa/telnet.h>
#include <cerrno>
#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

using namespace std::literals;

static constexpr auto MaxIncomingDataBufferSize = 2048u;
static constexpr auto CONNECTION_WAIT_TIME = 10u;
using byte = unsigned char;

bool IncomingData(int fd, const byte *incoming_data, int nBytes);
void AsyncHostnameLookup(struct sockaddr_in address, int outFd);

class Channel {
    int32_t id_{}; // the channel id
    int fd_{}; /* File descriptor of the channel */
    std::string hostname_;
    std::vector<byte> buffer_;
    uint16_t port_{};
    uint32_t netaddr_{};
    int width_{}, height_{}; /* Width and height of terminal */
    bool ansi_{}, got_term_{};
    bool echoing_{};
    bool sent_reconnect_message_{};
    bool connected_{}; /* Socket is ready to connect to Xania */
    pid_t host_lookup_pid_{}; /* PID of look-up process */
    int host_lookup_fds_[2]{}; /* FD of look-up processes pipe (read on 0) */
    std::string auth_char_name_; /* name of authorized character */

public:
    void newConnection(int fd, struct sockaddr_in address);
    void closeConnection();
    void sendInfoPacket() const;
    void sendConnectPacket();
    bool incomingData(const byte *incoming_data, int nBytes);
    void incomingHostnameInfo();
    void sendTelopts() const;

    void on_auth(std::string_view name) { auth_char_name_ = name; }
    [[nodiscard]] const std::string &authed_name() const { return auth_char_name_; }
    void on_reconnect_attempt();
    void mark_disconnected() { connected_ = false; }
    bool send_to_client(const void *data, size_t length) const {
        if (!fd_)
            return false;
        auto written = ::write(fd_, data, length);
        // TODO close fd_? remove it? if errored?
        return written == static_cast<ssize_t>(length);
    }

    [[nodiscard]] int highest_fd() const {
        if (host_lookup_pid_)
            return std::max(host_lookup_fds_[0], fd_);
        else
            return fd_;
    }
    // Intention here is to prevent direct access to the fd.
    [[nodiscard]] bool has_fd(int fd) const { return fd == fd_; }
    [[nodiscard]] bool is_closed() const { return fd_ == 0; }

    void set_echo(bool echo);
    void close();

    void set_ex_fds(fd_set &exIfds) {
        if (!fd_)
            return;
        FD_SET(fd_, &exIfds);
    }

    [[nodiscard]] bool is_hostname_fd(int fd) const { return fd_ && host_lookup_pid_ && host_lookup_fds_[0] == fd; }

    [[nodiscard]] bool has_lookup_pid(pid_t pid) const { return fd_ && host_lookup_pid_ == pid; }
    void on_lookup_died(int status);
    void set_id(int32_t id);
};

/* Global variables */
// todo: maybe eventually unordered_map<channel id, channel>
static constexpr auto MaxChannels = 64;
std::array<Channel, MaxChannels> channels;
int mudFd;
pid_t child;
int port = 9000;
bool connectedToXania = false, waitingForInit = false;
int listenSock;
time_t lastConnected;
fd_set ifds;
int debug = 0;

bool SendCom(int fd, byte a, byte b) {
    byte buf[4];
    buf[0] = IAC;
    buf[1] = a;
    buf[2] = b;
    return write(fd, buf, 3) == 3;
}

bool SendOpt(int fd, byte a) {
    byte buf[6];
    buf[0] = IAC;
    buf[1] = SB;
    buf[2] = a;
    buf[3] = TELQUAL_SEND;
    buf[4] = IAC;
    buf[5] = SE;
    return write(fd, buf, 6) == 6;
}

/* printf() to an fd */
__attribute__((format(printf, 2, 3))) bool fdprintf(int fd, const char *format, ...) {
    char buffer[4096];
    int len;

    va_list args;
    va_start(args, format);
    len = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    int numWritten = write(fd, buffer, len);
    return numWritten == len;
}

/* printf() to everyone */
__attribute__((format(printf, 1, 2))) void wall(const char *format, ...) {
    char buffer[4096];

    va_list args;
    va_start(args, format);
    int len = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    for (auto &chan : channels)
        chan.send_to_client(buffer, len);
}

/* log_out() replaces fprintf (stderr,...) */
__attribute__((format(printf, 1, 2))) void log_out(const char *format, ...) {
    char buffer[4096], *Time;
    time_t now;

    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    time(&now);
    Time = ctime(&now);
    Time[strlen(Time) - 1] = '\0'; // Kill the newline
    fprintf(stderr, "%d::%s %s\n", getpid(), Time, buffer);
}

unsigned long djb2_hash(std::string_view str) {
    unsigned long hash = 5381;
    for (auto c : str)
        hash = hash * 33 + c;
    return hash;
}

/**
 * Writes into hostbuf the supplied hostname, masked for privacy
 * and with a hashcode of the full hostname. This can be used by admins
 * to spot users coming from the same IP.
 */
std::string get_masked_hostname(std::string_view hostname) {
    return fmt::format("{}*** [#{}]", hostname.substr(0, 6), djb2_hash(hostname));
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
    if (!fd_)
        return;
    auto recon = "Attempting to reconnect to Xania"sv;
    if (sent_reconnect_message_) {
        if (!send_to_client(recon.data(), recon.size()))
            log_out("Unable to write to channel fd %d", fd_);
        sent_reconnect_message_ = true;
    }
    send_to_client(".", 1);
}

void Channel::close() {
    if (fd_) {
        ::close(fd_);
        FD_CLR(fd_, &ifds);
    }
    if (host_lookup_fds_[0]) {
        ::close(host_lookup_fds_[0]);
        FD_CLR(host_lookup_fds_[0], &ifds);
    }
    if (host_lookup_fds_[1]) {
        ::close(host_lookup_fds_[1]);
        FD_CLR(host_lookup_fds_[1], &ifds);
    }
    if (host_lookup_pid_) {
        if (kill(host_lookup_pid_, 9) < 0) {
            log_out("Couldn't kill child process (ident process has already exitted)");
        }
        host_lookup_pid_ = 0;
    }
    fd_ = 0;
}

void Channel::newConnection(int fd, struct sockaddr_in address) {
    // One day I will be a constructor...
    this->fd_ = fd;
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

    log_out("[%d] Incoming connection from %s on fd %d", id_, get_masked_hostname(hostname_).c_str(), fd);

    /* Send all options out */
    sendTelopts();

    /* Fire off the query to the ident server */
    pid_t forkRet = -1;
    if (pipe(host_lookup_fds_) == -1) {
        log_out("Pipe failed - next message (unable to fork) is due to this");
        host_lookup_fds_[0] = host_lookup_fds_[1] = 0;
    } else {
        forkRet = fork();
    }
    switch (forkRet) {
    case 0:
        // Child process - go and do the lookup
        // First close the child's copy of the parent's fd
        ::close(host_lookup_fds_[0]);
        log_out("ID: on %d", host_lookup_fds_[1]);
        host_lookup_fds_[0] = 0;
        AsyncHostnameLookup(address, host_lookup_fds_[1]);
        ::close(host_lookup_fds_[1]);
        log_out("ID: exiting");
        exit(0);
        break;
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
        FD_SET(host_lookup_fds_[0], &ifds);
        log_out("Forked hostname process has PID %d on %d", forkRet, host_lookup_fds_[0]);
        break;
    }

    FD_SET(fd, &ifds);

    // Tell them if the mud is down
    if (!connectedToXania)
        fdprintf(fd, "Xania is down at the moment - you will be connected as soon "
                     "as it is up again.\n\r");

    sendConnectPacket();
}

/* Create a new connection */
void NewConnection(int fd, sockaddr_in address) {
    auto channelIter = std::find_if(begin(channels), end(channels), [](const Channel &c) { return c.is_closed(); });
    if (channelIter == end(channels)) {
        fdprintf(fd, "Xania is out of channels!\n\rTry again soon\n\r");
        close(fd);
        log_out("Rejected connection - out of channels");
        return;
    }

    channelIter->newConnection(fd, address);
}

int HighestFd() {
    int hFd = 0;
    for (auto &chan : channels)
        hFd = std::max(hFd, chan.highest_fd());
    return hFd;
}

int32_t FindChannelId(int fd) {
    if (fd == 0) {
        log_out("Panic!  fd==0 in FindChannel!  Bailing out with -1");
        return -1;
    }
    if (auto it = std::find_if(begin(channels), end(channels), [&](const Channel &chan) { return chan.has_fd(fd); });
        it != end(channels))
        return static_cast<int32_t>(std::distance(begin(channels), it));
    return -1;
}

Channel *FindChannel(int fd) {
    if (auto channelId = FindChannelId(fd); channelId >= 0)
        return &channels[channelId];
    return nullptr;
}

void CloseConnection(int fd) {
    auto channelPtr = FindChannel(fd);
    if (!channelPtr) {
        log_out("Erk - unable to find channel for fd %d", fd);
        return;
    }
    channelPtr->closeConnection();
}

/*********************************/

bool SupportsAnsi(const char *src) {
    static const char *terms[] = {"xterm",      "xterm-colour", "xterm-color", "ansi",
                                  "xterm-ansi", "vt100",        "vt102",       "vt220"};
    size_t i;
    for (i = 0; i < (sizeof(terms) / sizeof(terms[0])); ++i)
        if (!strcasecmp(src, terms[i]))
            return true;
    return false;
}

void Channel::incomingHostnameInfo() {
    FD_CLR(host_lookup_fds_[0], &ifds);
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

bool IncomingData(int fd, const byte *incoming_data, int nBytes) {
    if (auto *channel = FindChannel(fd))
        return channel->incomingData(incoming_data, nBytes);
    log_out("Oh dear - I got data on fd %d, but no channel!", fd);
    return false;
}

bool Channel::incomingData(const byte *incoming_data, int nBytes) {
    /* Check for buffer overflow */
    if ((nBytes + buffer_.size()) > MaxIncomingDataBufferSize) {
        const char *ovf = ">>> Too much incoming data at once - PUT A LID ON IT!!\n\r";
        if (write(fd_, ovf, strlen(ovf)) != (ssize_t)strlen(ovf)) {
            // don't care if this fails..
        }
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
            c = *ptr;
            // TODO this is terribly ghettish. don't need to memmove, and all that. ugh. But write tests first...
            // then change...
            *ptr = '\0';
            /* Send it to the MUD */
            if (connectedToXania && connected_) {
                Packet p;
                p.type = PACKET_MESSAGE;
                p.channel = id_;
                p.nExtra = strlen((const char *)buffer_.data()) + 2;
                if (write(mudFd, &p, sizeof(p)) != sizeof(p))
                    return false;
                if (write(mudFd, buffer_.data(), p.nExtra - 2) != p.nExtra - 2)
                    return false;
                if (write(mudFd, "\n\r", 2) != 2)
                    return false;
            } else {
                // XXX NEED TO STORE DATA HERE
            }
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

/*********************************/

void AsyncHostnameLookup(struct sockaddr_in address, int outFd) {
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
    log_out("ooh %ld", nBytes);
}

/*********************************/

void SendToMUD(const Packet &p, const void *payload = nullptr) {
    if (connectedToXania && mudFd != -1) {
        if (write(mudFd, &p, sizeof(Packet)) != sizeof(Packet)) {
            perror("SendToMUD");
            exit(1);
        }
        if (p.nExtra)
            if (write(mudFd, payload, p.nExtra) != p.nExtra) {
                perror("SendToMUD");
                exit(1);
            }
    }
}

/*
 * Aborts a connection - closes the socket
 * and tells the MUD that the channel has closed
 */
void AbortConnection(int fd) {
    SendToMUD({PACKET_DISCONNECT, 0, FindChannelId(fd), {}});
    CloseConnection(fd);
}

void Channel::sendInfoPacket() const {
    char buffer[4096];
    auto data = new (buffer) InfoData;
    Packet p{PACKET_INFO, static_cast<uint32_t>(sizeof(InfoData) + hostname_.size() + 1), id_, {}};

    data->port = port_;
    data->netaddr = netaddr_;
    data->ansi = ansi_;
    strcpy(data->data, hostname_.c_str());
    SendToMUD(p, buffer);
}

void Channel::sendConnectPacket() {
    if (!fd_) return;
    if (connected_) {
        log_out("[%d] Attempt to send connect packet for already-connected channel", id_);
    } else if (connectedToXania && mudFd != -1) {
        // Have we already been authenticated?
        if (!auth_char_name_.empty()) {
            connected_ = true;
            SendToMUD({PACKET_RECONNECT, static_cast<uint32_t>(auth_char_name_.size() + 1), id_, {}},
                      auth_char_name_.c_str());
            sendInfoPacket();
            log_out("[%d] Sent reconnect packet to MUD for %s", id_, auth_char_name_.c_str());
        } else {
            connected_ = true;
            SendToMUD({PACKET_CONNECT, 0, id_, {}});
            sendInfoPacket();
            log_out("[%d] Sent connect packet to MUD", id_);
        }
        sent_reconnect_message_ = false;
    }
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
        FD_CLR(host_lookup_fds_[0], &ifds);
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

void MudHasCrashed(int sig) {
    (void)sig;
    close(mudFd);
    FD_CLR(mudFd, &ifds);
    mudFd = -1;
    connectedToXania = false;
    waitingForInit = false;

    wall("\n\rDoorman has lost connection to Xania.\n\r");

    for (auto &chan : channels)
        chan.mark_disconnected();
}

/*
 * Deal with new connections from outside
 */
void ProcessNewConnection() {
    sockaddr_in incoming{};
    socklen_t len = sizeof(incoming);
    int newFD;

    newFD = accept(listenSock, (struct sockaddr *)&incoming, &len);
    if (newFD == -1) {
        log_out("Unable to accept new connection!");
        perror("accept");
        return;
    }
    NewConnection(newFD, incoming);
}

/*
 * Attempt connection to the MUD
 */
void TryToConnectToXania() {
    sockaddr_un xaniaAddr{};

    lastConnected = time(nullptr);

    memset(&xaniaAddr, 0, sizeof(xaniaAddr));
    xaniaAddr.sun_family = PF_UNIX;
    snprintf(xaniaAddr.sun_path, sizeof(xaniaAddr.sun_path), XANIA_FILE, port,
             getenv("USER") ? getenv("USER") : "unknown");

    /*
     * Create a socket for connection to Xania itself
     */
    mudFd = socket(PF_UNIX, SOCK_STREAM, 0);
    if (mudFd < 0) {
        log_out("Unable to create a socket to connect to Xania with!");
        exit(1);
    }
    log_out("Descriptor %d is Xania", mudFd);

    if (connect(mudFd, (struct sockaddr *)&xaniaAddr, sizeof(xaniaAddr)) >= 0) {
        log_out("Connected to Xania");
        waitingForInit = true;
        wall("\n\rReconnected to Xania - Xania is still booting...\n\r");
        FD_SET(mudFd, &ifds);
    } else {
        log_out("Connection attempt to MUD failed.");
        for (auto &chan : channels)
            chan.on_reconnect_attempt();
        log_out("Closing descriptor %d (mud)", mudFd);
        close(mudFd);
        mudFd = -1;
    }
}

/*
 * Deal with a request from Xania
 */
void ProcessMUDMessage(int fd) {
    Packet p;
    int nBytes;

    nBytes = read(fd, (char *)&p, sizeof(Packet));

    if (nBytes != sizeof(Packet)) {
        log_out("doorman::Connection to MUD lost");
        MudHasCrashed(0);
    } else {
        char payload[16384];
        if (p.nExtra) {
            if (p.nExtra > sizeof(payload)) {
                log_out("payload too big: %d!", p.nExtra);
                MudHasCrashed(0);
                return;
            }
            int num_read = read(fd, payload, p.nExtra);
            if (num_read != (int)p.nExtra) {
                log_out("doorman::Connection to MUD lost");
                MudHasCrashed(0);
                return;
            }
        }
        auto &channel = channels[p.channel];

        switch (p.type) {
        case PACKET_MESSAGE:
            /* A message from the MUD */
            if (!channel.send_to_client(payload, p.nExtra)) {
                log_out("[%d] Received error %d (%s) on write - closing connection", p.channel, errno, strerror(errno));
                AbortConnection(fd);
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
            waitingForInit = false;
            connectedToXania = true;
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
}

/*
 * The main loop
 */
void ExecuteServerLoop() {
    struct timeval timeOut = {CONNECTION_WAIT_TIME, 0};
    int i;
    int nFDs, highestClientFd;
    fd_set tempIfds, exIfds;
    int maxFd;

    /*
     * Firstly, if not already connected to Xania, attempt connection
     */
    if (!connectedToXania && !waitingForInit) {
        if ((time(nullptr) - lastConnected) >= CONNECTION_WAIT_TIME) {
            TryToConnectToXania();
        }
    }

    /*
     * Select on a copy of the input fds
     * Also find the highest set FD
     */
    maxFd = std::max(listenSock, mudFd);
    highestClientFd = HighestFd();
    maxFd = std::max(maxFd, highestClientFd);

    tempIfds = ifds;
    // Set up exIfds only for normal sockets and xania
    FD_ZERO(&exIfds);
    if (mudFd > 0)
        FD_SET(mudFd, &exIfds);
    for (auto &channel : channels)
        channel.set_ex_fds(exIfds);

    nFDs = select(maxFd + 1, &tempIfds, nullptr, &exIfds, &timeOut);
    if (nFDs == -1 && errno != EINTR) {
        log_out("Unable to select()!");
        perror("select");
        exit(1);
    }
    /* Anything happened in the last 10 seconds? */
    if (nFDs <= 0)
        return;
    /* Has the MUD crashed out? */
    if (mudFd != -1 && FD_ISSET(mudFd, &exIfds)) {
        MudHasCrashed(0);
        return; /* Prevent falling into packet handling */
    }
    for (i = 0; i <= maxFd; ++i) {
        /* Kick out the freaky folks */
        if (FD_ISSET(i, &exIfds)) {
            AbortConnection(i);
        } else if (FD_ISSET(i, &tempIfds)) { // Pending incoming data
            /* Is it the listening connection? */
            if (i == listenSock) {
                ProcessNewConnection();
            } else if (i == mudFd) {
                ProcessMUDMessage(i);
            } else {
                /* It's a message from a user or ident - dispatch it */
                if (auto it = std::find_if(begin(channels), end(channels),
                                           [&](const Channel &chan) { return chan.is_hostname_fd(i); });
                    it != end(channels)) {
                    it->incomingHostnameInfo();
                } else {
                    int nBytes;
                    byte buf[256];
                    do {
                        nBytes = read(i, buf, sizeof(buf));
                        if (nBytes <= 0) {
                            int32_t channelId = FindChannelId(i);
                            SendToMUD({PACKET_DISCONNECT, 0, channelId, {}});
                            if (nBytes < 0) {
                                log_out("[%d] Received error %d (%s) on read - closing connection", channelId, errno,
                                        strerror(errno));
                            }
                            AbortConnection(i);
                        } else {
                            if (nBytes > 0)
                                IncomingData(i, buf, nBytes);
                        }
                        // See how many more bytes we can read
                        nBytes = 0; // XXX Need to look this ioctl up
                    } while (nBytes > 0);
                }
            }
        }
    }
}

void CheckForDeadLookups() {
    pid_t waiter;
    int status;

    do {
        waiter = waitpid(-1, &status, WNOHANG);
        if (waiter == -1) {
            break; // Usually 'no child processes'
        }
        if (waiter) {
            // Clear the zombie status
            waitpid(waiter, nullptr, 0);
            log_out("process %d died", waiter);
            // Find the corresponding channel, if still present
            if (auto chanIt = std::find_if(begin(channels), end(channels),
                                           [&](const Channel &chan) { return chan.has_lookup_pid(waiter); });
                chanIt != end(channels)) {
                chanIt->on_lookup_died(status);
            }
        }
    } while (waiter);
}

// Our luvverly options go here :
struct option OurOptions[] = {
    {"port", 1, nullptr, 'p'}, {"debug", 0, &debug, 1}, {"help", 0, nullptr, 'h'}, {nullptr, 0, nullptr, 0}};

void usage(void) {
    fprintf(stderr, "Usage: doorman [-h | --help] [-d | --debug] [-p | --port "
                    "port] [port]\n\r");
}

// Here we go:

int main(int argc, char *argv[]) {
    struct protoent *tcpProto = getprotobyname("tcp");
    int yup = 1;
    struct linger linger = {1, 2}; // linger seconds
    struct sockaddr_in sin;

    /*
     * Parse any arguments
     */
    port = 9000;
    for (;;) {
        int optType;
        int index = 0;

        optType = getopt_long(argc, argv, "dp:h", OurOptions, &index);
        if (optType == -1)
            break;
        switch (optType) {
        case 'p':
            port = atoi(optarg);
            if (port <= 0) {
                fprintf(stderr, "Invalid port '%s'\n\r", optarg);
                usage();
                exit(1);
            }
            break;
        case 'd': debug = 1; break;
        case 'h':
            usage();
            exit(0);
            break;
        }
    }

    if (optind == (argc - 1)) {
        port = atoi(argv[optind]);
        if (port <= 0) {
            fprintf(stderr, "Invalid port '%s'\n\r", argv[optind]);
            usage();
            exit(1);
        }
    } else if (optind < (argc - 1)) {
        usage();
        exit(1);
    }

    /*
     * Initialise the channel array
     */
    for (int32_t i = 0; i < static_cast<int32_t>(channels.size()); ++i)
        channels[i].set_id(i);

    /*
     * Bit of 'Hello Mum'
     */
    log_out("Doorman version 0.2 starting up");
    log_out("Attempting to bind to port %d", port);

    /*
     * Turn on core dumping under debug
     */
    if (debug) {
        struct rlimit coreLimit = {0, 16 * 1024 * 1024};
        int ret;
        log_out("Debugging enabled - core limit set to 16Mb");
        ret = setrlimit(RLIMIT_CORE, &coreLimit);
        if (ret < 0) {
            log_out("Unable to set limit - %d ('%s')", errno, strerror(errno));
        }
    }

    /*
     * Create a tcp socket, bind and listen on it
     */
    if (tcpProto == nullptr) {
        log_out("Unable to get TCP number!");
        exit(1);
    }
    listenSock = socket(PF_INET, SOCK_STREAM, tcpProto->p_proto);
    if (listenSock < 0) {
        log_out("Unable to create a socket!");
        exit(1);
    }

    /*
     * Prevent crashing on SIGPIPE if the MUD goes down, or if a connection
     * goes funny
     */
    signal(SIGPIPE, SIG_IGN);

    /*
     * Set up a few options
     */
    if (setsockopt(listenSock, SOL_SOCKET, SO_REUSEADDR, &yup, sizeof(yup)) < 0) {
        log_out("Unable to listen!");
        exit(1);
    }
    if (setsockopt(listenSock, SOL_SOCKET, SO_LINGER, &linger, sizeof(linger)) < 0) {
        log_out("Unable to listen!");
        exit(1);
    }

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = PF_INET;
    sin.sin_port = htons(port);
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(listenSock, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
        log_out("Unable to bind socket!");
        exit(1);
    }
    if (listen(listenSock, 4) < 0) {
        log_out("Unable to listen!");
        exit(1);
    }

    log_out("Bound to socket succesfully.");

    /*
     * Start the ball rolling
     */
    FD_ZERO(&ifds);
    FD_SET(listenSock, &ifds);
    lastConnected = 0;
    mudFd = -1;

    log_out("Doorman is ready to rock on port %d", port);

    /*
     * Loop forever!
     */
    for (;;) {
        // Do all the nitty-gritties
        ExecuteServerLoop();
        // Ensure we don't leave zombie processes hanging around
        CheckForDeadLookups();
    }
}
