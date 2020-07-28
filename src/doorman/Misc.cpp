#include "Misc.hpp"

#include <cstdarg>
#include <cstring>
#include <sys/types.h>
#include <unistd.h>

#include <fmt/format.h>

// TODO get rid of this file.

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
 * Returns the hostname, masked for privacy and with a hashcode of the full hostname. This can be used by admins
 * to spot users coming from the same IP.
 */
std::string get_masked_hostname(std::string_view hostname) {
    return fmt::format("{}*** [#{}]", hostname.substr(0, 6), djb2_hash(hostname));
}