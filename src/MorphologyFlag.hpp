/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

#include "Flag.hpp"
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

namespace MorphologyFlags {

constexpr auto NumMorphologyFlags = 26;
constexpr std::array<Flag<MorphologyFlag>, NumMorphologyFlags> AllMorphologyFlags = {{
    // clang-format off
    {MorphologyFlag::Edible, "edible"},
    {MorphologyFlag::Poison, "poison"},
    {MorphologyFlag::Magical, "magical"},
    {MorphologyFlag::InstantDecay, "instant_rot"},
    {MorphologyFlag::Other, "other"},
    {MorphologyFlag::Animal, "animal"},
    {MorphologyFlag::Sentient, "sentient"},
    {MorphologyFlag::Undead, "undead"},
    {MorphologyFlag::Construct, "construct"},
    {MorphologyFlag::Mist, "mist"},
    {MorphologyFlag::Intangible, "intangible"},
    {MorphologyFlag::Biped, "biped"},
    {MorphologyFlag::Centaur, "centaur"},
    {MorphologyFlag::Insect, "insect"},
    {MorphologyFlag::Spider, "spider"},
    {MorphologyFlag::Crustacean, "crustacean"},
    {MorphologyFlag::Worm, "worm"},
    {MorphologyFlag::Blob, "blob"},
    {MorphologyFlag::Mammal, "mammal"},
    {MorphologyFlag::Bird, "bird"},
    {MorphologyFlag::Reptile, "reptile"},
    {MorphologyFlag::Snake, "snake"},
    {MorphologyFlag::Dragon, "dragon"},
    {MorphologyFlag::Amphibian, "amphibian"},
    {MorphologyFlag::Fish, "fish"},
    {MorphologyFlag::ColdBlood, "cold_blooded"}
    // clang-format on
}};

}