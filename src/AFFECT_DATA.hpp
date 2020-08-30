#pragma once

#include "AffectLocation.hpp"
#include "Types.hpp"

#include <string>

// A single effect that affects an object or character.
struct AFFECT_DATA {
    AFFECT_DATA *next{};
    sh_int type{};
    sh_int level{};
    sh_int duration{};
    AffectLocation location{AffectLocation::None};
    sh_int modifier{};
    unsigned int bitvector{};

    void apply(Char &ch) const { modify(ch, true); }
    void unapply(Char &ch) const { modify(ch, false); }

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
    void modify(Char &ch, bool apply) const;
};
