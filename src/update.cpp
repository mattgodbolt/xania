/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                   */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  update.c:  keeps everything rolling                                  */
/*                                                                       */
/*************************************************************************/

#include "AFFECT_DATA.hpp"
#include "AREA_DATA.hpp"
#include "Descriptor.hpp"
#include "DescriptorList.hpp"
#include "TimeInfoData.hpp"
#include "WeatherData.hpp"
#include "comm.hpp"
#include "handler.hpp"
#include "interp.h"
#include "merc.h"
#include "save.hpp"
#include "string_utils.hpp"

#include <fmt/format.h>

#include <cstdio>
#include <cstring>
#include <ctime>
#include <sys/types.h>

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

/* Advancement stuff. */
void advance_level(Char *ch) {
    char buf[MAX_STRING_LENGTH];
    int add_hp;
    int add_mana;
    int add_move;
    int add_prac;

    using namespace std::chrono;
    ch->pcdata->last_level = (int)duration_cast<hours>(ch->total_played()).count();

    ch->set_title(fmt::format("the {}", title_table[ch->class_num][ch->level][ch->sex == SEX_FEMALE ? 1 : 0]));

    add_hp = con_app[get_curr_stat(ch, Stat::Con)].hitp
             + number_range(class_table[ch->class_num].hp_min, class_table[ch->class_num].hp_max);

    add_mana = number_range(
        0, class_table[ch->class_num].fMana * class_table[ch->class_num].fMana
               * (UMAX(0, get_curr_stat(ch, Stat::Wis) - 15) + 2 * UMAX(0, get_curr_stat(ch, Stat::Int) - 15)));

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

    add_move = number_range(1, (get_curr_stat(ch, Stat::Con) + get_curr_stat(ch, Stat::Dex)) / 6);
    add_prac = wis_app[get_curr_stat(ch, Stat::Wis)].practice;

    add_hp = UMAX(1, add_hp * 9 / 10);
    add_move = UMAX(6, add_move * 9 / 10);

    ch->max_hit += add_hp;
    ch->max_mana += add_mana;
    ch->max_move += add_move;
    ch->practice += add_prac;
    ch->train += 1;

    ch->pcdata->perm_hit += add_hp;
    ch->pcdata->perm_mana += add_mana;
    ch->pcdata->perm_move += add_move;

    if (ch->is_pc())
        REMOVE_BIT(ch->act, PLR_BOUGHT_PET);

    snprintf(buf, sizeof(buf), "Your gain is: %d/%d hp, %d/%d m, %d/%d mv %d/%d prac.\n\r", add_hp, ch->max_hit,
             add_mana, ch->max_mana, add_move, ch->max_move, add_prac, ch->practice);
    ch->send_to(buf);
    log_string("### {} has made a level in room {}", ch->name, ch->in_room->vnum);
    announce(fmt::format("|W### |P{}|W has made a level!!!|w", ch->name), ch);
}

void lose_level(Char *ch) {
    char buf[MAX_STRING_LENGTH];
    int add_hp;
    int add_mana;
    int add_move;
    int add_prac;

    using namespace std::chrono;
    ch->pcdata->last_level = (int)duration_cast<hours>(ch->total_played()).count();

    ch->set_title(fmt::format("the {}", title_table[ch->class_num][ch->level][ch->sex == SEX_FEMALE ? 1 : 0]));

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

    add_hp = -(UMAX(1, add_hp));
    add_mana = -(UMAX(1, add_mana));
    add_move = -(UMAX(6, add_move));

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
        REMOVE_BIT(ch->act, PLR_BOUGHT_PET);

    snprintf(buf, sizeof(buf), "Your gain is: %d/%d hp, %d/%d m, %d/%d mv %d/%d prac.\n\r", add_hp, ch->max_hit,
             add_mana, ch->max_mana, add_move, ch->max_move, add_prac, ch->practice);
    ch->send_to(buf);
    announce(fmt::format("|W### |P{}|W has lost a level!!!|w", ch->name), ch);
}

