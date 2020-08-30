#pragma once

#include "Types.hpp"

#include <string>
#include <string_view>

// Apply types (for affects).
// Used in #OBJECTS, so value is important.
enum class AffectLocation {
    None = 0,
    Str = 1,
    Dex = 2,
    Int = 3,
    Wis = 4,
    Con = 5,
    Sex = 6,
    Class = 7,
    Level = 8,
    Age = 9,
    Height = 10,
    Weight = 11,
    Mana = 12,
    Hit = 13,
    Move = 14,
    Gold = 15,
    Exp = 16,
    Ac = 17,
    Hitroll = 18,
    Damroll = 19,
    SavingPara = 20,
    SavingRod = 21,
    SavingPetri = 22,
    SavingBreath = 23,
    SavingSpell = 24,
};

[[nodiscard]] std::string_view name(AffectLocation location);

// A single effect that affects an object or character.
struct AFFECT_DATA {
    AFFECT_DATA *next{};
    sh_int type{};
    sh_int level{};
    sh_int duration{};
    AffectLocation location{AffectLocation::None};
    sh_int modifier{};
    unsigned int bitvector{};

    void apply(Char &ch) { modify(ch, true); }
    void unapply(Char &ch) { modify(ch, false); }

    struct Value {
        int hit{};
        int damage{};
        int worth{};
        Value &operator+=(const Value &rhs) noexcept {
            hit += rhs.hit;
            damage += rhs.damage;
            worth += rhs.worth;
            return *this;
        }
    };
    [[nodiscard]] Value worth() const noexcept;

    [[nodiscard]] bool affects_stats() const noexcept { return location != AffectLocation::None && modifier; }
    [[nodiscard]] std::string describe_item_effect(bool for_imm = false) const;
    [[nodiscard]] std::string describe_char_effect(bool for_imm = false) const;

    [[nodiscard]] bool is_skill() const noexcept;

private:
    void modify(Char &ch, bool apply);
};
