#pragma once

#include "ArmourClass.hpp"
#include "Dice.hpp"
#include "Logging.hpp"
#include "MProgProgram.hpp"
#include "Materials.hpp"
#include "Position.hpp"
#include "Sex.hpp"
#include "Shop.hpp"
#include "Types.hpp"

#include <array>

#include <magic_enum/magic_enum.hpp>
#include <optional>
#include <string>
#include <vector>

enum class BodySize;

// Prototype for a mob.
// This is the in-memory version of #MOBILES.
struct MobIndexData {
    SpecialFunc spec_fun{};
    std::optional<Shop> shop{};
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
    std::array<sh_int, magic_enum::enum_count<ArmourClass>()> ac{};
    sh_int attack_type{}; // attack_table index.
    long off_flags{};
    long imm_flags{};
    long res_flags{};
    long vuln_flags{};
    Position start_pos{};
    Position default_pos{};
    Sex sex;
    sh_int race{};
    long gold{};
    long morphology{};
    unsigned long parts{};
    BodySize body_size{};
    Material::Type material{}; // TODO: is this actually used in any meaningful way?
    std::vector<MProg::Program> mobprogs{};
    int progtypes{}; /* Used by MOBprogram */

    static std::optional<MobIndexData> from_file(FILE *fp, const Logger &logger);

private:
    MobIndexData(sh_int vnum, FILE *fp, const Logger &logger);
};
