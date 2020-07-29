#include "TelnetProtocol.hpp"

#include <arpa/telnet.h>

#include <algorithm>
#include <cstring>

static bool supports_ansi(const char *src) {
    static const char *terms[] = {"xterm",      "xterm-colour", "xterm-color", "ansi",
                                  "xterm-ansi", "vt100",        "vt102",       "vt220"};
    for (auto &term : terms)
        if (!strcasecmp(src, term))
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

    auto starting_point = 0ul;
    auto nLeft = buffer_.size();
    for (;;) {
        auto iac_it = std::find(std::next(buffer_.begin(), starting_point), buffer_.end(), IAC);
        if (iac_it == buffer_.end())
            break;
        // Next time we look for a command, start at this point. We're about to erase the IAC (if we parsed it).
        starting_point = std::distance(buffer_.begin(), iac_it);
        gsl::span<const byte> iac_command(&*iac_it, &*buffer_.end());
        auto num_consumed = on_command(iac_command);
        if (num_consumed == 0) {
            // If there wasn't enough data for the IAC command, then stop right now and do no further IAC processing: we
            // must await more data to interpret the command. However, process the rest of the buffer up to the IAC
            return; // TODO fix!
            //            nLeft = starting_point;
            //            break;
        }
        // Remove the command.
        buffer_.erase(std::next(buffer_.begin(), starting_point),
                      std::next(buffer_.begin(), starting_point) + num_consumed);
        nLeft = buffer_.size();
    }

    /* Second pass - look for whole lines of text */
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

static bool is_two_byte_command(byte b) {
    return b == NOP || b == DM || b == BREAK || b == IP || b == ABORT || b == AYT || b == EC || b == EL || b == GA
           || b == IAC || b < SE;
}

size_t TelnetProtocol::on_subcommand(gsl::span<const byte> command_sequence) {
    auto option_code = command_sequence[2];
    auto body = command_sequence.subspan(3);
    const std::array<byte, 2> iac_se = {IAC, SE};
    auto iac_se_it = std::search(body.begin(), body.end(), iac_se.begin(), iac_se.end());
    if (iac_se_it == body.end())
        return 0;
    body = body.subspan(0, std::distance(body.begin(), iac_se_it));
    switch (option_code) {
    case TELOPT_TTYPE:
        if (body.size() > 1) {
            std::string term(reinterpret_cast<const char *>(body.data() + 1), body.size() - 1);
            ansi_ = ::supports_ansi(term.c_str());
            handler_.on_terminal_type(term, ansi_);
        }
        break;

    case TELOPT_NAWS:
        if (body.size() >= 4) {
            width_ = (body[0] << 8u) | body[1];
            height_ = (body[2] << 8u) | body[3];
            handler_.on_terminal_size(width_, height_);
        }
        break;
    default: break;
    }
    return body.size() + 3 + 2;
}

size_t TelnetProtocol::on_command(gsl::span<const byte> command_sequence) {
    if (command_sequence.size() >= 2 && is_two_byte_command(command_sequence[1])) {
        // Handle (incorrectly) an escape (IAC, IAC) by swallowing up both IACs.
        return 2;
    }
    // If there's not enough space for a [WILL WONT DO DONT SE] [option]... return
    if (command_sequence.size() < 3)
        return 0;
    auto command = command_sequence[1];
    auto option_code = command_sequence[2];
    switch (command) {
    default: break;
    case WILL:
        switch (option_code) {
        case TELOPT_TTYPE:
            // Check to see if we've already got the info
            // Whoiseye's bloody DEC-TERM spams us multiply
            // otherwise...
            if (!got_term_) {
                got_term_ = true;
                send_opt(option_code);
            }
            break;
        case TELOPT_NAWS:
            // Do nothing for NAWS
            break;
        default: send_com(DONT, option_code); break;
        }
        return 3;
    case WONT: send_com(DONT, option_code); return 3;
    case DO:
        if (option_code == TELOPT_ECHO && echoing_ == false) {
            send_com(WILL, TELOPT_ECHO);
        } else {
            send_com(WONT, option_code);
        }
        return 3;

    case DONT: send_com(WONT, option_code); return 3;
    case SB: return on_subcommand(command_sequence);
    }
    throw std::runtime_error("Unexpected internal state");
}
