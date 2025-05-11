/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "Class.hpp"
#include "Constants.hpp"
#include "Stats.hpp"
#include "VnumObjects.hpp"
#include "string_utils.hpp"

#include <fmt/ranges.h>
#include <range/v3/algorithm/find_if.hpp>
#include <range/v3/view/transform.hpp>

namespace {

using namespace std::literals;

constexpr Class Mage{
    // clang-format off
    0,
    "mage"sv, "Mag"sv,
    Stat::Int,
    Objects::SchoolDagger,
    {3018, 9618, 10074},
    75, -1, -47,
    6, 8, 10,
    "mage basics"sv,
    "mage default"sv
    // clang-format on
};

constexpr Class Cleric{
    // clang-format off
    1,
    "cleric"sv, "Cle"sv,
    Stat::Wis,
    Objects::SchoolMace,
    {3003, 9619, 10114},
    75, -1, -51,
    7, 10, 8,
    "cleric basics"sv,
    "cleric default"sv
    // clang-format on
};

constexpr Class Knight{
    // clang-format off
    2,
    "knight"sv, "Kni"sv,
    Stat::Str,
    Objects::SchoolSword,
    {3028, 9639, 10086},
    75, -1, -61,
    10, 14, 6,
    "knight basics"sv,
    "knight default"sv
    // clang-format on
};

constexpr Class Barbarian{
    // clang-format off
    3,
    "barbarian"sv, "Bar"sv,
    Stat::Con,
    Objects::SchoolAxe,
    {3022, 9633, 10073},
    75, -1, -63,
    13, 17, 3,
    "barbarian basics"sv,
    "barbarian default"sv
    // clang-format on
};

constexpr std::array<Class const *, MAX_CLASS> class_table = {{&Mage, &Cleric, &Knight, &Barbarian}};

}

const std::array<Class const *, MAX_CLASS> &Class::table() { return class_table; }

const Class *Class::by_id(const sh_int id) {
    if (id < 0 || id >= MAX_CLASS) {
        return nullptr;
    }
    return class_table[id];
}

const Class *Class::by_name(std::string_view name) {
    const auto name_matches = [&name](const auto &c) { return matches_start(name, c->name); };
    if (const auto it = ranges::find_if(class_table, name_matches); it != class_table.end()) {
        return *it;
    }
    return nullptr;
}

std::string Class::names_csv() {
    return fmt::format("{}", fmt::join(class_table | ranges::views::transform(&Class::name), ", "sv));
}

const Class *Class::mage() { return &Mage; }

const Class *Class::cleric() { return &Cleric; }

const Class *Class::knight() { return &Knight; }

const Class *Class::barbarian() { return &Barbarian; }
