#pragma once

#include "Dice.hpp"
#include "Types.hpp"

#include <array>
#include <cstdio>
#include <optional>
#include <string>

struct SHOP_DATA;
struct MPROG_DATA;

// Prototype for a mob.
// This is the in-memory version of #MOBILES.
struct MobIndexData {
    SpecialFunc spec_fun{};
    SHOP_DATA *pShop{};
    sh_int vnum{};
    sh_int count{};
    sh_int killed{};
    std::string player_name;
    std::string short_descr;
    std::string long_descr;
    std::string description;
    unsigned long act{};
    unsigned long affected_by{};
    sh_int alignment{};
    sh_int group{}; // rom-2.4 style mob groupings -- TODO unused
    sh_int level{};
    sh_int hitroll{};
    Dice hit;
    Dice mana;
    Dice damage; // The bonus() here is the damroll
    std::array<sh_int, 4> ac{};
    sh_int dam_type{};
    long off_flags{};
    long imm_flags{};
    long res_flags{};
    long vuln_flags{};
    sh_int start_pos{};
    sh_int default_pos{};
    sh_int sex{};
    sh_int race{};
    long gold{};
    long form{};
    long parts{};
    sh_int size{};
    sh_int material{}; // TODO: is this actually used in any meaningful way?
    MPROG_DATA *mobprogs{}; /* Used by MOBprogram */
    int progtypes{}; /* Used by MOBprogram */

    static std::optional<MobIndexData> from_file(FILE *fp);

private:
    MobIndexData(sh_int vnum, FILE *fp);
};
