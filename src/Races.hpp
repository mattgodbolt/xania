/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once
#include "Constants.hpp"
#include "Stats.hpp"
#include "Types.hpp"

#include <string_view>

enum class BodySize;

struct race_type {
    const char *name; /* call name of the race */
    bool pc_race; /* can be chosen by pcs */
    unsigned long act; /* act bits for the race */
    unsigned long aff; /* aff bits for the race */
    unsigned long off; /* off bits for the race */
    unsigned long imm; /* imm bits for the race */
    unsigned long res; /* res bits for the race */
    unsigned long vuln; /* vuln bits for the race */
    unsigned long morphology; /* default morphology flag for the race */
    unsigned long parts; /* default parts for the race */
};

struct pc_race_type /* additional data for pc races */
{
    const char *name; /* MUST be in race_type */
    char who_name[6];
    sh_int points; /* cost in points of the race */
    sh_int class_mult[MAX_CLASS]; /* exp multiplier for class, * 100 */
    std::array<std::string_view, MAX_PC_RACE_BONUS_SKILLS> skills{}; /* bonus skills for the race */
    Stats stats; /* starting stats */
    Stats max_stats; /* maximum stats */
    BodySize body_size;
};

extern const struct race_type race_table[];
extern const struct pc_race_type pc_race_table[];
