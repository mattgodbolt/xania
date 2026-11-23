/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2026 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

#include "AffectLocation.hpp"

#include <string_view>
#include <vector>

struct Object;

// What the compare command tells a player about two objects (#144). It's deliberately vague: a verdict for two
// weapons or two pieces of armour, then a line for each stat that differs, with no numbers, so identify and lore
// are still worth having. Weapon flags (sharp, vampiric...) aren't compared.
namespace ObjectComparison {

// The total modifier an object gives to a location, the way equip_char applies it: the object's own affects, plus
// its index's unless it's been enchanted. All five saving throw locations count as one, AffectLocation::SavingSpell,
// because they all change the same saving throw.
[[nodiscard]] int total_modifier(const Object &obj, AffectLocation location);

// Weapons: the dice average plus damroll, which every hit adds; both doubled, to stay in whole numbers.
[[nodiscard]] int weapon_score(const Object &weapon);

// The protection an object gives: for armour, all four armour class values, including vs magic; for anything, minus
// four times its AC affects (which change all four values). Higher is better.
[[nodiscard]] int armour_score(const Object &obj);

struct Result {
    // act() formats, with $p the first object and $P the second, to send in order.
    std::vector<std::string_view> lines;
};

// Compares a with b. Two weapons or two pieces of armour get a verdict first; anything else that shares a wear
// slot gets the stat lines alone; things that can't be worn in the same place can't be compared.
[[nodiscard]] Result compare(const Object &a, const Object &b);

}
