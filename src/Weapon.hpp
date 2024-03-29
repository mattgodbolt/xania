/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

#include <optional>
#include <string>

enum class Weapon { Exotic = 0, Sword = 1, Dagger = 2, Spear = 3, Mace = 4, Axe = 5, Flail = 6, Whip = 7, Polearm = 8 };

namespace Weapons {

[[nodiscard]] std::optional<Weapon> try_from_name(std::string_view name);
[[nodiscard]] std::optional<Weapon> try_from_integer(const int num);
[[nodiscard]] std::string name_from_integer(const int num);

}