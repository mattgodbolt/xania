#pragma once

#include <string_view>

// Apply types (for affects).
// Used in #OBJECTS, so value is important.
enum class AffectLocation {
    None = 0,
    Str = 1,
    Dex = 2,
    Int = 3,
    Wis = 4,
    Con = 5,
    Sex = 6,
    Class = 7,
    Level = 8,
    Age = 9,
    Height = 10,
    Weight = 11,
    Mana = 12,
    Hit = 13,
    Move = 14,
    Gold = 15,
    Exp = 16,
    Ac = 17,
    Hitroll = 18,
    Damroll = 19,
    SavingPara = 20,
    SavingRod = 21,
    SavingPetri = 22,
    SavingBreath = 23,
    SavingSpell = 24,
};

[[nodiscard]] std::string_view name(AffectLocation location);
