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
constexpr std::array<Flag, NumBodyParts> AllBodyPartFlags = {{
    // clang-format off
    {to_int(BodyPartFlag::Head), "head"},
    {to_int(BodyPartFlag::Arms), "arms"},
    {to_int(BodyPartFlag::Legs), "legs"},
    {to_int(BodyPartFlag::Heart), "heart"},
    {to_int(BodyPartFlag::Brains), "brains"},
    {to_int(BodyPartFlag::Guts), "guts"},
    {to_int(BodyPartFlag::Hands), "hands"},
    {to_int(BodyPartFlag::Feet), "feet"},
    {to_int(BodyPartFlag::Fingers), "fingers"},
    {to_int(BodyPartFlag::Ear), "ears"},
    {to_int(BodyPartFlag::Eye), "eyes"},
    {to_int(BodyPartFlag::LongTongue), "long_tongue"},
    {to_int(BodyPartFlag::EyeStalks), "eyestalks"},
    {to_int(BodyPartFlag::Tentacles), "tentacles"},
    {to_int(BodyPartFlag::Fins), "fins"},
    {to_int(BodyPartFlag::Wings), "wings"},
    {to_int(BodyPartFlag::Tail), "tail"},
    {to_int(BodyPartFlag::Claws), "claws"},
    {to_int(BodyPartFlag::Fangs), "fangs"},
    {to_int(BodyPartFlag::Horns), "horns"},
    {to_int(BodyPartFlag::Scales), "scales"},
    {to_int(BodyPartFlag::Tusks), "tusks"}
    // clang-format on
}};

}