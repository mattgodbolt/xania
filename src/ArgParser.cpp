#include "ArgParser.hpp"

#include "string_utils.hpp"

std::string_view ArgParser::pop_argument() noexcept {
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

void ArgParser::ltrim() {
    // would be nice to use the one in string_utils, but that makes a string currently.
    while (!remaining_.empty() && isspace(remaining_.front()))
        remaining_.remove_prefix(1);
}

ArgParser::NumberArg ArgParser::pop_number_argument() noexcept {
    if (empty())
        return {};
    auto res = number_argument(pop_argument());
    return NumberArg{res.first, res.second};
}
