#pragma once

#include "Char.hpp"
#include "Descriptor.hpp"

#include <memory>
#include <string>
#include <string_view>

// The base directory is the installation root. Defaults to "..".
std::string get_base_dir();
std::string get_player_dir();
std::string filename_for_player(std::string_view player_name);

void save_char_obj(const Char *ch);
void save_char_obj(const Char *ch, FILE *fp);

struct LoadCharObjResult {
    bool newly_created{};
    std::unique_ptr<Char> character;
};
LoadCharObjResult try_load_player(std::string_view player_name);
void load_into_char(Char &character, FILE *fp);
