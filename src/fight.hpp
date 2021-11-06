/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

#include "Attacks.hpp"

#include <optional>
#include <string>

enum class DamageType;
struct Char;
struct skill_type;
struct InjuredPart;

std::string describe_fight_condition(const Char &victim);
bool is_safe(Char *ch, Char *victim);
bool is_safe_spell(Char *ch, Char *victim, bool area);
void violence_update();
void multi_hit(Char *ch, Char *victim);
void multi_hit(Char *ch, Char *victim, const skill_type *opt_skill);
bool damage(Char *ch, Char *victim, const int raw_damage, const AttackType attack_type, const DamageType damage_type);
void update_pos(Char *victim);
void stop_fighting(Char *ch, bool fBoth);
void raw_kill(Char *victim, std::optional<InjuredPart> opt_injured_part);
void death_cry(Char *ch);
