/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2026 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

#include "DamageTolerance.hpp"

struct Char;

// A vampiric weapon drinks a share of the damage each ordinary weapon hit deals (not spells, nor skill attacks
// such as backstab): the victim loses it and the wielder heals it, up to their max hit points.
namespace VampiricDrain {

// The share of a hit's damage that's drained, in percent.
inline constexpr int Percent = 5;

// How much a hit that dealt damage_dealt drains: Percent of it, rounded to the nearest point, then adjusted by the
// victim's tolerance of negative energy the way damage() adjusts damage (none from the immune, such as the undead,
// a third less from the resistant, half as much again from the vulnerable).
[[nodiscard]] int amount(int damage_dealt, DamageTolerance negative_tolerance);

// Drains from victim into wielder after a hit that dealt damage_dealt, and returns what it drained. Drinking life
// costs the wielder a point of alignment, down to Alignment::Satanic, on each hit that drains anything. The caller
// sends the messages and updates the victim's position.
int drain(Char &wielder, Char &victim, int damage_dealt);

}
