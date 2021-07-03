/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

#include "Types.hpp"
#include <string>

struct Char;

void remove_info_for_player(std::string_view player_name);
void update_info_cache(Char *ch);

// Rohan's structure to cache all char's info data for the finger command.
struct FingerInfo {
    std::string name;
    std::string info_message;
    std::string last_login_at;
    std::string last_login_from;
    sh_int invis_level{};
    bool i_message{};
    explicit FingerInfo(std::string_view name) : name(name) {}
    FingerInfo(std::string_view name, std::string_view info_message, std::string_view last_login_at,
               std::string_view last_login_from, sh_int invis_level, bool i_message)
        : name(name), info_message(info_message), last_login_at(last_login_at), last_login_from(last_login_from),
          invis_level(invis_level), i_message(i_message) {}
};
