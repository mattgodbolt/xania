#pragma once

#include "Constants.hpp"
#include "Types.hpp"
#include "clan.h"

#include <array>
#include <optional>
#include <string>

struct PC_DATA {
    std::string pwd;
    std::string bamfin;
    std::string bamfout;
    std::string title;
    std::string prompt;
    std::string afk;
    std::string prefix;
    std::string info_message;
    sh_int houroffset{};
    sh_int minoffset{};
    sh_int perm_hit{};
    sh_int perm_mana{};
    sh_int perm_move{};
    sh_int true_sex{};

    int last_level{};
    std::array<sh_int, 3> condition{0, 48, 48};
    std::array<sh_int, MAX_SKILL> learned{};
    std::array<bool, MAX_GROUP> group_known{};
    sh_int points{};
    bool confirm_delete{};
    bool colour{};
    std::optional<PCCLAN> pcclan;
};
