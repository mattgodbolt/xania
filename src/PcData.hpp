#pragma once

#include "Class.hpp"
#include "Constants.hpp"
#include "Duel.hpp"
#include "PcCustomization.hpp"
#include "PcNutrition.hpp"
#include "Pronouns.hpp"
#include "Sex.hpp"
#include "clan.h"

#include <array>
#include <optional>
#include <string>

struct PcData {
    Class const *class_type{Class::mage()};
    std::string pwd;
    std::string bamfin;
    std::string bamfout;
    std::string title;
    std::string prompt;
    std::string afk;
    std::string prefix;
    std::string info_message;
    // Timezone hours and minutes offset are unused currently.
    sh_int houroffset{};
    sh_int minoffset{};
    sh_int perm_hit{};
    sh_int perm_mana{};
    sh_int perm_move{};
    Sex true_sex;

    int last_level{};
    std::array<sh_int, 3> condition{0, 48, 48};
    PcNutrition inebriation = PcNutrition::sober();
    PcNutrition hunger = PcNutrition::fed();
    PcNutrition thirst = PcNutrition::hydrated();
    std::array<sh_int, MAX_SKILL> learned{};
    std::array<bool, MAX_GROUP> group_known{};
    sh_int points{};
    bool confirm_delete{};
    bool colour{};
    std::optional<PcClan> pcclan;
    Pronouns pronouns;
    // Populated when a new player character is being created.
    std::optional<PcCustomization> customization{};
    // Set when a player is invited to duel, unset when the duel ends.
    std::optional<Duels::Duel> duel{};
};
