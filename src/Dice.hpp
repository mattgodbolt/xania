#pragma once

#include "Rng.hpp"

#include <fmt/format.h>

class Dice {
    int number_{};
    int type_{};
    int bonus_{};

public:
    Dice() = default;
    explicit Dice(int number, int type, int bonus = 0) : number_(number), type_(type), bonus_(bonus) {}

    static Dice from_file(FILE *fp);
    [[nodiscard]] int number() const noexcept { return number_; }
    [[nodiscard]] int type() const noexcept { return type_; }
    [[nodiscard]] int bonus() const noexcept { return bonus_; }
    void bonus(int bonus) noexcept { bonus_ = bonus; }

    [[nodiscard]] int roll() const noexcept { return roll(Rng::global_rng()); }
    [[nodiscard]] int roll(Rng &rng) const noexcept { return rng.dice(number_, type_) + bonus_; }

    bool operator==(const Dice &rhs) const;
    bool operator!=(const Dice &rhs) const;
};

template <>
struct fmt::formatter<Dice> {
    constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }
    template <typename FormatContext>
    auto format(const Dice &dice, FormatContext &ctx) {
        if (dice.bonus())
            return format_to(ctx.out(), "{}d{}+{}", dice.number(), dice.type(), dice.bonus());
        else
            return format_to(ctx.out(), "{}d{}", dice.number(), dice.type());
    }
};
