/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "Races.hpp"
#include "AffectFlag.hpp"
#include "BodySize.hpp"
#include "CharActFlag.hpp"
#include "OffensiveFlag.hpp"
#include "ToleranceFlag.hpp"

#include <magic_enum.hpp>

/* race table */
/* when adding a new PC race ensure that it appears towards the top
   of this list! */
const struct race_type race_table[] = {
    /*
        {
       name,    pc_race?,
       act bits,   aff_by bits,   off bits,
       imm,     res,     vuln,
       form,    parts
        },
    */
    {"unique", false, 0, 0, 0, 0, 0, 0, G, A | D | F},

    {"human", true, 0, 0, 0, 0, 0, 0, A | H | M | V, A | B | C | D | E | F | G | H | I | J | K},

    {"hobbit", true, 0, 0, 0, 0, 0, 0, A | H | M | V, A | B | C | D | E | F | G | H | I | J | K},

    {"minotaur", true, 0, 0, 0, 0, 0, 0, A | H | M | V, A | B | C | D | E | F | G | H | I | J | K | W},

    {"elf", true, 0, to_int(AffectFlag::Infrared), 0, 0, to_int(ToleranceFlag::Charm), to_int(ToleranceFlag::Iron),
     A | H | M | V, A | B | C | D | E | F | G | H | I | J | K},

    {"half-elf", true, 0, to_int(AffectFlag::Infrared), 0, 0, 0, to_int(ToleranceFlag::Iron), A | H | M | V,
     A | B | C | D | E | F | G | H | I | J | K},

    {"dragon", true, 0, 0, 0, 0, 0, 0, A | H | M | V, A | C | D | E | F | H | J | K | P | U | V | X},

    {"eagle", true, 0, to_int(AffectFlag::Flying), 0, 0, 0, 0, A | H | M | V, A | C | D | E | F | H | J | P | K | U},

    {"orc", true, 0, to_int(AffectFlag::Infrared), 0, 0, 0, 0, A | H | M | V,
     A | B | C | D | E | F | G | H | I | J | K},

    {"dwarf", true, 0, to_int(AffectFlag::Infrared), 0, 0,
     to_int(ToleranceFlag::Poison) | to_int(ToleranceFlag::Disease), to_int(ToleranceFlag::Drowning), A | H | M | V,
     A | B | C | D | E | F | G | H | I | J | K},

    {"wolf", true, 0, 0, 0, 0, 0, 0, A | H | M | V, A | C | D | E | F | H | J | K | Q | U | V},
    /* commented out for the time being --Fara
       {
       "wraith",               true,
       0,              to_int(AffectFlag::Infrared),              0,
       to_int(ToleranceFlag::Cold),              0,              to_int(ToleranceFlag::Fire),
       A|H|M|V|cc,        A|C|D|E|F|G|H|I|J|K|L|U
    }
    ,*/

    {"giant", false, 0, 0, 0, 0, to_int(ToleranceFlag::Fire) | to_int(ToleranceFlag::Cold),
     to_int(ToleranceFlag::Mental) | to_int(ToleranceFlag::Lightning), A | H | M | V,
     A | B | C | D | E | F | G | H | J | K},

    {"bat", false, 0, to_int(AffectFlag::Flying) | to_int(AffectFlag::DarkVision),
     to_int(OffensiveFlag::Dodge) | to_int(OffensiveFlag::Fast), 0, 0, to_int(ToleranceFlag::Light), A | G | W,
     A | C | D | E | F | H | J | K | P},

    {"bear", false, 0, 0, to_int(OffensiveFlag::Crush) | to_int(OffensiveFlag::Disarm) | to_int(OffensiveFlag::Berserk),
     0, to_int(ToleranceFlag::Bash) | to_int(ToleranceFlag::Cold), 0, A | G | V,
     A | B | C | D | E | F | H | J | K | U | V},

    {"cat", false, 0, to_int(AffectFlag::DarkVision), to_int(OffensiveFlag::Fast) | to_int(OffensiveFlag::Dodge), 0, 0,
     0, A | G | V, A | C | D | E | F | H | J | K | Q | U | V},

    {"centipede", false, 0, to_int(AffectFlag::DarkVision), 0, 0,
     to_int(ToleranceFlag::Pierce) | to_int(ToleranceFlag::Cold), to_int(ToleranceFlag::Bash),
     O | Y /* insect, snake  */, A | F | Q | X},

    {"dog", false, 0, 0, to_int(OffensiveFlag::Fast), 0, 0, 0, A | G | V, A | C | D | E | F | H | J | K | U | V},

    {"doll", false, 0, 0, 0, to_int(ToleranceFlag::Magic), to_int(ToleranceFlag::Bash) | to_int(ToleranceFlag::Light),
     to_int(ToleranceFlag::Slash) | to_int(ToleranceFlag::Fire) | to_int(ToleranceFlag::Acid)
         | to_int(ToleranceFlag::Lightning) | to_int(ToleranceFlag::Energy),
     E | J | M | cc, A | B | C | G | H | K},

    {"fido", false, 0, 0, to_int(OffensiveFlag::Dodge) | to_int(OffensiveFlag::AssistRace), 0, 0,
     to_int(ToleranceFlag::Magic), B | G | V, A | C | D | E | F | H | J | K | Q | V},

    {"fox", false, 0, to_int(AffectFlag::DarkVision), to_int(OffensiveFlag::Fast) | to_int(OffensiveFlag::Dodge), 0, 0,
     0, A | G | V, A | C | D | E | F | H | J | K | Q | U | V},

    {"goblin", false, 0, to_int(AffectFlag::Infrared), 0, 0, to_int(ToleranceFlag::Disease),
     to_int(ToleranceFlag::Magic), A | H | M | V, A | B | C | D | E | F | G | H | I | J | K},

    {
        "hobgoblin", false, 0, to_int(AffectFlag::Infrared), 0, 0,
        to_int(ToleranceFlag::Disease) | to_int(ToleranceFlag::Poison), 0, A | H | M | V,
        A | B | C | D | E | F | G | H | I | J | K | Q /* includes a tail */

    },

    {"kobold", false, 0, to_int(AffectFlag::Infrared), 0, 0, to_int(ToleranceFlag::Poison),
     to_int(ToleranceFlag::Magic), A | B | H | M | V, A | B | C | D | E | F | G | H | I | J | K | Q},

    {"lizard", false, 0, 0, 0, 0, to_int(ToleranceFlag::Poison), to_int(ToleranceFlag::Cold), A | G | X | cc,
     A | C | D | E | F | H | K | Q | V},

    {"modron", false, 0, to_int(AffectFlag::Infrared),
     to_int(OffensiveFlag::AssistRace) | to_int(OffensiveFlag::AssistAlign),
     to_int(ToleranceFlag::Charm) | to_int(ToleranceFlag::Disease) | to_int(ToleranceFlag::Mental)
         | to_int(ToleranceFlag::Holy) | to_int(ToleranceFlag::Negative),
     to_int(ToleranceFlag::Fire) | to_int(ToleranceFlag::Cold) | to_int(ToleranceFlag::Acid), 0, H,
     A | B | C | G | H | J | K},

    {"pig", false, 0, 0, 0, 0, 0, 0, A | G | V, A | C | D | E | F | H | J | K},

    {"rabbit", false, 0, 0, to_int(OffensiveFlag::Dodge) | to_int(OffensiveFlag::Fast), 0, 0, 0, A | G | V,
     A | C | D | E | F | H | J | K},

    {"school monster", false, to_int(CharActFlag::NoAlign), 0, 0,
     to_int(ToleranceFlag::Charm) | to_int(ToleranceFlag::Summon), 0, to_int(ToleranceFlag::Magic), A | M | V,
     A | B | C | D | E | F | H | J | K | Q | U},

    {"snake", false, 0, 0, 0, 0, to_int(ToleranceFlag::Poison), to_int(ToleranceFlag::Cold), A | G | R | X | Y | cc,
     A | D | E | F | K | L | Q | V | X},

    {"song bird", false, 0, to_int(AffectFlag::Flying), to_int(OffensiveFlag::Fast) | to_int(OffensiveFlag::Dodge), 0,
     0, 0, A | G | W, A | C | D | E | F | H | K | P},

    {"troll", false, 0,
     to_int(AffectFlag::Regeneration) | to_int(AffectFlag::Infrared) | to_int(AffectFlag::DetectHidden),
     to_int(OffensiveFlag::Berserk), 0, to_int(ToleranceFlag::Charm) | to_int(ToleranceFlag::Bash),
     to_int(ToleranceFlag::Fire) | to_int(ToleranceFlag::Acid), B | M | V,
     A | B | C | D | E | F | G | H | I | J | K | U | V},

    {"water fowl", false, 0, to_int(AffectFlag::Swim) | to_int(AffectFlag::Flying), 0, 0,
     to_int(ToleranceFlag::Drowning), 0, A | G | W, A | C | D | E | F | H | K | P | Q},

    {"wyvern", false, 0,
     to_int(AffectFlag::Flying) | to_int(AffectFlag::DetectInvis) | to_int(AffectFlag::DetectHidden),
     to_int(OffensiveFlag::Bash) | to_int(OffensiveFlag::Fast) | to_int(OffensiveFlag::Dodge),
     to_int(ToleranceFlag::Poison), 0, to_int(ToleranceFlag::Light), B | Z | cc,
     A | C | D | E | F | H | J | K | P | Q | V | X},

    {nullptr, 0, 0, 0, 0, 0, 0, 0, 0, 0}};

