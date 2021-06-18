/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "InjuredPart.hpp"
#include "AttackType.hpp"
#include "Rng.hpp"
#include "merc.h"

#include <tuple>

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

std::string gen_body_part_description(const race_body_type &body_part, Rng &rng) {
    std::string desc;
    if (body_part.pair) { /* a paired part */
        switch (rng.number_range(0, 4) % 2) {
        case 0: desc = "right "; break;
        case 1:
        default: desc = "left "; break;
        }
    }
    if (body_part.part_flag & PART_ARMS) {
        switch (rng.number_range(0, 6) % 3) {
        case 0: desc += "bicep"; break;
        case 1: desc += "shoulder"; break;
        default: desc += "forearm"; break;
        }
    } else if (body_part.part_flag & PART_LEGS) {
        switch (rng.number_range(0, 6) % 3) {
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

bool InjuredPart::operator==(const InjuredPart &rhs) const {
    return std::forward_as_tuple(description, opt_spill_msg ? opt_spill_msg.value() : "",
                                 opt_spill_obj_vnum ? opt_spill_obj_vnum.value() : 0)
           == std::forward_as_tuple(rhs.description, rhs.opt_spill_msg ? rhs.opt_spill_msg.value() : "",
                                    rhs.opt_spill_obj_vnum ? rhs.opt_spill_obj_vnum.value() : 0);
}

InjuredPart InjuredPart::random_from_victim(const Char *ch, const Char *victim, const AttackType atk_type, Rng &rng) {
    if (is_attack_skill(atk_type, gsn_headbutt) || ch == victim) {
        return {"head", race_body_table[0].spill_msg, race_body_table[0].obj_vnum};
    }
    const auto size_diff = body_size_diff(ch, victim);
    for (auto tries = 0; tries < 8; tries++) {
        const auto partnum = rng.number_range(0, MAX_BODY_PARTS - 1);
        const auto &part = race_body_table[partnum];
        // Note that this looks up the victim's body parts by race, rather than reading the Char.parts field directly.
        // Which is probably more robust as the race_table is immutable.
        if (race_table[victim->race].parts & part.part_flag) {
            const auto chance = chance_to_hit_body_part(size_diff, part);
            if (rng.number_percent() < chance) {
                return {gen_body_part_description(part, rng),
                        part.spill_msg ? std::optional<std::string_view>{part.spill_msg} : std::nullopt,
                        part.obj_vnum ? std::optional<int>{part.obj_vnum} : std::nullopt};
            }
        }
    }
    return {"head", race_body_table[0].spill_msg, race_body_table[0].obj_vnum};
}

std::ostream &operator<<(std::ostream &os, const InjuredPart &part) {
    const auto str = fmt::format("description: {}, spill_msg: {}, spill_obj_vnum: {}", part.description,
                                 (part.opt_spill_msg ? *(part.opt_spill_msg) : "{}"),
                                 (part.opt_spill_obj_vnum ? part.opt_spill_obj_vnum.value() : 0));
    os << str;
    return os;
}
