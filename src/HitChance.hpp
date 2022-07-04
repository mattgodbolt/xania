/************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                     */
/*  (C) 2022 Xania Development Team                                     */
/*  See the header to file: merc.h for original code copyrights         */
/************************************************************************/
#pragma once
#include "Char.hpp"
#include "DamageType.hpp"
#include "SkillTables.hpp"

class HitChance {
public:
    HitChance(Char &attacker, Char &victim, const int weapon_skill, const skill_type *opt_skill,
              const DamageType damage_type)
        : attacker_(attacker), victim_(victim), weapon_skill_(weapon_skill), opt_skill_(opt_skill),
          damage_type_(damage_type) {}

public:
    [[nodiscard]] int calculate() const;

private:
    [[nodiscard]] int victim_armour_class() const;
    [[nodiscard]] int to_hit_armour_class() const;
    [[nodiscard]] int to_hit_armour_class_level0() const;
    [[nodiscard]] int to_hit_armour_class_level32() const;
    const Char &attacker_;
    const Char &victim_;
    const int weapon_skill_;
    const skill_type *opt_skill_;
    const DamageType damage_type_;
};
