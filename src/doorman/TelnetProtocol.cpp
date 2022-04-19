#include "TelnetProtocol.hpp"

#include <arpa/telnet.h>

#include "string_utils.hpp"
#include <algorithm>
#include <cstring>
#include <vector>

static bool supports_ansi(std::string_view detected_term) {
    static const std::vector<std::string_view> ansi_terms{"xterm", "mudlet", "ansi",      "vt100",
                                                          "vt102", "vt220",  "terminator"};
    for (auto &ansi_term : ansi_terms)
        if (matches_start(ansi_term, detected_term))
            return true;
    return false;
}

// For single byte commands like GA (go ahead)
void TelnetProtocol::send_com(byte a) {
    const byte buf[] = {IAC, a};
    handler_.send_bytes(buf);
}

void TelnetProtocol::send_com(byte a, byte b) {
    const byte buf[] = {IAC, a, b};
    handler_.send_bytes(buf);
}

void TelnetProtocol::send_opt(byte a) {
    const byte buf[] = {IAC, SB, a, TELQUAL_SEND, IAC, SE};
    handler_.send_bytes(buf);
}

gsl::span<const byte> TelnetProtocol::interpret_iacs() {
    auto starting_point = 0ul;
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
            // must await more data to interpret the command. However, process the rest of the buffer up to the IAC.
            return gsl::span<const byte>(buffer_).first(starting_point);
        }
        // Remove the command.
        buffer_.erase(std::next(buffer_.begin(), starting_point),
                      std::next(buffer_.begin(), starting_point + num_consumed));
    }
    return buffer_;
}

void TelnetProtocol::add_data(gsl::span<const byte> data) {
    buffer_.insert(buffer_.end(), data.begin(), data.end());

    auto span_to_scan = interpret_iacs();
    if (span_to_scan.empty())
        return;

    auto as_string_view = std::string_view(reinterpret_cast<const char *>(span_to_scan.data()), span_to_scan.size());
    auto remaining_data = handle_whole_lines(as_string_view);

    const auto num_consumed = as_string_view.size() - remaining_data.size();
    buffer_.erase(buffer_.begin(), std::next(buffer_.begin(), num_consumed));
}

std::string_view TelnetProtocol::handle_whole_lines(std::string_view data) {
    while (!data.empty()) {
        // Handle a \n\r or \r\n sequence; even if it crossed a buffer boundary.
        if ((previous_separator_ == '\n' && data.front() == '\r')
            || (previous_separator_ == '\r' && data.front() == '\n')) {
            data.remove_prefix(1);
            previous_separator_ = 0;
            continue;
        }

        auto cr_or_lf = data.find_first_of("\n\r");
        if (cr_or_lf == std::string_view::npos)
            break;

        previous_separator_ = data[cr_or_lf];
        handler_.on_line(data.substr(0, cr_or_lf));
        data.remove_prefix(cr_or_lf + 1);
    }
    return data;
}

// This is counter-intuitive, but it means that when terminal echo is undesirable (e.g. password input)
// send a IAC WILL ECHO to the client. This tells the client that the mud takes over the responsibility for
// echoing any inputs so that the client should not do it. And vice versa.
void TelnetProtocol::set_echo(bool client_should_echo) {
    send_com(client_should_echo ? WONT : WILL, TELOPT_ECHO);
    client_should_echo_ = client_should_echo;
}

void TelnetProtocol::send_telopts() {
    send_com(DO, TELOPT_TTYPE);
    send_com(DO, TELOPT_NAWS);
    send_com(WONT, TELOPT_ECHO);
}

void TelnetProtocol::send_go_ahead() { send_com(GA); }

static bool is_two_byte_command(byte b) {
    return b == NOP || b == DM || b == BREAK || b == IP || b == ABORT || b == AYT || b == EC || b == EL || b == GA
           || b == IAC || b < SE;
}

size_t TelnetProtocol::on_subcommand(gsl::span<const byte> command_sequence) {
    auto option_code = command_sequence[2];
    static constexpr auto outer_command_length = 3;
    auto body = command_sequence.subspan(outer_command_length);
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
            width_ = (static_cast<unsigned int>(body[0]) << 8u) | static_cast<unsigned int>(body[1]);
            height_ = (static_cast<unsigned int>(body[2]) << 8u) | static_cast<unsigned int>(body[3]);
            handler_.on_terminal_size(width_, height_);
        }
        break;
    default: break;
    }
    return outer_command_length + body.size() + iac_se.size();
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
        if (option_code == TELOPT_ECHO) {
            if (client_should_echo_) {
                send_com(WILL, TELOPT_ECHO);
                client_should_echo_ = false;
            }
        } else {
            // Doorman doesn't know about any other Telnet options so reply with WONT.
            send_com(WONT, option_code);
        }
        return 3;
    case DONT:
        if (option_code == TELOPT_ECHO) {
            if (!client_should_echo_) {
                send_com(WONT, TELOPT_ECHO);
                client_should_echo_ = true;
            }
        } else {
            // Doorman doesn't know about any other Telnet options so reply with WONT.
            send_com(WONT, option_code);
        }
        return 3;
    case SB: return on_subcommand(command_sequence);
    }
    throw std::runtime_error("Unexpected internal state");
}
