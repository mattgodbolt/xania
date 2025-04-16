/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                    */
/*  See the header to file: merc.h for original code copyrights          */
/*************************************************************************/

#include "AFFECT_DATA.hpp"
#include "AffectFlag.hpp"
#include "Char.hpp"
#include "CharActFlag.hpp"
#include "DamageTolerance.hpp"
#include "DamageType.hpp"
#include "Object.hpp"
#include "ObjectType.hpp"
#include "Room.hpp"
#include "RoomFlag.hpp"
#include "SkillTables.hpp"
#include "VnumRooms.hpp"
#include "act_info.hpp"
#include "comm.hpp"
#include "common/BitOps.hpp"
#include "db.h"
#include "fight.hpp"
#include "handler.hpp"
#include "lookup.h"
#include "magic.h"
#include "ride.hpp"

/* Faramir's rather devious tornado spell....watch out for players who
 * use this to annoy others! :)
 */

void tornado_teleport(Char *ch, Char *victim) {
    Room *room;

    for (;;) {
        room = get_room(number_range(0, 65535));
        if (room != nullptr)
            if (ch->can_see(*room) && !check_enum_bit(room->room_flags, RoomFlag::Private)
                && !check_enum_bit(room->room_flags, RoomFlag::Solitary))
                break;
    }

    if (victim != ch)
        victim->send_line("You are sucked helplessly into the tornado....");

    act("$n is consumed by the tornado and vanishes!", victim);
    char_from_room(victim);
    char_to_room(victim, room);
    victim->send_line("...you appear to have been blown to another part of Xania!");

    if (!ch->riding) {
        act("$n is blown into the room by a sudden blast of wind.", victim);
    } else {
        act("$n is blown into the room by a blast of wind, about 5 feet off the ground!", victim, nullptr, nullptr,
            To::Room);
        act("$n falls to the ground with a thud!", victim);
        act("You fall to the ground with a thud!", victim, nullptr, nullptr, To::Char);
        fallen_off_mount(ch);
    } /* end ride check */
    look_auto(victim);
}

void tornado_dam(Char *ch, Char *victim, int level) {
    int dam;
    int sn = skill_lookup("psychic tornado");

    dam = dice(level, 20);
    damage(ch, victim, dam, &skill_table[sn], DamageType::Mental);
}

void tornado_mental(Char *ch, Char *victim, int level) {

    AFFECT_DATA af;

    af.type = skill_lookup("psychic tornado");
    af.level = level;
    af.duration = 2;
    af.location = AffectLocation::Hitroll;
    af.modifier = -4;
    af.bitvector = to_int(AffectFlag::Blind);
    affect_to_char(victim, af);

    af.location = AffectLocation::Wis;
    af.modifier = -(1 + (level >= 20) + (level >= 30) + (level >= 50) + (level >= 75) + (level >= 91));
    af.bitvector = 0;
    affect_to_char(victim, af);
    af.location = AffectLocation::Int;
    af.modifier = -(1 + (level >= 20) + (level >= 30) + (level >= 50) + (level >= 75) + (level >= 91));
    affect_to_char(victim, af);
    ch->wait_state(PULSE_VIOLENCE * 2);
    victim->position = Position::Type::Resting;
}

void spell_psy_tornado(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    (void)sn;
    (void)level;
    Char *victim = spell_target.getChar();
    if (!victim)
        return;
    if (victim == ch) {
        ch->send_line("You can't cast that on yourself.");
        return;
    }

    if (victim->is_pc()) {
        ch->send_line("Not on that target.");
        return;
    }

    if (is_safe(ch, victim))
        return;

    if (ch->mana < (ch->max_mana - 100)) {
        ch->send_line("This powerful spell requires all of your psychic powers.");
        return;
    }

    if (check_damage_tolerance(victim, DamageType::Mental) == DamageTolerance::Immune) {
        act("The great mental fortitude of $N deflects your psychic blast!", ch, nullptr, victim, To::Char);
        act("$n attempts to unleash a psychic blast upon $N, but $s efforts backfire with\n\rcatastrophic effects!", ch,
            nullptr, victim, To::NotVict);
        act("$n attempts to unleash a psychic blast upon you, but your mental fortitude\n\rdeflects it.", ch, nullptr,
            victim, To::Vict);

        /* HIT THE BASTARD! */
        tornado_dam(victim, ch, ch->level);
        tornado_mental(victim, ch, ch->level);
        ch->send_line("You feel sick and disorientated.");
        act("$n is knocked to the ground and stunned.", ch, nullptr, victim, To::Room);
        return;
    } else {
        act("You summon all of your psychic powers and unleash them upon $N!", ch, nullptr, victim, To::Char);

        act("You detect a surge of psychic energy as $n unleashes his will upon $N!", ch, nullptr, victim, To::NotVict);

        act("A terrifying grimace appears on $n's face as $s unleashes a powerful psychic\n\rblast upon you!", ch,
            nullptr, victim, To::Vict);

        tornado_dam(ch, victim, ch->level);
        tornado_mental(ch, victim, ch->level);
        ch->send_line("Your mighty blast spirals out of control, forming a towering tornado of psychic energy!");
        act("$n's psychic blast spirals into a |Rhuge|w tornado of energy!", ch, nullptr, victim, To::Room);

        if (!check_enum_bit(victim->act, CharActFlag::Aggressive)) {
            if (number_percent() > 75)
                tornado_teleport(ch, victim);
        }

        for (auto &&uch : char_list) {
            auto *vch = uch.get();
            if (vch->in_room == nullptr)
                continue;
            if (vch->in_room == ch->in_room) {
                if (vch != ch && !is_safe_spell(ch, vch, true) && (vch != victim)
                    && (check_damage_tolerance(vch, DamageType::Mental) != DamageTolerance::Immune))
                    tornado_mental(ch, victim, ch->level);
                continue;
            }

            if (vch->in_room->area == ch->in_room->area)
                vch->send_line("A faint psychic surge dazes you momentarily.");
        }

        for (auto direction : all_directions) {
            /* No exits in that direction */
            if (const auto &exit = ch->in_room->exits[direction]) {
                if (auto *room = exit->u1.to_room) {
                    for (auto *current_person : room->people) {
                        current_person->send_line("Suddenly, a gale of psychic energy blows through the room!");
                        if (!is_safe_spell(ch, current_person, true)
                            && (check_damage_tolerance(current_person, DamageType::Mental) != DamageTolerance::Immune))
                            tornado_mental(ch, current_person, ch->level);

                    } /* Closes the for_each_char_loop */
                }
            }
        } /* closes main loop for each direction */
    }
}
