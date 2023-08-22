/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                   */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  update.c:  keeps everything rolling                                  */
/*                                                                       */
/*************************************************************************/

#include "update.hpp"
#include "AFFECT_DATA.hpp"
#include "AffectFlag.hpp"
#include "Area.hpp"
#include "Char.hpp"
#include "CharActFlag.hpp"
#include "Classes.hpp"
#include "DamageTolerance.hpp"
#include "DamageType.hpp"
#include "Descriptor.hpp"
#include "DescriptorList.hpp"
#include "Exit.hpp"
#include "ExitFlag.hpp"
#include "Logging.hpp"
#include "MProg.hpp"
#include "MProgTypeFlag.hpp"
#include "Object.hpp"
#include "ObjectIndex.hpp"
#include "ObjectType.hpp"
#include "ObjectWearFlag.hpp"
#include "PlayerActFlag.hpp"
#include "RoomFlag.hpp"
#include "SkillNumbers.hpp"
#include "SkillTables.hpp"
#include "TimeInfoData.hpp"
#include "Tip.hpp"
#include "Titles.hpp"
#include "VnumMobiles.hpp"
#include "VnumObjects.hpp"
#include "VnumRooms.hpp"
#include "WeatherData.hpp"
#include "Worn.hpp"
#include "act_comm.hpp"
#include "act_move.hpp"
#include "act_obj.hpp"
#include "challenge.hpp"
#include "comm.hpp"
#include "common/BitOps.hpp"
#include "db.h"
#include "fight.hpp"
#include "handler.hpp"
#include "interp.h"
#include "magic.h"
#include "save.hpp"
#include "skills.hpp"
#include "string_utils.hpp"

#include <fmt/format.h>

extern size_t max_on;

/*
 * Local functions.
 */
int hit_gain(Char *ch);
int mana_gain(Char *ch);
int move_gain(Char *ch);
void mobile_update();
void weather_update();
void ord();
void obj_update();
void aggr_update();

/* Added by Rohan to reset the count every day */
void count_update();
int count_updated = 0;

/* used for saving */
static const uint32_t save_every_n = 30u;
uint32_t save_number = 0;

/*
 * ch - the sentient mob
 * victim - the hapless victim
 *  this func added to prevent the spam which was occuring when a sentient
 * mob encountered his foe in a safe room --fara
 */

bool is_safe_sentient(Char *ch, const Char *victim) {
    if (ch->in_room == nullptr)
        return false;
    if (check_enum_bit(ch->in_room->room_flags, RoomFlag::Safe)) {
        ch->yell(fmt::format("|WIf it weren't for the law, you'd be dead meat {}!!!|w", victim->name));
        ch->sentient_victim.clear();
        return true;
    }
    return false;
}

/* Advancement stuff. */
void advance_level(Char *ch) {
    int add_hp;
    int add_mana;
    int add_move;
    int add_prac;

    using namespace std::chrono;
    ch->pcdata->last_level = (int)duration_cast<hours>(ch->total_played()).count();

    ch->set_title(fmt::format("the {}", title_table[ch->class_num][ch->level][ch->sex.is_male() ? 0 : 1]));

    add_hp = con_app[ch->curr_stat(Stat::Con)].hitp
             + number_range(class_table[ch->class_num].hp_min, class_table[ch->class_num].hp_max);

    add_mana = number_range(
        0, class_table[ch->class_num].fMana * class_table[ch->class_num].fMana
               * (std::max(0, ch->curr_stat(Stat::Wis) - 15) + 2 * std::max(0, ch->curr_stat(Stat::Int) - 15)));

    add_mana += 150;
    add_mana /= 300; /* =max (2*int+wis)/10 (10=mage.fMana)*/
    add_mana += class_table[ch->class_num].fMana / 2;
    /* thanx oshea for mana alg.
       For mage (25,25) gives 5 5%,   6-14 10%,   15 5%
         cleric         gives 4 7.8%, 5- 9 15.6%, 10 9%
         knight         gives 2 20%,  3- 4 40%,    5 1/900!
      barbarian         gives 1 56%,               2 44%   */

    /* I think that knight might want to have his fMana increase to say
       6 which would increase his average mana gain by just over 1.
       Bear in mind this are all for 25 int and wis!  They all go down
       to 100% for the first column with 15 int and wis.  */

    /* End of new section. */

    add_move = number_range(1, (ch->curr_stat(Stat::Con) + ch->curr_stat(Stat::Dex)) / 6);
    add_prac = wis_app[ch->curr_stat(Stat::Wis)].practice;

    add_hp = std::max(1, add_hp * 9 / 10);
    add_move = std::max(6, add_move * 9 / 10);

    ch->max_hit += add_hp;
    ch->max_mana += add_mana;
    ch->max_move += add_move;
    ch->practice += add_prac;
    ch->train += 1;

    ch->pcdata->perm_hit += add_hp;
    ch->pcdata->perm_mana += add_mana;
    ch->pcdata->perm_move += add_move;

    if (ch->is_pc())
        clear_enum_bit(ch->act, PlayerActFlag::PlrBoughtPet);

    ch->send_line("Your gain is: {}/{} hp, {}/{} m, {}/{} mv {}/{} prac.", add_hp, ch->max_hit, add_mana, ch->max_mana,
                  add_move, ch->max_move, add_prac, ch->practice);
    log_string("### {} has made a level in room {}", ch->name, ch->in_room->vnum);
    announce(fmt::format("|W### |P{}|W has made a level!!!|w", ch->name), ch);
}

