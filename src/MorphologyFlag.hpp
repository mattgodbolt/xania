/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

#include "common/StandardBits.hpp"

#include <magic_enum.hpp>

enum class MorphologyFlag : unsigned long {
    Edible = A,
    Poison = B,
    Magical = C,
    InstantDecay = D,
    Other = E, /* Defined By Material Bit */
    /* Actual Form */
    Animal = G,
    Sentient = H,
    Undead = I,
    Construct = J,
    Mist = K,
    Intangible = L,
    Biped = M,
    Centaur = N,
    Insect = O,
    Spider = P,
    Crustacean = Q,
    Worm = R,
    Blob = S,
    Mammal = V,
    Bird = W,
    Reptile = X,
    Snake = Y,
    Dragon = Z,
    Amphibian = aa,
    Fish = bb,
    ColdBlood = cc
};

[[nodiscard]] constexpr auto to_int(const MorphologyFlag flag) noexcept {
    return magic_enum::enum_integer<MorphologyFlag>(flag);
}
