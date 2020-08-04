/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                   */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  update.c:  keeps everything rolling                                  */
/*                                                                       */
/*************************************************************************/

#include "merc.h"
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

/* command procedures needed */
void do_quit(CHAR_DATA *ch, char *arg);
void do_wear(CHAR_DATA *ch, char *arg);
void do_yell(CHAR_DATA *ch, char *arg);

/* Few external functions */
int get_max_stat(CHAR_DATA *ch, int stat);

/*
 * Local functions.
 */
int hit_gain(CHAR_DATA *ch);
int mana_gain(CHAR_DATA *ch);
int move_gain(CHAR_DATA *ch);
void mobile_update();
void weather_update();
void char_update();
void obj_update();
void aggr_update();
void announce(char *, CHAR_DATA *ch);

/* Added by Rohan to reset the count every day */
void count_update();
int count_updated = 0;

/* used for saving */
static const uint32_t save_every_n = 30u;
uint32_t save_number = 0;

/* Advancement stuff. */
void advance_level(CHAR_DATA *ch) {
    char buf[MAX_STRING_LENGTH];
    int add_hp;
    int add_mana;
    int add_move;
    int add_prac;

    ch->pcdata->last_level = (ch->played + (int)(current_time - ch->logon)) / 3600;

    snprintf(buf, sizeof(buf), "the %s", title_table[ch->class_num][ch->level][ch->sex == SEX_FEMALE ? 1 : 0]);
    set_title(ch, buf);

    add_hp = con_app[get_curr_stat(ch, STAT_CON)].hitp
             + number_range(class_table[ch->class_num].hp_min, class_table[ch->class_num].hp_max);

    add_mana = number_range(
        0, class_table[ch->class_num].fMana * class_table[ch->class_num].fMana
               * (UMAX(0, get_curr_stat(ch, STAT_WIS) - 15) + 2 * UMAX(0, get_curr_stat(ch, STAT_INT) - 15)));

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

    add_move = number_range(1, (get_curr_stat(ch, STAT_CON) + get_curr_stat(ch, STAT_DEX)) / 6);
    add_prac = wis_app[get_curr_stat(ch, STAT_WIS)].practice;

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

    if (!IS_NPC(ch))
        REMOVE_BIT(ch->act, PLR_BOUGHT_PET);

    snprintf(buf, sizeof(buf), "Your gain is: %d/%d hp, %d/%d m, %d/%d mv %d/%d prac.\n\r", add_hp, ch->max_hit,
             add_mana, ch->max_mana, add_move, ch->max_move, add_prac, ch->practice);
    send_to_char(buf, ch);
    snprintf(log_buf, LOG_BUF_SIZE, "### %s has made a level in room %u\n\r", ch->name, ch->in_room->vnum);
    log_string(log_buf);
    snprintf(log_buf, LOG_BUF_SIZE, "|W### |P%s |Whas made a level!!!|w", ch->name);
    announce(log_buf, ch);
}

void lose_level(CHAR_DATA *ch) {
    char buf[MAX_STRING_LENGTH];
    int add_hp;
    int add_mana;
    int add_move;
    int add_prac;

    ch->pcdata->last_level = (ch->played + (int)(current_time - ch->logon)) / 3600;

    snprintf(buf, sizeof(buf), "the %s", title_table[ch->class_num][ch->level][ch->sex == SEX_FEMALE ? 1 : 0]);
    set_title(ch, buf);

    add_hp = con_app[get_max_stat(ch, STAT_CON)].hitp
             + number_range(class_table[ch->class_num].hp_min, class_table[ch->class_num].hp_max);
    add_mana = (number_range(2, (2 * get_max_stat(ch, STAT_INT) + get_max_stat(ch, STAT_WIS)) / 5)
                * class_table[ch->class_num].fMana)
               / 10;
    add_move = number_range(1, (get_max_stat(ch, STAT_CON) + get_max_stat(ch, STAT_DEX)) / 6);
    add_prac = -(wis_app[get_max_stat(ch, STAT_WIS)].practice);

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

    if (!IS_NPC(ch))
        REMOVE_BIT(ch->act, PLR_BOUGHT_PET);

    snprintf(buf, sizeof(buf), "Your gain is: %d/%d hp, %d/%d m, %d/%d mv %d/%d prac.\n\r", add_hp, ch->max_hit,
             add_mana, ch->max_mana, add_move, ch->max_move, add_prac, ch->practice);
    send_to_char(buf, ch);
    snprintf(log_buf, LOG_BUF_SIZE, "|W### |P%s|W has lost a level!!!|w", ch->name);
    announce(log_buf, ch);
}

