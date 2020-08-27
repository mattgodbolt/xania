#pragma once

#include <string_view>

struct CHAR_DATA;

[[nodiscard]] CHAR_DATA *get_char_room(CHAR_DATA *ch, std::string_view argument);
[[nodiscard]] CHAR_DATA *get_char_world(CHAR_DATA *ch, std::string_view argument);
void extract_char(CHAR_DATA *ch, bool delete_from_world);
void reap_old_chars();