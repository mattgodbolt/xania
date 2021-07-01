/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once
#include "Types.hpp"

// TODO: constants
#define LIQ_WATER 0
#define LIQ_MAX 17

struct liq_type {
    const char *liq_name;
    const char *liq_color;
    sh_int liq_affect[3];
};

extern const struct liq_type liq_table[];

struct materials_type {
    sh_int magical_resilience;
    const char *material_name;
};

extern const struct materials_type material_table[];
