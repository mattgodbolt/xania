#include "nth.hpp"

std::string_view impl::suffix_for(size_t value) {
    const auto last_two_digits = value % 100u;
    if (last_two_digits > 4 && last_two_digits < 20)
        return "th";

    const auto last_digit = value % 10u;
    switch (last_digit) {
    case 1: return "st";
    case 2: return "nd";
    case 3: return "rd";
    default: break;
    }
    return "th";
}