void lose_level(Char *ch) {
    int add_hp;
    int add_mana;
    int add_move;
    int add_prac;

    using namespace std::chrono;
    ch->pcdata->last_level = (int)duration_cast<hours>(ch->total_played()).count();

    ch->set_title(fmt::format("the {}", title_table[ch->class_num][ch->level][ch->sex.is_male() ? 0 : 1]));

    add_hp = con_app[ch->max_stat(Stat::Con)].hitp
             + number_range(class_table[ch->class_num].hp_min, class_table[ch->class_num].hp_max);
    add_mana = (number_range(2, (2 * ch->max_stat(Stat::Int) + ch->max_stat(Stat::Wis)) / 5)
                * class_table[ch->class_num].fMana)
               / 10;
    add_move = number_range(1, (ch->max_stat(Stat::Con) + ch->max_stat(Stat::Dex)) / 6);
    add_prac = -(wis_app[ch->max_stat(Stat::Wis)].practice);

    add_hp = add_hp * 9 / 10;
    add_mana = add_mana * 9 / 10;
    add_move = add_move * 9 / 10;

    add_hp = -(std::max(1, add_hp));
    add_mana = -(std::max(1, add_mana));
    add_move = -(std::max(6, add_move));

    /* I negate the additives! Death */

    ch->max_hit += add_hp;
    ch->max_mana += add_mana;
    ch->max_move += add_move;
    ch->practice += add_prac;
    ch->train -= 1;

    ch->pcdata->perm_hit += add_hp;
    ch->pcdata->perm_mana += add_mana;
    ch->pcdata->perm_move += add_move;

    if (ch->is_pc())
        clear_enum_bit(ch->act, PlayerActFlag::PlrBoughtPet);

    ch->send_line("Your gain is: {}/{} hp, {}/{} m, {}/{} mv {}/{} prac.", add_hp, ch->max_hit, add_mana, ch->max_mana,
                  add_move, ch->max_move, add_prac, ch->practice);
    announce(fmt::format("|W### |P{}|W has lost a level!!!|w", ch->name), ch);
}

void gain_exp(Char *ch, int gain) {
    if (ch->is_npc() || ch->level >= LEVEL_HERO)
        return;

    ch->exp = std::max(static_cast<long>(exp_per_level(ch, ch->pcdata->points)), ch->exp + gain);
    if (gain >= 0) {
        while ((ch->level < LEVEL_HERO)
               && ((ch->exp) >= ((int)exp_per_level(ch, ch->pcdata->points) * (ch->level + 1)))) {
            ch->send_line("|WYou raise a level!!|w");
            ch->level += 1;
            advance_level(ch);
        }
    }
}

/*
 * Regeneration stuff.
 */
