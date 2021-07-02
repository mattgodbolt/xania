/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

#include "Types.hpp"

/*
 * Area reset definition.
 */
struct ResetData {
    ResetData *next{};
    char command{};
    sh_int arg1{};
    sh_int arg2{};
    sh_int arg3{};
    sh_int arg4{};
};
