#pragma once

#include <cstdint>

struct Char;

// Short scalar types.
using sh_int = int16_t;
using ush_int = uint16_t;

// Function pointers for spells.
using SpellFunc = void (*)(int spell_num, int level, Char *ch, void *vo);
// Function pointer for NPC special behaviours.
using SpecialFunc = bool (*)(Char *ch);
// Function pointers for dispel magic checks
using DispelMagicFunc = bool (*)(int dis_level, Char *victim, int spell_num);
