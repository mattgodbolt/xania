/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "InjuredPart.hpp"
#include "AffectFlag.hpp"
#include "Attacks.hpp"
#include "BodyPartFlag.hpp"
#include "BodySize.hpp"
#include "Char.hpp"
#include "Races.hpp"
#include "Rng.hpp"
#include "SkillNumbers.hpp"
#include "VnumObjects.hpp"
#include "common/BitOps.hpp"
#include "handler.hpp"

#include <magic_enum.hpp>
#include <tuple>

namespace {

enum class PartHeight { Low = 0, Mid = 1, High = 2 };

struct body_part_attrs {
    const unsigned long part_flag; /* one of the PartFlag bits* */
    const char *name; /* verbose string */
    const bool pair; /* do we normally find a pair of these? e.g. arm*/
    const PartHeight height;
    const char *spill_msg; /* msg when victim killed */
    const int obj_vnum; /* object to deposit when dead */
};

const struct body_part_attrs body_part_attrs_table[NumBodyParts] = {
    /*
      format:
      {       BODY_PART, "description", found in a pair?, location }  */
    {to_int(BodyPartFlag::Head), "head", false, PartHeight::High, "$n's severed head plops on the ground.",
     objects::SeveredHead}, /* don't move from top of list */
    {to_int(BodyPartFlag::Arms), "arm", true, PartHeight::Mid, "$n's arm is sliced from $s dead body.",
     objects::SlicedArm}, /* double */

    {to_int(BodyPartFlag::Legs), "leg", true, PartHeight::Low, "$n's leg is sliced from $s dead body.",
     objects::SlicedLeg}, /* double */
    {to_int(BodyPartFlag::Heart), "chest", false, PartHeight::Mid, "$n's heart is torn from $s chest.",
     objects::TornHeart},

    {to_int(BodyPartFlag::Brains), "head", false, PartHeight::High,
     "$n's head is shattered, and $s brains splash all over you.", objects::Brains},

    {to_int(BodyPartFlag::Guts), "midriff", false, PartHeight::Mid, "$n spills $s guts all over the floor.",
     objects::Guts},

    {to_int(BodyPartFlag::Hands), "hand", true, PartHeight::Mid, 0, 0}, /* double */
    {to_int(BodyPartFlag::Feet), "shin", true, PartHeight::Low, 0, 0}, /* double. No feet! */
    {to_int(BodyPartFlag::Fingers), "hand", true, PartHeight::Mid, 0, 0}, /* double */
    {to_int(BodyPartFlag::Ear), "head", false, PartHeight::High, "$n's severed head plops on the ground.",
     objects::SeveredHead},
    {to_int(BodyPartFlag::Eye), "head", false, PartHeight::High, "$n's severed head plops on the ground.",
     objects::SeveredHead},
    {to_int(BodyPartFlag::LongTongue), "tongue", false, PartHeight::High, "$n's severed head plops on the ground.",
     objects::SeveredHead},
    {to_int(BodyPartFlag::EyeStalks), "eye stalks", false, PartHeight::Mid, 0, 0}, /* weird */
    {to_int(BodyPartFlag::Tentacles), "tentacles", false, PartHeight::Mid, 0, 0},
    {to_int(BodyPartFlag::Fins), "fin", false, PartHeight::Mid, 0, 0},
    {to_int(BodyPartFlag::Wings), "wings", false, PartHeight::Mid, "$n's wing is sliced off and lands with a crunch.",
     objects::SlicedWing},
    {to_int(BodyPartFlag::Tail), "tail", false, PartHeight::Low, 0, 0},
    {to_int(BodyPartFlag::Claws), "claws", false, PartHeight::Mid, "$n's claw flies off and narrowly misses you.",
     objects::SlicedClaw},
    {to_int(BodyPartFlag::Fangs), "fangs", false, PartHeight::High, 0, 0},
    {to_int(BodyPartFlag::Horns), "horn", false, PartHeight::High, 0, 0},
    {to_int(BodyPartFlag::Scales), "scales", false, PartHeight::Mid, "$n's heart is torn from $s chest.",
     objects::TornHeart},
    {to_int(BodyPartFlag::Tusks), "tusk", false, PartHeight::Mid, 0, 0}};

// This allows us to adjust the probability of where the char hits
// the victim. This is dependent on size, unless ch is affected by fly or
// haste in which case you are pretty likely to be able to hit them anywhere
int body_size_diff(const Char *ch, const Char *victim) {
    if (ch->is_aff_fly() || ch->is_aff_haste())
        return 0;
    return BodySizes::size_diff(ch->body_size, victim->body_size);
}

// Small creatures are more likely to hit the lower parts of their opponent
// and less likely to hit the higher parts, and vice versa
int chance_to_hit_body_part(const int body_size_diff, const body_part_attrs &body_part) {
    if (body_size_diff >= 2) {
        /* i.e. if we are quite a bit bigger than them */
        switch (body_part.height) {
        case PartHeight::High: return 100; // hitting high up
        case PartHeight::Mid: return 66;
        default: return 33;
        }
    } else if (body_size_diff <= -2) {
        /* i.e. the other way around this time */
        switch (body_part.height) {
        case PartHeight::High: return 33;
        case PartHeight::Mid: return 66;
        default: return 100;
        }
    } else if (body_size_diff == 0)
        return 100;
    else
        return 75;
}

std::string gen_body_part_description(const body_part_attrs &body_part, Rng &rng) {
    std::string desc;
    if (body_part.pair) { /* a paired part */
        switch (rng.number_range(0, 4) % 2) {
        case 0: desc = "right "; break;
        case 1:
        default: desc = "left "; break;
        }
    }
    if (check_enum_bit(body_part.part_flag, BodyPartFlag::Arms)) {
        switch (rng.number_range(0, 6) % 3) {
        case 0: desc += "bicep"; break;
        case 1: desc += "shoulder"; break;
        default: desc += "forearm"; break;
        }
    } else if (check_enum_bit(body_part.part_flag, BodyPartFlag::Legs)) {
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

const InjuredPart Head{"head", body_part_attrs_table[0].spill_msg, body_part_attrs_table[0].obj_vnum};

}

bool InjuredPart::operator==(const InjuredPart &rhs) const {
    return std::forward_as_tuple(description, opt_spill_msg ? opt_spill_msg.value() : "",
                                 opt_spill_obj_vnum ? opt_spill_obj_vnum.value() : 0)
           == std::forward_as_tuple(rhs.description, rhs.opt_spill_msg ? rhs.opt_spill_msg.value() : "",
                                    rhs.opt_spill_obj_vnum ? rhs.opt_spill_obj_vnum.value() : 0);
}

InjuredPart InjuredPart::random_from_victim(const Char *ch, const Char *victim, const AttackType atk_type, Rng &rng) {
    if (is_attack_skill(atk_type, gsn_headbutt) || ch == victim) {
        return Head;
    }
    const auto size_diff = body_size_diff(ch, victim);
    for (auto tries = 0; tries < 8; tries++) {
        const auto partnum = rng.number_range(0, NumBodyParts - 1);
        const auto &part = body_part_attrs_table[partnum];
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
    return Head;
}

std::ostream &operator<<(std::ostream &os, const InjuredPart &part) {
    const auto str = fmt::format("description: {}, spill_msg: {}, spill_obj_vnum: {}", part.description,
                                 (part.opt_spill_msg ? *(part.opt_spill_msg) : "{}"),
                                 (part.opt_spill_obj_vnum ? part.opt_spill_obj_vnum.value() : 0));
    os << str;
    return os;
}
