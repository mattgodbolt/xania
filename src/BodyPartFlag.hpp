/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

#include "common/StandardBits.hpp"

#include <magic_enum.hpp>

enum class BodyPartFlag : unsigned long {
    Head = A,
    Arms = B,
    Legs = C,
    Heart = D,
    Brains = E,
    Guts = F,
    Hands = G,
    Feet = H,
    Fingers = I,
    Ear = J,
    Eye = K,
    LongTongue = L,
    EyeStalks = M,
    Tentacles = N,
    Fins = O,
    Wings = P,
    Tail = Q,
    Claws = U,
    Fangs = V,
    Horns = W,
    Scales = X,
    Tusks = Y
};

[[nodiscard]] constexpr auto to_int(const BodyPartFlag flag) noexcept {
    return magic_enum::enum_integer<BodyPartFlag>(flag);
}

const inline auto NumBodyParts = 22;
