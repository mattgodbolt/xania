#pragma once

#include "Char.hpp"
#include "Descriptor.hpp"
#include "WrappedFd.hpp"

#include <memory>
#include <string>
#include <string_view>

// The base directory is the installation root. Defaults to "..".
std::string filename_for_player(std::string_view player_name);
std::string filename_for_god(std::string_view player_name);

class CharSaver {
public:
    // Save a player to its player file and related gods file.
    void save(const Char &ch) const;
    // A convenience so tests can write to specific destinations. Caller must close the files.
    void save(const Char &ch, FILE *god_file, FILE *player_file) const;
};

void save_char_obj(const Char *ch);
void save_char_obj(const Char *ch, FILE *fp);

struct LoadCharObjResult {
    bool newly_created{};
    std::unique_ptr<Char> character;
};
LoadCharObjResult try_load_player(std::string_view player_name);
void load_into_char(Char &character, LastLoginInfo &last_login, FILE *fp);
