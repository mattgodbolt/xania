#pragma once

#include <string>
#include <string_view>

[[nodiscard]] std::string smash_tilde(std::string_view str);
[[nodiscard]] bool is_number(const char *arg);
[[nodiscard]] int number_argument(const char *argument, char *arg);
[[nodiscard]] std::pair<int, const char *> number_argument(const char *argument);
