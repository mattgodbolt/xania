#include "nth.hpp"

std::string_view impl::suffix_for(size_t value) {
    if (value > 4 && value < 20)
        return "th";

    if (value % 10 == 1)
        return "st";
    if (value % 10 == 2)
        return "nd";
    if (value % 10 == 3)
        return "rd";

    return "th";
}