int hit_gain(Char *ch) {
    int gain;
    int number;

    if (ch->is_npc()) {
        gain = 5 + ch->level;
        if (ch->is_aff_regeneration())
            gain *= 2;

        switch (ch->position) {
        default: gain /= 2; break;
        case Position::Type::Sleeping: gain = 3 * gain / 2; break;
        case Position::Type::Resting: break;
        case Position::Type::Fighting: gain /= 3; break;
        }

    } else {
        gain = std::max(3, ch->curr_stat(Stat::Con) - 3 + ch->level);
        gain += class_table[ch->class_num].hp_max - 10;
        number = number_percent();
        if (number < ch->get_skill(gsn_fast_healing)) {
            gain += number * gain / 100;
            if (ch->hit < ch->max_hit)
                check_improve(ch, gsn_fast_healing, true, 8);
        }

        switch (ch->position) {
        case Position::Type::Sleeping: break;
        case Position::Type::Resting: gain /= 1.7; break;
        case Position::Type::Fighting: gain /= 4.5; break;
        default: gain /= 2; break;
        }

        if (ch->is_starving()) {
            gain /= 2;
        }
        if (ch->is_parched()) {
            gain /= 2;
        }
    }

    if (ch->is_aff_poison())
        gain /= 4;

    if (ch->is_aff_plague())
        gain /= 8;

    if (ch->is_aff_lethargy())
        gain *= 2;
    if (ch->is_aff_regeneration())
        gain *= 2;

    return std::min(gain, ch->max_hit - ch->hit);
}

int mana_gain(Char *ch) {
    int gain;
    int number;

    if (ch->is_npc()) {
        gain = 5 + ch->level;
        switch (ch->position) {
        default: gain /= 2; break;
        case Position::Type::Sleeping: gain = 3 * gain / 2; break;
        case Position::Type::Resting: break;
        case Position::Type::Fighting: gain /= 3; break;
        }
    } else {
        gain = (ch->curr_stat(Stat::Wis) + ch->curr_stat(Stat::Int) + ch->level) / 2;
        number = number_percent();
        if (number < ch->get_skill(gsn_meditation)) {
            gain += number * gain / 100;
            if (ch->mana < ch->max_mana)
                check_improve(ch, gsn_meditation, true, 8);
        }
        gain = (gain * class_table[ch->class_num].fMana) / 7;

        switch (ch->position) {
        case Position::Type::Sleeping: break;
        case Position::Type::Resting: gain /= 1.7; break;
        default: gain /= 2; break;
        }

        if (ch->is_starving()) {
            gain /= 2;
        }
        if (ch->is_parched()) {
            gain /= 2;
        }
    }

    if (ch->is_aff_poison())
        gain /= 4;

    if (ch->is_aff_plague())
        gain /= 8;

    if (ch->is_aff_lethargy())
        gain *= 2;

    return std::min(gain, ch->max_mana - ch->mana);
}

int move_gain(Char *ch) {
    int gain;

    if (ch->is_npc()) {
        gain = ch->level;
    } else {
        gain = std::max(15_s, ch->level);

        switch (ch->position) {
        case Position::Type::Sleeping: gain += ch->curr_stat(Stat::Dex); break;
        case Position::Type::Resting: gain += ch->curr_stat(Stat::Dex) / 2; break;
        default:; // unchanged
        }

        if (ch->is_starving()) {
            gain /= 2;
        }
        if (ch->is_parched()) {
            gain /= 2;
        }
    }

    if (ch->is_aff_poison())
        gain /= 4;

    if (ch->is_aff_plague())
        gain /= 8;

    if (ch->is_aff_haste())
        gain /= 2;

    if (ch->is_aff_lethargy())
        gain *= 2;

    return std::min(gain, ch->max_move - ch->move);
}

/*
 * Mob autonomous action.
 * This function takes 25% to 35% of ALL Merc cpu time.
 * -- Furey
 */
