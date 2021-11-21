/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

#include "Constants.hpp"

#include <array>

// Data for customizing player characters, only when a new player is
// being created and they choose to customize rather than accepting the defaults.
struct PcCustomization {
    std::array<bool, MAX_SKILL> skill_chosen{};
    std::array<bool, MAX_GROUP> group_chosen{};
    int points_chosen{};
};