void gain_exp(Char *ch, int gain) {
    if (ch->is_npc() || ch->level >= LEVEL_HERO)
        return;

    ch->exp = UMAX(exp_per_level(ch, ch->pcdata->points), ch->exp + gain);
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
        if (IS_AFFECTED(ch, AFF_REGENERATION))
            gain *= 2;

        switch (ch->position) {
        default: gain /= 2; break;
        case POS_SLEEPING: gain = 3 * gain / 2; break;
        case POS_RESTING: break;
        case POS_FIGHTING: gain /= 3; break;
        }

    } else {
        gain = UMAX(3, get_curr_stat(ch, Stat::Con) - 3 + ch->level);
        gain += class_table[ch->class_num].hp_max - 10;
        number = number_percent();
        if (number < get_skill_learned(ch, gsn_fast_healing)) {
            gain += number * gain / 100;
            if (ch->hit < ch->max_hit)
                check_improve(ch, gsn_fast_healing, true, 8);
        }

        switch (ch->position) {
        case POS_SLEEPING: break;
        case POS_RESTING: gain /= 1.7; break;
        case POS_FIGHTING: gain /= 4.5; break;
        default: gain /= 2; break;
        }

        if (ch->pcdata->condition[COND_FULL] == 0)
            gain /= 2;

        if (ch->pcdata->condition[COND_THIRST] == 0)
            gain /= 2;
    }

    if (IS_AFFECTED(ch, AFF_POISON))
        gain /= 4;

    if (IS_AFFECTED(ch, AFF_PLAGUE))
        gain /= 8;

    if (IS_AFFECTED(ch, AFF_LETHARGY))
        gain *= 2;
    if (IS_AFFECTED(ch, AFF_REGENERATION))
        gain *= 2;

    return UMIN(gain, ch->max_hit - ch->hit);
}

int mana_gain(Char *ch) {
    int gain;
    int number;

    if (ch->is_npc()) {
        gain = 5 + ch->level;
        switch (ch->position) {
        default: gain /= 2; break;
        case POS_SLEEPING: gain = 3 * gain / 2; break;
        case POS_RESTING: break;
        case POS_FIGHTING: gain /= 3; break;
        }
    } else {
        gain = (get_curr_stat(ch, Stat::Wis) + get_curr_stat(ch, Stat::Int) + ch->level) / 2;
        number = number_percent();
        if (number < get_skill_learned(ch, gsn_meditation)) {
            gain += number * gain / 100;
            if (ch->mana < ch->max_mana)
                check_improve(ch, gsn_meditation, true, 8);
        }
        gain = (gain * class_table[ch->class_num].fMana) / 7;

        switch (ch->position) {
        case POS_SLEEPING: break;
        case POS_RESTING: gain /= 1.7; break;
        default: gain /= 2; break;
        }

        if (ch->pcdata->condition[COND_FULL] == 0)
            gain /= 2;

        if (ch->pcdata->condition[COND_THIRST] == 0)
            gain /= 2;
    }

    if (IS_AFFECTED(ch, AFF_POISON))
        gain /= 4;

    if (IS_AFFECTED(ch, AFF_PLAGUE))
        gain /= 8;

    if (IS_AFFECTED(ch, AFF_LETHARGY))
        gain *= 2;

    return UMIN(gain, ch->max_mana - ch->mana);
}

int move_gain(Char *ch) {
    int gain;

    if (ch->is_npc()) {
        gain = ch->level;
    } else {
        gain = UMAX(15, ch->level);

        switch (ch->position) {
        case POS_SLEEPING: gain += get_curr_stat(ch, Stat::Dex); break;
        case POS_RESTING: gain += get_curr_stat(ch, Stat::Dex) / 2; break;
        }

        if (ch->pcdata->condition[COND_FULL] == 0)
            gain /= 2;

        if (ch->pcdata->condition[COND_THIRST] == 0)
            gain /= 2;
    }

    if (IS_AFFECTED(ch, AFF_POISON))
        gain /= 4;

    if (IS_AFFECTED(ch, AFF_PLAGUE))
        gain /= 8;

    if (IS_AFFECTED(ch, AFF_HASTE))
        gain /= 2;

    if (IS_AFFECTED(ch, AFF_LETHARGY))
        gain *= 2;

    return UMIN(gain, ch->max_move - ch->move);
}

void gain_condition(Char *ch, int iCond, int value) {
    int condition;

    if (value == 0 || ch->is_npc() || ch->level >= LEVEL_HERO)
        return;

    condition = ch->pcdata->condition[iCond];
    if (condition == -1)
        return;
    ch->pcdata->condition[iCond] = URANGE(0, condition + value, 48);

    if (ch->pcdata->condition[iCond] == 0) {
        switch (iCond) {
        case COND_FULL: ch->send_line("You are hungry."); break;

        case COND_THIRST: ch->send_line("You are thirsty."); break;

        case COND_DRUNK:
            if (condition != 0)
                ch->send_line("You are sober.");
            break;
        }
    }
}

