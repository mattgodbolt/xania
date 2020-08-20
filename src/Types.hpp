#pragma once

#include <cstdint>

struct CHAR_DATA;

// Short scalar types.
using sh_int = int16_t;
using ush_int = uint16_t;

// Function pointers for spells.
using SpellFunc = void (*)(int spell_num, int level, CHAR_DATA *ch, void *vo);
// Function pointer for NPC special behaviours.
using SpecialFunc = bool (*)(CHAR_DATA *ch);