void mobile_update() {
    /* Examine all mobs. */
    for (auto *ch : char_list) {
        if (ch->is_pc() || ch->in_room == nullptr || ch->is_aff_charm())
            continue;

        if (ch->in_room->area->empty_since_last_reset() && !check_enum_bit(ch->act, CharActFlag::UpdateAlways))
            continue;

        /* Examine call for special procedure */
        if (ch->spec_fun) {
            if ((*ch->spec_fun)(ch))
                continue;
        }

        /* That's all for sleeping / busy monster, and empty zones */
        if (ch->is_pos_preoccupied())
            continue;

        if (ch->in_room->area->occupied()) {
            MProg::random_trigger(ch);
            /* If ch dies or changes
               position due to it's random
               trigger continue - Kahn */
            if (ch->is_pos_preoccupied())
                continue;
        }
        /* Scavenge */
        if (check_enum_bit(ch->act, CharActFlag::Scavenger) && !ch->in_room->contents.empty() && number_bits(6) == 0) {
            int max = 1;
            Object *obj_best{};
            for (auto *obj : ch->in_room->contents) {
                if (obj->is_takeable() && can_loot(ch, obj) && obj->cost > max && obj->cost > 0) {
                    obj_best = obj;
                    max = obj->cost;
                }
            }

            if (obj_best) {
                obj_from_room(obj_best);
                obj_to_char(obj_best, ch);
                act("$n gets $p.", ch, obj_best, nullptr, To::Room);
                do_wear(ch, ArgParser("all"));
            }
        }

        /* Wander */
        auto opt_direction = try_cast_direction(number_bits(5));
        if (!check_enum_bit(ch->act, CharActFlag::Sentinel) && number_bits(4) == 0 && opt_direction) {
            const auto &exit = ch->in_room->exits[*opt_direction];
            if (exit && exit->u1.to_room != nullptr && !check_enum_bit(exit->exit_info, ExitFlag::Closed)
                && !check_enum_bit(exit->u1.to_room->room_flags, RoomFlag::NoMob)
                && (!check_enum_bit(ch->act, CharActFlag::StayArea) || exit->u1.to_room->area == ch->in_room->area)) {
                move_char(ch, *opt_direction);
            }
        }
    }
}
/*
 * Update the weather.
 */
void weather_update() {
    time_info.advance();
    auto weather_before = weather_info;
    weather_info.update(Rng::global_rng(), time_info);

    if (auto update_msg = weather_info.describe_change(weather_before); !update_msg.empty()) {
        for (auto &d : descriptors().playing()) {
            if (d.character()->is_outside() && d.character()->is_pos_awake())
                d.character()->send_to(update_msg);
        }
    }
}

/**
 * If a char was idle and been sent to the limbo room and they send a command, return them to
 * their previous room.
 */
void move_active_char_from_limbo(Char *ch) {
    if (ch == nullptr || ch->desc == nullptr || !ch->desc->is_playing() || ch->was_in_room == nullptr
        || ch->in_room != get_room(Rooms::Limbo))
        return;

    ch->timer = 0;
    char_from_room(ch);
    char_to_room(ch, ch->was_in_room);
    ch->was_in_room = nullptr;
    act("$n has returned from the void.", ch);
    if (ch->pet) { /* move pets too */
        char_from_room(ch->pet);
        char_to_room(ch->pet, ch->in_room);
        ch->pet->was_in_room = nullptr;
        act("$n has returned from the void.", ch->pet);
    }
}

/**
 * If a chars is idle, move it into the "limbo" room along with its pets.
 */
void move_idle_char_to_limbo(Char *ch) {
    if (++ch->timer >= 12) {
        if (ch->was_in_room == nullptr && ch->in_room != nullptr) {
            ch->was_in_room = ch->in_room;
            if (ch->fighting != nullptr)
                stop_fighting(ch, true);
            act("$n disappears into the void.", ch);
            ch->send_line("You disappear into the void.");
            if (ch->level > 1)
                save_char_obj(ch);
            char_from_room(ch);
            char_to_room(ch, get_room(Rooms::Limbo));
            if (ch->pet) { /* move pets too */
                if (ch->pet->fighting)
                    stop_fighting(ch->pet, true);
                act("$n flickers and phases out", ch->pet);
                ch->pet->was_in_room = ch->pet->in_room;
                char_from_room(ch->pet);
                char_to_room(ch->pet, get_room(Rooms::Limbo));
            }
        }
    }
}

/*
 * Update all chars, including mobs.
 */
