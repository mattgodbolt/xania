#pragma once

#include <string_view>

struct CHAR_DATA;

[[nodiscard]] CHAR_DATA *get_char_room(CHAR_DATA *ch, std::string_view argument);
[[nodiscard]] CHAR_DATA *get_char_world(CHAR_DATA *ch, std::string_view argument);
