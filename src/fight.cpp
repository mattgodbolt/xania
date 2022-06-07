/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                    */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  fight.c:  combat! (yes Xania can be a violent place at times)        */
/*                                                                       */
/*************************************************************************/

#include "fight.hpp"
#include "AFFECT_DATA.hpp"
#include "AffectFlag.hpp"
#include "ArmourClass.hpp"
#include "BodySize.hpp"
#include "CharActFlag.hpp"
#include "Classes.hpp"
#include "CommFlag.hpp"
#include "DamageMessages.hpp"
#include "DamageTolerance.hpp"
#include "DamageType.hpp"
#include "Exit.hpp"
#include "ExitFlag.hpp"
#include "Format.hpp"
#include "InjuredPart.hpp"
#include "Logging.hpp"
#include "MProg.hpp"
#include "Materials.hpp"
#include "MorphologyFlag.hpp"
#include "Object.hpp"
#include "ObjectExtraFlag.hpp"
#include "ObjectIndex.hpp"
#include "ObjectType.hpp"
#include "OffensiveFlag.hpp"
#include "PlayerActFlag.hpp"
#include "Races.hpp"
#include "Room.hpp"
#include "RoomFlag.hpp"
#include "SkillNumbers.hpp"
#include "SkillTables.hpp"
#include "TimeInfoData.hpp"
#include "VnumMobiles.hpp"
#include "VnumObjects.hpp"
#include "VnumRooms.hpp"
#include "Weapon.hpp"
#include "WeaponFlag.hpp"
#include "Wear.hpp"
#include "act_comm.hpp"
#include "act_move.hpp"
#include "act_obj.hpp"
#include "act_wiz.hpp"
#include "challenge.hpp"
#include "comm.hpp"
#include "common/BitOps.hpp"
#include "db.h"
#include "handler.hpp"
#include "interp.h"
#include "lookup.h"
#include "ride.hpp"
#include "save.hpp"
#include "skills.hpp"
#include "string_utils.hpp"
#include "update.hpp"

#include <fmt/format.h>

#include <ctime>
#include <sys/types.h>

// Cap on damage deliverable by any single hit.
// Nothing scientific about this value, it was probably plucked out of thin air.
static constexpr auto DAMAGE_CAP = 1000;
static constexpr auto EXP_LOSS_ON_DEATH = 200;

void spell_poison(int spell_num, int level, Char *ch, const SpellTarget &spell_target);
void spell_plague(int spell_num, int level, Char *ch, const SpellTarget &spell_target);

/*
 * Local functions.
 */
void check_assist(Char *ch, Char *victim);
bool check_dodge(Char *ch, Char *victim);
void check_killer(Char *ch, Char *victim);
bool check_parry(Char *ch, Char *victim);
bool check_shield_block(Char *ch, Char *victim);
void announce(std::string_view buf, const Char *ch);
void death_cry(Char *ch);
void group_gain(Char *ch, Char *victim);
int xp_compute(Char *gch, Char *victim, int total_levels);
bool is_safe(Char *ch, Char *victim);
void make_corpse(Char *ch);
void one_hit(Char *ch, Char *victim, const skill_type *opt_skill);
void mob_hit(Char *ch, Char *victim, const skill_type *opt_skill);
void set_fighting(Char *ch, Char *victim);
void disarm(Char *ch, Char *victim);
void lose_level(Char *ch);

/*
 * Control the fights going on.
 * Called periodically by update_handler.
 */
void violence_update() {
    for (auto *ch : char_list) {
        auto *victim = ch->fighting;
        if (!victim || ch->in_room == nullptr)
            continue;

        if (ch->is_pos_awake() && ch->in_room == victim->in_room)
            multi_hit(ch, victim);
        else
            stop_fighting(ch, false);

        if ((victim = ch->fighting) == nullptr)
            continue;

        MProg::hitprcnt_trigger(ch, victim);
        MProg::fight_trigger(ch, victim);
        check_assist(ch, victim);
    }
}

/* for auto assisting */
void check_assist(Char *ch, Char *victim) {
    for (auto *rch : ch->in_room->people) {
        if (rch->is_pos_awake() && rch->fighting == nullptr) {

            /* quick check for ASSIST_PLAYER */
            if (ch->is_pc() && rch->is_npc() && check_enum_bit(rch->off_flags, OffensiveFlag::AssistPlayers)
                && rch->level + 6 > victim->level) {
                // moog: copied from do_emote:
                act("|W$n screams and attacks!|w", rch);
                act("|W$n screams and attacks!|w", rch, nullptr, nullptr, To::Char);
                multi_hit(rch, victim);
                continue;
            }

            /* PCs next */
            if (ch->is_pc() || ch->is_aff_charm()) {
                if (((rch->is_pc() && check_enum_bit(rch->act, PlayerActFlag::PlrAutoAssist)) || rch->is_aff_charm())
                    && is_same_group(ch, rch))
                    multi_hit(rch, victim);

                continue;
            }

            /* now check the NPC cases */

            if (ch->is_npc() && !ch->is_aff_charm())

            {
                if ((rch->is_npc() && check_enum_bit(rch->off_flags, OffensiveFlag::AssistAll))

                    || (rch->is_npc() && rch->race == ch->race
                        && check_enum_bit(rch->off_flags, OffensiveFlag::AssistRace))

                    || (rch->is_npc() && check_enum_bit(rch->off_flags, OffensiveFlag::AssistAlign)
                        && ((rch->is_good() && ch->is_good()) || (rch->is_evil() && ch->is_evil())
                            || (rch->is_neutral() && ch->is_neutral())))

                    || (rch->mobIndex == ch->mobIndex && check_enum_bit(rch->off_flags, OffensiveFlag::AssistVnum)))

                {
                    if (number_bits(1) == 0)
                        continue;

                    Char *target = nullptr;
                    int number = 0;
                    for (auto *vch : ch->in_room->people) {
                        if (can_see(rch, vch) && is_same_group(vch, victim) && number_range(0, number) == 0) {
                            target = vch;
                            number++;
                        }
                    }

                    if (target != nullptr) {
                        // moog: copied from do_emote:
                        act("|W$n screams and attacks!|w", rch);
                        act("|W$n screams and attacks!|w", rch, nullptr, nullptr, To::Char);
                        multi_hit(rch, target);
                    }
                }
            }
        }
    }
}

namespace {
std::string_view wound_for(int percent) {
    if (percent >= 100)
        return "is in excellent condition.";
    if (percent >= 90)
        return "has a few scratches.";
    if (percent >= 75)
        return "has some small wounds and bruises.";
    if (percent >= 50)
        return "has quite a few wounds.";
    if (percent >= 30)
        return "has some big nasty wounds and scratches.";
    if (percent >= 15)
        return "looks pretty hurt.";
    if (percent >= 0)
        return "is in |rawful condition|w.";
    return "is |Rbleeding to death|w.";
}

void send_dam_messages(const Char *ch, const Char *victim, const DamageMessages &messages) {
    if (ch == victim) {
        act(messages.to_room(), ch);
        act(messages.to_char(), ch, nullptr, nullptr, To::Char);
    } else {
        act(messages.to_room(), ch, nullptr, victim, To::NotVict);
        act(messages.to_char(), ch, nullptr, victim, To::Char);
        act(messages.to_victim(), ch, nullptr, victim, To::Vict);
    }
}

// Calculates the max damage that the bash skill may do based on the current chance to hit
// and the size of the attacker.
int calc_max_bash_damage(const BodySize attacker_size, const int chance) {
    return 2 + (2 * magic_enum::enum_integer<BodySize>(attacker_size) + (chance / 20));
}

// Calculates the max damage the trip skill may do based on the victim's size.
int calc_max_trip_damage(const BodySize victim_size) { return 2 + 2 * magic_enum::enum_integer<BodySize>(victim_size); }

}

std::string describe_fight_condition(const Char &victim) {
    auto percent = victim.max_hit > 0 ? victim.hit * 100 / victim.max_hit : -1;
    return fmt::format("{} {}\n\r", InitialCap{victim.short_name()}, wound_for(percent));
}

void multi_hit(Char *ch, Char *victim) { multi_hit(ch, victim, nullptr); }

/*
 * Do one group of attacks.
 */
void multi_hit(Char *ch, Char *victim, const skill_type *opt_skill) {
    int chance;

    if (!ch->in_room || !victim->in_room)
        return;

    /* if mob has sentient bit set, set victim name in memory */
    if (check_enum_bit(victim->act, CharActFlag::Sentient) && ch->is_pc()) {
        victim->sentient_victim = ch->name;
    };

    /* decrement the wait */
    if (ch->desc == nullptr)
        ch->wait = std::max(0, ch->wait - PULSE_VIOLENCE);

    /* no attacks for stunnies -- just a check */
    if (ch->is_pos_sleeping() || ch->is_pos_stunned_or_dying())
        return;

    if (ch->is_npc()) {
        mob_hit(ch, victim, opt_skill);
        return;
    }

    one_hit(ch, victim, opt_skill);

    if (ch->in_room != nullptr && victim->in_room != nullptr && ch->in_room->vnum == Rooms::ChallengeArena
        && victim->in_room->vnum == Rooms::ChallengeArena) {
        act(describe_fight_condition(*victim), ch, nullptr, victim, To::NotVict);
    }

    if (ch->fighting != victim)
        return;

    if (ch->is_aff_haste() && !ch->is_aff_lethargy())
        one_hit(ch, victim, opt_skill);

    if (ch->fighting != victim || Attacks::is_attack_skill(opt_skill, gsn_backstab))
        return;

    chance = get_skill(ch, gsn_second_attack) / 2;
    if (number_percent() < chance) {
        one_hit(ch, victim, opt_skill);
        check_improve(ch, gsn_second_attack, true, 5);
        if (ch->fighting != victim)
            return;
    }

    chance = get_skill(ch, gsn_third_attack) / 4;
    if (number_percent() < chance) {
        one_hit(ch, victim, opt_skill);
        check_improve(ch, gsn_third_attack, true, 6);
        if (ch->fighting != victim)
            return;
    }
}

