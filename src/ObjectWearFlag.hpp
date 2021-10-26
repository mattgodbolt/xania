/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

#include "common/StandardBits.hpp"

#include <magic_enum.hpp>

enum class ObjectWearFlag : unsigned int {
    Take = A,
    Finger = B,
    Neck = C,
    Body = D,
    Head = E,
    Legs = F,
    Feet = G,
    Hands = H,
    Arms = I,
    Shield = J,
    About = K,
    Waist = L,
    Wrist = M,
    Wield = N,
    Hold = O,
    TwoHands = P,
    Ears = Q
};

[[nodiscard]] constexpr auto to_int(const ObjectWearFlag flag) noexcept {
    return magic_enum::enum_integer<ObjectWearFlag>(flag);
}
