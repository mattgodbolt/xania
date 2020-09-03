#include "AFFECT_DATA.hpp"

#include "Char.hpp"
#include "Logging.hpp"
#include "merc.h"

#include <fmt/format.h>

using namespace std::literals;

void AFFECT_DATA::modify(Char &ch, bool apply) const {
    if (apply) {
        SET_BIT(ch.affected_by, bitvector);
    } else {
        REMOVE_BIT(ch.affected_by, bitvector);
    }
    auto mod = apply ? modifier : -modifier;
    switch (location) {
    case AffectLocation::None: break;

    case AffectLocation::Str: ch.mod_stat[Stat::Str] += mod; break;
    case AffectLocation::Dex: ch.mod_stat[Stat::Dex] += mod; break;
    case AffectLocation::Int: ch.mod_stat[Stat::Int] += mod; break;
    case AffectLocation::Wis: ch.mod_stat[Stat::Wis] += mod; break;
    case AffectLocation::Con: ch.mod_stat[Stat::Con] += mod; break;

    case AffectLocation::Sex: ch.sex += mod; break;
    case AffectLocation::Mana: ch.max_mana += mod; break;
    case AffectLocation::Hit: ch.max_hit += mod; break;
    case AffectLocation::Move: ch.max_move += mod; break;

    case AffectLocation::Ac:
        for (auto &ac : ch.armor)
            ac += mod;
        break;
    case AffectLocation::Hitroll: ch.hitroll += mod; break;
    case AffectLocation::Damroll: ch.damroll += mod; break;

    case AffectLocation::SavingPara:
    case AffectLocation::SavingRod:
    case AffectLocation::SavingPetri:
    case AffectLocation::SavingBreath:
    case AffectLocation::SavingSpell: ch.saving_throw += mod; break;

    case AffectLocation::Class:
    case AffectLocation::Level:
    case AffectLocation::Age:
    case AffectLocation::Height:
    case AffectLocation::Weight:
    case AffectLocation::Gold:
    case AffectLocation::Exp:
        // TODO(193) should we do something with these?
        break;
    }
}

AFFECT_DATA::Value AFFECT_DATA::worth() const noexcept {
    switch (location) {
    case AffectLocation::Sex:
    case AffectLocation::None: break;

    case AffectLocation::Str:
    case AffectLocation::Dex:
    case AffectLocation::Int:
    case AffectLocation::Wis:
    case AffectLocation::Con:
        if (modifier > 0)
            return Value{0, 0, modifier};
        break;

    case AffectLocation::Hitroll: return Value{modifier, 0, 0};
    case AffectLocation::Damroll: return Value{0, modifier, 0};

    case AffectLocation::SavingPara:
    case AffectLocation::SavingRod:
    case AffectLocation::SavingPetri:
    case AffectLocation::SavingBreath:
    case AffectLocation::SavingSpell:
    case AffectLocation::Ac:
        if (modifier < 0)
            if (modifier > 0)
                return Value{0, 0, -modifier};
        break;

    case AffectLocation::Mana:
    case AffectLocation::Hit:
    case AffectLocation::Move:
        if (modifier > 0)
            return Value{0, 0, (modifier + 6) / 7};

    case AffectLocation::Class:
    case AffectLocation::Level:
    case AffectLocation::Age:
    case AffectLocation::Height:
    case AffectLocation::Weight:
    case AffectLocation::Gold:
    case AffectLocation::Exp:
        // TODO(193) should we do something with these?
        break;
    }
    return {};
}

std::string_view name(AffectLocation location) {
    switch (location) {
    case AffectLocation::None: return "none"sv;
    case AffectLocation::Str: return "strength"sv;
    case AffectLocation::Dex: return "dexterity"sv;
    case AffectLocation::Int: return "intelligence"sv;
    case AffectLocation::Wis: return "wisdom"sv;
    case AffectLocation::Con: return "constitution"sv;
    case AffectLocation::Sex: return "sex"sv;
    case AffectLocation::Class: return "class"sv;
    case AffectLocation::Level: return "level"sv;
    case AffectLocation::Age: return "age"sv;
    case AffectLocation::Mana: return "mana"sv;
    case AffectLocation::Hit: return "hp"sv;
    case AffectLocation::Move: return "moves"sv;
    case AffectLocation::Gold: return "gold"sv;
    case AffectLocation::Exp: return "experience"sv;
    case AffectLocation::Ac: return "armor class"sv;
    case AffectLocation::Hitroll: return "hit roll"sv;
    case AffectLocation::Damroll: return "damage roll"sv;
    case AffectLocation::SavingPara: return "save vs paralysis"sv;
    case AffectLocation::SavingRod: return "save vs rod"sv;
    case AffectLocation::SavingPetri: return "save vs petrification"sv;
    case AffectLocation::SavingBreath: return "save vs breath"sv;
    case AffectLocation::SavingSpell: return "save vs spell"sv;
    case AffectLocation::Height: return "height"sv;
    case AffectLocation::Weight: return "weight"sv;
    }

    bug("Affect_location_name: unknown location {}.", static_cast<int>(location));
    return "(unknown)"sv;
}

std::string AFFECT_DATA::describe_item_effect(bool for_imm) const {
    if (for_imm) {
        return fmt::format("{} by {} with bits {}, level {}", name(location), modifier, affect_bit_name(bitvector),
                           level);
    } else {
        return fmt::format("{} by {}", name(location), modifier);
    }
}
std::string AFFECT_DATA::describe_char_effect(bool for_imm) const {
    if (for_imm) { // for imm commands like 'stat mob' and 'stat affects', display all the details.
        if (is_skill())
            return fmt::format(" modifies {} by {} with bits {}, level {}", name(location), modifier,
                               affect_bit_name(bitvector), level);
        else
            return fmt::format(" modifies {} by {} for {} hours with bits {}, level {}", name(location), modifier,
                               duration, affect_bit_name(bitvector), level);
    } else {
        if (is_skill())
            return "";
        else if (location == AffectLocation::None)
            return fmt::format(" for {} hours", duration);
        else
            return fmt::format(" modifies {} by {} for {} hours", name(location), modifier, duration);
    }
}

bool AFFECT_DATA::is_skill() const noexcept { return type == gsn_sneak || type == gsn_ride; }