/* procedure for all mobile attacks */
void mob_hit(Char *ch, Char *victim, const skill_type *opt_skill) {
    int chance, number;
    // NPCs with the backstab act bit have a chance to land a backstab on the victim if they're unhurt.
    // Typically this will be the initial blow from an aggressive mob.
    if (check_enum_bit(ch->off_flags, OffensiveFlag::Backstab) && victim->hit == victim->max_hit
        && number_percent() < ch->get_skill(gsn_backstab)) {
        one_hit(ch, victim, &skill_table[gsn_backstab]);
        return;
    } else
        one_hit(ch, victim, opt_skill);
    if (ch->fighting != victim)
        return;

    /* Area attack -- BALLS nasty! */

    if (check_enum_bit(ch->off_flags, OffensiveFlag::AreaAttack)) {
        for (auto *vch : ch->in_room->people) {
            if ((vch != victim && vch->fighting == ch))
                one_hit(ch, vch, opt_skill);
        }
    }

    if (ch->is_aff_haste() || check_enum_bit(ch->off_flags, OffensiveFlag::Fast))
        one_hit(ch, victim, opt_skill);

    // Perform no additional hits if the mob is not actually fighting the victim (a 'single blow' situation presumably)
    // or if the caller specified that backstab is being used, probably so that a backstabbing mob doesn't wreck the
    // player.
    if (ch->fighting != victim || Attacks::is_attack_skill(opt_skill, gsn_backstab))
        return;

    chance = get_skill(ch, gsn_second_attack) / 2;
    if (number_percent() < chance) {
        one_hit(ch, victim, opt_skill);
        if (ch->fighting != victim)
            return;

        // NPCs only launch a 3rd attack if their 2nd one succeeded.
        // This reduces the odds of 3 attacks (possibly 4 if they have OffensiveFlag::Fast too)
        // from landing, which can be quite nasty if it happens.
        chance = get_skill(ch, gsn_third_attack) / 4;
        if (number_percent() < chance) {
            one_hit(ch, victim, opt_skill);
            if (ch->fighting != victim)
                return;
        }
    }
    /* oh boy!  Fun stuff! */

    if (ch->wait > 0)
        return;

    number = number_range(0, 2);

    //   if (number == 1 && check_enum_bit(ch->act,CharActFlag::Mage))
    //      /*  { mob_cast_mage(ch,victim); return; } */ ;
    //
    //   if (number == 2 && check_enum_bit(ch->act,CharActFlag::Cleric))
    //      /* { mob_cast_cleric(ch,victim); return; } */ ;

    /* now for the skills */

    number = number_range(0, 7);

    switch (number) {
    case (0):
        if (check_enum_bit(ch->off_flags, OffensiveFlag::Bash))
            do_bash(ch, ArgParser(""));
        break;

    case (1):
        if (check_enum_bit(ch->off_flags, OffensiveFlag::Berserk) && !ch->is_aff_berserk())
            do_berserk(ch);
        break;

    case (2):
        if (check_enum_bit(ch->off_flags, OffensiveFlag::Disarm)
            || (get_weapon_sn(ch) != gsn_hand_to_hand
                && (check_enum_bit(ch->act, CharActFlag::Warrior) || check_enum_bit(ch->act, CharActFlag::Thief)))) {
            if (victim->is_aff_talon()) {
                act("$n tries to disarm you, but your talon like grip stops them!", ch, nullptr, victim, To::Vict);
                act("$n tries to disarm $N, but fails.", ch, nullptr, victim, To::NotVict);
            } else
                do_disarm(ch);
        }
        break;
    case (3):
        if (check_enum_bit(ch->off_flags, OffensiveFlag::Kick))
            do_kick(ch, ArgParser(""));
        break;

    case (4):
        if (check_enum_bit(ch->off_flags, OffensiveFlag::KickDirt))
            do_dirt(ch, ArgParser(""));
        break;

    case (5):
        if (check_enum_bit(ch->off_flags, OffensiveFlag::Tail)) {
            /* do_tail(ch,"") */;
        }
        break;

    case (6):
        if (check_enum_bit(ch->off_flags, OffensiveFlag::Trip))
            do_trip(ch, ArgParser(""));
        break;

    case (7):
        if (check_enum_bit(ch->off_flags, OffensiveFlag::Crush)) {
            /* do_crush(ch,"") */;
        }
        break;
    }
}

namespace {

int to_hit_armour_class_level0(const Char *ch) {
    if (ch->is_npc()) {
        return -4;
    } else {
        return class_table[ch->class_num].to_hit_ac_level0;
    }
}

int to_hit_armour_class_level32(const Char *ch) {
    if (ch->is_npc()) {
        if (check_enum_bit(ch->act, CharActFlag::Warrior))
            return -76;
        else if (check_enum_bit(ch->act, CharActFlag::Thief))
            return -70;
        else if (check_enum_bit(ch->act, CharActFlag::Cleric))
            return -66;
        else if (check_enum_bit(ch->act, CharActFlag::Mage))
            return -63;
        else
            return -70; // i.e. the same as a mob with Act Thief
    } else {
        return class_table[ch->class_num].to_hit_ac_level32;
    }
}

int calc_to_hit_armour_class(const Char *ch, const Char *victim, const int weapon_skill, const skill_type *opt_skill) {
    const auto to_hit_ac_level0 = to_hit_armour_class_level0(ch);
    const auto to_hit_ac_level32 = to_hit_armour_class_level32(ch);
    auto to_hit_ac = interpolate(ch->level, to_hit_ac_level0, to_hit_ac_level32);
    to_hit_ac *= (weapon_skill / 100.f);
    to_hit_ac -= ch->get_hitroll();
    // Blindness caused by blind spell or dirt kick reduces your chance of landing a blow
    if (!can_see(ch, victim))
        to_hit_ac *= 0.8f;
    if (Attacks::is_attack_skill(opt_skill, gsn_backstab)) {
        to_hit_ac -= get_skill(ch, gsn_backstab) / 3;
    }
    if (to_hit_ac >= 0) {
        to_hit_ac = -1;
    }
    to_hit_ac = -to_hit_ac;
    return to_hit_ac;
}

int calc_victim_armour_class(const Char *victim, const DamageType damage_type) {
    int victim_ac;
    switch (damage_type) {
    case (DamageType::Pierce): victim_ac = victim->get_armour_class(ArmourClass::Pierce); break;
    case (DamageType::Bash): victim_ac = victim->get_armour_class(ArmourClass::Bash); break;
    case (DamageType::Slash): victim_ac = victim->get_armour_class(ArmourClass::Slash); break;
    default: victim_ac = victim->get_armour_class(ArmourClass::Exotic); break;
    };

    // Victim is more vulnerable if they're stunned or sleeping,
    // and a little less so if they're resting or sitting.
    if (victim->is_pos_sleeping() || victim->is_pos_stunned_or_dying())
        victim_ac *= 0.8f;
    else if (victim->is_pos_relaxing())
        victim_ac *= 0.9f;

    if (victim_ac >= 0) {
        victim_ac = -1;
    }
    victim_ac = -victim_ac;
    return victim_ac;
}

int calc_hit_chance(const Char *ch, const Char *victim, const int weapon_skill, const skill_type *opt_skill,
                    const DamageType damage_type) {
    const auto to_hit_ac = calc_to_hit_armour_class(ch, victim, weapon_skill, opt_skill);
    const auto victim_ac = calc_victim_armour_class(victim, damage_type);
    const auto to_hit_ratio = ((float)to_hit_ac / (float)victim_ac) * 100;
    // Regardless of how strong to_hit_ac is relative to victim ac, there's always a chance to hit or miss.
    const auto hit_chance = std::clamp(to_hit_ratio, 10.0f, 95.0f);
    return hit_chance;
}

}

/*
 * Hit one guy once.
 */
void one_hit(Char *ch, Char *victim, const skill_type *opt_skill) {
    int dam;
    int diceroll;
    bool self_hitting = false;
    AFFECT_DATA *af;

    /* just in case */
    if (victim == ch || ch == nullptr || victim == nullptr)
        return;

    /*
     * Check for insanity
     */
    if ((af = find_affect(ch, gsn_insanity)) != nullptr) {
        int chance = std::clamp(af->level / 5, 2, 5);
        if (number_percent() < chance) {
            act("In your confused state, you attack yourself!", ch, nullptr, victim, To::Char);
            act("$n stumbles and in a confused state, hits $r.", ch, nullptr, victim, To::Room);
            victim = ch;
            self_hitting = true;
        }
    }

    /*
     * Can't beat a dead char!
     * Guard against weird room-leavings.
     */
    if (victim->is_pos_dead() || ch->in_room != victim->in_room)
        return;

    const auto wield = get_eq_char(ch, Wear::Wield);
    const auto atk_table_idx = (wield && wield->type == ObjectType::Weapon) ? wield->value[3] : ch->attack_type;
    const auto attack = Attacks::at(atk_table_idx);
    const auto damage_type = attack->damage_type;
    AttackType atk_type;
    if (!opt_skill) {
        atk_type = attack;
    } else {
        atk_type = opt_skill;
    }

    const auto wielding_skill_num = get_weapon_sn(ch);
    const auto weapon_skill = get_weapon_skill(ch, wielding_skill_num);
    const auto hit_chance = calc_hit_chance(ch, victim, weapon_skill, opt_skill, damage_type);
    diceroll = dice(1, 100);

    if (diceroll >= hit_chance) {
        /* Miss. */
        damage(ch, victim, 0, atk_type, damage_type);
        return;
    }

    /*
     * Hit.
     * Calc damage.
     */
    if (ch->is_npc() && wield == nullptr)
        dam = ch->damage.roll();

    else {
        if (weapon_skill != -1)
            check_improve(ch, wielding_skill_num, true, 5);
        if (wield != nullptr) {
            dam = dice(wield->value[1], wield->value[2]) * weapon_skill / 100;

            /* Sharp weapon flag implemented by Wandera */
            if ((wield != nullptr) && (wield->type == ObjectType::Weapon) && !self_hitting)
                if (check_enum_bit(wield->value[4], WeaponFlag::Sharp) && number_percent() > 98) {
                    dam *= 2;
                    act("Sunlight glints off your sharpened blade!", ch, nullptr, victim, To::Char);
                    act("Sunlight glints off $n's sharpened blade!", ch, nullptr, victim, To::NotVict);
                    act("Sunlight glints off $n's sharpened blade!", ch, nullptr, victim, To::Vict);
                }

            /* Vorpal weapon flag implemented by Wandera and Death*/
            /* Previously this quadrupled damage if you landed a lucky hit. The bonus is now a bit less overpowered. */
            if ((wield != nullptr) && (wield->type == ObjectType::Weapon)) {
                if (check_enum_bit(wield->value[4], WeaponFlag::Vorpal)) {
                    if (dam == (1 + wield->value[2]) * wield->value[1] / 2) {
                        dam *= 1.3;
                        act("With a blood curdling scream you leap forward swinging\n\ryour weapon in a great arc.", ch,
                            nullptr, victim, To::Char);
                        act("$n screams and leaps forwards swinging $s weapon in a great arc.", ch, nullptr, victim,
                            To::NotVict);
                        act("$n screams and leaps towards you swinging $s weapon in a great arc.", ch, nullptr, victim,
                            To::Vict);
                    }
                }
            }

            if (get_eq_char(ch, Wear::Shield) == nullptr) /* no shield = more */
                dam = (int)((float)dam * 21.f / 20.f);
        } else
            dam = number_range(1 + 4 * weapon_skill / 100, 2 * ch->level / 3 * weapon_skill / 100);
    }

    /*
     * Bonuses.
     */
    if (get_skill(ch, gsn_enhanced_damage) > 0) {
        diceroll = number_percent();
        if (diceroll <= get_skill(ch, gsn_enhanced_damage)) {
            check_improve(ch, gsn_enhanced_damage, true, 6);
            dam += dam * diceroll / 100;
        }
    }

    if (victim->is_pos_sleeping() || victim->is_pos_stunned_or_dying())
        dam *= 2;
    else if (victim->is_pos_relaxing())
        dam = dam * 3 / 2;

    if (wield && Attacks::is_attack_skill(opt_skill, gsn_backstab)) {
        if (wield->value[0] != 2)
            dam *= 2 + ch->level / 10;
        else
            dam *= 2 + ch->level / 8;
    }

    dam += ch->get_damroll() * std::min(100, weapon_skill) / 100;

    if (dam <= 0)
        dam = 1;
    if (dam > DAMAGE_CAP)
        dam = DAMAGE_CAP;

    damage(ch, victim, dam, atk_type, damage_type);

    if (wield == nullptr || wield->type != ObjectType::Weapon)
        return;

    auto spell_target = SpellTarget(victim);
    if ((check_enum_bit(wield->value[4], WeaponFlag::Poisoned)) && !victim->is_aff_poison()) {
        if (number_percent() > 75) {
            int p_sn = skill_lookup("poison");
            spell_poison(p_sn, wield->level, ch, spell_target);
        }
    }

    if ((check_enum_bit(wield->value[4], WeaponFlag::Plagued)) && !victim->is_aff_plague()) {
        if (number_percent() > 75) {
            int p_sn = skill_lookup("plague");
            spell_plague(p_sn, wield->level, ch, spell_target);
        }
    }
}

