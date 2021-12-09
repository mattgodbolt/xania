/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2021 Xania Development Team                                 */
/*  See the header to file: merc.h for original code copyrights          */
/*************************************************************************/

#pragma once

#include "ArgParser.hpp"
#include "Char.hpp"
#include "Flag.hpp"
#include "Types.hpp"
#include "common/BitOps.hpp"

#include <fmt/format.h>
#include <range/v3/algorithm/find_if.hpp>
#include <range/v3/view/filter.hpp>
#include <range/v3/view/transform.hpp>

#include <array>
#include <cstdlib>
#include <optional>
#include <string_view>

// Serializes a 32 bit int using the MUD's ASCII flag format.
std::string serialize_flags(const unsigned int value);

// Returns a string containing the names of all flag bits that are set on current_val, excluding
// any bits that are above the Char's level.
template <std::size_t SIZE, typename Bits>
std::string format_set_flags(const std::array<Flag, SIZE> &flags, const Char *ch, const Bits current_val) {
    const auto flag_list =
        fmt::format("{}", fmt::join(flags | ranges::views::filter([&ch, &current_val](const auto &flag) {
                                        return ch->level >= flag.min_level && check_bit(current_val, flag.bit);
                                    }) | ranges::views::transform(&Flag::name),
                                    " "));
    return flag_list.empty() ? "none" : "|C" + flag_list + "|w";
}

// Returns a string containing the names of all flag bits that are set on current_val.
template <std::size_t SIZE, typename Bits>
std::string format_set_flags(const std::array<Flag, SIZE> &flags, const Bits current_val) {
    const auto flag_list = fmt::format("{}", fmt::join(flags | ranges::views::filter([&current_val](const auto &flag) {
                                                           return check_bit(current_val, flag.bit);
                                                       }) | ranges::views::transform(&Flag::name),
                                                       " "));
    return flag_list.empty() ? "none" : "|C" + flag_list + "|w";
}

template <std::size_t SIZE>
std::string format_available_flags(const std::array<Flag, SIZE> &flags, const Char *ch) {
    const auto flag_list = fmt::format("{}", fmt::join(flags | ranges::views::filter([&ch](const auto &flag) {
                                                           return ch->level >= flag.min_level;
                                                       }) | ranges::views::transform(&Flag::name),
                                                       " "));
    return flag_list.empty() ? "none" : "|C" + flag_list + "|w";
}

template <std::size_t SIZE>
std::optional<unsigned long> get_flag_bit_by_name(const std::array<Flag, SIZE> &flags, std::string_view requested_name,
                                                  const sh_int trust_level) {
    if (auto it = ranges::find_if(flags,
                                  [&requested_name, &trust_level](const auto &flag) {
                                      return flag.name == requested_name && trust_level >= flag.min_level;
                                  });
        it != flags.end()) {
        return it->bit;
    } else {
        return std::nullopt;
    }
}

template <std::size_t SIZE, typename Bits>
unsigned long flag_set(const std::array<Flag, SIZE> &flags, ArgParser args, const Bits current_val, Char *ch) {
    const auto show_usage = [&]() {
        ch->send_line(format_set_flags(flags, ch, current_val));
        ch->send_line("Available flags:");
        ch->send_line(format_available_flags(flags, ch));
        return current_val;
    };

    if (args.empty()) {
        return show_usage();
    }
    auto updated_val = current_val;
    enum class BitOp { Toggle, Set, Clear };
    for (auto flag_name : args) {
        auto operation = BitOp::Toggle;
        switch (flag_name[0]) {
        case '+':
            operation = BitOp::Set;
            flag_name.remove_prefix(1);
            break;
        case '-':
            operation = BitOp::Clear;
            flag_name.remove_prefix(1);
            break;
        }
        if (auto opt_bit = get_flag_bit_by_name(flags, flag_name, ch->get_trust())) {
            switch (operation) {
            case BitOp::Toggle: toggle_bit(updated_val, *opt_bit); break;
            case BitOp::Set: set_bit(updated_val, *opt_bit); break;
            case BitOp::Clear: clear_bit(updated_val, *opt_bit); break;
            }
        } else {
            return show_usage();
        }
    }
    ch->send_line(format_set_flags(flags, ch, updated_val));
    return updated_val;
}