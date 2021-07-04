/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

struct Char;

// An enum representing a Char's resistance to a particular damage class.
enum class DamageResistance { None = 0, Immune = 1, Resistant = 2, Vulnerable = 3 };

[[nodiscard]] DamageResistance check_immune(const Char *ch, const int dam_type);