/**
 * Loot a corpse and sacrifice it after something dies.
 */
void loot_and_sacrifice_corpse(Char *looter, Char *victim, sh_int victim_room_vnum) {
    Object *corpse;
    if (looter->is_pc() && victim->is_npc() && looter->in_room->vnum == victim_room_vnum) {
        corpse = get_obj_list(looter, "corpse", looter->in_room->contents);
        if (check_enum_bit(looter->act, PlayerActFlag::PlrAutoLoot) && corpse
            && !corpse->contains.empty()) { /* exists and not empty */
            do_get(looter, ArgParser("all corpse"));
        }
        if (check_enum_bit(looter->act, PlayerActFlag::PlrAutoGold) && corpse && !corpse->contains.empty()
            && /* exists and not empty */
            !check_enum_bit(looter->act, PlayerActFlag::PlrAutoLoot)) {
            do_get(looter, ArgParser("gold corpse"));
        }
        if (check_enum_bit(looter->act, PlayerActFlag::PlrAutoSac)) {
            if (corpse && !corpse->contains.empty()) {
                return; /* leave if corpse has treasure */
            } else {
                do_sacrifice(looter, ArgParser("corpse"));
            }
        }
    }
}

/*
 * Inflict damage from a hit. The raw damage is adjusted based on damage cap
 * and spell effects.
 */
bool damage(Char *ch, Char *victim, const int raw_damage, const AttackType atk_type, const DamageType damage_type) {
    int temp;
    Object *wield;
    AFFECT_DATA *octarineFire;
    bool immune;
    sh_int victim_room_vnum;
    auto adjusted_damage = raw_damage;

    if (victim->is_pos_dead())
        return false;

    /*
     * Cap the damage if required, even for imms, and bug it if it's a player doing something suspicious.
     */
    if (raw_damage > DAMAGE_CAP) {
        if (ch->is_pc() && ch->is_mortal()) {
            bug("Player {} fighting {} in #{}, damage {} exceeds {} cap!", ch->name, victim->name, ch->in_room->vnum,
                raw_damage, DAMAGE_CAP);
        }
        adjusted_damage = DAMAGE_CAP;
    }

    /*
     * New code to make octarine fire a little bit more effective
     */
    if ((octarineFire = find_affect(victim, gsn_octarine_fire)) != nullptr) {

        /*
         * Increase by (level / 8) %
         */
        adjusted_damage += (int)((float)adjusted_damage * (float)octarineFire->level / 800.f);
    }

    if (victim != ch) {
        /*
         * Certain attacks are forbidden.
         * Most other attacks are returned.
         */
        if (is_safe(ch, victim))
            return false;
        check_killer(ch, victim);

        if (!victim->is_pos_stunned_or_dying()) {
            if (victim->fighting == nullptr)
                set_fighting(victim, ch);
            if (victim->timer <= 4)
                victim->position = Position::Type::Fighting;
        }

        if (!victim->is_pos_stunned_or_dying()) {
            if (ch->fighting == nullptr)
                set_fighting(ch, victim);

            /*
             * If victim is charmed, ch might attack victim's master.
             */
            if (ch->is_npc() && victim->is_npc() && victim->is_aff_charm() && victim->master != nullptr
                && victim->master->in_room == ch->in_room && number_bits(3) == 0) {
                stop_fighting(ch, false);
                multi_hit(ch, victim->master);
                return false;
            }
        }

        /*
         * More charm stuff.
         */
        if (victim->master == ch)
            stop_follower(victim);
    }

    /*
     * Inviso attacks ... not.
     */
    if (ch->is_aff_invisible()) {
        affect_strip(ch, gsn_invis);
        affect_strip(ch, gsn_mass_invis);
        clear_enum_bit(ch->affected_by, AffectFlag::Invisible);
        act("$n fades into existence.", ch);
    }

    /*
     * Damage modifiers.
     */
    if (victim->is_aff_sanctuary())
        adjusted_damage /= 1.6f;

    if (victim->is_aff_protection_evil() && ch->is_evil())
        adjusted_damage -= adjusted_damage / 4;

    if (victim->is_aff_protection_good() && ch->is_good())
        adjusted_damage -= adjusted_damage / 4;

    immune = false;

    // #257 Enhance so that some skill based attacks like bash and trip can be dodged?
    if (ch != victim && ch && std::holds_alternative<const attack_type *>(atk_type)) {
        if (check_parry(ch, victim))
            return false;
        if (check_dodge(ch, victim))
            return false;
        if (check_shield_block(ch, victim))
            return false;
    }

    switch (check_damage_tolerance(victim, damage_type)) {
    case (DamageTolerance::Immune):
        immune = true;
        adjusted_damage = 0;
        break;
    case (DamageTolerance::Resistant): adjusted_damage -= adjusted_damage / 3; break;
    case (DamageTolerance::Vulnerable): adjusted_damage += adjusted_damage / 2; break;
    default:;
    }
    if (((wield = get_eq_char(ch, Wear::Wield)) != nullptr) && check_material_vulnerability(victim, wield))
        adjusted_damage += adjusted_damage / 2;

    const auto injured_part = InjuredPart::random_from_victim(ch, victim, atk_type, Rng::global_rng());
    const DamageContext dam_context{adjusted_damage, atk_type, damage_type, immune, injured_part};
    const auto dam_messages = DamageMessages::create(ch, victim, dam_context, Rng::global_rng());
    send_dam_messages(ch, victim, dam_messages);

    if (adjusted_damage == 0)
        return false;

    /*
     * Hurt the victim.
     * Inform the victim of his new state.
     */
    victim->hit -= adjusted_damage;
    if (victim->is_pc() && victim->level >= LEVEL_IMMORTAL && victim->hit < 1)
        victim->hit = 1;
    update_pos(victim);

    wield = get_eq_char(ch, Wear::Wield);

    if ((wield != nullptr) && (wield->type == ObjectType::Weapon)
        && (check_enum_bit(wield->value[4], WeaponFlag::Vampiric))) {
        ch->hit += (adjusted_damage / 100) * 10;
        victim->hit -= (adjusted_damage / 100) * 10;
    }

    switch (victim->position) {
    case Position::Type::Mortal:
        act("|r$n is mortally wounded, and will die soon, if not aided.|w", victim);
        victim->send_line("|rYou are mortally wounded, and will die soon, if not aided.|w");
        break;

    case Position::Type::Incap:
        act("|r$n is incapacitated and will slowly die, if not aided.|w", victim);
        victim->send_line("|rYou are incapacitated and will slowly die, if not aided.|w");
        break;

    case Position::Type::Stunned:
        act("|r$n is stunned, but will probably recover.|w", victim);
        victim->send_line("|rYou are stunned, but will probably recover.|w");
        break;

    case Position::Type::Dead:
        act("|R$n is DEAD!!|w", victim);
        victim->send_line("|RYou have been KILLED!!|w");
        break;

    default:
        if (adjusted_damage > victim->max_hit / 4)
            victim->send_line("That really did |RHURT|w!");
        if (victim->hit < victim->max_hit / 4)
            victim->send_line("You sure are |RBLEEDING|w!");
        break;
    }

    /*
     * Sleep spells and extremely wounded folks.
     */
    if (!victim->is_pos_awake())
        stop_fighting(victim, false);

    /*
     * Payoff for killing things.
     */
    if (victim->is_pos_dead()) {
        group_gain(ch, victim);

        if (victim->is_pc()) {
            auto exp_level = exp_per_level(victim, victim->pcdata->points);
            auto exp_total = (victim->level * exp_level);
            temp = do_check_chal(victim);
            if (temp == 1)
                return true;

            log_string("{} killed by {} at {}", victim->name, ch->short_name(), victim->in_room->vnum);
            announce(fmt::format("|P###|w Sadly, {} was killed by {}.", victim->name, ch->short_name()), victim);

            for (auto *squib : victim->in_room->people) {
                if ((squib->is_npc()) && (squib->mobIndex->vnum == Mobiles::LesserMinionDeath)) {
                    act("$n swings his scythe and ushers $N's soul into the next world.", squib, nullptr, victim,
                        To::Room);
                    break;
                }
            }

            /*
             * Apply the death penalty.
             */
            if (victim->level >= 26) {
                if (victim->exp - EXP_LOSS_ON_DEATH < exp_total) {
                    victim->send_line("You lose a level!!  ");
                    victim->level -= 1;
                    lose_level(victim);
                }
            }
            gain_exp(victim, -EXP_LOSS_ON_DEATH);
        } else {

            if (victim->level >= (ch->level + 30)) {
                bug("|R### {} just killed {} - {} levels above them!|w", ch->short_name(), victim->short_descr,
                    victim->level - ch->level);
            }
        }
        victim_room_vnum = victim->in_room->vnum;
        raw_kill(victim, injured_part);

        for (auto *mob : char_list)
            if (mob->is_npc() && check_enum_bit(mob->act, CharActFlag::Sentient)
                && matches(mob->sentient_victim, victim->name))
                mob->sentient_victim.clear();
        /**
         * If the final blow was a pet or charmed mob, its greedy master gets to autoloot.
         */
        Char *looter = (ch->is_aff_charm() && !(ch->master->is_npc())) ? ch->master : ch;
        loot_and_sacrifice_corpse(looter, victim, victim_room_vnum);
        return true;
    }

    if (victim == ch)
        return true;

    /*
     * Take care of link dead people.
     */
    if (victim->is_pc() && victim->desc == nullptr) {
        if (number_range(0, victim->wait) == 0) {
            do_recall(victim, ArgParser(""));
            return true;
        }
    }

    /*
     * Wimp out?
     */
    if (victim->is_npc() && adjusted_damage > 0 && victim->wait < PULSE_VIOLENCE / 2) {
        if ((check_enum_bit(victim->act, CharActFlag::Wimpy) && number_bits(2) == 0
             && victim->hit < victim->max_hit / 5)
            || (victim->is_aff_charm() && victim->master != nullptr && victim->master->in_room != victim->in_room))
            do_flee(victim);
    }

    if (victim->is_pc() && victim->hit > 0 && victim->hit <= victim->wimpy && victim->wait < PULSE_VIOLENCE / 2)
        do_flee(victim);

    /*
     *  Check for being thrown from your horse/other mount
     */

    if ((victim->riding != nullptr) && (ch != nullptr)) {
        int fallchance;
        if (victim->is_npc()) {
            fallchance = std::clamp(victim->level * 2, 2, 98);
        } else {
            fallchance = victim->get_skill(gsn_ride);
        }
        switch (damage_type) {
        case DamageType::Bash: break;
        case DamageType::Pierce:
        case DamageType::Slash:
        case DamageType::Energy:
        case DamageType::Harm: fallchance += 10; break;
        default: fallchance += 40;
        }
        fallchance += (victim->level - ch->level) / 4;
        fallchance = std::clamp(fallchance, 20, 100);
        if (number_percent() > fallchance) {
            /* Oh dear something nasty happened to you ...
               You fell off your horse! */
            thrown_off(victim, victim->riding);
        } else {
            check_improve(victim, gsn_ride, true, 4);
        }
    }
    return true;
}

