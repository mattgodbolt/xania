#pragma once

#include <string>
#include <string_view>

#include "string_utils_impl.hpp"

// Replace all tildes with dashes. Useful til we revamp the player files...
[[nodiscard]] std::string smash_tilde(std::string_view str);
// Replaces all instances of from with to. Returns a copy of message.
[[nodiscard]] std::string replace_strings(std::string message, std::string_view from_str, std::string_view to_str);
// Trims leading whitespace, returning a new string.
[[nodiscard]] std::string ltrim(std::string_view str);
// Trims leading and trailing whitespace, returning a new string
[[nodiscard]] std::string trim(std::string_view str);
// Skips leading and trailing whitespace, and compacts intermediate spaces down to a single space.
[[nodiscard]] std::string reduce_spaces(std::string_view str);
// Trims off the last line of a string (terminated with \n\r).
[[nodiscard]] std::string remove_last_line(std::string_view str);

// Sanitizes user input: removes non-printing characters and applies "\b" backspaces
[[nodiscard]] std::string sanitise_input(std::string_view str);

// Return true if an argument is completely numeric.
[[nodiscard]] bool is_number(const char *arg);

// Given a string like 14.foo, return 14 and 'foo'.
[[nodiscard]] int number_argument(const char *argument, char *arg); // <- deprecated
[[nodiscard]] std::pair<int, std::string_view> number_argument(std::string_view argument);

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

// Returns an initial-capped string.
[[nodiscard]] std::string capitalize(std::string_view text);

// Kill off the errant capital letters that have been creeping into the definite article of short descriptions of
// objects and rooms of late.  This routine de-capitalises anything beginning with 'The ' or 'A ' or 'An '.
[[nodiscard]] std::string lower_case_articles(std::string_view text);

// Returns true iff the second string is a prefix of the first (or the two strings are
// identical).
[[nodiscard]] bool has_prefix(std::string_view haystack, std::string_view needle);

// Colourises (or strips out, if use_ansi is false) the strange pipe-delimited text format we use for colour.
[[nodiscard]] std::string colourise_mud_string(bool use_ansi, std::string_view txt);

// Upper-cases the first character, taking into account things like `|Rthe goblin is DEAD!!`
[[nodiscard]] std::string upper_first_character(std::string_view sv);

// Compares two strings: are they referring to the same thing. That currently means "case insensitive comparison".
[[nodiscard]] bool matches(std::string_view lhs, std::string_view rhs);

// Similar to matches() but checks if rhs starts with lhs, case insensitively.
// lhs must be at least one character long and must not be longer than rhs.
[[nodiscard]] bool matches_start(std::string_view lhs, std::string_view rhs);

// Is 'needle' contained inside 'haystack' case insensitively?
[[nodiscard]] bool matches_inside(std::string_view needle, std::string_view haystack);

// See if 'str' is a match for 'namelist'.
// All parts of str must match one of the part of namelist.
[[nodiscard]] bool is_name(std::string_view str, std::string_view namelist);