void char_update() {
    Char *ch_quit;

    ch_quit = nullptr;

    /* update save counter */
    save_number++;

    if (save_number == save_every_n)
        save_number = 0;

    for (auto *ch : char_list) {
        if (ch->timer > 30)
            ch_quit = ch;

        if (ch->position >= Position::Type::Stunned) {
            if (ch->hit < ch->max_hit)
                ch->hit += hit_gain(ch);
            else
                ch->hit = ch->max_hit;

            if (ch->mana < ch->max_mana)
                ch->mana += mana_gain(ch);
            else
                ch->mana = ch->max_mana;

            if (ch->move < ch->max_move)
                ch->move += move_gain(ch);
            else
                ch->move = ch->max_move;
        }

        if (ch->position == Position::Type::Stunned)
            update_pos(ch);

        if (ch->is_pc() && ch->level < LEVEL_IMMORTAL) {
            Object *obj;

            if ((obj = get_eq_char(ch, Worn::Light)) != nullptr && obj->type == ObjectType::Light
                && obj->value[2] > 0) {
                if (--obj->value[2] == 0 && ch->in_room != nullptr) {
                    --ch->in_room->light;
                    act("$p goes out.", ch, obj, nullptr, To::Room);
                    act("$p flickers and goes out.", ch, obj, nullptr, To::Char);
                    extract_obj(obj);
                } else if (obj->value[2] <= 5 && ch->in_room != nullptr)
                    act("$p flickers.", ch, obj, nullptr, To::Char);
            }

            if (ch->is_immortal())
                ch->timer = 0;

            move_idle_char_to_limbo(ch);
            ch->delta_inebriation(-1);
            ch->delta_hunger(-1);
            ch->delta_thirst(-1);
            if (const auto opt_nutrition = ch->describe_nutrition()) {
                ch->send_line(*opt_nutrition);
            }
        }

        if (!ch->affected.empty()) {
            std::unordered_set<int> removed_this_tick_with_msg;
            for (auto &af : ch->affected) {
                if (af.duration > 0) {
                    af.duration--;
                    if (number_range(0, 4) == 0 && af.level > 0)
                        af.level--; /* spell strength fades with time */
                } else if (af.duration >= 0) {
                    if (af.type > 0 && skill_table[af.type].msg_off)
                        removed_this_tick_with_msg.emplace(af.type);
                    affect_remove(ch, af);
                }
            }
            // Only report wear-offs for those affects who are completely gone.
            for (auto sn : removed_this_tick_with_msg)
                if (!ch->is_affected_by(sn))
                    ch->send_line("{}", skill_table[sn].msg_off);
        }

        /* scan all undead zombies created by raise_dead style spells
           and randomly decay them */
        if (ch->is_npc() && ch->mobIndex->vnum == Mobiles::Zombie) {
            if (number_percent() > 90) {
                act("$n fits violently before decaying in to a pile of dust.", ch);
                extract_char(ch, true);
                continue;
            }
        }

        /*
         * Careful with the damages here,
         *   MUST NOT refer to ch after damage taken,
         *   as it may be lethal damage (on NPC).
         */

        if (ch->is_affected_by(gsn_plague) && ch != nullptr) {
            int save, dam;

            if (ch->in_room == nullptr)
                return;

            act("$n writhes in agony as plague sores erupt from $s skin.", ch);
            ch->send_line("You writhe in agony from the plague.");
            auto *existing_plague = ch->affected.find_by_skill(gsn_plague);
            if (existing_plague == nullptr) {
                clear_enum_bit(ch->affected_by, AffectFlag::Plague);
                return;
            }

            if (existing_plague->level == 1)
                return;

            AFFECT_DATA plague;
            plague.type = gsn_plague;
            plague.level = existing_plague->level - 1;
            plague.duration = number_range(1, 2 * plague.level);
            plague.location = AffectLocation::Str;
            plague.modifier = -5;
            plague.bitvector = to_int(AffectFlag::Plague);

            for (auto *vch : ch->in_room->people) {
                switch (check_damage_tolerance(vch, DamageType::Disease)) {
                case DamageTolerance::None: save = existing_plague->level - 4; break;
                case DamageTolerance::Immune: save = 0; break;
                case DamageTolerance::Resistant: save = existing_plague->level - 8; break;
                case DamageTolerance::Vulnerable: save = existing_plague->level; break;
                default: save = existing_plague->level - 4; break;
                }

                if (save != 0 && !saves_spell(save, vch) && vch->is_mortal() && !vch->is_aff_plague()
                    && number_bits(4) == 0) {
                    vch->send_line("You feel hot and feverish.");
                    act("$n shivers and looks very ill.", vch);
                    affect_join(vch, plague);
                }
            }

            dam = std::min(ch->level, 5_s);
            ch->mana = std::max(0, ch->mana - dam);
            ch->move = std::max(0, ch->move - dam);
            damage(ch, ch, dam, &skill_table[gsn_plague], DamageType::Disease);
        } else if (ch->is_aff_poison() && ch != nullptr) {
            act("$n shivers and suffers.", ch);
            ch->send_line("You shiver and suffer.");
            damage(ch, ch, 2, &skill_table[gsn_poison], DamageType::Poison);
        } else if (ch->position == Position::Type::Incap && number_range(0, 1) == 0) {
            damage(ch, ch, 1, Attacks::at(0), DamageType::None);
        } else if (ch->position == Position::Type::Mortal) {
            damage(ch, ch, 1, Attacks::at(0), DamageType::None);
        }
    }

    /*
     * Autosave and autoquit.
     * Check that these chars still exist.
     */
    for (auto *ch : char_list) {
        if (ch->desc != nullptr && ch->desc->channel() % save_every_n == save_number)
            save_char_obj(ch);

        if (ch == ch_quit)
            do_quit(ch);
    }
}

