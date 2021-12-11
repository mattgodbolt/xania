#pragma once

#include "string_utils.hpp"
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <utility>

template <typename T>
class CommandSet {
public:
    // add a command called 'name' with a value, which is available to characters
    // at level 'level' or above.
    void add(const std::string &name_in, T &&value, int level = 0) {
        auto name = lower_case(name_in);
        commands_.try_emplace(name, item{.value = std::move(value), .level = level, .order = order_++});
    }

    std::optional<T> get(std::string_view name_in, int level) const {
        auto name = lower_case(name_in);
        std::optional<T> result = std::nullopt;
        int best_order = order_;
        // lower_bound gets the lowest item whose key is great than or equal to name.
        for (auto it = commands_.lower_bound(name); it != commands_.end(); it++) {
            // If this command is too high a level for the user, skip on to the next one,
            // so that whatever level the user is, there are no weird gaps that might give(?-i)second
            // a hint as to the imm commands.
            if (it->second.level > level)
                continue;
            // This command is the right level for the user. However, we may have skipped past
            // the first valid prefix, or indeed there may be no command with a valid prefix.
            // So if this item doesn't match the requested key, drop out to the 'null' return.
            if (!has_prefix(it->first, name))
                break;
            // If we got here, the user is of the right level to use the command, and the command
            // matches what was requested. If it was added earlier than the last one we picked,
            // then use it.
            if (it->second.order < best_order) {
                best_order = it->second.order;
                result = it->second.value;
            }
        }
        return result;
    }

    void enumerate(std::function<void(const std::string &name, T value, int level)> fn) const {
        for (auto it = commands_.begin(); it != commands_.end(); it++) {
            fn(it->first, it->second.value, it->second.level);
        }
    }

    std::function<void(const std::string &name, T value, int level)>
    level_restrict(int min_level, int max_level,
                   std::function<void(const std::string &name, T value, int level)> fn) const {
        return [min_level, max_level, fn](const std::string &name, T value, int level) {
            if (level >= min_level && level <= max_level)
                fn(name, value, level);
        };
    }

private:
    struct item {
        T value;
        int level;
        int order;
    };
    std::map<std::string, item> commands_;
    int order_ = 0;
};
