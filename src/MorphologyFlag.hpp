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
constexpr std::array<Flag, NumMorphologyFlags> AllMorphologyFlags = {{
    // clang-format off
    {to_int(MorphologyFlag::Edible), "edible"},
    {to_int(MorphologyFlag::Poison), "poison"},
    {to_int(MorphologyFlag::Magical), "magical"},
    {to_int(MorphologyFlag::InstantDecay), "instant_rot"},
    {to_int(MorphologyFlag::Other), "other"},
    {to_int(MorphologyFlag::Animal), "animal"},
    {to_int(MorphologyFlag::Sentient), "sentient"},
    {to_int(MorphologyFlag::Undead), "undead"},
    {to_int(MorphologyFlag::Construct), "construct"},
    {to_int(MorphologyFlag::Mist), "mist"},
    {to_int(MorphologyFlag::Intangible), "intangible"},
    {to_int(MorphologyFlag::Biped), "biped"},
    {to_int(MorphologyFlag::Centaur), "centaur"},
    {to_int(MorphologyFlag::Insect), "insect"},
    {to_int(MorphologyFlag::Spider), "spider"},
    {to_int(MorphologyFlag::Crustacean), "crustacean"},
    {to_int(MorphologyFlag::Worm), "worm"},
    {to_int(MorphologyFlag::Blob), "blob"},
    {to_int(MorphologyFlag::Mammal), "mammal"},
    {to_int(MorphologyFlag::Bird), "bird"},
    {to_int(MorphologyFlag::Reptile), "reptile"},
    {to_int(MorphologyFlag::Snake), "snake"},
    {to_int(MorphologyFlag::Dragon), "dragon"},
    {to_int(MorphologyFlag::Amphibian), "amphibian"},
    {to_int(MorphologyFlag::Fish), "fish"},
    {to_int(MorphologyFlag::ColdBlood), "cold_blooded"}
    // clang-format on
}};

}