/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

enum class DamageType;
struct Char;

// An enum representing a Char's tolerance for a particular damage type.
enum class DamageTolerance { None = 0, Immune = 1, Resistant = 2, Vulnerable = 3 };

[[nodiscard]] DamageTolerance check_damage_tolerance(const Char *ch, const DamageType damage_type);