bool is_safe(Char *ch, Char *victim) {
    /* no killing in shops hack */
    if (victim->is_npc() && victim->mobIndex->shop) {
        ch->send_line("The shopkeeper wouldn't like that.");
        return true;
    }
    /* no killing healers, adepts, etc */
    if (victim->is_npc()
        && (check_enum_bit(victim->act, CharActFlag::Train) || check_enum_bit(victim->act, CharActFlag::Practice)
            || check_enum_bit(victim->act, CharActFlag::Healer))) {
        ch->send_line("I don't think {} would approve!", deity_name);
        return true;
    }

    /* no fighting in safe rooms */
    if (check_enum_bit(ch->in_room->room_flags, RoomFlag::Safe)) {
        ch->send_line("Not in this room.");
        return true;
    }

    if (victim->fighting == ch)
        return false;

    if (ch->is_npc()) {
        /* charmed mobs and pets cannot attack players */
        if (victim->is_pc() && (ch->is_aff_charm() || check_enum_bit(ch->act, CharActFlag::Pet)))
            return true;

        return false;
    }

    else /* Not NPC */
    {
        if (ch->is_immortal())
            return false;

        /* no pets */
        if (victim->is_npc() && check_enum_bit(victim->act, CharActFlag::Pet)) {
            act("But $N looks so cute and cuddly...", ch, nullptr, victim, To::Char);
            return true;
        }

        /* no charmed mobs unless char is the the owner */
        if (victim->is_aff_charm() && ch != victim->master) {
            ch->send_line("You don't own that monster.");
            return true;
        }

        /* no player killing */
        if (victim->is_pc() && !fighting_duel(ch, victim)) {
            ch->send_line("Sorry, player killing is not permitted.");
            return true;
        }

        return false;
    }
}

bool is_safe_spell(Char *ch, Char *victim, bool area) {
    /* can't zap self (crash bug) */
    if (ch == victim)
        return true;
    /* immortals not hurt in area attacks */
    if (victim->is_immortal() && area)
        return true;

    /* no killing in shops hack */
    if (victim->is_npc() && victim->mobIndex->shop)
        return true;

    /* no killing healers, adepts, etc */
    if (victim->is_npc()
        && (check_enum_bit(victim->act, CharActFlag::Train) || check_enum_bit(victim->act, CharActFlag::Practice)
            || check_enum_bit(victim->act, CharActFlag::Healer)))
        return true;

    /* no fighting in safe rooms */
    if (check_enum_bit(ch->in_room->room_flags, RoomFlag::Safe))
        return true;

    if (victim->fighting == ch)
        return false;

    if (ch->is_npc()) {
        /* charmed mobs and pets cannot attack players */
        if (victim->is_pc() && (ch->is_aff_charm() || check_enum_bit(ch->act, CharActFlag::Pet)))
            return true;

        /* area affects don't hit other mobiles */
        if (victim->is_npc() && area)
            return true;

        return false;
    }

    else /* Not NPC */
    {
        if (ch->is_immortal() && !area)
            return false;

        /* no pets */
        if (victim->is_npc() && check_enum_bit(victim->act, CharActFlag::Pet))
            return true;

        /* no charmed mobs unless char is the the owner */
        if (victim->is_aff_charm() && ch != victim->master)
            return true;

        /* no player killing */
        if (victim->is_pc())
            return true;

        /* cannot use spells if not in same group */
        if (victim->fighting != nullptr && !is_same_group(ch, victim->fighting))
            return true;

        return false;
    }
}

/*
 * See if an attack justifies a KILLER flag.
 */
void check_killer(Char *ch, Char *victim) {
    /*
     * Follow charm thread to responsible character.
     * Attacking someone's charmed char is hostile!
     */
    while (victim->is_aff_charm() && victim->master != nullptr)
        victim = victim->master;

    /*
     * NPC's are fair game.
     * So are killers and thieves.
     */
    if (victim->is_npc() || check_enum_bit(victim->act, PlayerActFlag::PlrKiller)
        || check_enum_bit(victim->act, PlayerActFlag::PlrThief))
        return;

    /*
     * Charm-o-rama.
     */
    if (check_enum_bit(ch->affected_by, AffectFlag::Charm)) {
        if (ch->master == nullptr) {
            bug("Check_killer: {} bad AffectFlag::Charm", ch->short_name());
            affect_strip(ch, gsn_charm_person);
            clear_enum_bit(ch->affected_by, AffectFlag::Charm);
            return;
        }
        /*
          ch->master ->send_to( "*** You are now a KILLER!! ***\n\r");
                  set_enum_bit(ch->master->act, PlayerActFlag::PlrKiller);
        */
        stop_follower(ch);
        return;
    }

    /*
     * NPC's are cool of course (as long as not charmed).
     * Hitting yourself is cool too (bleeding).
     * So is being immortal (Alander's idea).
     * And current killers stay as they are.
     */
    if (ch->is_npc() || ch == victim || ch->level >= LEVEL_IMMORTAL || check_enum_bit(ch->act, PlayerActFlag::PlrKiller)
        || fighting_duel(ch, victim))
        return;

    ch->send_line("*** You are now a KILLER!! ***");
    set_enum_bit(ch->act, PlayerActFlag::PlrKiller);
    save_char_obj(ch);
}

namespace {
bool is_wielding_whip(Char *attacker) {
    if (const auto *weapon = get_eq_char(attacker, Wear::Wield)) {
        return Weapons::try_from_integer(weapon->value[0]) == Weapon::Whip;
    } else {
        return false;
    }
}
}

/*
 * Check for parry. Mod by Faramir 10/8/96 so whips can't be blocked
 * or parried. mwahahaha
 */
bool check_parry(Char *ch, Char *victim) {
    if (!victim->is_pos_awake())
        return false;
    if (get_eq_char(victim, Wear::Wield) == nullptr)
        return false;
    if (is_wielding_whip(ch)) {
        return false;
    }
    auto chance = victim->get_skill(gsn_parry) / 3;
    chance = std::max(5, chance + victim->level - ch->level);
    if (number_percent() >= chance)
        return false;
    if (check_enum_bit(victim->comm, CommFlag::ShowDefence))
        act("You parry $n's attack.", ch, nullptr, victim, To::Vict);
    if (check_enum_bit(ch->comm, CommFlag::ShowDefence))
        act("$N parries your attack.", ch, nullptr, victim, To::Char);
    check_improve(victim, gsn_parry, true, 6);
    return true;
}

/*
 * Check for shield block.
 */
bool check_shield_block(Char *ch, Char *victim) {
    if (!victim->is_pos_awake())
        return false;
    if (get_eq_char(victim, Wear::Shield) == nullptr)
        return false;
    if (is_wielding_whip(ch)) {
        return false;
    }
    auto chance = victim->get_skill(gsn_shield_block) / 3;
    chance = std::max(5, chance + victim->level - ch->level);
    if (number_percent() >= chance)
        return false;

    if (check_enum_bit(victim->comm, CommFlag::ShowDefence))
        act("You block $n's attack.", ch, nullptr, victim, To::Vict);
    if (check_enum_bit(ch->comm, CommFlag::ShowDefence))
        act("$N blocks your attack.", ch, nullptr, victim, To::Char);
    check_improve(victim, gsn_shield_block, true, 6);
    return true;
}

/*
 * Check for dodge.
 */
bool check_dodge(Char *ch, Char *victim) {
    if (!victim->is_pos_awake())
        return false;

    auto chance = victim->get_skill(gsn_dodge) / 3;
    auto ddex = get_curr_stat(victim, Stat::Dex) - get_curr_stat(ch, Stat::Dex);
    chance += ddex;
    chance = std::max(5, chance + victim->level - ch->level);
    if (number_percent() >= chance)
        return false;

    if (check_enum_bit(victim->comm, CommFlag::ShowDefence))
        act("You dodge $n's attack.", ch, nullptr, victim, To::Vict);
    if (check_enum_bit(ch->comm, CommFlag::ShowDefence))
        act("$N dodges your attack.", ch, nullptr, victim, To::Char);
    check_improve(victim, gsn_dodge, true, 6);
    return true;
}

/*
 * Set position of a victim.
 */
void update_pos(Char *victim) {
    if (victim->hit > 0) {
        if (victim->is_pos_stunned_or_dying())
            victim->position = Position::Type::Standing;
        return;
    }

    if (victim->is_npc() && victim->hit < 1) {
        victim->position = Position::Type::Dead;
        return;
    }

    if (victim->hit <= -11) {
        victim->position = Position::Type::Dead;
        return;
    }

    if (victim->hit <= -6)
        victim->position = Position::Type::Mortal;
    else if (victim->hit <= -3)
        victim->position = Position::Type::Incap;
    else
        victim->position = Position::Type::Stunned;
}

/*
 * Start fights.
 */
void set_fighting(Char *ch, Char *victim) {

    if (ch->fighting != nullptr) {
        bug("Set_fighting: already fighting");
        return;
    }

    if (ch->is_aff_sleep())
        affect_strip(ch, gsn_sleep);

    ch->fighting = victim;
    ch->position = Position::Type::Fighting;
}

/*
 * Stop fights.
 */
void stop_fighting(Char *ch, bool fBoth) {
    for (auto *fch : char_list) {
        if (fch == ch || (fBoth && fch->fighting == ch)) {
            fch->fighting = nullptr;
            fch->position = fch->is_npc() ? ch->default_pos : Position::Type::Standing;
            update_pos(fch);
        }
    }
}

/*
 * Make a corpse out of a character.
 */