void gain_exp(CHAR_DATA *ch, int gain) {
    if (IS_NPC(ch) || ch->level >= LEVEL_HERO)
        return;

    ch->exp = UMAX(exp_per_level(ch, ch->pcdata->points), ch->exp + gain);
    if (gain >= 0) {
        while ((ch->level < LEVEL_HERO)
               && ((ch->exp) >= (unsigned long)((int)exp_per_level(ch, ch->pcdata->points) * ((int)ch->level + 1)))) {
            send_to_char("|WYou raise a level!!|w\n\r", ch);
            ch->level += 1;
            advance_level(ch);
        }
    }
}

/*
 * Regeneration stuff.
 */
int hit_gain(CHAR_DATA *ch) {
    int gain;
    int number;

    if (IS_NPC(ch)) {
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
        gain = UMAX(3, get_curr_stat(ch, STAT_CON) - 3 + ch->level / 1.5);
        gain += class_table[ch->class_num].hp_max - 7;
        number = number_percent();
        if (number < get_skill_learned(ch, gsn_fast_healing)) {
            gain += number * gain / 100;
            if (ch->hit < ch->max_hit)
                check_improve(ch, gsn_fast_healing, true, 8);
        }

        switch (ch->position) {
        case POS_SLEEPING: break;
        case POS_RESTING: gain /= 1.5; break;
        case POS_FIGHTING: gain /= 3; break;
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

int mana_gain(CHAR_DATA *ch) {
    int gain;
    int number;

    if (IS_NPC(ch)) {
        gain = 5 + ch->level;
        switch (ch->position) {
        default: gain /= 2; break;
        case POS_SLEEPING: gain = 3 * gain / 2; break;
        case POS_RESTING: break;
        case POS_FIGHTING: gain /= 3; break;
        }
    } else {
        gain = (get_curr_stat(ch, STAT_WIS) + get_curr_stat(ch, STAT_INT) + ch->level) / 2;
        number = number_percent();
        if (number < get_skill_learned(ch, gsn_meditation)) {
            gain += number * gain / 100;
            if (ch->mana < ch->max_mana)
                check_improve(ch, gsn_meditation, true, 8);
        }
        gain = (gain * class_table[ch->class_num].fMana) / 6;

        switch (ch->position) {
        case POS_SLEEPING: break;
        case POS_RESTING: gain /= 1.5; break;
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

int move_gain(CHAR_DATA *ch) {
    int gain;

    if (IS_NPC(ch)) {
        gain = ch->level;
    } else {
        gain = UMAX(15, ch->level);

        switch (ch->position) {
        case POS_SLEEPING: gain += get_curr_stat(ch, STAT_DEX); break;
        case POS_RESTING: gain += get_curr_stat(ch, STAT_DEX) / 2; break;
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

void gain_condition(CHAR_DATA *ch, int iCond, int value) {
    int condition;

    if (value == 0 || IS_NPC(ch) || ch->level >= LEVEL_HERO)
        return;

    condition = ch->pcdata->condition[iCond];
    if (condition == -1)
        return;
    ch->pcdata->condition[iCond] = URANGE(0, condition + value, 48);

    if (ch->pcdata->condition[iCond] == 0) {
        switch (iCond) {
        case COND_FULL: send_to_char("You are hungry.\n\r", ch); break;

        case COND_THIRST: send_to_char("You are thirsty.\n\r", ch); break;

        case COND_DRUNK:
            if (condition != 0)
                send_to_char("You are sober.\n\r", ch);
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
    CHAR_DATA *ch;
    CHAR_DATA *ch_next;
    EXIT_DATA *pexit;
    int door;

    /* Examine all mobs. */
    for (ch = char_list; ch != nullptr; ch = ch_next) {
        ch_next = ch->next;

        if (!IS_NPC(ch) || ch->in_room == nullptr || IS_AFFECTED(ch, AFF_CHARM))
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
        if (IS_SET(ch->act, ACT_SCAVENGER) && ch->in_room->contents != nullptr && number_bits(6) == 0) {
            OBJ_DATA *obj;
            OBJ_DATA *obj_best;
            int max;

            max = 1;
            obj_best = 0;
            for (obj = ch->in_room->contents; obj; obj = obj->next_content) {
                if (CAN_WEAR(obj, ITEM_TAKE) && can_loot(ch, obj) && obj->cost > max && obj->cost > 0) {
                    obj_best = obj;
                    max = obj->cost;
                }
            }

            if (obj_best) {
                obj_from_room(obj_best);
                obj_to_char(obj_best, ch);
                act("$n gets $p.", ch, obj_best, nullptr, TO_ROOM);
                do_wear(ch, "all");
            }
        }

        /* Wander */
        if (!IS_SET(ch->act, ACT_SENTINEL) && number_bits(4) == 0 && (door = number_bits(5)) <= 5
            && (pexit = ch->in_room->exit[door]) != nullptr && pexit->u1.to_room != nullptr
            && !IS_SET(pexit->exit_info, EX_CLOSED) && !IS_SET(pexit->u1.to_room->room_flags, ROOM_NO_MOB)
            && (!IS_SET(ch->act, ACT_STAY_AREA) || pexit->u1.to_room->area == ch->in_room->area)) {
            move_char(ch, door);
        }
    }
}

/*
 * Update the weather.
 */
void weather_update() {
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;
    int diff;

    buf[0] = '\0';

    switch (++time_info.hour) {
    case 5:
        weather_info.sunlight = SUN_LIGHT;
        strcat(buf, "The day has begun.\n\r");
        break;

    case 6:
        weather_info.sunlight = SUN_RISE;
        strcat(buf, "The sun rises in the east.\n\r");
        break;

    case 19:
        weather_info.sunlight = SUN_SET;
        strcat(buf, "The sun slowly disappears in the west.\n\r");
        break;

    case 20:
        weather_info.sunlight = SUN_DARK;
        strcat(buf, "The night has begun.\n\r");
        break;

    case 24:
        time_info.hour = 0;
        time_info.day++;
        break;
    }

    if (time_info.day >= 35) {
        time_info.day = 0;
        time_info.month++;
    }

    if (time_info.month >= 17) {
        time_info.month = 0;
        time_info.year++;
    }

    /*
     * Weather change.
     */
    if (time_info.month >= 9 && time_info.month <= 16)
        diff = weather_info.mmhg > 985 ? -2 : 2;
    else
        diff = weather_info.mmhg > 1015 ? -2 : 2;

    weather_info.change += diff * dice(1, 4) + dice(2, 6) - dice(2, 6);
    weather_info.change = UMAX(weather_info.change, -12);
    weather_info.change = UMIN(weather_info.change, 12);

    weather_info.mmhg += weather_info.change;
    weather_info.mmhg = UMAX(weather_info.mmhg, 960);
    weather_info.mmhg = UMIN(weather_info.mmhg, 1040);

    switch (weather_info.sky) {
    default:
        bug("Weather_update: bad sky %d.", weather_info.sky);
        weather_info.sky = SKY_CLOUDLESS;
        break;

    case SKY_CLOUDLESS:
        if (weather_info.mmhg < 990 || (weather_info.mmhg < 1010 && number_bits(2) == 0)) {
            strcat(buf, "The sky is getting cloudy.\n\r");
            weather_info.sky = SKY_CLOUDY;
        }
        break;

    case SKY_CLOUDY:
        if (weather_info.mmhg < 970 || (weather_info.mmhg < 990 && number_bits(2) == 0)) {
            strcat(buf, "It starts to rain.\n\r");
            weather_info.sky = SKY_RAINING;
        }

        if (weather_info.mmhg > 1030 && number_bits(2) == 0) {
            strcat(buf, "The clouds disappear.\n\r");
            weather_info.sky = SKY_CLOUDLESS;
        }
        break;

    case SKY_RAINING:
        if (weather_info.mmhg < 970 && number_bits(2) == 0) {
            strcat(buf, "Lightning flashes in the sky.\n\r");
            weather_info.sky = SKY_LIGHTNING;
        }

        if (weather_info.mmhg > 1030 || (weather_info.mmhg > 1010 && number_bits(2) == 0)) {
            strcat(buf, "The rain stopped.\n\r");
            weather_info.sky = SKY_CLOUDY;
        }
        break;

    case SKY_LIGHTNING:
        if (weather_info.mmhg > 1010 || (weather_info.mmhg > 990 && number_bits(2) == 0)) {
            strcat(buf, "The lightning has stopped.\n\r");
            weather_info.sky = SKY_RAINING;
            break;
        }
        break;
    }

    if (buf[0] != '\0') {
        for (d = descriptor_list; d != nullptr; d = d->next) {
            if (d->connected == CON_PLAYING && IS_OUTSIDE(d->character) && IS_AWAKE(d->character))
                send_to_char(buf, d->character);
        }
    }
}

/**
 * If a char was idle and been sent to the limbo room and they send a command, return them to
 * their previous room.
 */
void move_active_char_from_limbo(CHAR_DATA *ch) {
    if (ch == nullptr || ch->desc == nullptr || ch->desc->connected != CON_PLAYING || ch->was_in_room == nullptr
        || ch->in_room != get_room_index(ROOM_VNUM_LIMBO))
        return;

    ch->timer = 0;
    char_from_room(ch);
    char_to_room(ch, ch->was_in_room);
    ch->was_in_room = nullptr;
    act("$n has returned from the void.", ch, nullptr, nullptr, TO_ROOM);
    if (ch->pet) { /* move pets too */
        char_from_room(ch->pet);
        char_to_room(ch->pet, ch->in_room);
        ch->pet->was_in_room = nullptr;
        act("$n has returned from the void.", ch->pet, nullptr, nullptr, TO_ROOM);
    }
    return;
}

/**
 * If a chars is idle, move it into the "limbo" room along with its pets.
 */
void move_idle_char_to_limbo(CHAR_DATA *ch) {
    if (++ch->timer >= 12) {
        if (ch->was_in_room == nullptr && ch->in_room != nullptr) {
            ch->was_in_room = ch->in_room;
            if (ch->fighting != nullptr)
                stop_fighting(ch, true);
            act("$n disappears into the void.", ch, nullptr, nullptr, TO_ROOM);
            send_to_char("You disappear into the void.\n\r", ch);
            if (ch->level > 1)
                save_char_obj(ch);
            char_from_room(ch);
            char_to_room(ch, get_room_index(ROOM_VNUM_LIMBO));
            if (ch->pet) { /* move pets too */
                if (ch->pet->fighting)
                    stop_fighting(ch->pet, true);
                act("$n flickers and phases out", ch->pet, nullptr, nullptr, TO_ROOM);
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
    CHAR_DATA *ch;
    CHAR_DATA *ch_next;
    CHAR_DATA *ch_quit;

    ch_quit = nullptr;

    /* update save counter */
    save_number++;

    if (save_number == save_every_n)
        save_number = 0;

    for (ch = char_list; ch != nullptr; ch = ch_next) {
        AFFECT_DATA *paf;
        AFFECT_DATA *paf_next;

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

        if (!IS_NPC(ch) && ch->level < LEVEL_IMMORTAL) {
            OBJ_DATA *obj;

            if ((obj = get_eq_char(ch, WEAR_LIGHT)) != nullptr && obj->item_type == ITEM_LIGHT && obj->value[2] > 0) {
                if (--obj->value[2] == 0 && ch->in_room != nullptr) {
                    --ch->in_room->light;
                    act("$p goes out.", ch, obj, nullptr, TO_ROOM);
                    act("$p flickers and goes out.", ch, obj, nullptr, TO_CHAR);
                    extract_obj(obj);
                } else if (obj->value[2] <= 5 && ch->in_room != nullptr)
                    act("$p flickers.", ch, obj, nullptr, TO_CHAR);
            }

            if (IS_IMMORTAL(ch))
                ch->timer = 0;

            move_idle_char_to_limbo(ch);
            gain_condition(ch, COND_DRUNK, -1);
            gain_condition(ch, COND_FULL, -1);
            gain_condition(ch, COND_THIRST, -1);
        }

        for (paf = ch->affected; paf != nullptr; paf = paf_next) {
            paf_next = paf->next;
            if (paf->duration > 0) {
                paf->duration--;
                if (number_range(0, 4) == 0 && paf->level > 0)
                    paf->level--; /* spell strength fades with time */
            } else if (paf->duration < 0)
                ;
            else {
                if (paf_next == nullptr || paf_next->type != paf->type || paf_next->duration > 0) {
                    if (paf->type > 0 && skill_table[paf->type].msg_off) {
                        send_to_char(skill_table[paf->type].msg_off, ch);
                        send_to_char("\n\r", ch);

                        /*********************************/
                        /*if ( paf->type == gsn_berserk
                                              || paf->type == skill_lookup("frenzy"))
                                              {
                                                  wield = get_eq_char(ch, WEAR_WIELD);
                                                  if ( (wield !=nullptr)
                                                  && (wield->item_type == ITEM_WEAPON)
                                                  && (IS_SET(wield->value[4], WEAPON_FLAMING)) )
                                                  {

                                                      copy = get_object(wield->pIndexData->vnum);
                                                      wield->value[3] = copy->value[3];
                                                      extract_obj(copy);
                                                  }
                                              }*/
                        /*********************************/
                    }
                }

                affect_remove(ch, paf);
            }
        }

        /* scan all undead zombies created by raise_dead style spells
           and randomly decay them */
        if (IS_NPC(ch) && ch->pIndexData->vnum == MOB_VNUM_ZOMBIE) {
            if (number_percent() > 90) {
                act("$n fits violently before decaying in to a pile of dust.", ch, nullptr, nullptr, TO_ROOM);
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
            AFFECT_DATA *af, plague;
            CHAR_DATA *vch;
            int save, dam;

            if (ch->in_room == nullptr)
                return;

            act("$n writhes in agony as plague sores erupt from $s skin.", ch, nullptr, nullptr, TO_ROOM);
            send_to_char("You writhe in agony from the plague.\n\r", ch);
            for (af = ch->affected; af != nullptr; af = af->next) {
                if (af->type == gsn_plague)
                    break;
            }

            if (af == nullptr) {
                REMOVE_BIT(ch->affected_by, AFF_PLAGUE);
                return;
            }

            if (af->level == 1)
                return;

            plague.type = gsn_plague;
            plague.level = af->level - 1;
            plague.duration = number_range(1, 2 * plague.level);
            plague.location = APPLY_STR;
            plague.modifier = -5;
            plague.bitvector = AFF_PLAGUE;

            for (vch = ch->in_room->people; vch != nullptr; vch = vch->next_in_room) {
                switch (check_immune(vch, DAM_DISEASE)) {
                case (IS_NORMAL): save = af->level - 4; break;
                case (IS_IMMUNE): save = 0; break;
                case (IS_RESISTANT): save = af->level - 8; break;
                case (IS_VULNERABLE): save = af->level; break;
                default: save = af->level - 4; break;
                }

                if (save != 0 && !saves_spell(save, vch) && !IS_IMMORTAL(vch) && !IS_AFFECTED(vch, AFF_PLAGUE)
                    && number_bits(4) == 0) {
                    send_to_char("You feel hot and feverish.\n\r", vch);
                    act("$n shivers and looks very ill.", vch, nullptr, nullptr, TO_ROOM);
                    affect_join(vch, &plague);
                }
            }

            dam = UMIN(ch->level, 5);
            ch->mana -= dam;
            ch->move -= dam;
            damage(ch, ch, dam, gsn_plague, DAM_DISEASE);
        } else if (IS_AFFECTED(ch, AFF_POISON) && ch != nullptr) {
            act("$n shivers and suffers.", ch, nullptr, nullptr, TO_ROOM);
            send_to_char("You shiver and suffer.\n\r", ch);
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

        if (ch->desc != nullptr && ch->desc->descriptor % save_every_n == save_number)
            save_char_obj(ch);

        if (ch == ch_quit)
            do_quit(ch, "");
    }
}

/*
 * Update all objs.
 * This function is performance sensitive.
 */
void obj_update() {
    OBJ_DATA *obj;
    OBJ_DATA *obj_next;
    AFFECT_DATA *paf, *paf_next;

    for (obj = object_list; obj != nullptr; obj = obj_next) {
        CHAR_DATA *rch;
        char *message;

        obj_next = obj->next;

        /* go through affects and decrement */
        for (paf = obj->affected; paf != nullptr; paf = paf_next) {
            paf_next = paf->next;
            if (paf->duration > 0) {
                paf->duration--;
                if (number_range(0, 4) == 0 && paf->level > 0)
                    paf->level--; /* spell strength fades with time */
            } else if (paf->duration < 0)
                ;
            else {
                if (paf_next == nullptr || paf_next->type != paf->type || paf_next->duration > 0) {
                    if (paf->type > 0 && skill_table[paf->type].msg_off) {
                        act_new(skill_table[paf->type].msg_off, obj->carried_by, obj, nullptr, POS_SLEEPING, TO_CHAR);
                    }
                }

                affect_remove_obj(obj, paf);
            }
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
            if (IS_NPC(obj->carried_by) && obj->carried_by->pIndexData->pShop != nullptr)
                obj->carried_by->gold += obj->cost;
            else
                act(message, obj->carried_by, obj, nullptr, TO_CHAR);
        } else if (obj->in_room != nullptr && (rch = obj->in_room->people) != nullptr) {
            if (!(obj->in_obj && obj->in_obj->pIndexData->vnum == OBJ_VNUM_PIT && !CAN_WEAR(obj->in_obj, ITEM_TAKE))) {
                act(message, rch, obj, nullptr, TO_ROOM);
                act(message, rch, obj, nullptr, TO_CHAR);
            }
        }

        if (obj->item_type == ITEM_CORPSE_PC && obj->contains) { /* save the contents */
            OBJ_DATA *t_obj, *next_obj;

            for (t_obj = obj->contains; t_obj != nullptr; t_obj = next_obj) {
                next_obj = t_obj->next_content;
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
void do_aggressive_sentient(CHAR_DATA *, CHAR_DATA *);
void aggr_update() {
    CHAR_DATA *wch;
    CHAR_DATA *wch_next;
    CHAR_DATA *ch;
    CHAR_DATA *ch_next;

    for (wch = char_list; wch != nullptr; wch = wch_next) {
        wch_next = wch->next;

        /* Merc-2.2 MOBProgs - Faramir 31/8/1998 */
        /* MOBProgram ACT_PROG trigger */
        if (IS_NPC(wch) && wch->mpactnum > 0 && wch->in_room->area->nplayer > 0) {
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

        if (IS_NPC(wch) || wch->level >= LEVEL_IMMORTAL || wch->in_room == nullptr || wch->in_room->area->empty)
            continue;

        for (ch = wch->in_room->people; ch != nullptr; ch = ch_next) {

            ch_next = ch->next_in_room;
            if (IS_NPC(ch))
                do_aggressive_sentient(wch, ch);
        }
    }
}

void do_aggressive_sentient(CHAR_DATA *wch, CHAR_DATA *ch) {
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;
    CHAR_DATA *victim;
    int count;
    char buf[MAX_STRING_LENGTH];
    bool shout = false;

    if (IS_SET(ch->act, ACT_SENTIENT) && ch->fighting == nullptr && !IS_AFFECTED(ch, AFF_CALM) && IS_AWAKE(ch)
        && !IS_AFFECTED(ch, AFF_CHARM) && can_see(ch, wch)) {
        if (ch->hit == ch->max_hit && ch->mana == ch->max_mana) {
            free_string(ch->sentient_victim);
            ch->sentient_victim = str_dup("");
        }
        if ((ch->sentient_victim != nullptr) && (!str_cmp(wch->name, ch->sentient_victim))) {
            if (is_safe_sentient(ch, wch))
                return;
            snprintf(buf, sizeof(buf), "|WAha! I never forget a face, prepare to die %s!!!|w", wch->name);
            if (IS_SET(ch->comm, COMM_NOSHOUT)) {
                shout = true;
                REMOVE_BIT(ch->comm, COMM_NOSHOUT);
            }
            do_yell(ch, buf);
            if (shout)
                SET_BIT(ch->comm, COMM_NOSHOUT);
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

            if (!IS_NPC(vch) && vch->level < LEVEL_IMMORTAL && ch->level >= vch->level - 5
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

bool is_safe_sentient(CHAR_DATA *ch, CHAR_DATA *wch) {

    char buf[MAX_STRING_LENGTH];
    bool shout = false;

    if (ch->in_room == nullptr)
        return false;
    if (IS_SET(ch->in_room->room_flags, ROOM_SAFE)) {
        snprintf(buf, sizeof(buf), "|WIf it weren't for the law, you'd be dead meat %s!!!|w", wch->name);
        if (IS_SET(ch->comm, COMM_NOSHOUT)) {
            shout = true;
            REMOVE_BIT(ch->comm, COMM_NOSHOUT);
        }
        do_yell(ch, buf);
        if (shout)
            SET_BIT(ch->comm, COMM_NOSHOUT);
        free_string(ch->sentient_victim);
        ch->sentient_victim = str_dup("");
        return true;
    }
    return false;
}

/* This function resets the player count everyday */
void count_update() {
    struct tm *cur_time;
    int current_day;
    cur_time = localtime(&current_time);
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
    tail_chain();
}
