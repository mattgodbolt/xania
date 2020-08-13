/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                    */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  magic2.c:  more spells, magic.c is a monster file                    */
/*                                                                       */
/*************************************************************************/
/*
 * Xania --
 * newer spells. magic.c is a monster already.
 */

#include "challeng.h"
#include "comm.hpp"
#include "interp.h"
#include "magic.h"
#include "merc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

/* Faramir's rather devious tornado spell....watch out for players who
 * use this to annoy others! :)
 */

void tornado_teleport(CHAR_DATA *ch, CHAR_DATA *victim) {
    ROOM_INDEX_DATA *pRoomIndex;

    for (;;) {
        pRoomIndex = get_room_index(number_range(0, 65535));
        if (pRoomIndex != nullptr)
            if (can_see_room(ch, pRoomIndex) && !IS_SET(pRoomIndex->room_flags, ROOM_PRIVATE)
                && !IS_SET(pRoomIndex->room_flags, ROOM_SOLITARY))
                break;
    }

    if (victim != ch)
        send_to_char("You are sucked helplessly into the tornado....\n\r", victim);

    act("$n is consumed by the tornado and vanishes!", victim);
    char_from_room(victim);
    char_to_room(victim, pRoomIndex);
    send_to_char("...you appear to have been blown to another part of Xania!", victim);

    if (!ch->riding) {
        act("$n is blown into the room by a sudden blast of wind.", victim);
    } else {
        act("$n is blown into the room by a blast of wind, about 5 feet off the ground!", victim, nullptr, nullptr,
            To::Room);
        act("$n falls to the ground with a thud!", victim);
        act("You fall to the ground with a thud!", victim, nullptr, nullptr, To::Char);
        fallen_off_mount(ch);
    } /* end ride check */
    do_look(victim, "auto");
}

void tornado_dam(CHAR_DATA *ch, CHAR_DATA *victim, int level) {
    int dam;
    int sn = skill_lookup("psychic tornado");

    dam = dice(level, 20);
    damage(ch, victim, dam, sn, DAM_MENTAL);
}

void tornado_mental(CHAR_DATA *ch, CHAR_DATA *victim, int level) {

    AFFECT_DATA af;

    af.type = skill_lookup("psychic tornado");
    af.level = level;
    af.duration = 2;
    af.location = APPLY_HITROLL;
    af.modifier = -4;
    af.bitvector = AFF_BLIND;
    affect_to_char(victim, &af);

    af.location = APPLY_WIS;
    af.modifier = -(1 + (level >= 20) + (level >= 30) + (level >= 50) + (level >= 75) + (level >= 91));
    af.bitvector = 0;
    affect_to_char(victim, &af);
    af.location = APPLY_INT;
    af.modifier = -(1 + (level >= 20) + (level >= 30) + (level >= 50) + (level >= 75) + (level >= 91));
    affect_to_char(victim, &af);
    WAIT_STATE(ch, PULSE_VIOLENCE * 2);
    victim->position = POS_RESTING;
}

void spell_psy_tornado(int sn, int level, CHAR_DATA *ch, void *vo) {
    (void)sn;
    (void)level;
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    CHAR_DATA *current_person;
    CHAR_DATA *next_person;
    ROOM_INDEX_DATA *room;
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;
    EXIT_DATA *pexit;
    int direction;

    if (victim == ch) {
        send_to_char("You can't cast that on yourself.\n\r", ch);
        return;
    }

    if (!IS_NPC(victim)) {
        send_to_char("Not on that target.\n\r", ch);
        return;
    }

    if (is_safe(ch, victim))
        return;

    if (ch->in_room->vnum == CHAL_ROOM) {
        send_to_char("Not in the challenge room.\n\r", ch);
        return;
    }

    if (ch->mana < (ch->max_mana - 100)) {
        send_to_char("This powerful spell requires all of your psychic powers.\n\r", ch);
        return;
    }

    if (check_immune(victim, DAM_MENTAL) == IS_IMMUNE) {
        act("The great mental fortitude of $N deflects your psychic blast!", ch, nullptr, victim, To::Char);
        act("$n attempts to unleash a psychic blast upon $N, but $s efforts backfire with\n\rcatastrophic effects!", ch,
            nullptr, victim, To::NotVict);
        act("$n attempts to unleash a psychic blast upon you, but your mental fortitude\n\rdeflects it.", ch, nullptr,
            victim, To::Vict);

        /* HIT THE BASTARD! */
        tornado_dam(victim, ch, ch->level);
        tornado_mental(victim, ch, ch->level);
        send_to_char("You feel sick and disorientated.\n\r", ch);
        act("$n is knocked to the ground and stunned.", ch, nullptr, victim, To::Room);
        return;
    } else {
        act("You summon all of your psychic powers and unleash them upon $N!", ch, nullptr, victim, To::Char);

        act("You detect a surge of psychic energy as $n unleashes his will upon $N!", ch, nullptr, victim, To::NotVict);

        act("A terrifying grimace appears on $n's face as $s unleashes a powerful psychic\n\rblast upon you!", ch,
            nullptr, victim, To::Vict);

        tornado_dam(ch, victim, ch->level);
        tornado_mental(ch, victim, ch->level);
        send_to_char("Your mighty blast spirals out of control, forming a towering tornado of psychic energy!\n\r", ch);
        act("$n's psychic blast spirals into a |Rhuge|w tornado of energy!", ch, nullptr, victim, To::Room);

        if (!IS_SET(victim->act, ACT_AGGRESSIVE)) {
            if (number_percent() > 75)
                tornado_teleport(ch, victim);
        }

        for (vch = char_list; vch != nullptr; vch = vch_next) {
            vch_next = vch->next;
            if (vch->in_room == nullptr)
                continue;
            if (vch->in_room == ch->in_room) {
                if (vch != ch && !is_safe_spell(ch, vch, true) && (vch != victim)
                    && (check_immune(vch, DAM_MENTAL) != IS_IMMUNE))
                    tornado_mental(ch, victim, ch->level);
                continue;
            }

            if (vch->in_room->area == ch->in_room->area)
                send_to_char("A faint psychic surge dazes you momentarily.\n\r", vch);
        }

        for (direction = 0; direction < 6; direction++) {
            /* No exits in that direction */

            pexit = ch->in_room->exit[direction];

            if (pexit != nullptr) {
                room = pexit->u1.to_room;
                if (room != nullptr) {

                    for (current_person = room->people; current_person != nullptr; current_person = next_person) {
                        next_person = current_person->next_in_room;

                        send_to_char("Suddenly, a gale of psychic energy blows through the room!\n\r", current_person);
                        if (!is_safe_spell(ch, current_person, true)
                            && (check_immune(current_person, DAM_MENTAL) != IS_IMMUNE))
                            tornado_mental(ch, current_person, ch->level);

                    } /* Closes the for_each_char_loop */
                }
            }
        } /* closes main loop for each direction */
    }
}