void make_corpse(Char *ch) {
    std::string name;

    Object *corpse{};
    if (ch->is_npc()) {
        name = ch->short_descr;
        corpse = create_object(get_obj_index(Objects::NonPlayerCorpse));
        corpse->timer = number_range(3, 6);
        if (ch->gold > 0) {
            obj_to_obj(create_money(ch->gold), corpse);
            ch->gold = 0;
        }
        corpse->cost = 0;
    } else {
        name = ch->name;
        corpse = create_object(get_obj_index(Objects::PlayerCorpse));
        corpse->timer = number_range(25, 40);
        clear_enum_bit(ch->act, PlayerActFlag::PlrCanLoot);
        if (!check_enum_bit(ch->act, PlayerActFlag::PlrKiller) && !check_enum_bit(ch->act, PlayerActFlag::PlrThief))
            corpse->owner = ch->name;
        corpse->cost = 0;
    }

    corpse->level = ch->level;

    corpse->short_descr = fmt::sprintf(corpse->short_descr, name);
    corpse->description = fmt::sprintf(corpse->description, name);

    for (auto *obj : ch->carrying) {
        obj_from_char(obj);
        if (obj->type == ObjectType::Potion)
            obj->timer = number_range(500, 1000);
        if (obj->type == ObjectType::Scroll)
            obj->timer = number_range(1000, 2500);
        if (check_enum_bit(obj->extra_flags, ObjectExtraFlag::RotDeath))
            obj->timer = number_range(5, 10);
        clear_enum_bit(obj->extra_flags, ObjectExtraFlag::VisDeath);
        clear_enum_bit(obj->extra_flags, ObjectExtraFlag::RotDeath);

        if (check_enum_bit(obj->extra_flags, ObjectExtraFlag::Inventory))
            extract_obj(obj);
        else
            obj_to_obj(obj, corpse);
    }

    obj_to_room(corpse, ch->in_room);
}

void death_cry(Char *ch) {
    Room *was_in_room;
    std::string_view msg;

    switch (number_range(0, 1)) {
    case 1:
        if (ch->material == Material::Type::None) {
            msg = "$n splatters blood on your armor.";
            break;
        }
        // fall through
    default: msg = "$n hits the ground ... DEAD."; break;
    }

    act(msg, ch);
    if (ch->is_npc())
        msg = "You hear something's death cry.";
    else
        msg = "You hear someone's death cry.";

    was_in_room = ch->in_room;
    for (auto direction : all_directions) {
        if (const auto &exit = was_in_room->exits[direction];
            exit && exit->u1.to_room != nullptr && exit->u1.to_room != was_in_room) {
            ch->in_room = exit->u1.to_room;
            act(msg, ch);
        }
    }
    ch->in_room = was_in_room;
}

void detach_injured_part(const Char *victim, std::optional<InjuredPart> opt_injured_part) {
    if (!opt_injured_part) {
        return;
    }
    const auto part = *opt_injured_part;
    if (part.opt_spill_msg) {
        act(part.opt_spill_msg.value(), victim);
    }
    if (part.opt_spill_obj_vnum) {
        const auto vnum = part.opt_spill_obj_vnum.value();
        auto *obj = create_object(get_obj_index(vnum));
        obj->timer = number_range(4, 7);
        obj->short_descr = fmt::sprintf(obj->short_descr, victim->short_name());
        obj->description = fmt::sprintf(obj->description, victim->short_name());

        if (obj->type == ObjectType::Food) {
            if (check_enum_bit(victim->morphology, MorphologyFlag::Poison))
                obj->value[3] = 1;
            else if (!check_enum_bit(victim->morphology, MorphologyFlag::Edible))
                obj->type = ObjectType::Trash;
        }

        obj_to_room(obj, victim->in_room);
    }
}

void raw_kill(Char *victim, std::optional<InjuredPart> opt_injured_part) {
    stop_fighting(victim, true);
    MProg::death_trigger(victim);
    death_cry(victim);
    detach_injured_part(victim, opt_injured_part);

    if (!in_duel(victim))
        make_corpse(victim);

    if (victim->is_npc()) {
        victim->mobIndex->killed++;
        extract_char(victim, true);
        return;
    }

    if (!in_duel(victim))
        extract_char(victim, false);
    for (auto &af : victim->affected)
        affect_remove(victim, af);
    victim->affected_by = race_table[victim->race].aff;
    if (!in_duel(victim))
        victim->armor.fill(-1);
    victim->position = Position::Type::Resting;
    victim->hit = std::max(1_s, victim->hit);
    victim->mana = std::max(1_s, victim->mana);
    victim->move = std::max(1_s, victim->move);
    /* RT added to prevent infinite deaths */
    if (!in_duel(victim)) {
        clear_enum_bit(victim->act, PlayerActFlag::PlrKiller);
        clear_enum_bit(victim->act, PlayerActFlag::PlrThief);
        clear_enum_bit(victim->act, PlayerActFlag::PlrBoughtPet);
    }
    /*  save_char_obj( victim ); */
}

void group_gain(Char *ch, Char *victim) {
    /*
     * Monsters don't get kill xp's or alignment changes.
     * P-killing doesn't help either.
     * Dying of mortal wounds or poison doesn't give xp to anyone!
     */
    if (victim->is_pc() || victim == ch)
        return;

    auto members = 0;
    auto group_levels = 0;
    for (auto *gch : ch->in_room->people) {
        if (is_same_group(gch, ch)) {
            members++;
            group_levels += gch->level;
        }
    }

    if (members == 0) {
        bug("Group_gain: members.");
        members = 1;
        group_levels = ch->level;
    }
    for (auto *gch : ch->in_room->people) {
        if (!is_same_group(gch, ch) || gch->is_npc())
            continue;
        /*
           Basic exp should be based on the highest level PC in the Group
        */
        const auto xp = xp_compute(gch, victim, group_levels);
        gch->send_line(fmt::format("You receive {} experience points.", xp));
        gain_exp(gch, xp);
        for (auto *obj : ch->carrying) {
            if (obj->wear_loc == Wear::None)
                continue;

            if ((obj->is_anti_evil() && ch->is_evil()) || (obj->is_anti_good() && ch->is_good())
                || (obj->is_anti_neutral() && ch->is_neutral())) {
                act("You are zapped by $p.", ch, obj, nullptr, To::Char);
                act("$n is zapped by $p.", ch, obj, nullptr, To::Room);
                obj_from_char(obj);
                obj_to_room(obj, ch->in_room);
            }
        }
    }
}

/*
 * Compute xp for a kill.
 * Also adjust alignment of killer.
 * Edit this function to change xp computations.
 */
int xp_compute(Char *gch, Char *victim, int total_levels) {
    int xp, base_exp;
    int align, level_range;
    int change;
    int base_level = gch->level;
    int highest_level = 0;
    int time_per_level = 0;

    if (total_levels > gch->level) /* Must be in a group */
    {
        /* Find level of highest PC in group */
        for (auto *tmpch : gch->in_room->people)
            if (is_same_group(tmpch, gch))
                if (tmpch->level > highest_level)
                    highest_level = tmpch->level;

        base_level = highest_level;
    }

    level_range = victim->level - base_level;

    /* compute the base exp */
    switch (level_range) {
    default: base_exp = 0; break;
    case -9: base_exp = 1; break;
    case -8: base_exp = 2; break;
    case -7: base_exp = 5; break;
    case -6: base_exp = 9; break;
    case -5: base_exp = 11; break;
    case -4: base_exp = 22; break;
    case -3: base_exp = 33; break;
    case -2: base_exp = 55; break;
    case -1: base_exp = 72; break;
    case 0: base_exp = 85; break;
    case 1: base_exp = 100; break;
    case 2: base_exp = 120; break;
    case 3: base_exp = 144; break;
    case 4: base_exp = 160; break;
    }

    if (level_range > 4)
        base_exp = 150 + 20 * (level_range - 4);

    /* do alignment computations */

    align = victim->alignment - gch->alignment;

    if (check_enum_bit(victim->act, CharActFlag::NoAlign)) {
        /* no change */
    } else if (align > Alignment::Good) { /* monster is more good than slayer */
        change = (align - Alignment::Good) * base_exp / Alignment::Good * base_level / total_levels;
        change = std::max(1, change);
        gch->alignment = std::max(Alignment::Satanic, static_cast<sh_int>(gch->alignment - change));
    } else if (align < Alignment::Evil) { /* monster is more evil than slayer */
        change = (-1 * align - Alignment::Evil) * base_exp / Alignment::Good * base_level / total_levels;
        change = std::max(1, change);
        gch->alignment = std::min(Alignment::Angelic, static_cast<sh_int>(gch->alignment + change));
    } else { /* improve this someday */
        change = gch->alignment * base_exp / Alignment::Good * base_level / total_levels;
        gch->alignment -= change;
    }

    /* Calculate exp multiplier. The principle is, attackers get a larger bonus
       when slaying enemies that are of the opposite alignment, and are slightly
       penalised when slaying enemies of similar alignment.
    */
    if (check_enum_bit(victim->act, CharActFlag::NoAlign))
        xp = base_exp;
    else if (gch->alignment > Alignment::Good) { /* for goodie two shoes */
        if (victim->alignment < Alignment::Demonic)
            xp = base_exp * 1.25f;

        else if (victim->alignment < Alignment::Evil)
            xp = base_exp * 1.13f;

        else if (victim->alignment > Alignment::Saintly)
            xp = base_exp * 0.75f;

        else if (victim->alignment > Alignment::Good)
            xp = base_exp * 0.85f;

        else if ((victim->alignment > Alignment::Kind))
            xp = base_exp * 0.90f;

        else
            xp = base_exp;
    } else if (gch->alignment < Alignment::Evil) { /* for baddies */
        if (victim->alignment > Alignment::Saintly)
            xp = base_exp * 1.25f;

        else if (victim->alignment > Alignment::Good)
            xp = base_exp * 1.13f;

        else if (victim->alignment < Alignment::Demonic)
            xp = base_exp * 0.75f;

        else if (victim->alignment < Alignment::Evil)
            xp = base_exp * 0.85f;

        else if (victim->alignment < Alignment::Mean)
            xp = base_exp * 0.90f;

        else
            xp = base_exp;
    } else if (gch->alignment > Alignment::Amiable) { /* a little good */
        if (victim->alignment < Alignment::Evil)
            xp = base_exp * 1.17f;

        else if (victim->alignment > Alignment::Saintly)
            xp = base_exp * 0.75f;

        else if (victim->alignment > Alignment::Neutral)
            xp = base_exp * 0.90;

        else
            xp = base_exp;
    } else if (gch->alignment < Alignment::Depraved) { /* a little bad */
        if (victim->alignment > Alignment::Good)
            xp = base_exp * 1.17f;

        else if (victim->alignment < Alignment::Demonic)
            xp = base_exp * 0.75f;

        else if (victim->alignment < Alignment::Neutral)
            xp = base_exp * 0.90f;

        else
            xp = base_exp;
    } else { /* neutral */
        if (victim->alignment > Alignment::Good || victim->alignment < Alignment::Evil)
            xp = base_exp * 1.25f;

        else if (victim->alignment > Alignment::Amiable || victim->alignment < Alignment::Depraved)
            xp = base_exp * 1.13f;

        else
            xp = base_exp;
    }

    /* more exp at the low levels */
    if (base_level < 6)
        xp = 10 * xp / (base_level + 4);

    /* we have to scale down XP at higher levels as players progressively
       fight against tougher opponents and the original 60 level mud
       calculations don't scale up right */
    /* less at high  -- modified 23/01/2000 --Fara */
    /* level 35, shave off 11%, level 69, shave off 23%, level 91 36% */

    if (base_level > 35 && base_level < 70)
        xp = (xp * 100) / (base_level / 3 + 100);

    if (base_level >= 70)
        xp = (xp * 100) / (base_level / 2.5 + 100);

    /* compute quarter-hours per level */

    using namespace std::chrono;
    time_per_level = 4 * duration_cast<hours>(gch->total_played()).count() / base_level;

    /* ensure minimum of 6 quarts (1.5 hours) per level */
    time_per_level = std::clamp(time_per_level, 2, 6);

    if (base_level < 15)
        time_per_level = std::max(time_per_level, (8 - base_level));
    xp = xp * time_per_level / 6;

    /* randomize the rewards */
    xp = number_range(xp * 3 / 4, xp * 5 / 4);

    /* adjust for grouping */
    xp = (xp * gch->level / total_levels) * 1.5;

    return xp;
}

