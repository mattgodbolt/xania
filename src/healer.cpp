/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                    */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  healer.c: healer code written for Merc 2.0 by Russ Taylor            */
/*                                                                       */
/*************************************************************************/

/* Healer code written for Merc 2.0 muds by Alander
   direct questions or comments to rtaylor@cie-2.uoregon.edu
   any use of this code must include this header */

#include "comm.hpp"
#include "magic.h"
#include "merc.h"
#include "string_utils.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>

void do_heal(Char *ch, const char *argument) {
    Char *mob;
    char arg[MAX_INPUT_LENGTH];
    int cost, sn;
    SpellFunc spell;
    const char *words;

    /* check for healer */
    for (mob = ch->in_room->people; mob; mob = mob->next_in_room) {
        if (mob->is_npc() && IS_SET(mob->act, ACT_IS_HEALER))
            break;
    }

    if (mob == nullptr) {
        ch->send_line("You can't do that here.");
        return;
    }

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        /* display price list */
        act("$N says 'I offer the following spells:'", ch, nullptr, mob, To::Char);
        ch->send_line("  light: cure light wounds      1000 gold");
        ch->send_line("  serious: cure serious wounds  1600 gold");
        ch->send_line("  critical: cure critical wounds  2500 gold");
        ch->send_line("  heal: healing spell           5000 gold");
        ch->send_line("  blind: cure blindness         2000 gold");
        ch->send_line("  disease: cure disease         1500 gold");
        ch->send_line("  poison:  cure poison          2500 gold");
        ch->send_line("  uncurse: remove curse         5000 gold");
        ch->send_line("  refresh: restore movement      500 gold");
        ch->send_line("  mana:  restore mana           1000 gold");
        ch->send_line(" Type heal <type> to be healed.");
        return;
    }

    if (matches_start(arg, "light")) {
        spell = spell_cure_light;
        sn = skill_lookup("cure light");
        words = "judicandus dies";
        cost = 1000;
    } else if (matches_start(arg, "serious")) {
        spell = spell_cure_serious;
        sn = skill_lookup("cure serious");
        words = "judicandus gzfuajg";
        cost = 1600;
    } else if (matches_start(arg, "critical")) {
        spell = spell_cure_critical;
        sn = skill_lookup("cure critical");
        words = "judicandus qfuhuqar";
        cost = 2500;
    } else if (matches_start(arg, "heal")) {
        spell = spell_heal;
        sn = skill_lookup("heal");
        words = "pzar";
        cost = 5000;
    } else if (matches_start(arg, "blind")) {
        spell = spell_cure_blindness;
        sn = skill_lookup("cure blindness");
        words = "judicandus noselacri";
        cost = 2000;
    } else if (matches_start(arg, "disease")) {
        spell = spell_cure_disease;
        sn = skill_lookup("cure disease");
        words = "judicandus eugzagz";
        cost = 1500;
    } else if (matches_start(arg, "poison")) {
        spell = spell_cure_poison;
        sn = skill_lookup("cure poison");
        words = "judicandus sausabru";
        cost = 2500;
    } else if (matches_start(arg, "uncurse")) {
        spell = spell_remove_curse;
        sn = skill_lookup("remove curse");
        words = "candussido judifgz";
        cost = 5000;
    } else if (matches_start(arg, "refresh")) {
        spell = spell_refresh;
        sn = skill_lookup("refresh");
        words = "candusima";
        cost = 500;
    } else if (matches_start(arg, "mana")) {
        spell = nullptr;
        sn = -1;
        words = "energizer";
        cost = 1000;
    } else {
        act("$N says 'Sorry, I am unfamiliar with that spell. Type 'heal' for a list of spells.'", ch, nullptr, mob,
            To::Char);
        return;
    }

    if (cost > ch->gold) {
        act("$N says 'You do not have enough gold for my services.'", ch, nullptr, mob, To::Char);
        return;
    }

    WAIT_STATE(ch, PULSE_VIOLENCE);

    ch->gold -= cost;
    mob->gold += cost;
    act("$n utters the words '$T'.", mob, nullptr, words, To::Room);

    if (spell == nullptr) /* restore mana trap...kinda hackish */
    {
        ch->mana += dice(2, 8) + mob->level / 4;
        ch->mana = UMIN(ch->mana, ch->max_mana);
        ch->send_line("A warm glow passes through you.");
        return;
    }

    if (sn == -1)
        return;

    spell(sn, mob->level, mob, ch);
}
