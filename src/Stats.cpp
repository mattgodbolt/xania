#include "Stats.hpp"

#include "string_utils.hpp"

std::optional<Stat> try_parse_stat(std::string_view stat_name) {
    for (auto stat : all_stats)
        if (matches(stat_name, to_short_string(stat)) || matches(stat_name, to_long_string(stat)))
            return stat;
    return std::nullopt;
}
