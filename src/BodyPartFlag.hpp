/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

#include "Flag.hpp"
#include "common/StandardBits.hpp"

#include <magic_enum/magic_enum.hpp>

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

using BF = Flag<BodyPartFlag>;
constexpr auto Head = BF{BodyPartFlag::Head, "head"};
constexpr auto Arms = BF{BodyPartFlag::Arms, "arms"};
constexpr auto Legs = BF{BodyPartFlag::Legs, "legs"};
constexpr auto Heart = BF{BodyPartFlag::Heart, "heart"};
constexpr auto Brains = BF{BodyPartFlag::Brains, "brains"};
constexpr auto Guts = BF{BodyPartFlag::Guts, "guts"};
constexpr auto Hands = BF{BodyPartFlag::Hands, "hands"};
constexpr auto Feet = BF{BodyPartFlag::Feet, "feet"};
constexpr auto Fingers = BF{BodyPartFlag::Fingers, "fingers"};
constexpr auto Ear = BF{BodyPartFlag::Ear, "ears"};
constexpr auto Eye = BF{BodyPartFlag::Eye, "eyes"};
constexpr auto LongTongue = BF{BodyPartFlag::LongTongue, "long_tongue"};
constexpr auto EyeStalks = BF{BodyPartFlag::EyeStalks, "eyestalks"};
constexpr auto Tentacles = BF{BodyPartFlag::Tentacles, "tentacles"};
constexpr auto Fins = BF{BodyPartFlag::Fins, "fins"};
constexpr auto Wings = BF{BodyPartFlag::Wings, "wings"};
constexpr auto Tail = BF{BodyPartFlag::Tail, "tail"};
constexpr auto Claws = BF{BodyPartFlag::Claws, "claws"};
constexpr auto Fangs = BF{BodyPartFlag::Fangs, "fangs"};
constexpr auto Horns = BF{BodyPartFlag::Horns, "horns"};
constexpr auto Scales = BF{BodyPartFlag::Scales, "scales"};
constexpr auto Tusks = BF{BodyPartFlag::Tusks, "tusks"};

const inline auto NumBodyParts = 22;
constexpr std::array<Flag<BodyPartFlag>, NumBodyParts> AllBodyPartFlags = {
    {Head,       Arms,      Legs,      Heart, Brains, Guts, Hands, Feet,  Fingers, Ear,    Eye,
     LongTongue, EyeStalks, Tentacles, Fins,  Wings,  Tail, Claws, Fangs, Horns,   Scales, Tusks}};

}