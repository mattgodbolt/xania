#pragma once

#include "Types.hpp"

#include <array>
#include <optional>
#include <string_view>

enum class Stat {
    // Order is important: this is the order these are specified in the pfiles and area files.
    Str = 0,
    Int = 1,
    Wis = 2,
    Dex = 3,
    Con = 4
};
static constexpr inline auto MAX_STATS = 5;
inline std::string_view to_short_string(Stat stat) {
    using namespace std::literals;
    switch (stat) {
    case Stat::Str: return "str"sv;
    case Stat::Int: return "int"sv;
    case Stat::Wis: return "wis"sv;
    case Stat::Dex: return "dex"sv;
    case Stat::Con: return "con"sv;
    }
    return "(unknown)"sv;
}
inline std::string_view to_long_string(Stat stat) {
    using namespace std::literals;
    switch (stat) {
    case Stat::Str: return "strength"sv;
    case Stat::Int: return "intelligence"sv;
    case Stat::Wis: return "wisdom"sv;
    case Stat::Dex: return "dexterity"sv;
    case Stat::Con: return "constitution"sv;
    }
    return "(unknown)"sv;
}
inline constexpr std::array all_stats = {Stat::Str, Stat::Int, Stat::Wis, Stat::Dex, Stat::Con};

std::optional<Stat> try_parse_stat(std::string_view stat_name);

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