/*
 * Disarm a creature.
 * Caller must check for successful attack.
 */
void disarm(Char *ch, Char *victim) {
    Object *obj;

    if ((obj = get_eq_char(victim, Wear::Wield)) == nullptr)
        return;

    if (obj->is_no_remove()) {
        act("$S weapon won't budge!", ch, nullptr, victim, To::Char);
        act("$n tries to disarm you, but your weapon won't budge!", ch, nullptr, victim, To::Vict);
        act("$n tries to disarm $N, but fails.", ch, nullptr, victim, To::NotVict);
        return;
    }

    act("|W$n disarms you and sends your weapon flying!|w", ch, nullptr, victim, To::Vict);
    act("|WYou disarm $N!|w", ch, nullptr, victim, To::Char);
    act("|W$n disarms $N!|w", ch, nullptr, victim, To::NotVict);

    obj_from_char(obj);
    if (obj->is_no_drop() || obj->is_inventory())
        obj_to_char(obj, victim);
    else {
        obj_to_room(obj, victim->in_room);
        if (victim->is_npc() && victim->wait == 0 && can_see_obj(victim, obj))
            get_obj(victim, obj, nullptr);
    }
}

void do_berserk(Char *ch) {
    int chance, hp_percent;
    /*    Object *wield = get_eq_char( ch, Wear::Wield );*/

    if ((chance = get_skill(ch, gsn_berserk)) == 0
        || (ch->is_npc() && !check_enum_bit(ch->off_flags, OffensiveFlag::Berserk))
        || (ch->is_pc() && ch->level < get_skill_level(ch, gsn_berserk))) {
        ch->send_line("You turn red in the face, but nothing happens.");
        return;
    }

    if (ch->is_aff_berserk() || ch->is_affected_by(gsn_berserk) || ch->is_affected_by(skill_lookup("frenzy"))) {
        ch->send_line("|rYou get a little madder|r.");
        return;
    }

    if (ch->is_aff_calm()) {
        ch->send_line("You're feeling too mellow to berserk.");
        return;
    }

    if (ch->mana < 50) {
        ch->send_line("You can't get up enough energy.");
        return;
    }

    /* modifiers */

    if (ch->is_pos_fighting())
        chance += 10;

    /* damage -- below 50% of hp helps, above hurts */
    hp_percent = 100 * ch->hit / ch->max_hit;
    chance += 25 - hp_percent / 2;

    if (number_percent() < chance) {
        AFFECT_DATA af;

        ch->wait_state(PULSE_VIOLENCE);
        ch->mana -= 50;
        ch->move /= 2;

        /* heal a little damage */
        ch->hit += ch->level * 2;
        ch->hit = std::min(ch->hit, ch->max_hit);

        ch->send_line("|RYour pulse races as you are consumed by rage!|w");
        act("$n gets a wild look in $s eyes.", ch);
        check_improve(ch, gsn_berserk, true, 2);

        af.type = gsn_berserk;
        af.level = ch->level;
        af.duration = number_fuzzy(ch->level / 8);
        af.modifier = std::max(1, ch->level / 5);
        af.bitvector = to_int(AffectFlag::Berserk);

        af.location = AffectLocation::Hitroll;
        affect_to_char(ch, af);

        af.location = AffectLocation::Damroll;
        affect_to_char(ch, af);

        af.modifier = std::max(10_s, ch->level);
        af.location = AffectLocation::Ac;
        affect_to_char(ch, af);

        /* if ( (wield !=nullptr) && (wield->type == ObjectType::Weapon) &&
              (check_enum_bit(wield->value[4], WeaponFlag::Flaming)))
            {
              ch->send_line("Your great energy causes your weapon to burst into
  flame.");
            wield->value[3] = 29;
            }*/

    } else {
        ch->wait_state(3 * PULSE_VIOLENCE);
        ch->mana -= 25;
        ch->move /= 2;

        ch->send_line("Your pulse speeds up, but nothing happens.");
        check_improve(ch, gsn_berserk, false, 2);
    }
}

void do_bash(Char *ch, ArgParser args) {
    int chance;
    if ((chance = get_skill(ch, gsn_bash)) == 0 || (ch->is_npc() && !check_enum_bit(ch->off_flags, OffensiveFlag::Bash))
        || (ch->is_pc() && ch->level < get_skill_level(ch, gsn_bash))) {
        ch->send_line("Bashing? What's that?");
        return;
    }
    if (ch->riding) {
        ch->send_line("You can't bash while mounted.");
        return;
    }
    Char *victim;
    const auto target = args.shift();
    if (target.empty()) {
        victim = ch->fighting;
        if (!victim) {
            ch->send_line("But you aren't fighting anyone!");
            return;
        }
    } else if (!(victim = get_char_room(ch, target))) {
        ch->send_line("They aren't here.");
        return;
    }

    if (victim->is_pos_relaxing() || victim->is_pos_stunned_or_dying()) {
        act("You'll have to let $M get back up first.", ch, nullptr, victim, To::Char);
        return;
    }

    if (victim == ch) {
        ch->send_line("You try to bash your brains out, but fail.");
        return;
    }

    if (is_safe(ch, victim))
        return;

    if (victim->fighting != nullptr && !is_same_group(ch, victim->fighting)) {
        ch->send_line("Kill stealing is not permitted.");
        return;
    }

    if (ch->is_aff_charm() && ch->master == victim) {
        act("But $N is your friend!", ch, nullptr, victim, To::Char);
        return;
    }

    /* modifiers */

    /* size  and weight */
    chance += ch->carry_weight / 25;
    chance -= victim->carry_weight / 20;

    if (ch->body_size < victim->body_size)
        chance += BodySizes::size_diff(ch->body_size, victim->body_size) * 25;
    else
        chance += BodySizes::size_diff(ch->body_size, victim->body_size) * 10;

    /* stats */
    chance += get_curr_stat(ch, Stat::Str);
    chance -= get_curr_stat(victim, Stat::Dex) * 4 / 3;

    /* speed */
    if (check_enum_bit(ch->off_flags, OffensiveFlag::Fast) || ch->is_aff_haste())
        chance += 10;
    if (check_enum_bit(victim->off_flags, OffensiveFlag::Fast) || victim->is_aff_haste())
        chance -= 20;

    /* level */
    chance += (ch->level - victim->level) * 2;

    /* now the attack */
    if (number_percent() < chance) {

        act("$n sends you sprawling with a powerful bash!", ch, nullptr, victim, To::Vict);
        act("You slam into $N, and send $M flying!", ch, nullptr, victim, To::Char);
        act("$n sends $N sprawling with a powerful bash.", ch, nullptr, victim, To::NotVict);
        check_improve(ch, gsn_bash, true, 1);

        if (fighting_duel(ch, victim))
            victim->wait_state(2 * PULSE_VIOLENCE);
        else
            victim->wait_state(3 * PULSE_VIOLENCE);
        ch->wait_state(skill_table[gsn_bash].beats);
        victim->position = Position::Type::Resting;
        damage(ch, victim, number_range(2, calc_max_bash_damage(ch->body_size, chance)), &skill_table[gsn_bash],
               DamageType::Bash);
        if (victim->ridden_by != nullptr) {
            thrown_off(victim->ridden_by, victim);
        }
    } else {
        damage(ch, victim, 0, &skill_table[gsn_bash], DamageType::Bash);
        act("You fall flat on your face!", ch, nullptr, victim, To::Char);
        act("$n falls flat on $s face.", ch, nullptr, victim, To::NotVict);
        act("You evade $n's bash, causing $m to fall flat on $s face.", ch, nullptr, victim, To::Vict);
        check_improve(ch, gsn_bash, false, 1);
        ch->position = Position::Type::Resting;
        ch->wait_state(skill_table[gsn_bash].beats * 3 / 2);
        if (ch->ridden_by != nullptr) {
            thrown_off(ch->ridden_by, ch);
        }
    }
}