const struct pc_race_type pc_race_table[] = {
    {"null race", "", 0, {100, 100, 100, 100}, {""}, {13, 13, 13, 13, 13}, {18, 18, 18, 18, 18}, BodySize::Tiny},

    /*
        {
       "race name",   short name,    points,  { class multipliers },
       { bonus skills },
       { base stats },      { max stats },    size
        },
    */

    {"human", "Human", 0, {100, 100, 100, 100}, {""}, {14, 14, 14, 14, 14}, {18, 18, 18, 18, 18}, BodySize::Medium},

    {"hobbit",
     "Hobbt",
     20,
     {100, 120, 100, 120},
     {"backstab", "pick lock"},
     {13, 15, 16, 16, 14},
     {17, 19, 21, 20, 17},
     BodySize::Small},

    {"minotaur",
     "Mintr",
     15,
     {140, 100, 150, 100},
     {"headbutt"},
     {16, 14, 15, 11, 16},
     {21, 18, 19, 15, 20},
     BodySize::Large},

    {"elf",
     " Elf ",
     5,
     {100, 125, 100, 150},
     {"sneak", "hide"},
     {11, 16, 14, 16, 11},
     {16, 21, 18, 21, 15},
     BodySize::Medium},

    {"half-elf",
     "H/Elf",
     15,
     {125, 100, 100, 125},
     {"dodge"},
     {14, 15, 14, 15, 14},
     {18, 20, 18, 19, 18},
     BodySize::Medium},

    {"dragon",
     "Dragn",
     40,
     {100, 100, 100, 100},
     {"draconian", "transportation", "bash"},
     {16, 16, 16, 13, 16},
     {21, 20, 20, 16, 21},
     BodySize::Large},

    {"eagle",
     "Eagle",
     10,
     {110, 100, 100, 150},
     {"fly", "lore"},
     {13, 15, 15, 13, 14},
     {17, 20, 20, 17, 18},
     BodySize::Small},

    {"orc",
     " Orc ",
     10,
     {100, 100, 120, 100},
     {"fast healing", "berserk"},
     {16, 12, 16, 12, 16},
     {20, 16, 20, 16, 20},
     BodySize::Large},

    {"dwarf",
     "Dwarf",
     5,
     {150, 100, 120, 100},
     {"haggle", "peek"},
     {16, 12, 16, 10, 16},
     {20, 16, 20, 14, 21},
     BodySize::Medium},

    {"wolf",
     "Wolf ",
     20,
     {150, 150, 120, 100},
     {"berserk", "second attack"},
     {16, 11, 15, 16, 15},
     {21, 14, 19, 21, 19},
     BodySize::Medium},
    /* commented out for the time being --Fara
    {
       "wraith",          "Wrait",       15,       {
          120, 100, 100, 100       }
       ,
       {
          "meditation"       }
       ,
       {
          13, 16, 16, 14, 14       }
       , {
          17, 20, 20, 16, 18       }
       , BodySize::Medium
       }*/

};
