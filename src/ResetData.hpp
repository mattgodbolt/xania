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
    const char command{};
    const sh_int arg1{};
    const sh_int arg2{};
    const sh_int arg3{};
    const sh_int arg4{};
};
