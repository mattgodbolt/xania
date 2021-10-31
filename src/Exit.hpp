/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

#include "Types.hpp"

struct Room;

/*
 * Exit data.
 */
struct Exit {
    union {
        Room *to_room;
        sh_int vnum;
    } u1;
    unsigned int exit_info{};
    sh_int key{};
    char *keyword{};
    char *description{};

    Exit *next{};
    unsigned int rs_flags{};
    bool is_one_way{};
};

// If an exit's vnum is this value the exit goes nowhere but it can be looked at.
static constexpr inline auto EXIT_VNUM_COSMETIC = -1;