/*
 * Update all objs.
 * This function is performance sensitive.
 */
void obj_update() {
    for (auto &&obj : object_list) {
        const char *message;

        /* go through affects and decrement */
        if (!obj->affected.empty()) {
            std::unordered_set<int> removed_this_tick_with_msg;
            for (auto &af : obj->affected) {
                if (af.duration > 0) {
                    af.duration--;
                    if (number_range(0, 4) == 0 && af.level > 0)
                        af.level--; /* spell strength fades with time */
                } else if (af.duration >= 0) {
                    if (af.type > 0 && skill_table[af.type].msg_off)
                        removed_this_tick_with_msg.emplace(af.type);
                    affect_remove_obj(obj.get(), af);
                }
            }
            // Only report wear-offs for those affects who are completely gone.
            for (auto sn : removed_this_tick_with_msg)
                act(skill_table[sn].msg_off, obj->carried_by, obj.get(), nullptr, To::Char, MobTrig::Yes,
                    Position::Type::Sleeping);
        }

        if (obj->timer <= 0 || --obj->timer > 0)
            continue;

        switch (obj->type) {
        default: message = "$p crumbles into dust."; break;
        case ObjectType::Fountain: message = "$p dries up."; break;
        case ObjectType::Npccorpse: message = "$p decays into dust."; break;
        case ObjectType::Pccorpse: message = "$p decays into dust."; break;
        case ObjectType::Food: message = "$p decomposes."; break;
        case ObjectType::Potion: message = "$p has evaporated from disuse."; break;
        case ObjectType::Portal: message = "$p shimmers and fades away."; break;
        }

        if (obj->carried_by != nullptr) {
            if ((obj->carried_by->is_npc()) && obj->carried_by->mobIndex->shop)
                obj->carried_by->gold += obj->cost;
            else
                act(message, obj->carried_by, obj.get(), nullptr, To::Char);
        } else if (obj->in_room != nullptr && !obj->in_room->people.empty()) {
            if (!(obj->in_obj && obj->in_obj->objIndex->vnum == Objects::Pit && !obj->in_obj->is_takeable())) {
                // seems like we pick someone to emote for convenience here...
                auto *rch = *obj->in_room->people.begin();
                act(message, rch, obj.get(), nullptr, To::Room);
                act(message, rch, obj.get(), nullptr, To::Char);
            }
        }

        if (obj->type == ObjectType::Pccorpse && !obj->contains.empty()) { /* save the contents */
            for (auto *t_obj : obj->contains) {
                obj_from_obj(t_obj);

                if (obj->in_obj) /* in another object */
                    obj_to_obj(t_obj, obj->in_obj);

                if (obj->carried_by) /* carried */
                    obj_to_char(t_obj, obj->carried_by);

                if (obj->in_room == nullptr) /* destroy it */
                    extract_obj(t_obj);

                else /* to a room */
                    obj_to_room(t_obj, obj->in_room);
            }
        }

        extract_obj(obj.get());
    }
}

/*
 * Aggress.
 *
 * for each mortal PC
 *     for each mob in room
 *         aggress on some random PC
 *
 * This function takes 25% to 35% of ALL Merc cpu time.
 * Unfortunately, checking on each PC move is too tricky,
 *   because we don't the mob to just attack the first PC
 *   who leads the party into the room.
 *
 * -- Furey
 */
