/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2021 Xania Development Team                                 */
/*  See the header to file: merc.h for original code copyrights          */
/*************************************************************************/

#pragma once

#include "Types.hpp"

#include <string_view>

// See FlagFormat.hpp for the flag serialization & formatting routines.

// Properties of an individual status bit. Used by Rooms, Objects and other things.
struct Flag {
    const unsigned int bit;
    const sh_int min_level;
    std::string_view name;
};
