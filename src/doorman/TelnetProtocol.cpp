#include "TelnetProtocol.hpp"

#include <arpa/telnet.h>

#include <cstring>

static bool supports_ansi(const char *src) {
    static const char *terms[] = {"xterm",      "xterm-colour", "xterm-color", "ansi",
                                  "xterm-ansi", "vt100",        "vt102",       "vt220"};
    size_t i;
    for (i = 0; i < (sizeof(terms) / sizeof(terms[0])); ++i)
        if (!strcasecmp(src, terms[i]))
            return true;
    return false;
}

void TelnetProtocol::send_com(byte a, byte b) {
    const byte buf[] = {IAC, a, b};
    handler_.send_bytes(buf);
}

void TelnetProtocol::send_opt(byte a) {
    const byte buf[] = {IAC, SB, a, TELQUAL_SEND, IAC, SE};
    handler_.send_bytes(buf);
}

void TelnetProtocol::add_data(gsl::span<const byte> data) {
    buffer_.insert(buffer_.end(), data.begin(), data.end());

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
                        send_opt(*(ptr + 2));
                    }
                    break;
                case TELOPT_NAWS:
                    // Do nothing for NAWS
                    break;
                default: send_com(DONT, *(ptr + 2)); break;
                }
                memmove(ptr, ptr + 3, nLeft - 3);
                nLeft -= 3;
                break;
            case WONT:
                send_com(DONT, *(ptr + 2));
                memmove(ptr, ptr + 3, nLeft - 3);
                nLeft -= 3;
                break;
            case DO:
                if (ptr[2] == TELOPT_ECHO && echoing_ == false) {
                    send_com(WILL, TELOPT_ECHO);
                    memmove(ptr, ptr + 3, nLeft - 3);
                    nLeft -= 3;
                } else {
                    send_com(WONT, *(ptr + 2));
                    memmove(ptr, ptr + 3, nLeft - 3);
                    nLeft -= 3;
                }
                break;
            case DONT:
                send_com(WONT, *(ptr + 2));
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
                case TELOPT_TTYPE: {
                    std::string term((const char *)ptr + 4);
                    ansi_ = ::supports_ansi(term.c_str());
                    handler_.on_terminal_type(term, ansi_);
                    break;
                }
                case TELOPT_NAWS:
                    width_ = (ptr[3] << 8u) | ptr[4];
                    height_ = (ptr[5] << 8u) | ptr[6];
                    handler_.on_terminal_size(width_, height_);
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
            handler_.on_line(std::string_view(reinterpret_cast<const char *>(buffer_.data()), ptr - buffer_.data()));
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

void TelnetProtocol::set_echo(bool echo) {
    send_com(echo ? WONT : WILL, TELOPT_ECHO);
    echoing_ = echo;
}

void TelnetProtocol::send_telopts() {
    send_com(DO, TELOPT_TTYPE);
    send_com(DO, TELOPT_NAWS);
    send_com(WONT, TELOPT_ECHO);
}
