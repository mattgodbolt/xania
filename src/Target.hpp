/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

// Target types.
enum class Target {
    Ignore = 0,
    CharOffensive = 1,
    CharDefensive = 2,
    CharSelf = 3,
    ObjectInventory = 4,
    CharObject = 5,
    CharOther = 6
};