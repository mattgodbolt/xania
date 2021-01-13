#include "mask_hostname.hpp"

#include <fmt/format.h>

namespace {

unsigned long djb2_hash(std::string_view str) {
    unsigned long hash = 5381;
    for (auto c : str)
        hash = hash * 33 + c;
    return hash;
}

}

// Returns the hostname, masked for privacy and with a hashcode of the full hostname. This can be used by admins to spot
// users coming from the same IP.
std::string get_masked_hostname(std::string_view hostname) {
    return fmt::format("{}*** [#{}]", hostname.substr(0, 6), djb2_hash(hostname));
}