void do_dirt(Char *ch, ArgParser args) {
    Char *victim;
    int chance;

    if ((chance = get_skill(ch, gsn_dirt)) == 0
        || (ch->is_npc() && !check_enum_bit(ch->off_flags, OffensiveFlag::KickDirt))
        || (ch->is_pc() && ch->level < get_skill_level(ch, gsn_dirt))) {
        ch->send_line("You get your feet dirty.");
        return;
    }

    if (ch->riding) {
        ch->send_line("It's hard to dirt kick when your feet are off the ground!");
        return;
    }

    if (!args.empty()) {
        victim = ch->fighting;
        if (!victim) {
            ch->send_line("But you aren't in combat!");
            return;
        }
    } else if (!(victim = get_char_room(ch, args.shift()))) {
        ch->send_line("They aren't here.");
        return;
    }

    if (victim->is_aff_blind()) {
        act("$E's already been blinded.", ch, nullptr, victim, To::Char);
        return;
    }

    if (victim == ch) {
        ch->send_line("You can't kick dirt in your own eyes!");
        return;
    }

    if (is_safe(ch, victim))
        return;

    if (victim->fighting != nullptr && !is_same_group(ch, victim->fighting)) {
        ch->send_line("Kill stealing is not permitted.");
        return;
    }

    if (ch->is_aff_charm() && ch->master == victim) {
        act("But $N is such a good friend!", ch, nullptr, victim, To::Char);
        return;
    }

    /* modifiers */

    /* dexterity */
    chance += get_curr_stat(ch, Stat::Dex);
    chance -= 2 * get_curr_stat(victim, Stat::Dex);

    /* speed  */
    if (check_enum_bit(ch->off_flags, OffensiveFlag::Fast) || ch->is_aff_haste())
        chance += 10;
    if (check_enum_bit(victim->off_flags, OffensiveFlag::Fast) || victim->is_aff_haste())
        chance -= 25;

    /* level */
    chance += (ch->level - victim->level) * 2;

    /* sloppy hack to prevent false zeroes */
    if (chance % 5 == 0)
        chance += 1;

    /* terrain */

    switch (ch->in_room->sector_type) {
    case SectorType::Inside: chance -= 20; break;

    case SectorType::Mountain:
    case SectorType::City: chance -= 10; break;

    case SectorType::Field: chance += 5; break;

    case SectorType::Unused:
    case SectorType::NonSwimmableWater:
    case SectorType::SwimmableWater:
    case SectorType::Air: chance = 0; break;

    case SectorType::Desert: chance += 10; break;

    case SectorType::Forest:
    case SectorType::Hills: break;
    }

    if (chance == 0) {
        ch->send_line("There isn't any dirt to kick.");
        return;
    }

    /* now the attack */
    if (number_percent() < chance) {
        AFFECT_DATA af;
        act("$n is blinded by the dirt in $s eyes!", victim);
        damage(ch, victim, number_range(2, 5), &skill_table[gsn_dirt], DamageType::None);
        victim->send_line("You can't see a thing!");
        check_improve(ch, gsn_dirt, true, 2);
        ch->wait_state(skill_table[gsn_dirt].beats);

        af.type = gsn_dirt;
        af.level = ch->level;
        af.duration = 0;
        af.location = AffectLocation::Hitroll;
        af.modifier = -4;
        af.bitvector = to_int(AffectFlag::Blind);

        affect_to_char(victim, af);
    } else {
        damage(ch, victim, 0, &skill_table[gsn_dirt], DamageType::None);
        check_improve(ch, gsn_dirt, false, 2);
        ch->wait_state(skill_table[gsn_dirt].beats);
    }
}

void do_trip(Char *ch, ArgParser args) {
    Char *victim;
    int chance;
    if ((chance = get_skill(ch, gsn_trip)) == 0 || (ch->is_npc() && !check_enum_bit(ch->off_flags, OffensiveFlag::Trip))
        || (ch->is_pc() && ch->level < get_skill_level(ch, gsn_trip))) {
        ch->send_line("Tripping?  What's that?");
        return;
    }

    if (ch->riding) {
        ch->send_line("You can't trip while mounted.");
        return;
    }
    if (args.empty()) {
        victim = ch->fighting;
        if (!victim) {
            ch->send_line("But you aren't fighting anyone!");
            return;
        }
    } else if (!(victim = get_char_room(ch, args.shift()))) {
        ch->send_line("They aren't here.");
        return;
    }
    if (is_safe(ch, victim))
        return;

    if (victim->fighting != nullptr && !is_same_group(ch, victim->fighting)) {
        ch->send_line("Kill stealing is not permitted.");
        return;
    }

    if (victim->is_aff_fly() || (victim->riding != nullptr)) {
        act("$S feet aren't on the ground.", ch, nullptr, victim, To::Char);
        return;
    }

    if (victim->is_pos_relaxing() || victim->is_pos_stunned_or_dying()) {
        act("$N is already down.", ch, nullptr, victim, To::Char);
        return;
    }

    if (victim == ch) {
        ch->send_line("You fall flat on your face!");
        ch->wait_state(2 * skill_table[gsn_trip].beats);
        act("$n trips over $s own feet!", ch);
        return;
    }

    if (ch->is_aff_charm() && ch->master == victim) {
        act("$N is your beloved master.", ch, nullptr, victim, To::Char);
        return;
    }

    /* modifiers */

    /* size */
    if (ch->body_size < victim->body_size)
        chance += BodySizes::size_diff(ch->body_size, victim->body_size) * 10; /* bigger = harder to trip */

    /* dex */
    chance += get_curr_stat(ch, Stat::Dex);
    chance -= get_curr_stat(victim, Stat::Dex) * 3 / 2;

    /* speed */
    if (check_enum_bit(ch->off_flags, OffensiveFlag::Fast) || ch->is_aff_haste())
        chance += 10;
    if (check_enum_bit(victim->off_flags, OffensiveFlag::Fast) || victim->is_aff_haste())
        chance -= 20;

    /* level */
    chance += (ch->level - victim->level) * 2;

    /* now the attack */
    if (number_percent() < chance) {
        act("$n trips you and you go down!", ch, nullptr, victim, To::Vict);
        act("You trip $N and $N goes down!", ch, nullptr, victim, To::Char);
        act("$n trips $N, sending $M to the ground.", ch, nullptr, victim, To::NotVict);
        check_improve(ch, gsn_trip, true, 1);

        victim->wait_state(2 * PULSE_VIOLENCE);
        ch->wait_state(skill_table[gsn_trip].beats);
        victim->position = Position::Type::Resting;
        damage(ch, victim, number_range(2, calc_max_trip_damage(victim->body_size)), &skill_table[gsn_trip],
               DamageType::Bash);
        if (victim->ridden_by != nullptr) {
            thrown_off(victim->ridden_by, victim);
        }
    } else {
        damage(ch, victim, 0, &skill_table[gsn_trip], DamageType::Bash);
        ch->wait_state(skill_table[gsn_trip].beats * 2 / 3);
        check_improve(ch, gsn_trip, false, 1);
    }
}

void do_kill(Char *ch, ArgParser args) {
    if (args.empty()) {
        ch->send_line("Kill whom?");
        return;
    }

    auto *victim = get_char_room(ch, args.shift());
    if (!victim) {
        ch->send_line("They aren't here.");
        return;
    }

    if (victim->is_pc()) {
        if (!check_enum_bit(victim->act, PlayerActFlag::PlrKiller)
            && !check_enum_bit(victim->act, PlayerActFlag::PlrThief)) {
            ch->send_line("You must MURDER a player.");
            return;
        }
    }

    if (victim == ch) {
        ch->send_line("You hit yourself.  Ouch!");
        multi_hit(ch, ch);
        return;
    }

    if (is_safe(ch, victim))
        return;

    if (victim->fighting != nullptr && !is_same_group(ch, victim->fighting)) {
        ch->send_line("Kill stealing is not permitted.");
        return;
    }

    if (ch->is_aff_charm() && ch->master == victim) {
        act("$N is your beloved master.", ch, nullptr, victim, To::Char);
        return;
    }

    if (ch->is_pos_fighting()) {
        ch->send_line("You do the best you can!");
        return;
    }

    ch->wait_state(1 * PULSE_VIOLENCE);
    check_killer(ch, victim);
    multi_hit(ch, victim);
}

void do_murde(Char *ch) { ch->send_line("If you want to MURDER, spell it out."); }

void do_murder(Char *ch, ArgParser args) {
    if (args.empty()) {
        ch->send_line("Murder whom?");
        return;
    }
    if (ch->is_aff_charm() || (ch->is_npc() && check_enum_bit(ch->act, CharActFlag::Pet)))
        return;
    auto *victim = get_char_room(ch, args.shift());
    if (!victim) {
        ch->send_line("They aren't here.");
        return;
    }

    if (victim == ch) {
        ch->send_line("Suicide is a mortal sin.");
        return;
    }

    if (is_safe(ch, victim))
        return;

    if (victim->fighting != nullptr && !is_same_group(ch, victim->fighting)) {
        ch->send_line("Kill stealing is not permitted.");
        return;
    }

    if (ch->is_aff_charm() && ch->master == victim) {
        act("$N is your beloved master.", ch, nullptr, victim, To::Char);
        return;
    }

    if (ch->is_pos_fighting()) {
        ch->send_line("You do the best you can!");
        return;
    }

    ch->wait_state(PULSE_VIOLENCE);
    victim->yell(fmt::format("Help! I am being attacked by {}!", ch->short_name()));
    check_killer(ch, victim);
    multi_hit(ch, victim);
}

void do_backstab(Char *ch, ArgParser args) {
    if (args.empty()) {
        ch->send_line("Backstab whom?");
        return;
    }
    auto *victim = get_char_room(ch, args.shift());
    if (!victim) {
        ch->send_line("They aren't here.");
        return;
    }
    const auto chance = ch->get_skill(gsn_backstab);
    if (chance <= 0) {
        ch->send_line("That might not be a good idea. You might hurt yourself.");
        return;
    }

    if (victim == ch) {
        ch->send_line("How can you sneak up on yourself?");
        return;
    }

    if (is_safe(ch, victim))
        return;

    if (victim->fighting != nullptr && !is_same_group(ch, victim->fighting)) {
        ch->send_line("Kill stealing is not permitted.");
        return;
    }
    if (!get_eq_char(ch, Wear::Wield)) {
        ch->send_line("You need to wield a weapon to backstab.");
        return;
    }

    if (victim->fighting != nullptr) {
        ch->send_line("You can't backstab someone who's fighting.");
        return;
    }

    if ((victim->hit < victim->max_hit))
        if (can_see(victim, ch)) {
            act("$N is hurt and suspicious ... you can't sneak up.", ch, nullptr, victim, To::Char);
            return;
        }

    check_killer(ch, victim);
    ch->wait_state(skill_table[gsn_backstab].beats);
    if (!victim->is_pos_awake() || number_percent() < chance) {
        check_improve(ch, gsn_backstab, true, 1);
        multi_hit(ch, victim, &skill_table[gsn_backstab]);
    } else {
        check_improve(ch, gsn_backstab, false, 1);
        damage(ch, victim, 0, &skill_table[gsn_backstab], DamageType::None); // zero damage hit, trigger a fight.
    }
}

void do_flee(Char *ch) {
    Room *was_in;
    Room *now_in;
    Char *victim;
    int attempt;

    if ((victim = ch->fighting) == nullptr) {
        if (ch->is_pos_fighting())
            ch->position = Position::Type::Standing;
        ch->send_line("You aren't fighting anyone.");
        return;
    }

    if (ch->is_aff_lethargy()) {
        act("You are too lethargic to flee.", ch, nullptr, victim, To::Char);
        return;
    }

    was_in = ch->in_room;
    for (attempt = 0; attempt < 6; attempt++) {
        auto direction = random_direction();
        const auto &exit = was_in->exits[direction];
        if (!exit || exit->u1.to_room == nullptr || check_enum_bit(exit->exit_info, ExitFlag::Closed)
            || (ch->is_npc() && check_enum_bit(exit->u1.to_room->room_flags, RoomFlag::NoMob)))
            continue;

        move_char(ch, direction);
        if ((now_in = ch->in_room) == was_in)
            continue;

        ch->in_room = was_in;
        if (ch->ridden_by != nullptr)
            thrown_off(ch->ridden_by, ch);
        act("$n has fled!", ch);
        ch->in_room = now_in;

        if (ch->is_pc()) {
            ch->send_line("You flee from combat!  You lose 10 exps.");
            gain_exp(ch, -10);
        }

        stop_fighting(ch, true);
        do_flee_check(ch);
        return;
    }

    ch->send_line("PANIC! You couldn't escape!");
}

