#pragma once

#include <string>
#include <variant>

struct attack_type;
struct skill_type;

// When an attack is made via multi_hit(), one_hit() or directly to damage()
// it may be a regular physical blow (attack_type) or a particular skill.
using AttackType = std::variant<const attack_type *, const skill_type *>;

// Each time damage is delivered to a victim it strikes a semi-randomly selected
// body part, represented by this struct. Currently this is purely cosmetic and
// there's no notion of sustained damage to a part. Nor is there a notion of worn
// armour or spell effects protecting specific body parts. Those protections are
// used in calculations that benefit the entire body.
struct InjuredPart {
    const unsigned long part_flag;
    const std::string description;
};

// Combines damage related values used when generating damage messages sent to a player.
struct DamageContext {
    const int damage;
    const AttackType atk_type;
    const int dam_type;
    const bool immune;
    const InjuredPart &injured_part;
};