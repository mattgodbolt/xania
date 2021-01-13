#pragma once

#include <string_view>

struct Char;

[[nodiscard]] Char *get_char_room(Char *ch, std::string_view argument);
[[nodiscard]] Char *get_char_world(Char *ch, std::string_view argument);
void extract_char(Char *ch, bool delete_from_world);
void reap_old_chars();
[[nodiscard]] int race_lookup(std::string_view name);
