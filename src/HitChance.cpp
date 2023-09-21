/************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                     */
/*  (C) 2022 Xania Development Team                                     */
/*  See the header to file: merc.h for original code copyrights         */
/************************************************************************/
#include "HitChance.hpp"
#include "Attacks.hpp"
#include "Char.hpp"
#include "CharActFlag.hpp"
#include "Class.hpp"
#include "SkillNumbers.hpp"
#include "common/BitOps.hpp"

namespace {
constexpr auto NpcToHitAC0 = -4;
constexpr auto NpcWarriorToHitAC32 = -76;
constexpr auto NpcThiefToHitAC32 = -70;
constexpr auto NpcClericToHitAC32 = -66;
constexpr auto NpcMageToHitAC32 = -63;
constexpr auto VictimPronePenalty = 0.8f;
constexpr auto VictimRelaxedPenalty = 0.9f;
constexpr auto AttackerCantSeeTargetPenalty = 0.8f;
constexpr auto HitChanceLowerBound = 10.0f;
constexpr auto HitChanceUpperBound = 95.0f;
constexpr auto BackstabSkillDivisor = 3;
}

extern int interpolate(int level, int value_00, int value_32);

int HitChance::calculate() const {
    const auto to_hit_ac = to_hit_armour_class();
    const auto victim_ac = victim_armour_class();
    const auto to_hit_ratio = ((float)to_hit_ac / (float)victim_ac) * 100;
    // Regardless of how strong to_hit_ac is relative to victim ac, there's always a chance to hit or miss.
    const auto hit_chance = std::clamp(to_hit_ratio, HitChanceLowerBound, HitChanceUpperBound);
    return hit_chance;
}

int HitChance::victim_armour_class() const {
    int victim_ac;
    switch (damage_type_) {
    case (DamageType::Pierce): victim_ac = victim_.get_armour_class(ArmourClass::Pierce); break;
    case (DamageType::Bash): victim_ac = victim_.get_armour_class(ArmourClass::Bash); break;
    case (DamageType::Slash): victim_ac = victim_.get_armour_class(ArmourClass::Slash); break;
    default: victim_ac = victim_.get_armour_class(ArmourClass::Exotic); break;
    };

    // Victim is more vulnerable if they're stunned or sleeping,
    // and a little less so if they're resting or sitting.
    if (victim_.is_pos_sleeping() || victim_.is_pos_stunned_or_dying())
        victim_ac *= VictimPronePenalty;
    else if (victim_.is_pos_relaxing())
        victim_ac *= VictimRelaxedPenalty;

    if (victim_ac >= 0) {
        victim_ac = -1;
    }
    victim_ac = -victim_ac;
    return victim_ac;
}

int HitChance::to_hit_armour_class() const {
    const auto to_hit_ac_level0 = to_hit_armour_class_level0();
    const auto to_hit_ac_level32 = to_hit_armour_class_level32();
    auto to_hit_ac = interpolate(attacker_.level, to_hit_ac_level0, to_hit_ac_level32);
    to_hit_ac *= (weapon_skill_ / 100.f);
    to_hit_ac -= attacker_.get_hitroll();
    // Blindness caused by blind spell or dirt kick reduces your chance of landing a blow
    if (!attacker_.can_see(victim_))
        to_hit_ac *= AttackerCantSeeTargetPenalty;
    if (Attacks::is_attack_skill(opt_skill_, gsn_backstab)) {
        to_hit_ac -= attacker_.get_skill(gsn_backstab) / BackstabSkillDivisor;
    }
    if (to_hit_ac >= 0) {
        to_hit_ac = -1;
    }
    to_hit_ac = -to_hit_ac;
    return to_hit_ac;
}

int HitChance::to_hit_armour_class_level0() const {
    if (attacker_.is_npc()) {
        return NpcToHitAC0;
    } else {
        return attacker_.pcdata->class_type->to_hit_ac_level0;
    }
}

int HitChance::to_hit_armour_class_level32() const {
    if (attacker_.is_npc()) {
        if (check_enum_bit(attacker_.act, CharActFlag::Warrior))
            return NpcWarriorToHitAC32;
        else if (check_enum_bit(attacker_.act, CharActFlag::Thief))
            return NpcThiefToHitAC32;
        else if (check_enum_bit(attacker_.act, CharActFlag::Cleric))
            return NpcClericToHitAC32;
        else if (check_enum_bit(attacker_.act, CharActFlag::Mage))
            return NpcMageToHitAC32;
        else
            return NpcThiefToHitAC32; // historically, NPCs without a class act flag hit like a thief
    } else {
        return attacker_.pcdata->class_type->to_hit_ac_level32;
    }
}
