/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

#include "Flag.hpp"
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

namespace BodyPartFlags {

const inline auto NumBodyParts = 22;
constexpr std::array<Flag<BodyPartFlag>, NumBodyParts> AllBodyPartFlags = {{
    // clang-format off
    {BodyPartFlag::Head, "head"},
    {BodyPartFlag::Arms, "arms"},
    {BodyPartFlag::Legs, "legs"},
    {BodyPartFlag::Heart, "heart"},
    {BodyPartFlag::Brains, "brains"},
    {BodyPartFlag::Guts, "guts"},
    {BodyPartFlag::Hands, "hands"},
    {BodyPartFlag::Feet, "feet"},
    {BodyPartFlag::Fingers, "fingers"},
    {BodyPartFlag::Ear, "ears"},
    {BodyPartFlag::Eye, "eyes"},
    {BodyPartFlag::LongTongue, "long_tongue"},
    {BodyPartFlag::EyeStalks, "eyestalks"},
    {BodyPartFlag::Tentacles, "tentacles"},
    {BodyPartFlag::Fins, "fins"},
    {BodyPartFlag::Wings, "wings"},
    {BodyPartFlag::Tail, "tail"},
    {BodyPartFlag::Claws, "claws"},
    {BodyPartFlag::Fangs, "fangs"},
    {BodyPartFlag::Horns, "horns"},
    {BodyPartFlag::Scales, "scales"},
    {BodyPartFlag::Tusks, "tusks"}
    // clang-format on
}};

}