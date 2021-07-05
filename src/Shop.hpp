/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

#include "Types.hpp"
/*
 * Shop types.
 */
static inline constexpr auto MaxTrade = 5u;

enum class ObjectType;

struct Shop {
    Shop *next{}; /* Next shop in list            */
    sh_int keeper{}; /* Vnum of shop keeper mob      */
    ObjectType buy_type[MaxTrade]; /* Item types shop will buy     */
    sh_int profit_buy{}; /* Cost multiplier for buying   */
    sh_int profit_sell{}; /* Cost multiplier for selling  */
    unsigned int open_hour{}; /* First opening hour           */
    unsigned int close_hour{}; /* First closing hour           */
};