/*
 * Mob autonomous action.
 * This function takes 25% to 35% of ALL Merc cpu time.
 * -- Furey
 */
void mobile_update() {
    Char *ch;
    Char *ch_next;
    EXIT_DATA *pexit;

    /* Examine all mobs. */
    for (ch = char_list; ch != nullptr; ch = ch_next) {
        ch_next = ch->next;

        if (ch->is_pc() || ch->in_room == nullptr || IS_AFFECTED(ch, AFF_CHARM))
            continue;

        if (ch->in_room->area->empty && !IS_SET(ch->act, ACT_UPDATE_ALWAYS))
            continue;

        /* Examine call for special procedure */
        if (ch->spec_fun != 0) {
            if ((*ch->spec_fun)(ch))
                continue;
        }

        /* That's all for sleeping / busy monster, and empty zones */
        if (ch->position != POS_STANDING)
            continue;

        /* Merc-2.2 MOBProgs - Faramir 31/8/1998 */
        /* MOBprogram random trigger */
        if (ch->in_room->area->nplayer > 0) {
            mprog_random_trigger(ch);
            /* If ch dies or changes
               position due to it's random
               trigger continue - Kahn */
            if (ch->position < POS_STANDING)
                continue;
        }
        /* Scavenge */
        if (IS_SET(ch->act, ACT_SCAVENGER) && !ch->in_room->contents.empty() && number_bits(6) == 0) {
            int max = 1;
            OBJ_DATA *obj_best{};
            for (auto *obj : ch->in_room->contents) {
                if (CAN_WEAR(obj, ITEM_TAKE) && can_loot(ch, obj) && obj->cost > max && obj->cost > 0) {
                    obj_best = obj;
                    max = obj->cost;
                }
            }

            if (obj_best) {
                obj_from_room(obj_best);
                obj_to_char(obj_best, ch);
                act("$n gets $p.", ch, obj_best, nullptr, To::Room);
                do_wear(ch, "all");
            }
        }

        /* Wander */
        auto opt_door = try_cast_direction(number_bits(5));
        if (!IS_SET(ch->act, ACT_SENTINEL) && number_bits(4) == 0 && opt_door
            && (pexit = ch->in_room->exit[*opt_door]) != nullptr && pexit->u1.to_room != nullptr
            && !IS_SET(pexit->exit_info, EX_CLOSED) && !IS_SET(pexit->u1.to_room->room_flags, ROOM_NO_MOB)
            && (!IS_SET(ch->act, ACT_STAY_AREA) || pexit->u1.to_room->area == ch->in_room->area)) {
            move_char(ch, *opt_door);
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
            if (IS_OUTSIDE(d.character()) && IS_AWAKE(d.character()))
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
        || ch->in_room != get_room_index(ROOM_VNUM_LIMBO))
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
            char_to_room(ch, get_room_index(ROOM_VNUM_LIMBO));
            if (ch->pet) { /* move pets too */
                if (ch->pet->fighting)
                    stop_fighting(ch->pet, true);
                act("$n flickers and phases out", ch->pet);
                ch->pet->was_in_room = ch->pet->in_room;
                char_from_room(ch->pet);
                char_to_room(ch->pet, get_room_index(ROOM_VNUM_LIMBO));
            }
        }
    }
}

/*
 * Update all chars, including mobs.
 */
