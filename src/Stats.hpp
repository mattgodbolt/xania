#pragma once

#include "Types.hpp"

#include <array>

enum class Stat {
    // Order is important: this is the order these are specified in the pfiles and area files.
    Str = 0,
    Int = 1,
    Wis = 2,
    Dex = 3,
    Con = 4
};
static constexpr inline auto MAX_STATS = 5;

class Stats {
    std::array<sh_int, MAX_STATS> stats_{};

public:
    Stats() = default;
    Stats(sh_int strength, sh_int intelligence, sh_int wisdom, sh_int dexterity, sh_int constitution)
        : stats_{{strength, intelligence, wisdom, dexterity, constitution}} {}
    sh_int &operator[](Stat stat) { return stats_[static_cast<size_t>(stat)]; }
    sh_int operator[](Stat stat) const { return stats_[static_cast<size_t>(stat)]; }
    friend auto begin(Stats &stat) { return stat.stats_.begin(); }
    friend auto end(Stats &stat) { return stat.stats_.end(); }
};
