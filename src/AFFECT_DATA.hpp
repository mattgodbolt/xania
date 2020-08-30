#pragma once

#include "Types.hpp"

// Apply types (for affects).
// Used in #OBJECTS, so value is important.
#define APPLY_NONE 0
#define APPLY_STR 1
#define APPLY_DEX 2
#define APPLY_INT 3
#define APPLY_WIS 4
#define APPLY_CON 5
#define APPLY_SEX 6
#define APPLY_CLASS 7
#define APPLY_LEVEL 8
#define APPLY_AGE 9
//#define APPLY_HEIGHT 10
//#define APPLY_WEIGHT 11
#define APPLY_MANA 12
#define APPLY_HIT 13
#define APPLY_MOVE 14
#define APPLY_GOLD 15
#define APPLY_EXP 16
#define APPLY_AC 17
#define APPLY_HITROLL 18
#define APPLY_DAMROLL 19
#define APPLY_SAVING_PARA 20
#define APPLY_SAVING_ROD 21
#define APPLY_SAVING_PETRI 22
#define APPLY_SAVING_BREATH 23
#define APPLY_SAVING_SPELL 24

// A single effect that affects an object or character.
struct AFFECT_DATA {
    AFFECT_DATA *next{};
    sh_int type{};
    sh_int level{};
    sh_int duration{};
    sh_int location{};
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

private:
    void modify(Char &ch, bool apply);
};
