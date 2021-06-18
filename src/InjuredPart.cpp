/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "InjuredPart.hpp"
#include "AttackType.hpp"
#include "Rng.hpp"
#include "merc.h"

namespace {

// This allows us to adjust the probability of where the char hits
// the victim. This is dependent on size, unless ch is affected by fly or
// haste in which case you are pretty likely to be able to hit them anywhere
int body_size_diff(const Char *ch, const Char *victim) {
    if (is_affected(ch, AFF_FLYING) || is_affected(ch, AFF_HASTE))
        return 0;
    return ch->size - victim->size;
}

// Small creatures are more likely to hit the lower parts of their opponent
// and less likely to hit the higher parts, and vice versa
int chance_to_hit_body_part(const int body_size_diff, const race_body_type &body_part) {
    if (body_size_diff >= 2) {
        /* i.e. if we are quite a bit bigger than them */
        switch (body_part.pos) {
        case 3: return 100; // hitting high up
        case 2: return 66;
        default: return 33;
        }
    } else if (body_size_diff <= -2) {
        /* i.e. the other way around this time */
        switch (body_part.pos) {
        case 3: return 33;
        case 2: return 66;
        default: return 100;
        }
    } else if (body_size_diff == 0)
        return 100;
    else
        return 75;
}

std::string gen_body_part_description(const race_body_type &body_part) {
    std::string desc;
    if (body_part.pair) { /* a paired part */
        switch (number_range(0, 4) % 2) {
        case 0: desc = "left "; break;
        case 1:
        default: desc = "left "; break;
        }
    }
    if (body_part.part_flag & PART_ARMS) {
        switch (number_range(0, 6) % 3) {
        case 0: desc += "bicep"; break;
        case 1: desc += "shoulder"; break;
        default: desc += "forearm"; break;
        }
    } else if (body_part.part_flag & PART_LEGS) {
        switch (number_range(0, 6) % 3) {
        case 0: desc += "thigh"; break;
        case 1: desc += "knee"; break;
        default: desc += "calf"; break;
        }
    } else {
        desc += body_part.name;
    }
    return desc;
}
}

InjuredPart InjuredPart::random_from_victim(const Char *ch, const Char *victim, const AttackType atk_type, Rng &rng) {
    if (is_attack_skill(atk_type, gsn_headbutt) || ch == victim) {
        return {PART_HEAD, "head"};
    }
    const auto size_diff = body_size_diff(ch, victim);
    for (auto tries = 0; tries < 5; tries++) {
        const auto random_part = rng.number_range(0, MAX_BODY_PARTS - 1);
        if (race_table[victim->race].parts & race_body_table[random_part].part_flag) {
            const auto chance = chance_to_hit_body_part(size_diff, race_body_table[random_part]);
            if (rng.number_percent() < chance) {
                return {race_body_table[random_part].part_flag,
                        gen_body_part_description(race_body_table[random_part])};
            }
        }
    }
    return {PART_HEAD, "head"};
}
