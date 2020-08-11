#pragma once

#include <list>
#include <string>
#include <string_view>

#include "string_utils_impl.hpp"

// Replace all tildes with dashes. Useful til we revamp the player files...
[[nodiscard]] std::string smash_tilde(std::string_view str);
// Replaces all instances of from with to. Returns a copy of message.
[[nodiscard]] std::string replace_strings(std::string message, std::string_view from_str, std::string_view to_str);
// Skips whitespace, returning a new string.
[[nodiscard]] std::string skip_whitespace(std::string_view str);
// Trims off the last line of a string (terminated with \n\r).
[[nodiscard]] std::string remove_last_line(std::string_view str);

// Sanitizes user input: removes non-printing characters and applies "\b" backspaces
[[nodiscard]] std::string sanitise_input(std::string_view str);

// Return true if an argument is completely numeric.
[[nodiscard]] bool is_number(const char *arg);

// Given a string like 14.foo, return 14 and 'foo'.
[[nodiscard]] int number_argument(const char *argument, char *arg); // <- deprecated
[[nodiscard]] std::pair<int, const char *> number_argument(const char *argument);

// Iterate over lines in a string. `for (auto line : line_iter(...)) { ... }`
inline auto line_iter(std::string_view sv) { return impl::LineSplitter{sv}; }

// Collect the individual lines of text into a container. Templatized on the storage type, so you can pick whether the
// container returns references to the original string (e.g. vector<string_view>) or copies (vector<string>).
template <typename Container>
Container split_lines(std::string_view input) {
    Container result;
    for (auto sv : line_iter(input))
        result.emplace_back(sv);
    return result;
}

// Returns the string, lower-cased.
[[nodiscard]] std::string lower_case(std::string_view str);

// Returns true iff the second string is a prefix of the first (or the two strings are
// identical).
[[nodiscard]] bool has_prefix(std::string_view haystack, std::string_view needle);

// Colourises (or strips out, if use_ansi is false) the strange pipe-delimited text format we use for colour.
[[nodiscard]] std::string colourise_mud_string(bool use_ansi, std::string_view txt);