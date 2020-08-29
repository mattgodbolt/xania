#pragma once

#include "Types.hpp"

// A single effect that affects an object or character.
struct AFFECT_DATA {
    AFFECT_DATA *next{};
    sh_int type{};
    sh_int level{};
    sh_int duration{};
    sh_int location{};
    sh_int modifier{};
    int bitvector{};
};
