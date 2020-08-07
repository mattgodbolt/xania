#pragma once

#include <string>
#include <string_view>

// Replace all tildes with dashes. Useful til we revamp the player files...
[[nodiscard]] std::string smash_tilde(std::string_view str);
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