void do_rescue(Char *ch, ArgParser args) {
    if (args.empty()) {
        ch->send_line("Rescue whom?");
        return;
    }
    auto *victim = get_char_room(ch, args.shift());
    if (!victim) {
        ch->send_line("They aren't here.");
        return;
    }
    if (victim == ch) {
        ch->send_line("What about fleeing instead?");
        return;
    }
    if (!is_same_group(ch, victim)) {
        ch->send_line("Kill stealing is not permitted.");
        return;
    }
    if (ch->is_pc() && victim->is_npc()) {
        ch->send_line("Doesn't need your help!");
        return;
    }
    if (ch->fighting == victim) {
        ch->send_line("Too late.");
        return;
    }
    auto *victim_fighting = victim->fighting;
    if (!victim_fighting) {
        ch->send_line("That person isn't fighting right now!");
        return;
    }
    if (victim_fighting->fighting != victim) {
        ch->send_line("That person doesn't need rescuing right now!");
        return;
    }
    ch->wait_state(skill_table[gsn_rescue].beats);
    if (ch->is_pc() && number_percent() > ch->get_skill(gsn_rescue)) {
        ch->send_line("You fail the rescue.");
        check_improve(ch, gsn_rescue, false, 1);
        return;
    }

    act("You rescue $N!", ch, nullptr, victim, To::Char);
    act("$n rescues you!", ch, nullptr, victim, To::Vict);
    act("$n rescues $N!", ch, nullptr, victim, To::NotVict);
    check_improve(ch, gsn_rescue, true, 1);

    stop_fighting(victim_fighting, false);
    stop_fighting(victim, false);

    check_killer(ch, victim_fighting);
    set_fighting(ch, victim_fighting);
    set_fighting(victim_fighting, ch);
}

/* TheMoog woz 'ere */
/* er...and me!...OFTEN!     */
/* ER..AND WANDERA...*/
/* A.N.D. BLOODY ME AS WELL (Death) */
/* blimey o'reilly, so was Faramir */
/* <RoFL!> ---- ok I'd accept this...were headbutt tough to code! <boggle> */

/* and what a surprise, Oshea's had some mods too. */

/* !!!! */
/* ok now the code....yes you did get to it eventually..*/

void do_headbutt(Char *ch, ArgParser args) {
    Char *victim;
    if (ch->is_pc() && ch->level < get_skill_level(ch, gsn_headbutt)) {
        ch->send_line("That might not be a good idea. You might hurt yourself.");
        return;
    }

    if (ch->is_npc() && !check_enum_bit(ch->off_flags, OffensiveFlag::Headbutt))
        return;

    if (args.empty()) {
        if (ch->fighting == nullptr) {
            ch->send_line("You headbutt the air furiously!");
            return;
        } else
            victim = ch->fighting;
    } else if (!(victim = get_char_room(ch, args.shift()))) {
        ch->send_line("They aren't here.");
        return;
    } else if (victim != ch->fighting && (ch->fighting != nullptr)) {
        ch->send_line("No way! You are still fighting!");
        return;
    }

    if (ch->riding != nullptr) {
        ch->send_line("You cannot headbutt whilst mounted.");
        return;
    }
    if (victim->is_pc() && !fighting_duel(ch, victim)) {
        ch->send_line("You can only legally headbutt a player if you are duelling with them.");
        return;
    }

    if (victim->fighting != nullptr && ((ch->fighting != victim) && (victim->fighting != ch))) {
        ch->send_line("Hey, they're already fighting someone else!");
        return;
    }

    ch->wait_state(skill_table[gsn_headbutt].beats);

    if (auto chance = number_percent(); chance < ch->get_skill(gsn_headbutt)) {
        damage(ch, victim, number_range(ch->level / 3, ch->level), &skill_table[gsn_headbutt], DamageType::Bash);
        check_improve(ch, gsn_headbutt, true, 2);

        chance = chance - ch->level + victim->level;
        if ((chance < 5) || !victim->is_aff_blind()) {
            act("$n is blinded by the blood running into $s eyes!", victim);
            victim->send_line("Blood runs into your eyes - you can't see!");
            AFFECT_DATA af;
            af.type = gsn_headbutt;
            af.level = ch->level;
            af.duration = 0;
            af.location = AffectLocation::Hitroll;
            af.modifier = -5;
            af.bitvector = to_int(AffectFlag::Blind);

            affect_to_char(victim, af);
        }
    } else {
        act("$N dodges your headbutt. You feel disoriented.", ch, nullptr, victim, To::Char);
        act("$N dodges $n's headbutt.", ch, nullptr, victim, To::Room);
        act("You dodge $n's headbutt.", ch, nullptr, victim, To::Vict);
        ch->wait_state(skill_table[gsn_headbutt].beats / 2);
    }
}

/* Wandera's little baby is just slipping in here */
/**/
void do_sharpen(Char *ch) {
    Object *weapon;
    int chance;

    if ((weapon = get_eq_char(ch, Wear::Wield)) == nullptr) {
        ch->send_line("You must be wielding a weapon to sharpen it.");
        return;
    }

    if (weapon->type != ObjectType::Weapon) {
        ch->send_line("You can't sharpen it. It is not a weapon.");
        return;
    }

    if (ch->is_pc() && ch->level < get_skill_level(ch, gsn_sharpen)) {
        ch->send_line("You can't do that or you may cut yourself.");
        return;
    }
    if (check_enum_bit(weapon->value[4], WeaponFlag::Sharp)) {
        ch->send_line("It can't get any sharper.");
        return;
    }

    chance = get_skill(ch, gsn_sharpen);
    if (number_percent() <= chance) {
        set_enum_bit(weapon->value[4], WeaponFlag::Sharp);
        ch->send_line("You sharpen the weapon to a fine, deadly point.");
    } else {
        ch->send_line("Your lack of skill removes all bonuses on this weapon.");
        weapon->objIndex->condition -= 10; /* reduce condition of weapon*/
        weapon->value[4] = 0; /* Wipe all bonuses */
    }
    check_improve(ch, gsn_sharpen, true, 5);
}

void do_kick(Char *ch, ArgParser args) {
    Char *victim;
    if (ch->is_pc() && ch->level < get_skill_level(ch, gsn_kick)) {
        ch->send_line("You better leave the martial arts to fighters.");
        return;
    }

    if (ch->is_npc() && !check_enum_bit(ch->off_flags, OffensiveFlag::Kick))
        return;

    if (ch->riding) {
        ch->send_line("You can't kick - your feet are still in the stirrups!");
        return;
    }

    if (args.empty()) {
        if (!ch->fighting) {
            ch->send_line("Kick who?");
            return;
        } else
            victim = ch->fighting;
    } else if (!(victim = get_char_room(ch, args.shift()))) {
        ch->send_line("They aren't here.");
        return;
    } else if (victim != ch->fighting && ch->fighting) {
        ch->send_line("No way! You are still fighting!");
        return;
    }

    ch->wait_state(skill_table[gsn_kick].beats);
    if (number_percent() < ch->get_skill(gsn_kick)) {
        damage(ch, victim, number_range(1, ch->level), &skill_table[gsn_kick], DamageType::Bash);
        check_improve(ch, gsn_kick, true, 1);
    } else {
        damage(ch, victim, 0, &skill_table[gsn_kick], DamageType::Bash); // zero damage hit, trigger a fight.
        check_improve(ch, gsn_kick, false, 1);
    }
}

void do_disarm(Char *ch) {
    Char *victim;
    Object *obj;
    int chance, hth, ch_weapon, vict_weapon, ch_vict_weapon;

    hth = 0;

    if ((chance = get_skill(ch, gsn_disarm)) == 0) {
        ch->send_line("You don't know how to disarm opponents.");
        return;
    }

    if (get_eq_char(ch, Wear::Wield) == nullptr
        && ((hth = get_skill(ch, gsn_hand_to_hand)) == 0
            || (ch->is_npc() && !check_enum_bit(ch->off_flags, OffensiveFlag::Disarm)))) {
        ch->send_line("You must wield a weapon to disarm.");
        return;
    }

    if ((victim = ch->fighting) == nullptr) {
        ch->send_line("You aren't fighting anyone.");
        return;
    }

    if ((obj = get_eq_char(victim, Wear::Wield)) == nullptr) {
        ch->send_line("Your opponent is not wielding a weapon.");
        return;
    }

    if (victim->is_aff_talon()) {
        act("$N's talon-like grip stops you from disarming $M!", ch, nullptr, victim, To::Char);
        act("$n tries to disarm you, but your talon like grip stops them!", ch, nullptr, victim, To::Vict);
        act("$n tries to disarm $N, but fails.", ch, nullptr, victim, To::NotVict);
        return;
    }

    /* find weapon skills */
    ch_weapon = get_weapon_skill(ch, get_weapon_sn(ch));
    vict_weapon = get_weapon_skill(victim, get_weapon_sn(victim));
    ch_vict_weapon = get_weapon_skill(ch, get_weapon_sn(victim));

    /* modifiers */

    /* skill */
    if (get_eq_char(ch, Wear::Wield) == nullptr)
        chance = chance * hth / 150;
    else
        chance = chance * ch_weapon / 100;

    chance += (ch_vict_weapon / 2 - vict_weapon) / 2;

    /* dex vs. strength */
    chance += get_curr_stat(ch, Stat::Dex);
    chance -= 2 * get_curr_stat(victim, Stat::Str);

    /* level */
    chance += (ch->level - victim->level) * 2;

    /* and now the attack */
    if (number_percent() < chance) {
        ch->wait_state(skill_table[gsn_disarm].beats);
        disarm(ch, victim);
        check_improve(ch, gsn_disarm, true, 1);
    } else {
        ch->wait_state(skill_table[gsn_disarm].beats);
        act("You fail to disarm $N.", ch, nullptr, victim, To::Char);
        act("$n tries to disarm you, but fails.", ch, nullptr, victim, To::Vict);
        act("$n tries to disarm $N, but fails.", ch, nullptr, victim, To::NotVict);
        check_improve(ch, gsn_disarm, false, 1);
    }
}

void do_sla(Char *ch) { ch->send_line("If you want to SLAY, spell it out."); }

void do_slay(Char *ch, ArgParser args) {
    if (args.empty()) {
        ch->send_line("Slay whom?");
        return;
    }
    auto *victim = get_char_room(ch, args.shift());
    if (!victim) {
        ch->send_line("They aren't here.");
        return;
    }
    if (ch == victim) {
        ch->send_line("Suicide is a mortal sin.");
        return;
    }
    if (victim->is_pc() && victim->level >= ch->get_trust()) {
        ch->send_line("You failed.");
        return;
    }
    act("You slay $M in cold blood!", ch, nullptr, victim, To::Char);
    act("$n slays you in cold blood!", ch, nullptr, victim, To::Vict);
    act("$n slays $N in cold blood!", ch, nullptr, victim, To::NotVict);
    raw_kill(victim, std::nullopt);
}
