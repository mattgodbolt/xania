#pragma once

#include <string>
#include <string_view>

/**
 * Returns the hostname, masked for privacy and with a hashcode of the full hostname. This can be used by admins
 * to spot users coming from the same IP.
 */
std::string get_masked_hostname(std::string_view hostname);

using byte = unsigned char;
