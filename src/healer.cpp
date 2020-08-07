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

#include "magic.h"
#include "merc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>

void do_heal(CHAR_DATA *ch, const char *argument) {
    CHAR_DATA *mob;
    char arg[MAX_INPUT_LENGTH];
    int cost, sn;
    SpellFunc spell;
    const char *words;

    /* check for healer */
    for (mob = ch->in_room->people; mob; mob = mob->next_in_room) {
        if (IS_NPC(mob) && IS_SET(mob->act, ACT_IS_HEALER))
            break;
    }

    if (mob == nullptr) {
        send_to_char("You can't do that here.\n\r", ch);
        return;
    }

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        /* display price list */
        act("$N says 'I offer the following spells:'", ch, nullptr, mob, TO_CHAR);
        send_to_char("  light: cure light wounds      1000 gold\n\r", ch);
        send_to_char("  serious: cure serious wounds  1600 gold\n\r", ch);
        send_to_char("  critic: cure critical wounds  2500 gold\n\r", ch);
        send_to_char("  heal: healing spell	      5000 gold\n\r", ch);
        send_to_char("  blind: cure blindness         2000 gold\n\r", ch);
        send_to_char("  disease: cure disease         1500 gold\n\r", ch);
        send_to_char("  poison:  cure poison	      2500 gold\n\r", ch);
        send_to_char("  uncurse: remove curse	      5000 gold\n\r", ch);
        send_to_char("  refresh: restore movement      500 gold\n\r", ch);
        send_to_char("  mana:  restore mana	      1000 gold\n\r", ch);
        send_to_char(" Type heal <type> to be healed.\n\r", ch);
        return;
    }

    switch (arg[0]) {
    case 'l':
        spell = spell_cure_light;
        sn = skill_lookup("cure light");
        words = "judicandus dies";
        cost = 1000;
        break;

    case 's':
        spell = spell_cure_serious;
        sn = skill_lookup("cure serious");
        words = "judicandus gzfuajg";
        cost = 1600;
        break;

    case 'c':
        spell = spell_cure_critical;
        sn = skill_lookup("cure critical");
        words = "judicandus qfuhuqar";
        cost = 2500;
        break;

    case 'h':
        spell = spell_heal;
        sn = skill_lookup("heal");
        words = "pzar";
        cost = 5000;
        break;

    case 'b':
        spell = spell_cure_blindness;
        sn = skill_lookup("cure blindness");
        words = "judicandus noselacri";
        cost = 2000;
        break;

    case 'd':
        spell = spell_cure_disease;
        sn = skill_lookup("cure disease");
        words = "judicandus eugzagz";
        cost = 1500;
        break;

    case 'p':
        spell = spell_cure_poison;
        sn = skill_lookup("cure poison");
        words = "judicandus sausabru";
        cost = 2500;
        break;

    case 'u':
        spell = spell_remove_curse;
        sn = skill_lookup("remove curse");
        words = "candussido judifgz";
        cost = 5000;
        break;

    case 'r':
        spell = spell_refresh;
        sn = skill_lookup("refresh");
        words = "candusima";
        cost = 500;
        break;

    case 'm':
        spell = nullptr;
        sn = -1;
        words = "energizer";
        cost = 1000;
        break;

    default: act("$N says 'Type 'heal' for a list of spells.'", ch, nullptr, mob, TO_CHAR); return;
    }

    if (cost > ch->gold) {
        act("$N says 'You do not have enough gold for my services.'", ch, nullptr, mob, TO_CHAR);
        return;
    }

    WAIT_STATE(ch, PULSE_VIOLENCE);

    ch->gold -= cost;
    mob->gold += cost;
    act("$n utters the words '$T'.", mob, nullptr, words, TO_ROOM);

    if (spell == nullptr) /* restore mana trap...kinda hackish */
    {
        ch->mana += dice(2, 8) + mob->level / 4;
        ch->mana = UMIN(ch->mana, ch->max_mana);
        send_to_char("A warm glow passes through you.\n\r", ch);
        return;
    }

    if (sn == -1)
        return;

    spell(sn, mob->level, mob, ch);
}