void do_aggressive_sentient(Char *, Char *);
void aggr_update() {
    for (auto *wch : char_list) {
        /* MOBProgram Act trigger */
        if (wch->is_npc() && !wch->mpact.empty() && wch->in_room->area->occupied()) {
            for (const auto &mpact : wch->mpact) {
                MProg::wordlist_check(mpact.act_message_trigger(), wch, mpact.character(), mpact.object(),
                                      mpact.target(), MProg::TypeFlag::Act);
            }
            wch->mpact.clear();
        }

        if (wch->is_npc() || wch->level >= LEVEL_IMMORTAL || wch->in_room == nullptr
            || wch->in_room->area->empty_since_last_reset())
            continue;

        for (auto *ch : wch->in_room->people) {
            if (ch->is_npc())
                do_aggressive_sentient(wch, ch);
        }
    }
}

void do_aggressive_sentient(Char *wch, Char *ch) {
    if (check_enum_bit(ch->act, CharActFlag::Sentient) && ch->fighting == nullptr && !ch->is_aff_calm()
        && ch->is_pos_awake() && !ch->is_aff_charm() && ch->can_see(*wch)) {
        if (ch->hit == ch->max_hit && ch->mana == ch->max_mana)
            ch->sentient_victim.clear();
        if (matches(wch->name, ch->sentient_victim)) {
            if (is_safe_sentient(ch, wch))
                return;
            ch->yell(fmt::format("|WAha! I never forget a face, prepare to die {}!!!|w", wch->name));
            multi_hit(ch, wch);
        }
    }
    if (check_enum_bit(ch->act, CharActFlag::Aggressive) && !check_enum_bit(ch->in_room->room_flags, RoomFlag::Safe)
        && !ch->is_aff_calm() && (ch->fighting == nullptr) // Changed by Moog
        && !ch->is_aff_charm() && ch->is_pos_awake()
        && !(check_enum_bit(ch->act, CharActFlag::Wimpy) && wch->is_pos_awake()) && ch->can_see(*wch)
        && !number_bits(1)) {

        /*
         * Ok we have a 'wch' player character and a 'ch' npc aggressor.
         * Now make the aggressor fight a RANDOM pc victim in the room,
         *   giving each 'vch' an equal chance of selection.
         */

        int count = 0;
        Char *victim = nullptr;
        for (auto *vch : wch->in_room->people) {
            if (vch->is_pc() && vch->level < LEVEL_IMMORTAL && ch->level >= vch->level - 5
                && (!check_enum_bit(ch->act, CharActFlag::Wimpy) || !vch->is_pos_awake()) && ch->can_see(*vch)) {
                if (number_range(0, count) == 0)
                    victim = vch;
                count++;
            }
        }

        if (victim != nullptr)
            multi_hit(ch, victim);
    }
}

/* This function resets the player count every day */
void count_update() {
    struct tm *cur_time;
    int current_day;
    auto as_tt = Clock::to_time_t(current_time);
    cur_time = localtime(&as_tt);
    current_day = cur_time->tm_mday;
    /* Initialise count_updated if this first time called */
    if (count_updated == 0) {
        count_updated = current_day;
        return;
    } else {
        /* Has the day changed? */
        if (count_updated == current_day)
            return;
        /* Day has changed, reset max_on to 0 and change count_updated
           to current_day, log it also */
        else {
            max_on = 0;
            count_updated = current_day;
            log_string("The player counter has been reset.");
        }
    }
}

/*
 * Handle all kinds of updates.
 * Called once per pulse from game loop.
 * Random times to defeat tick-timing clients and players.
 */

void update_handler() {
    static int pulse_area;
    static int pulse_mobile;
    static int pulse_violence;
    static int pulse_point;

    if (--pulse_area <= 0) {
        pulse_area = number_range(PULSE_AREA / 2, 3 * PULSE_AREA / 2);
        area_update();
    }

    if (--pulse_mobile <= 0) {
        pulse_mobile = PULSE_MOBILE;
        mobile_update();
    }

    if (--pulse_violence <= 0) {
        pulse_violence = PULSE_VIOLENCE;
        violence_update();
    }

    if (--pulse_point <= 0) {
        log_new("TICK", CharExtraFlag::WiznetTick, 0);
        pulse_point = PULSE_TICK;
        /* number_range( PULSE_TICK / 2, 3 * PULSE_TICK / 2 ); */
        weather_update();
        char_update();
        obj_update();
        /*Rohan: update player counter if necessary */
        count_update();
        /*Rohan's challenge safety net*/
        do_chal_tick();

        /* tip wizard */
        tip_players();
    }

    aggr_update();
}