void char_update() {
    Char *ch;
    Char *ch_next;
    Char *ch_quit;

    ch_quit = nullptr;

    /* update save counter */
    save_number++;

    if (save_number == save_every_n)
        save_number = 0;

    for (ch = char_list; ch != nullptr; ch = ch_next) {
        ch_next = ch->next;

        if (ch->timer > 30)
            ch_quit = ch;

        if (ch->position >= POS_STUNNED) {
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

        if (ch->position == POS_STUNNED)
            update_pos(ch);

        if (ch->is_pc() && ch->level < LEVEL_IMMORTAL) {
            OBJ_DATA *obj;

            if ((obj = get_eq_char(ch, WEAR_LIGHT)) != nullptr && obj->item_type == ITEM_LIGHT && obj->value[2] > 0) {
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
            gain_condition(ch, COND_DRUNK, -1);
            gain_condition(ch, COND_FULL, -1);
            gain_condition(ch, COND_THIRST, -1);
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
        if (ch->is_npc() && ch->pIndexData->vnum == MOB_VNUM_ZOMBIE) {
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

        if (is_affected(ch, gsn_plague) && ch != nullptr) {
            Char *vch;
            int save, dam;

            if (ch->in_room == nullptr)
                return;

            act("$n writhes in agony as plague sores erupt from $s skin.", ch);
            ch->send_line("You writhe in agony from the plague.");
            auto *existing_plague = ch->affected.find_by_skill(gsn_plague);
            if (existing_plague == nullptr) {
                REMOVE_BIT(ch->affected_by, AFF_PLAGUE);
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
            plague.bitvector = AFF_PLAGUE;

            for (vch = ch->in_room->people; vch != nullptr; vch = vch->next_in_room) {
                switch (check_immune(vch, DAM_DISEASE)) {
                case IS_NORMAL: save = existing_plague->level - 4; break;
                case IS_IMMUNE: save = 0; break;
                case IS_RESISTANT: save = existing_plague->level - 8; break;
                case IS_VULNERABLE: save = existing_plague->level; break;
                default: save = existing_plague->level - 4; break;
                }

                if (save != 0 && !saves_spell(save, vch) && vch->is_mortal() && !IS_AFFECTED(vch, AFF_PLAGUE)
                    && number_bits(4) == 0) {
                    vch->send_line("You feel hot and feverish.");
                    act("$n shivers and looks very ill.", vch);
                    affect_join(vch, plague);
                }
            }

            dam = UMIN(ch->level, 5);
            ch->mana -= dam;
            ch->move -= dam;
            damage(ch, ch, dam, gsn_plague, DAM_DISEASE);
        } else if (IS_AFFECTED(ch, AFF_POISON) && ch != nullptr) {
            act("$n shivers and suffers.", ch);
            ch->send_line("You shiver and suffer.");
            damage(ch, ch, 2, gsn_poison, DAM_POISON);
        } else if (ch->position == POS_INCAP && number_range(0, 1) == 0) {
            damage(ch, ch, 1, TYPE_UNDEFINED, DAM_NONE);
        } else if (ch->position == POS_MORTAL) {
            damage(ch, ch, 1, TYPE_UNDEFINED, DAM_NONE);
        }
    }

    /*
     * Autosave and autoquit.
     * Check that these chars still exist.
     */
    for (ch = char_list; ch != nullptr; ch = ch_next) {
        ch_next = ch->next;

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
    for (auto *obj : object_list) {
        Char *rch;
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
                    affect_remove_obj(obj, af);
                }
            }
            // Only report wear-offs for those affects who are completely gone.
            for (auto sn : removed_this_tick_with_msg)
                act(skill_table[sn].msg_off, obj->carried_by, obj, nullptr, To::Char, POS_SLEEPING);
        }

        if (obj->timer <= 0 || --obj->timer > 0)
            continue;

        switch (obj->item_type) {
        default: message = "$p crumbles into dust."; break;
        case ITEM_FOUNTAIN: message = "$p dries up."; break;
        case ITEM_CORPSE_NPC: message = "$p decays into dust."; break;
        case ITEM_CORPSE_PC: message = "$p decays into dust."; break;
        case ITEM_FOOD: message = "$p decomposes."; break;
        case ITEM_POTION: message = "$p has evaporated from disuse."; break;
        case ITEM_PORTAL: message = "$p shimmers and fades away."; break;
        }

        if (obj->carried_by != nullptr) {
            if ((obj->carried_by->is_npc()) && obj->carried_by->pIndexData->pShop != nullptr)
                obj->carried_by->gold += obj->cost;
            else
                act(message, obj->carried_by, obj, nullptr, To::Char);
        } else if (obj->in_room != nullptr && (rch = obj->in_room->people) != nullptr) {
            if (!(obj->in_obj && obj->in_obj->pIndexData->vnum == OBJ_VNUM_PIT && !CAN_WEAR(obj->in_obj, ITEM_TAKE))) {
                act(message, rch, obj, nullptr, To::Room);
                act(message, rch, obj, nullptr, To::Char);
            }
        }

        if (obj->item_type == ITEM_CORPSE_PC && !obj->contains.empty()) { /* save the contents */
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

        extract_obj(obj);
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
    Char *wch;
    Char *wch_next;
    Char *ch;
    Char *ch_next;

    for (wch = char_list; wch != nullptr; wch = wch_next) {
        wch_next = wch->next;

        /* Merc-2.2 MOBProgs - Faramir 31/8/1998 */
        /* MOBProgram ACT_PROG trigger */
        if (wch->is_npc() && wch->mpactnum > 0 && wch->in_room->area->nplayer > 0) {
            MPROG_ACT_LIST *tmp_act, *tmp2_act;
            for (tmp_act = wch->mpact; tmp_act != nullptr; tmp_act = tmp_act->next) {
                mprog_wordlist_check(tmp_act->buf, wch, tmp_act->ch, tmp_act->obj, tmp_act->vo, ACT_PROG);
                free_string(tmp_act->buf);
            }
            for (tmp_act = wch->mpact; tmp_act != nullptr; tmp_act = tmp2_act) {
                tmp2_act = tmp_act->next;
                free_mem(tmp_act, sizeof(MPROG_ACT_LIST));
            }
            wch->mpactnum = 0;
            wch->mpact = nullptr;
        }

        if (wch->is_npc() || wch->level >= LEVEL_IMMORTAL || wch->in_room == nullptr || wch->in_room->area->empty)
            continue;

        for (ch = wch->in_room->people; ch != nullptr; ch = ch_next) {

            ch_next = ch->next_in_room;
            if (ch->is_npc())
                do_aggressive_sentient(wch, ch);
        }
    }
}

void do_aggressive_sentient(Char *wch, Char *ch) {
    Char *vch;
    Char *vch_next;
    Char *victim;
    int count;

    if (IS_SET(ch->act, ACT_SENTIENT) && ch->fighting == nullptr && !IS_AFFECTED(ch, AFF_CALM) && IS_AWAKE(ch)
        && !IS_AFFECTED(ch, AFF_CHARM) && can_see(ch, wch)) {
        if (ch->hit == ch->max_hit && ch->mana == ch->max_mana)
            ch->sentient_victim.clear();
        if (matches(wch->name, ch->sentient_victim)) {
            if (is_safe_sentient(ch, wch))
                return;
            ch->yell(fmt::format("|WAha! I never forget a face, prepare to die {}!!!|w", wch->name));
            multi_hit(ch, wch, TYPE_UNDEFINED);
        }
    }
    if (IS_SET(ch->act, ACT_AGGRESSIVE) && !IS_SET(ch->in_room->room_flags, ROOM_SAFE) && !IS_AFFECTED(ch, AFF_CALM)
        && (ch->fighting == nullptr) // Changed by Moog
        && !IS_AFFECTED(ch, AFF_CHARM) && IS_AWAKE(ch) && !(IS_SET(ch->act, ACT_WIMPY) && IS_AWAKE(wch))
        && can_see(ch, wch) && !number_bits(1) == 0) {

        /*
         * Ok we have a 'wch' player character and a 'ch' npc aggressor.
         * Now make the aggressor fight a RANDOM pc victim in the room,
         *   giving each 'vch' an equal chance of selection.
         */

        count = 0;
        victim = nullptr;
        for (vch = wch->in_room->people; vch != nullptr; vch = vch_next) {
            vch_next = vch->next_in_room;

            if (vch->is_pc() && vch->level < LEVEL_IMMORTAL && ch->level >= vch->level - 5
                && (!IS_SET(ch->act, ACT_WIMPY) || !IS_AWAKE(vch)) && can_see(ch, vch)) {
                if (number_range(0, count) == 0)
                    victim = vch;
                count++;
            }
        }

        if (victim != nullptr)
            multi_hit(ch, victim, TYPE_UNDEFINED);
    }
}

/*
 * ch - the sentient mob
 * wch - the hapless victim
 *  this func added to prevent the spam which was occuring when a sentient
 * mob encountered his foe in a safe room --fara
 */

bool is_safe_sentient(Char *ch, Char *wch) {
    if (ch->in_room == nullptr)
        return false;
    if (IS_SET(ch->in_room->room_flags, ROOM_SAFE)) {
        ch->yell(fmt::format("|WIf it weren't for the law, you'd be dead meat {}!!!|w", wch->name));
        ch->sentient_victim.clear();
        return true;
    }
    return false;
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
        log_new("TICK", EXTRA_WIZNET_TICK, 0);
        pulse_point = PULSE_TICK;
        /* number_range( PULSE_TICK / 2, 3 * PULSE_TICK / 2 ); */
        weather_update();
        char_update();
        obj_update();
        /*Rohan: update player counter if necessary */
        count_update();
        /*Rohan's challenge safety net*/
        do_chal_tick();
        /* web update */
        web_who();

        /* tip wizard */
        if (ignore_tips == false) /* once every two ticks */
            tip_players();
    }

    aggr_update();
}