/* commented out for time being --Fara  */

// void spell_raise_dead (int sn, int level, CHAR_DATA *ch, void *vo)
//{
// OBJ_DATA *victim;
// AFFECT_DATA af;
//
/* no corpse raising in a lawful place please */
//
//  if (IS_SET(ch->in_room->room_flags , ROOM_LAW)) {
//  send_to_char( "Raising the dead is not permitted here.\n\r"
//   , ch);
//  return;
//}
//
//
//
/*
 * Find a corpse to work on
 */
//  for (victim = ch->in_room->contents; victim; victim = victim->next_content) {
//  if (victim->pIndexData->vnum == OBJ_VNUM_CORPSE_NPC) {
//    CHAR_DATA *zombie;
//    MOB_INDEX_DATA *zIndex;
//    OBJ_DATA *obj, *objNext;
//    int zLevel, i;
//    float zScale;
//
//    zIndex = get_mob_index (MOB_VNUM_ZOMBIE);
//    if (zIndex == nullptr) {
// bug ("Unable to find a zombie mob in spell_raise_dead!");
// send_to_char ("Urk!  Something terrible has happened!\n\r", ch);
// return;
//    }
//
/*
 * Work out some stats for the zombie based on its level
 * The zombie default level is 10, everythin is scaled
 * linearly
//     */
//    zLevel = UMIN (ch->level, victim->level) - 5; /* XXX needs sorting out */
//    if (zLevel < 1)
// zLevel = 1;
//    zScale = zLevel / 10.f;
//
//    zombie = create_mobile(zIndex);
//    char_to_room (zombie, ch->in_room);
//
//    zombie->level = zLevel;
//    zombie->hit *= zScale;
//    zombie->max_hit *= zScale;
//    zombie->mana *= zScale;
//    zombie->max_mana *= zScale;
//    zombie->move *= zScale;
//    zombie->max_move *= zScale;
//    zombie->hitroll *= zScale;
//    zombie->damroll *= zScale;
//
//    for (i = 0; i < 4; ++i)
// zombie->armor[i] *= zScale;
//
//    zombie->alignment = URANGE (-1000, ch->alignment - 100, -100);
//
/*
 * Do some sparks and fizzes XXX
 */
//      act ("$n calls upon black magic to raise the dead!\n\rThe $p comes to life as a zombie!",
//   ch, victim, nullptr, To::Room);
//    send_to_char ("You disturb the slumber of a corpse!\n\r", ch);
//
/* and now make this zombie a member of the group */
/* uses code very similar to spell_charm_person() */
//
//    zombie->affected_by |= AFF_CHARM;
//    add_follower (zombie, ch);
//    zombie->leader = ch;
//    af.type      = sn;
//    af.level    = level;
//    af.duration  = number_fuzzy( level / 4 );
//    af.location  = 0;
//    af.modifier  = 0;
//    af.bitvector = AFF_CHARM;
//    affect_to_char( zombie, &af );
//    act( "Isn't $n just so nice?", ch, nullptr, victim, To::Vict );
//    act("$N gazes at you through blood-chilling eye sockets.",ch,nullptr,zombie,To::Char);
//
/*
 * Equip the zombie with the corpse contents
 */
//    for (obj = victim->contains; obj; obj = objNext) {
// objNext = obj->next_content;
// if (obj->item_type == ITEM_MONEY) {
//  zombie->gold += obj->value[0];
//  extract_obj (obj);
//} else {
//  obj_from_obj (obj);
//  obj_to_char (obj, zombie);
//}
//
//    }
//
/*
 * Delete the corpse
 */
//    extract_obj (victim);
//
//    return;
//  }
//}
/*
 * No corpses
 */
// send_to_char ("Raising the dead requires a fresh corpse to work with.\n\r", ch);
// return;
//}
