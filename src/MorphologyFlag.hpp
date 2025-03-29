/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

#include "Flag.hpp"
#include "common/StandardBits.hpp"

#include <magic_enum/magic_enum.hpp>

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

using MF = Flag<MorphologyFlag>;
constexpr auto Edible = MF{MorphologyFlag::Edible, "edible"};
constexpr auto Poison = MF{MorphologyFlag::Poison, "poison"};
constexpr auto Magical = MF{MorphologyFlag::Magical, "magical"};
constexpr auto InstantDecay = MF{MorphologyFlag::InstantDecay, "instant_rot"};
constexpr auto Other = MF{MorphologyFlag::Other, "other"};
constexpr auto Animal = MF{MorphologyFlag::Animal, "animal"};
constexpr auto Sentient = MF{MorphologyFlag::Sentient, "sentient"};
constexpr auto Undead = MF{MorphologyFlag::Undead, "undead"};
constexpr auto Construct = MF{MorphologyFlag::Construct, "construct"};
constexpr auto Mist = MF{MorphologyFlag::Mist, "mist"};
constexpr auto Intangible = MF{MorphologyFlag::Intangible, "intangible"};
constexpr auto Biped = MF{MorphologyFlag::Biped, "biped"};
constexpr auto Centaur = MF{MorphologyFlag::Centaur, "centaur"};
constexpr auto Insect = MF{MorphologyFlag::Insect, "insect"};
constexpr auto Spider = MF{MorphologyFlag::Spider, "spider"};
constexpr auto Crustacean = MF{MorphologyFlag::Crustacean, "crustacean"};
constexpr auto Worm = MF{MorphologyFlag::Worm, "worm"};
constexpr auto Blob = MF{MorphologyFlag::Blob, "blob"};
constexpr auto Mammal = MF{MorphologyFlag::Mammal, "mammal"};
constexpr auto Bird = MF{MorphologyFlag::Bird, "bird"};
constexpr auto Reptile = MF{MorphologyFlag::Reptile, "reptile"};
constexpr auto Snake = MF{MorphologyFlag::Snake, "snake"};
constexpr auto Dragon = MF{MorphologyFlag::Dragon, "dragon"};
constexpr auto Amphibian = MF{MorphologyFlag::Amphibian, "amphibian"};
constexpr auto Fish = MF{MorphologyFlag::Fish, "fish"};
constexpr auto ColdBlood = MF{MorphologyFlag::ColdBlood, "cold_blooded"};

constexpr auto NumMorphologyFlags = 26;
constexpr std::array<MF, NumMorphologyFlags> AllMorphologyFlags = {
    {Edible, Poison,     Magical, InstantDecay, Other,  Animal,    Sentient,   Undead,   Construct,
     Mist,   Intangible, Biped,   Centaur,      Insect, Spider,    Crustacean, Worm,     Blob,
     Mammal, Bird,       Reptile, Snake,        Dragon, Amphibian, Fish,       ColdBlood}};

}