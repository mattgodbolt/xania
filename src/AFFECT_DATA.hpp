#pragma once

#include "AffectLocation.hpp"
#include "Types.hpp"

#include <string>

// A single effect that affects an object or character.
struct AFFECT_DATA {
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

        friend Value operator+(const Value &lhs, const Value &rhs) noexcept {
            return Value{lhs.hit + rhs.hit, lhs.damage + rhs.damage, lhs.worth + rhs.worth};
        }
    };
    [[nodiscard]] Value worth() const noexcept;

    [[nodiscard]] bool affects_stats() const noexcept { return location != AffectLocation::None && modifier; }
    [[nodiscard]] std::string describe_item_effect(bool for_imm = false) const;
    [[nodiscard]] std::string describe_char_effect(bool for_imm = false) const;

    [[nodiscard]] bool is_skill() const noexcept;
    bool operator==(const AFFECT_DATA &rhs) const;
    bool operator!=(const AFFECT_DATA &rhs) const;

private:
    void modify(Char &ch, bool apply) const;
};
