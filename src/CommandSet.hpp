#pragma once

#include "string_utils.hpp"
#include <functional>
#include <map>
#include <string>
#include <utility>

template <typename T>
class CommandSet {
public:
    CommandSet() {}

    // add a command called 'name' with a value, which is available to characters
    // at level 'level' or above.
    void add(const std::string &name_in, T value, int level) {
        auto name = lower_case(name_in);
        commands_[name] = std::make_pair(value, level);
    }

    std::optional<const T> get(const std::string &name_in, int level) const {
        auto name = lower_case(name_in);
        // lower_bound gets the lowest item whose key is great than or equal to name.
        for (auto it = commands_.lower_bound(name); it != commands_.end(); it++) {
            // If this command is too high a level for the user, skip on to the next one,
            // so that whatever level the user is, there are no weird gaps that might give
            // a hint as to the imm commands.
            if (it->second.second > level)
                continue;
            // This command is the right level for the user. However, we may have skipped past
            // the first valid prefix, or indeed there may be no command with a valid prefix.
            // So if this item doesn't match the requested key, drop out to the 'null' return.
            if (!has_prefix(it->first, name))
                break;
            // If we got here, the user is of the right level to use the command, and the command
            // matches what was requested.
            return it->second.first;
        }
        return std::nullopt;
    }

    void enumerate(int min_level, int max_level,
                   std::function<void(const std::string &name, const T value, int level)> fn) const {
        for (auto it = commands_.begin(); it != commands_.end(); it++) {
            auto level = it->second.second;
            if (level < min_level || level > max_level)
                continue;
            fn(it->first, it->second.first, level);
        }
    }

private:
    std::map<std::string, std::pair<T, int>> commands_;
};
