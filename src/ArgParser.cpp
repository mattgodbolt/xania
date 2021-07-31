#include "ArgParser.hpp"

#include "string_utils.hpp"

std::string_view ArgParser::shift() noexcept {
    ltrim();

    if (empty())
        return {};

    auto terminator = ' ';
    if (remaining_.front() == '\'' || remaining_.front() == '"') {
        terminator = remaining_.front();
        remaining_.remove_prefix(1);
    }

    auto terminator_pos = remaining_.find(terminator);
    if (terminator_pos == std::string_view::npos) {
        auto res = remaining_;
        remaining_ = std::string_view();
        return res;
    }

    auto res = remaining_.substr(0, terminator_pos);
    remaining_.remove_prefix(terminator_pos + 1);
    ltrim();

    return res;
}

std::string_view ArgParser::commandline_shift() noexcept {
    ltrim();
    if (empty())
        return {};
    if (std::ispunct(remaining_.front())) {
        auto res = remaining_.substr(0, 1);
        remaining_.remove_prefix(1);
        ltrim();
        return res;
    } else {
        return shift();
    }
}

void ArgParser::ltrim() {
    // would be nice to use the one in string_utils, but that makes a string currently.
    while (!remaining_.empty() && isspace(remaining_.front()))
        remaining_.remove_prefix(1);
}

ArgParser::NumberArg ArgParser::shift_numbered_arg() noexcept {
    if (empty())
        return {};
    auto res = number_argument(shift());
    return NumberArg{res.first, res.second};
}

std::optional<int> ArgParser::try_shift_number() noexcept {
    if (empty())
        return {};
    auto prev_remaining = remaining_;
    auto arg = shift();
    if (is_number(arg))
        return parse_number(arg);
    remaining_ = prev_remaining;
    return {};
}
