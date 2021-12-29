/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

#include "common/StandardBits.hpp"

#include <magic_enum.hpp>

namespace MProg {

// Based on Merc-2.2 MOBProgs.
// These are bits because in MobIndexData, a mob's template can be flagged as having multiple mob prog types.
enum class TypeFlag {
    Error = -1,
    InFile = 0,
    Act = A,
    Speech = B,
    Random = C,
    Fight = D,
    Death = E,
    HitPercent = F,
    Entry = G,
    Greet = H,
    AllGreet = I,
    Give = J,
    Bribe = K
};

[[nodiscard]] constexpr auto to_int(const TypeFlag flag) noexcept { return magic_enum::enum_integer<TypeFlag>(flag); }

}
