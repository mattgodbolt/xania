/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                    */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  act_info.c: standard information functions                           */
/*                                                                       */
/*************************************************************************/

#include "buffer.h"
#include "db.h"
#include "merc.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>

/* command procedures needed */
DECLARE_DO_FUN(do_exits);
DECLARE_DO_FUN(do_look);
DECLARE_DO_FUN(do_help);
DECLARE_DO_FUN(do_mstat);
void do_affected(CHAR_DATA *ch);

char *const where_name[] = {"<used as light>     ", "<worn on finger>    ", "<worn on finger>    ",
                            "<worn around neck>  ", "<worn around neck>  ", "<worn on body>      ",
                            "<worn on head>      ", "<worn on legs>      ", "<worn on feet>      ",
                            "<worn on hands>     ", "<worn on arms>      ", "<worn as shield>    ",
                            "<worn about body>   ", "<worn about waist>  ", "<worn around wrist> ",
                            "<worn around wrist> ", "<wielded>           ", "<held>              "};

/* for do_count */
int max_on = 0;

/*
 * Local functions.
 */
char *format_obj_to_char args((OBJ_DATA * obj, CHAR_DATA *ch, bool fShort));
void show_list_to_char args((OBJ_DATA * list, CHAR_DATA *ch, bool fShort, bool fShowNothing));
void show_char_to_char_0 args((CHAR_DATA * victim, CHAR_DATA *ch));
void show_char_to_char_1 args((CHAR_DATA * victim, CHAR_DATA *ch));
void show_char_to_char args((CHAR_DATA * list, CHAR_DATA *ch));
bool check_blind args((CHAR_DATA * ch));

/* Mg's funcy shun */
void set_prompt args((CHAR_DATA * ch, char *prompt));

char *format_obj_to_char(OBJ_DATA *obj, CHAR_DATA *ch, bool fShort) {
    static char buf[MAX_STRING_LENGTH];

    buf[0] = '\0';
    if (IS_OBJ_STAT(obj, ITEM_INVIS))
        strcat(buf, "(|cInvis|w) ");
    if (IS_AFFECTED(ch, AFF_DETECT_EVIL) && IS_OBJ_STAT(obj, ITEM_EVIL))
        strcat(buf, "(|rRed Aura|w) ");
    if (IS_AFFECTED(ch, AFF_DETECT_MAGIC) && IS_OBJ_STAT(obj, ITEM_MAGIC))
        strcat(buf, "(|gMagical|w) ");
    if (IS_OBJ_STAT(obj, ITEM_GLOW))
        strcat(buf, "(|WGlowing|w) ");
    if (IS_OBJ_STAT(obj, ITEM_HUM))
        strcat(buf, "(|yHumming|w) ");

    if (fShort) {
        if (obj->short_descr != NULL)
            strcat(buf, obj->short_descr);
    } else {
        if (obj->description != NULL)
            strcat(buf, obj->description);
    }

    if ((int)strlen(buf) <= 0)
        strcat(buf, "This object has no description. Please inform the IMP.");

    return buf;
}

/*
 * Show a list to a character.
 * Can coalesce duplicated items.
 */
void show_list_to_char(OBJ_DATA *list, CHAR_DATA *ch, bool fShort, bool fShowNothing) {
    char buf[MAX_STRING_LENGTH];
    char **prgpstrShow;
    int *prgnShow;
    char *pstrShow;
    OBJ_DATA *obj;
    int nShow;
    int iShow;
    int count;
    bool fCombine;
    BUFFER *buffer;

    if (ch->desc == NULL)
        return;

    /*
     * Alloc space for output lines.
     */
    count = 0;
    for (obj = list; obj != NULL; obj = obj->next_content)
        count++;
    prgpstrShow = alloc_mem(count * sizeof(char *));
    prgnShow = alloc_mem(count * sizeof(int));
    nShow = 0;
    buffer = buffer_create();

    /*
     * Format the list of objects.
     */
    for (obj = list; obj != NULL; obj = obj->next_content) {
        if (obj->wear_loc == WEAR_NONE && can_see_obj(ch, obj)) {
            pstrShow = format_obj_to_char(obj, ch, fShort);
            fCombine = FALSE;

            if (IS_NPC(ch) || IS_SET(ch->comm, COMM_COMBINE)) {
                /*
                 * Look for duplicates, case sensitive.
                 * Matches tend to be near end so run loop backwords.
                 */
                for (iShow = nShow - 1; iShow >= 0; iShow--) {
                    if (!strcmp(prgpstrShow[iShow], pstrShow)) {
                        prgnShow[iShow]++;
                        fCombine = TRUE;
                        break;
                    }
                }
            }

            /*
             * Couldn't combine, or didn't want to.
             */
            if (!fCombine) {
                prgpstrShow[nShow] = str_dup(pstrShow);
                prgnShow[nShow] = 1;
                nShow++;
            }
        }
    }

    /*
     * Output the formatted list.
     */
    for (iShow = 0; iShow < nShow; iShow++) {
        if (IS_NPC(ch) || IS_SET(ch->comm, COMM_COMBINE)) {
            if (prgnShow[iShow] != 1) {
                snprintf(buf, sizeof(buf), "(%2d) ", prgnShow[iShow]);
                buffer_addline(buffer, buf);
            } else {
                buffer_addline(buffer, "     ");
            }
        }
        buffer_addline(buffer, prgpstrShow[iShow]);
        buffer_addline(buffer, "\n\r");
        free_string(prgpstrShow[iShow]);
    }

    if (fShowNothing && nShow == 0) {
        if (IS_NPC(ch) || IS_SET(ch->comm, COMM_COMBINE))
            buffer_addline(buffer, "     ");
        buffer_addline(buffer, "Nothing.\n\r");
    }

    /*
     * Clean up.
     */
    free_mem(prgpstrShow, count * sizeof(char *));
    free_mem(prgnShow, count * sizeof(int));

    buffer_send(buffer, ch);
}

void show_char_to_char_0(CHAR_DATA *victim, CHAR_DATA *ch) {
    char buf[MAX_STRING_LENGTH];

    buf[0] = '\0';

    if (IS_AFFECTED(victim, AFF_INVISIBLE))
        strcat(buf, "(|WInvis|w) ");
    if (!IS_NPC(victim) && IS_SET(victim->act, PLR_WIZINVIS))
        strcat(buf, "(|RWizi|w) ");
    if (!IS_NPC(victim) && IS_SET(victim->act, PLR_PROWL))
        strcat(buf, "(|RProwl|w) ");
    if (IS_AFFECTED(victim, AFF_HIDE))
        strcat(buf, "(|WHide|w) ");
    if (IS_AFFECTED(victim, AFF_CHARM))
        strcat(buf, "(|yCharmed|w) ");
    if (IS_AFFECTED(victim, AFF_PASS_DOOR))
        strcat(buf, "(|bTranslucent|w) ");
    if (IS_AFFECTED(victim, AFF_FAERIE_FIRE))
        strcat(buf, "(|rPink Aura|w) ");
    if (IS_AFFECTED(victim, AFF_OCTARINE_FIRE))
        strcat(buf, "(|GOctarine Aura|w) ");
    if (IS_EVIL(victim) && IS_AFFECTED(ch, AFF_DETECT_EVIL))
        strcat(buf, "(|rRed Aura|w) ");
    if (IS_AFFECTED(victim, AFF_SANCTUARY))
        strcat(buf, "(|WWhite Aura|w) ");
    if (!IS_NPC(victim) && IS_SET(victim->act, PLR_KILLER))
        strcat(buf, "(|RKILLER|w) ");
    if (!IS_NPC(victim) && IS_SET(victim->act, PLR_THIEF))
        strcat(buf, "(|RTHIEF|w) ");

    if (is_affected(ch, gsn_bless)) {
        if (IS_SET(victim->act, ACT_UNDEAD)) {
            strcat(buf, "(|bUndead|w) ");
        }
    }

    if (victim->position == victim->start_pos && victim->long_descr[0] != '\0') {
        strcat(buf, victim->long_descr);
        send_to_char(buf, ch);
        return;
    }

    strcat(buf, PERS(victim, ch));
    if (!IS_NPC(victim) && !IS_SET(ch->comm, COMM_BRIEF))
        strcat(buf, victim->pcdata->title);

    switch (victim->position) {
    case POS_DEAD: strcat(buf, " is |RDEAD|w!!"); break;
    case POS_MORTAL: strcat(buf, " is |Rmortally wounded.|w"); break;
    case POS_INCAP: strcat(buf, " is |rincapacitated.|w"); break;
    case POS_STUNNED: strcat(buf, " is |rlying here stunned.|w"); break;
    case POS_SLEEPING: strcat(buf, " is sleeping here."); break;
    case POS_RESTING: strcat(buf, " is resting here."); break;
    case POS_SITTING: strcat(buf, " is sitting here."); break;
    case POS_STANDING:
        if (victim->riding != NULL) {
            /*       strcat( buf, " is here, riding %s.", victim->riding->name);*/
            strcat(buf, " is here, riding ");
            strcat(buf, victim->riding->name);
            strcat(buf, ".");
        } else {
            strcat(buf, " is here.");
        }
        break;
    case POS_FIGHTING:
        strcat(buf, " is here, fighting ");
        if (victim->fighting == NULL)
            strcat(buf, "thin air??");
        else if (victim->fighting == ch)
            strcat(buf, "|RYOU!|w");
        else if (victim->in_room == victim->fighting->in_room) {
            strcat(buf, PERS(victim->fighting, ch));
            strcat(buf, ".");
        } else
            strcat(buf, "somone who left??");
        break;
    }

    strcat(buf, "\n\r");
    buf[0] = UPPER(buf[0]);
    send_to_char(buf, ch);

    return;
}

void show_char_to_char_1(CHAR_DATA *victim, CHAR_DATA *ch) {
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    OBJ_DATA *obj;
    int iWear;
    int percent;
    bool found;

    if (can_see(victim, ch)) {
        if (ch == victim)
            act("$n looks at $mself.", ch, NULL, NULL, TO_ROOM);
        else {
            act("$n looks at you.", ch, NULL, victim, TO_VICT);
            act("$n looks at $N.", ch, NULL, victim, TO_NOTVICT);
        }
    }

    if (victim->description[0] != '\0') {
        send_to_char(victim->description, ch);
    } else {
        act("You see nothing special about $M.", ch, NULL, victim, TO_CHAR);
    }

    if (victim->max_hit > 0)
        percent = (100 * victim->hit) / victim->max_hit;
    else
        percent = -1;

    strcpy(buf, PERS(victim, ch));

    if (percent >= 100)
        strcat(buf, " is in excellent condition.\n\r");
    else if (percent >= 90)
        strcat(buf, " has a few scratches.\n\r");
    else if (percent >= 75)
        strcat(buf, " has some small wounds and bruises.\n\r");
    else if (percent >= 50)
        strcat(buf, " has quite a few wounds.\n\r");
    else if (percent >= 30)
        strcat(buf, " has some big nasty wounds and scratches.\n\r");
    else if (percent >= 15)
        strcat(buf, " looks pretty hurt.\n\r");
    else if (percent >= 0)
        strcat(buf, " is in |rawful condition|w.\n\r");
    else
        strcat(buf, " is |Rbleeding to death|w.\n\r");

    buf[0] = UPPER(buf[0]);
    send_to_char(buf, ch);

    found = FALSE;
    for (iWear = 0; iWear < MAX_WEAR; iWear++) {
        if ((obj = get_eq_char(victim, iWear)) != NULL && can_see_obj(ch, obj)) {
            if (!found) {
                send_to_char("\n\r", ch);
                act("$N is using:", ch, NULL, victim, TO_CHAR);
                found = TRUE;
            }
            if (obj->wear_string != NULL) {
                snprintf(buf2, sizeof(buf2), "<%s>", obj->wear_string);
                snprintf(buf, sizeof(buf), "%-20s", buf2);
                send_to_char(buf, ch);
            } else {
                send_to_char(where_name[iWear], ch);
            }
            send_to_char(format_obj_to_char(obj, ch, TRUE), ch);
            send_to_char("\n\r", ch);
        }
    }

    if (victim != ch && !IS_NPC(ch) && number_percent() < get_skill_learned(ch, gsn_peek)
        && IS_SET(ch->act, PLR_AUTOPEEK)) {
        send_to_char("\n\rYou peek at the inventory:\n\r", ch);
        check_improve(ch, gsn_peek, TRUE, 4);
        show_list_to_char(victim->carrying, ch, TRUE, TRUE);
    }

    return;
}

void do_peek(CHAR_DATA *ch, char *argument) {
    CHAR_DATA *victim;
    char arg1[MAX_INPUT_LENGTH];

    if (ch->desc == NULL)
        return;

    if (ch->position < POS_SLEEPING) {
        send_to_char("You can't see anything but stars!\n\r", ch);
        return;
    }

    if (ch->position == POS_SLEEPING) {
        send_to_char("You can't see anything, you're sleeping!\n\r", ch);
        return;
    }

    if (!check_blind(ch))
        return;

    if (!IS_NPC(ch) && !IS_SET(ch->act, PLR_HOLYLIGHT) && room_is_dark(ch->in_room)) {
        send_to_char("It is pitch black ... \n\r", ch);
        show_char_to_char(ch->in_room->people, ch);
        return;
    }

    argument = one_argument(argument, arg1);

    if ((victim = get_char_room(ch, arg1)) != NULL) {
        if (victim != ch && !IS_NPC(ch) && number_percent() < get_skill_learned(ch, gsn_peek)) {
            send_to_char("\n\rYou peek at their inventory:\n\r", ch);
            check_improve(ch, gsn_peek, TRUE, 4);
            show_list_to_char(victim->carrying, ch, TRUE, TRUE);
        }
    } else
        send_to_char("They aren't here.\n\r", ch);
    return;
}

void show_char_to_char(CHAR_DATA *list, CHAR_DATA *ch) {
    CHAR_DATA *rch;

    for (rch = list; rch != NULL; rch = rch->next_in_room) {
        if (rch == ch)
            continue;

        if (!IS_NPC(rch) && IS_SET(rch->act, PLR_WIZINVIS) && get_trust(ch) < rch->invis_level)
            continue;

        if (can_see(ch, rch)) {
            /*            If(!IS_NPC(ch) && ch->pcdata->colour)
                       {
                           if(IS_NPC(rch))
                              snprintf(buf, sizeof(buf), "%c[1;32m", 27);
                           else
                              snprintf(buf, sizeof(buf), "%c[1;35m", 27);
                           send_to_char(buf, ch);
                        }*/
            show_char_to_char_0(rch, ch);
        }
        /*        if (!IS_NPC(ch) && ch->pcdata->colour)
          {
                   snprintf(buf, sizeof(buf), "%c[0;37m", 27);
                   send_to_char(buf, ch);
          } */
        else if (room_is_dark(ch->in_room) && IS_AFFECTED(rch, AFF_INFRARED)) {
            send_to_char("You see |Rglowing red|w eyes watching |RYOU!|w\n\r", ch);
        }
    }

    return;
}

bool check_blind(CHAR_DATA *ch) {

    if (!IS_NPC(ch) && IS_SET(ch->act, PLR_HOLYLIGHT))
        return TRUE;

    if (IS_AFFECTED(ch, AFF_BLIND)) {
        send_to_char("You can't see a thing!\n\r", ch);
        return FALSE;
    }

    return TRUE;
}

/* changes your scroll */
void do_scroll(CHAR_DATA *ch, char *argument) {
    char arg[MAX_INPUT_LENGTH];
    char buf[100];
    int lines;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        if (ch->lines == 0) {
            send_to_char("|cPaging is set to maximum.|w\n\r", ch);
            ch->lines = 52;
        } else {
            snprintf(buf, sizeof(buf), "|cYou currently display %d lines per page.|w\n\r", ch->lines + 2);
            send_to_char(buf, ch);
        }
        return;
    }

    if (!is_number(arg)) {
        send_to_char("|cYou must provide a number.|w\n\r", ch);
        return;
    }

    lines = atoi(arg);

    if (lines == 0) {
        send_to_char("|cPaging at maximum.|w\n\r", ch);
        ch->lines = 52;
        return;
    }

    /* Pager limited to 52 due to memory complaints relating to
     * buffer code ...short term fix :) --Faramir
     */

    if (lines < 10 || lines > 52) {
        send_to_char("|cYou must provide a reasonable number.|w\n\r", ch);
        return;
    }

    snprintf(buf, sizeof(buf), "|cScroll set to %d lines.|w\n\r", lines);
    send_to_char(buf, ch);
    ch->lines = lines - 2;
}

/* RT does socials */
void do_socials(CHAR_DATA *ch, char *argument) {
    (void)argument;
    char buf[MAX_STRING_LENGTH];
    int iSocial;
    int col;

    col = 0;

    for (iSocial = 0; social_table[iSocial].name[0] != '\0'; iSocial++) {
        snprintf(buf, sizeof(buf), "%-12s", social_table[iSocial].name);
        send_to_char(buf, ch);
        if (++col % 6 == 0)
            send_to_char("\n\r", ch);
    }

    if (col % 6 != 0)
        send_to_char("\n\r", ch);
    return;
}

/* RT Commands to replace news, motd, imotd, etc from ROM */

void do_motd(CHAR_DATA *ch, char *argument) {
    (void)argument;
    do_help(ch, "motd");
}

void do_imotd(CHAR_DATA *ch, char *argument) {
    (void)argument;
    do_help(ch, "imotd");
}

void do_rules(CHAR_DATA *ch, char *argument) {
    (void)argument;
    do_help(ch, "rules");
}

void do_story(CHAR_DATA *ch, char *argument) {
    (void)argument;
    do_help(ch, "story");
}

void do_changes(CHAR_DATA *ch, char *argument) {
    (void)argument;
    do_help(ch, "changes");
}

void do_wizlist(CHAR_DATA *ch, char *argument) {
    (void)argument;
    do_help(ch, "wizlist");
}

/* RT this following section holds all the auto commands from ROM, as well as
   replacements for config */

void do_autolist(CHAR_DATA *ch, char *argument) {
    (void)argument;
    /* lists most player flags */
    if (IS_NPC(ch))
        return;

    send_to_char("   action     status\n\r", ch);
    send_to_char("---------------------\n\r", ch);

    send_to_char("ANSI colour    ", ch);
    if (ch->pcdata->colour)
        send_to_char("|RON|w\n\r", ch);
    else
        send_to_char("|ROFF|w\n\r", ch);

    send_to_char("autoaffect     ", ch);
    if (IS_SET(ch->comm, COMM_AFFECT))
        send_to_char("|RON|w\n\r", ch);
    else
        send_to_char("|ROFF|w\n\r", ch);

    send_to_char("autoassist     ", ch);
    if (IS_SET(ch->act, PLR_AUTOASSIST))
        send_to_char("|RON|w\n\r", ch);
    else
        send_to_char("|ROFF|w\n\r", ch);

    send_to_char("autoexit       ", ch);
    if (IS_SET(ch->act, PLR_AUTOEXIT))
        send_to_char("|RON|w\n\r", ch);
    else
        send_to_char("|ROFF|w\n\r", ch);

    send_to_char("autogold       ", ch);
    if (IS_SET(ch->act, PLR_AUTOGOLD))
        send_to_char("|RON|w\n\r", ch);
    else
        send_to_char("|ROFF|w\n\r", ch);

    send_to_char("autoloot       ", ch);
    if (IS_SET(ch->act, PLR_AUTOLOOT))
        send_to_char("|RON|w\n\r", ch);
    else
        send_to_char("|ROFF|w\n\r", ch);

    send_to_char("autopeek       ", ch);
    if (IS_SET(ch->act, PLR_AUTOPEEK))
        send_to_char("|RON|w\n\r", ch);
    else
        send_to_char("|ROFF|w\n\r", ch);

    send_to_char("autosac        ", ch);
    if (IS_SET(ch->act, PLR_AUTOSAC))
        send_to_char("|RON|w\n\r", ch);
    else
        send_to_char("|ROFF|w\n\r", ch);

    send_to_char("autosplit      ", ch);
    if (IS_SET(ch->act, PLR_AUTOSPLIT))
        send_to_char("|RON|w\n\r", ch);
    else
        send_to_char("|ROFF|w\n\r", ch);

    send_to_char("prompt         ", ch);
    if (IS_SET(ch->comm, COMM_PROMPT))
        send_to_char("|RON|w\n\r", ch);
    else
        send_to_char("|ROFF|w\n\r", ch);

    send_to_char("combine items  ", ch);
    if (IS_SET(ch->comm, COMM_COMBINE))
        send_to_char("|RON|w\n\r", ch);
    else
        send_to_char("|ROFF|w\n\r", ch);

    if (!IS_SET(ch->act, PLR_CANLOOT))
        send_to_char("Your corpse is safe from thieves.\n\r", ch);
    else
        send_to_char("Your corpse may be looted.\n\r", ch);

    if (IS_SET(ch->act, PLR_NOSUMMON))
        send_to_char("You cannot be summoned.\n\r", ch);
    else
        send_to_char("You can be summoned.\n\r", ch);

    if (IS_SET(ch->act, PLR_NOFOLLOW))
        send_to_char("You do not welcome followers.\n\r", ch);
    else
        send_to_char("You accept followers.\n\r", ch);

    if (IS_SET(ch->comm, COMM_BRIEF))
        send_to_char("Only brief descriptions are being shown.\n\r", ch);
    else
        send_to_char("Full descriptions are being shown.\n\r", ch);

    if (IS_SET(ch->comm, COMM_COMPACT))
        send_to_char("Compact mode is set.\n\r", ch);
    else
        send_to_char("Compact mode is not set.\n\r", ch);

    if (IS_SET(ch->comm, COMM_SHOWAFK))
        send_to_char("Messages sent to you will be shown when afk.\n\r", ch);
    else
        send_to_char("Messages sent to you will not be shown when afk.\n\r", ch);

    if (IS_SET(ch->comm, COMM_SHOWDEFENCE))
        send_to_char("Shield blocks, parries, and dodges are being shown.\n\r", ch);
    else
        send_to_char("Shield blocks, parries, and dodges are not being shown.\n\r", ch);
}

void do_autoaffect(CHAR_DATA *ch, char *argument) {
    (void)argument;
    if (IS_NPC(ch))
        return;

    if (IS_SET(ch->comm, COMM_AFFECT)) {
        send_to_char("Autoaffect removed.\n\r", ch);
        REMOVE_BIT(ch->comm, COMM_AFFECT);
    } else {
        send_to_char("Affects will now be shown in score.\n\r", ch);
        SET_BIT(ch->comm, COMM_AFFECT);
    }
}
void do_autoassist(CHAR_DATA *ch, char *argument) {
    (void)argument;
    if (IS_NPC(ch))
        return;

    if (IS_SET(ch->act, PLR_AUTOASSIST)) {
        send_to_char("Autoassist removed.\n\r", ch);
        REMOVE_BIT(ch->act, PLR_AUTOASSIST);
    } else {
        send_to_char("You will now assist when needed.\n\r", ch);
        SET_BIT(ch->act, PLR_AUTOASSIST);
    }
}

void do_autoexit(CHAR_DATA *ch, char *argument) {
    (void)argument;
    if (IS_NPC(ch))
        return;

    if (IS_SET(ch->act, PLR_AUTOEXIT)) {
        send_to_char("Exits will no longer be displayed.\n\r", ch);
        REMOVE_BIT(ch->act, PLR_AUTOEXIT);
    } else {
        send_to_char("Exits will now be displayed.\n\r", ch);
        SET_BIT(ch->act, PLR_AUTOEXIT);
    }
}

void do_autogold(CHAR_DATA *ch, char *argument) {
    (void)argument;
    if (IS_NPC(ch))
        return;

    if (IS_SET(ch->act, PLR_AUTOGOLD)) {
        send_to_char("Autogold removed.\n\r", ch);
        REMOVE_BIT(ch->act, PLR_AUTOGOLD);
    } else {
        send_to_char("Automatic gold looting set.\n\r", ch);
        SET_BIT(ch->act, PLR_AUTOGOLD);
    }
}

void do_autoloot(CHAR_DATA *ch, char *argument) {
    (void)argument;
    if (IS_NPC(ch))
        return;

    if (IS_SET(ch->act, PLR_AUTOLOOT)) {
        send_to_char("Autolooting removed.\n\r", ch);
        REMOVE_BIT(ch->act, PLR_AUTOLOOT);
    } else {
        send_to_char("Automatic corpse looting set.\n\r", ch);
        SET_BIT(ch->act, PLR_AUTOLOOT);
    }
}

void do_autopeek(CHAR_DATA *ch, char *argument) {
    (void)argument;
    if (IS_NPC(ch))
        return;

    if (IS_SET(ch->act, PLR_AUTOPEEK)) {
        send_to_char("Autopeeking removed.\n\r", ch);
        REMOVE_BIT(ch->act, PLR_AUTOPEEK);
    } else {
        send_to_char("Automatic peeking set.\n\r", ch);
        SET_BIT(ch->act, PLR_AUTOPEEK);
    }
}

void do_autosac(CHAR_DATA *ch, char *argument) {
    (void)argument;
    if (IS_NPC(ch))
        return;

    if (IS_SET(ch->act, PLR_AUTOSAC)) {
        send_to_char("Autosacrificing removed.\n\r", ch);
        REMOVE_BIT(ch->act, PLR_AUTOSAC);
    } else {
        send_to_char("Automatic corpse sacrificing set.\n\r", ch);
        SET_BIT(ch->act, PLR_AUTOSAC);
    }
}

void do_autosplit(CHAR_DATA *ch, char *argument) {
    (void)argument;
    if (IS_NPC(ch))
        return;

    if (IS_SET(ch->act, PLR_AUTOSPLIT)) {
        send_to_char("Autosplitting removed.\n\r", ch);
        REMOVE_BIT(ch->act, PLR_AUTOSPLIT);
    } else {
        send_to_char("Automatic gold splitting set.\n\r", ch);
        SET_BIT(ch->act, PLR_AUTOSPLIT);
    }
}

void do_brief(CHAR_DATA *ch, char *argument) {
    (void)argument;
    if (IS_SET(ch->comm, COMM_BRIEF)) {
        send_to_char("Full descriptions activated.\n\r", ch);
        REMOVE_BIT(ch->comm, COMM_BRIEF);
    } else {
        send_to_char("Short descriptions activated.\n\r", ch);
        SET_BIT(ch->comm, COMM_BRIEF);
    }
}

void do_colour(CHAR_DATA *ch, char *argument) {
    (void)argument;
    if IS_NPC (ch)
        return;

    if (ch->pcdata->colour) {
        send_to_char("You feel less COLOURFUL.\n\r", ch);

        ch->pcdata->colour = 0;
    } else {
        ch->pcdata->colour = 1;
        send_to_char("You feel more |RC|GO|BL|rO|gU|bR|cF|YU|PL|W!.|w\n\r", ch);
    }
}

void do_showafk(CHAR_DATA *ch, char *argument) {
    (void)argument;
    if (IS_SET(ch->comm, COMM_SHOWAFK)) {
        send_to_char("Messages sent to you will now not be shown when afk.\n\r", ch);
        REMOVE_BIT(ch->comm, COMM_SHOWAFK);
    } else {
        send_to_char("Messages sent to you will now be shown when afk.\n\r", ch);
        SET_BIT(ch->comm, COMM_SHOWAFK);
    }
}
void do_showdefence(CHAR_DATA *ch, char *argument) {
    (void)argument;
    if (IS_SET(ch->comm, COMM_SHOWDEFENCE)) {
        send_to_char("Shield blocks, parries and dodges will not be shown during combat.\n\r", ch);
        REMOVE_BIT(ch->comm, COMM_SHOWDEFENCE);
    } else {
        send_to_char("Shield blocks, parries and dodges will be shown during combat.\n\r", ch);
        SET_BIT(ch->comm, COMM_SHOWDEFENCE);
    }
}

void do_compact(CHAR_DATA *ch, char *argument) {
    (void)argument;
    if (IS_SET(ch->comm, COMM_COMPACT)) {
        send_to_char("Compact mode removed.\n\r", ch);
        REMOVE_BIT(ch->comm, COMM_COMPACT);
    } else {
        send_to_char("Compact mode set.\n\r", ch);
        SET_BIT(ch->comm, COMM_COMPACT);
    }
}

void do_prompt(CHAR_DATA *ch, char *argument) {

    /* PCFN 24-05-97  Oh dear - it seems that you can't set prompt while switched
       into a MOB.  Let's change that.... */

    if (IS_NPC(ch)) {
        if (ch->desc->original)
            ch = ch->desc->original;
        else
            return;
    }

    if (str_cmp(argument, "off") == 0) {
        send_to_char("You will no longer see prompts.\n\r", ch);
        REMOVE_BIT(ch->comm, COMM_PROMPT);
        return;
    }
    if (str_cmp(argument, "on") == 0) {
        send_to_char("You will now see prompts.\n\r", ch);
        SET_BIT(ch->comm, COMM_PROMPT);
        return;
    }

    /* okay that was the old stuff */
    smash_tilde(argument);
    set_prompt(ch, argument);
    send_to_char("Ok - prompt set.\n\r", ch);
    SET_BIT(ch->comm, COMM_PROMPT);
}

void do_combine(CHAR_DATA *ch, char *argument) {
    (void)argument;
    if (IS_SET(ch->comm, COMM_COMBINE)) {
        send_to_char("Long inventory selected.\n\r", ch);
        REMOVE_BIT(ch->comm, COMM_COMBINE);
    } else {
        send_to_char("Combined inventory selected.\n\r", ch);
        SET_BIT(ch->comm, COMM_COMBINE);
    }
}

void do_noloot(CHAR_DATA *ch, char *argument) {
    (void)argument;
    if (IS_NPC(ch))
        return;

    if (IS_SET(ch->act, PLR_CANLOOT)) {
        send_to_char("Your corpse is now safe from thieves.\n\r", ch);
        REMOVE_BIT(ch->act, PLR_CANLOOT);
    } else {
        send_to_char("Your corpse may now be looted.\n\r", ch);
        SET_BIT(ch->act, PLR_CANLOOT);
    }
}

void do_nofollow(CHAR_DATA *ch, char *argument) {
    (void)argument;
    if (IS_NPC(ch))
        return;

    if (IS_SET(ch->act, PLR_NOFOLLOW)) {
        send_to_char("You now accept followers.\n\r", ch);
        REMOVE_BIT(ch->act, PLR_NOFOLLOW);
    } else {
        send_to_char("You no longer accept followers.\n\r", ch);
        SET_BIT(ch->act, PLR_NOFOLLOW);
        die_follower(ch);
    }
}

void do_nosummon(CHAR_DATA *ch, char *argument) {
    (void)argument;
    if (IS_NPC(ch)) {
        if (IS_SET(ch->imm_flags, IMM_SUMMON)) {
            send_to_char("You are no longer immune to summon.\n\r", ch);
            REMOVE_BIT(ch->imm_flags, IMM_SUMMON);
        } else {
            send_to_char("You are now immune to summoning.\n\r", ch);
            SET_BIT(ch->imm_flags, IMM_SUMMON);
        }
    } else {
        if (IS_SET(ch->act, PLR_NOSUMMON)) {
            send_to_char("You are no longer immune to summon.\n\r", ch);
            REMOVE_BIT(ch->act, PLR_NOSUMMON);
        } else {
            send_to_char("You are now immune to summoning.\n\r", ch);
            SET_BIT(ch->act, PLR_NOSUMMON);
        }
    }
}

void do_lore(CHAR_DATA *ch, OBJ_DATA *obj, char *pdesc) {
    int sn = skill_lookup("identify");

    if (!IS_NPC(ch) && number_percent() > get_skill_learned(ch, skill_lookup("lore"))) {
        send_to_char(pdesc, ch);
        check_improve(ch, gsn_lore, FALSE, 1);
        return;
    } else {
        if (!IS_IMMORTAL(ch))
            WAIT_STATE(ch, skill_table[sn].beats);
        send_to_char(pdesc, ch);
        check_improve(ch, gsn_lore, TRUE, 1);
        (*skill_table[sn].spell_fun)(sn, ch->level, ch, (void *)obj);
        return;
    }
}

void do_look(CHAR_DATA *ch, char *argument) {
    char buf[MAX_STRING_LENGTH];
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];
    EXIT_DATA *pexit;
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    char *pdesc;
    int door;
    int sn;
    int number, count;

    if (ch->desc == NULL)
        return;

    if (ch->position < POS_SLEEPING) {
        send_to_char("You can't see anything but stars!\n\r", ch);
        return;
    }

    if (ch->position == POS_SLEEPING) {
        send_to_char("You can't see anything, you're sleeping!\n\r", ch);
        return;
    }

    if (!check_blind(ch))
        return;

    if (!IS_NPC(ch) && !IS_SET(ch->act, PLR_HOLYLIGHT) && room_is_dark(ch->in_room)) {
        send_to_char("It is pitch black ... \n\r", ch);
        show_char_to_char(ch->in_room->people, ch);
        return;
    }

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);
    number = number_argument(arg1, arg3);
    count = 0;

    if (arg1[0] == '\0' || !str_cmp(arg1, "auto")) {
        /* 'look' or 'look auto' */
        /*        if ((!IS_NPC(ch)) && ( ch->pcdata->colour))
          {
                   snprintf(buf, sizeof(buf), "%c[1;31m", 27);
                   send_to_char(buf, ch);
          } */
        send_to_char("|R", ch);
        send_to_char(ch->in_room->name, ch);
        send_to_char("\n\r|w", ch);
        /*
                if( (!IS_NPC(ch)) && (ch->pcdata->colour))
          {
                   snprintf(buf, sizeof(buf), "%c[0;37m", 27);
                   send_to_char(buf, ch);
                } */
        if (arg1[0] == '\0' || (!IS_NPC(ch) && !IS_SET(ch->comm, COMM_BRIEF))) {
            send_to_char("  ", ch);
            send_to_char(ch->in_room->description, ch);
        }

        if (!IS_NPC(ch) && IS_SET(ch->act, PLR_AUTOEXIT)) {
            send_to_char("\n\r", ch);
            do_exits(ch, "auto");
        }

        show_list_to_char(ch->in_room->contents, ch, FALSE, FALSE);
        show_char_to_char(ch->in_room->people, ch);
        return;
    }

    if (!str_cmp(arg1, "i") || !str_cmp(arg1, "in")) {
        /* 'look in' */
        if (arg2[0] == '\0') {
            send_to_char("Look in what?\n\r", ch);
            return;
        }

        if ((obj = get_obj_here(ch, arg2)) == NULL) {
            send_to_char("You do not see that here.\n\r", ch);
            return;
        }

        switch (obj->item_type) {
        default: send_to_char("That is not a container.\n\r", ch); break;

        case ITEM_DRINK_CON:
            if (obj->value[1] <= 0) {
                send_to_char("It is empty.\n\r", ch);
                break;
            }

            snprintf(buf, sizeof(buf), "It's %s full of a %s liquid.\n\r",
                     obj->value[1] < obj->value[0] / 4 ? "less than"
                                                       : obj->value[1] < 3 * obj->value[0] / 4 ? "about" : "more than",
                     liq_table[obj->value[2]].liq_color);

            send_to_char(buf, ch);
            break;

        case ITEM_CONTAINER:
        case ITEM_CORPSE_NPC:
        case ITEM_CORPSE_PC:
            if (IS_SET(obj->value[1], CONT_CLOSED)) {
                send_to_char("It is closed.\n\r", ch);
                break;
            }

            act("$p contains:", ch, obj, NULL, TO_CHAR);
            show_list_to_char(obj->contains, ch, TRUE, TRUE);
            break;
        }
        return;
    }

    if ((victim = get_char_room(ch, arg1)) != NULL) {
        show_char_to_char_1(victim, ch);
        return;
    }

    for (obj = ch->carrying; obj != NULL; obj = obj->next_content) {
        if (can_see_obj(ch, obj)) {
            pdesc = get_extra_descr(arg3, obj->extra_descr);
            if (pdesc != NULL) {
                if (++count == number) {
                    sn = skill_lookup("lore");
                    if (sn < 0 || (!IS_NPC(ch) && ch->level < get_skill_level(ch, sn))) {
                        send_to_char(pdesc, ch);
                        return;
                    } else {
                        do_lore(ch, obj, pdesc);
                        return;
                    }
                } else
                    continue;
            }

            pdesc = get_extra_descr(arg3, obj->pIndexData->extra_descr);
            if (pdesc != NULL) {
                if (++count == number) {
                    sn = skill_lookup("lore");
                    if (sn < 0 || (!IS_NPC(ch) && ch->level < get_skill_level(ch, sn))) {
                        send_to_char(pdesc, ch);
                        return;
                    } else {
                        do_lore(ch, obj, pdesc);
                        return;
                    }
                } else
                    continue;
            }

            if (is_name(arg3, obj->name))
                if (++count == number) {
                    send_to_char(obj->description, ch);
                    send_to_char("\n\r", ch);
                    do_lore(ch, obj, "");
                    return;
                }
        }
    }

    for (obj = ch->in_room->contents; obj != NULL; obj = obj->next_content) {
        if (can_see_obj(ch, obj)) {
            pdesc = get_extra_descr(arg3, obj->extra_descr);
            if (pdesc != NULL)
                if (++count == number) {
                    send_to_char(pdesc, ch);
                    return;
                }

            pdesc = get_extra_descr(arg3, obj->pIndexData->extra_descr);
            if (pdesc != NULL)
                if (++count == number) {
                    send_to_char(pdesc, ch);
                    return;
                }
        }

        if (is_name(arg3, obj->name))
            if (++count == number) {
                send_to_char(obj->description, ch);
                send_to_char("\n\r", ch);
                return;
            }
    }

    if (count > 0 && count != number) {
        if (count == 1)
            snprintf(buf, sizeof(buf), "You only see one %s here.\n\r", arg3);
        else
            snprintf(buf, sizeof(buf), "You only see %d %s's here.\n\r", count, arg3);

        send_to_char(buf, ch);
        return;
    }

    pdesc = get_extra_descr(arg1, ch->in_room->extra_descr);
    if (pdesc != NULL) {
        send_to_char(pdesc, ch);
        return;
    }

    if (!str_cmp(arg1, "n") || !str_cmp(arg1, "north"))
        door = 0;
    else if (!str_cmp(arg1, "e") || !str_cmp(arg1, "east"))
        door = 1;
    else if (!str_cmp(arg1, "s") || !str_cmp(arg1, "south"))
        door = 2;
    else if (!str_cmp(arg1, "w") || !str_cmp(arg1, "west"))
        door = 3;
    else if (!str_cmp(arg1, "u") || !str_cmp(arg1, "up"))
        door = 4;
    else if (!str_cmp(arg1, "d") || !str_cmp(arg1, "down"))
        door = 5;
    else {
        send_to_char("You do not see that here.\n\r", ch);
        return;
    }

    /* 'look direction' */
    if ((pexit = ch->in_room->exit[door]) == NULL) {
        send_to_char("Nothing special there.\n\r", ch);
        return;
    }

    if (pexit->description != NULL && pexit->description[0] != '\0')
        send_to_char(pexit->description, ch);
    else
        send_to_char("Nothing special there.\n\r", ch);

    if (pexit->keyword != NULL && pexit->keyword[0] != '\0' && pexit->keyword[0] != ' ') {
        if (IS_SET(pexit->exit_info, EX_CLOSED)) {
            act("The $d is closed.", ch, NULL, pexit->keyword, TO_CHAR);
        } else if (IS_SET(pexit->exit_info, EX_ISDOOR)) {
            act("The $d is open.", ch, NULL, pexit->keyword, TO_CHAR);
        }
    }

    return;
}

/* RT added back for the hell of it */
void do_read(CHAR_DATA *ch, char *argument) { do_look(ch, argument); }

void do_examine(CHAR_DATA *ch, char *argument) {
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        send_to_char("Examine what?\n\r", ch);
        return;
    }

    do_look(ch, arg);

    if ((obj = get_obj_here(ch, arg)) != NULL) {
        switch (obj->item_type) {
        default: break;

        case ITEM_DRINK_CON:
        case ITEM_CONTAINER:
        case ITEM_CORPSE_NPC:
        case ITEM_CORPSE_PC:
            send_to_char("When you look inside, you see:\n\r", ch);
            snprintf(buf, sizeof(buf), "in %s", arg);
            do_look(ch, buf);
        }
    }

    return;
}

/*
 * Thanks to Zrin for auto-exit part.
 */
void do_exits(CHAR_DATA *ch, char *argument) {
    extern char *const dir_name[];
    char buf[MAX_STRING_LENGTH];
    EXIT_DATA *pexit;
    bool found;
    bool fAuto;
    int door;

    buf[0] = '\0';
    fAuto = !str_cmp(argument, "auto");

    if (!check_blind(ch))
        return;

    /*    if ((!IS_NPC(ch)) && (ch->pcdata->colour))
        {
           snprintf(buf, sizeof(buf), "%c[1;37m", 27);
           send_to_char(buf, ch);
        } */
    strcpy(buf, fAuto ? "|W[Exits:" : "Obvious exits:\n\r");

    found = FALSE;
    for (door = 0; door <= 5; door++) {
        if ((pexit = ch->in_room->exit[door]) != NULL && pexit->u1.to_room != NULL
            && can_see_room(ch, pexit->u1.to_room) && !IS_SET(pexit->exit_info, EX_CLOSED)) {
            found = TRUE;
            if (fAuto) {
                strcat(buf, " ");
                strcat(buf, dir_name[door]);
            } else {
                snprintf(buf + strlen(buf), sizeof(buf), "%-5s - %s\n\r", capitalize(dir_name[door]),
                         room_is_dark(pexit->u1.to_room) ? "Too dark to tell" : pexit->u1.to_room->name);
            }
        }
    }

    if (!found)
        strcat(buf, fAuto ? " none" : "None.\n\r");

    if (fAuto)
        strcat(buf, "]\n\r|w");

    send_to_char(buf, ch);
    /*    if ( (!IS_NPC(ch)) && (ch->pcdata->colour))
        {
           snprintf(buf, sizeof(buf), "%c[0;37m", 27);
           send_to_char( buf, ch );
        } */
    return;
}

void do_worth(CHAR_DATA *ch, char *argument) {
    (void)argument;
    char buf[MAX_STRING_LENGTH];

    if (IS_NPC(ch)) {
        snprintf(buf, sizeof(buf), "You have %d gold.\n\r", (int)ch->gold);
        send_to_char(buf, ch);
        return;
    }

    snprintf(buf, sizeof(buf), "You have %d gold, and %d experience (%d exp to level).\n\r", (int)ch->gold,
             (int)ch->exp, (int)((ch->level + 1) * exp_per_level(ch, ch->pcdata->points) - ch->exp));

    send_to_char(buf, ch);

    return;
}

int final_len(char *string) {
    int count = 0;
    while (*string) {
        if (*string == '|') {
            string++;
            if (*string == '|') {
                count++;
                string++;
            } else {
                if (*string)
                    string++;
            }
        } else {
            count++;
            string++;
        }
    }
    return count;
}

#define SC_COLWIDTH 24

char *next_column(char *buf, int col_width) {
    int eff_len = final_len(buf);
    int len = strlen(buf);
    int num_spaces = (eff_len < col_width) ? (col_width - eff_len) : 1;
    memset(buf + len, ' ', num_spaces);
    return buf + len + num_spaces;
}

const char *position_desc[] = {"dead",    "mortally wounded", "incapacitated", "stunned", "sleeping",
                               "resting", "sitting",          "fighting",      "standing"};

const char *armour_desc[] = {"divinely armoured against", "almost invulnerable to",     "superbly armoured against",
                             "heavily armoured against",  "very well-armoured against", "well-armoured against",
                             "armoured against",          "somewhat armoured against",  "slighty armoured against",
                             "barely protected from",     "defenseless against",        "hopelessly vulnerable to"};

void describe_armour(CHAR_DATA *ch, int type, char *name) {
    char buf[MAX_STRING_LENGTH];
    int armour_index = (GET_AC(ch, type) + 120) / 20;

    if (armour_index < 0)
        armour_index = 0;
    if (armour_index > 11)
        armour_index = 11;

    if (ch->level < 25)
        snprintf(buf, sizeof(buf), "You are |y%s |W%s|w.\n\r", armour_desc[armour_index], name);
    else
        snprintf(buf, sizeof(buf), "You are |y%s |W%s|w. (|W%d|w)\n\r", armour_desc[armour_index], name,
                 GET_AC(ch, type));
    send_to_char(buf, ch);
}

static void describe_condition(CHAR_DATA *ch) {
    char buf[MAX_STRING_LENGTH];
    int drunk = (ch->pcdata->condition[COND_DRUNK] > 10);
    int hungry = (ch->pcdata->condition[COND_FULL] == 0);
    int thirsty = (ch->pcdata->condition[COND_THIRST] == 0);
    static const char *delimiters[] = {"", " and ", ", "};

    if (!drunk && !hungry && !thirsty)
        return;
    snprintf(buf, sizeof(buf), "You are %s%s%s%s%s.\n\r", drunk ? "|Wdrunk|w" : "",
             drunk ? delimiters[hungry + thirsty] : "", hungry ? "|Whungry|w" : "", (thirsty && hungry) ? " and " : "",
             thirsty ? "|Wthirsty|w" : "");
    send_to_char(buf, ch);
}

static const char *align_descriptions[] = {"|Rsatanic", "|Rdemonic", "|Yevil",    "|Ymean",   "|Mneutral",
                                           "|Gkind",    "|Ggood",    "|Wsaintly", "|Wangelic"};

const char *get_align_description(int align) {
    int index = (align + 1000) * 8 / 2000;
    if (index < 0) /* Let's be paranoid, eh? */
        index = 0;
    if (index > 8)
        index = 8;
    return align_descriptions[index];
}

void do_score(CHAR_DATA *ch, char *argument) {
    (void)argument;
    char buf[MAX_STRING_LENGTH];

    snprintf(buf, sizeof(buf), "|wYou are: |W%s%s|w.\n\r", ch->name, IS_NPC(ch) ? "" : ch->pcdata->title);
    send_to_char(buf, ch);

    if (get_trust(ch) == ch->level)
        snprintf(buf, sizeof(buf), "Level: |W%d|w", ch->level);
    else
        snprintf(buf, sizeof(buf), "Level: |W%d|w (trust |W%d|w)", ch->level, get_trust(ch));
    snprintf(next_column(buf, SC_COLWIDTH), sizeof(buf), "Age: |W%d|w years (|W%d|w hours)\n\r", get_age(ch),
             (ch->played + (int)(current_time - ch->logon)) / 3600);
    send_to_char(buf, ch);

    snprintf(buf, sizeof(buf), "Race: |W%s|w", race_table[ch->race].name);
    snprintf(next_column(buf, SC_COLWIDTH), sizeof(buf), "Class: |W%s|w\n\r",
             IS_NPC(ch) ? "mobile" : class_table[ch->class].name);
    send_to_char(buf, ch);

    snprintf(buf, sizeof(buf), "Sex: |W%s|w", ch->sex == 0 ? "sexless" : ch->sex == 1 ? "male" : "female");
    snprintf(next_column(buf, SC_COLWIDTH), sizeof(buf), "Position: |W%s|w\n\r", position_desc[ch->position]);
    send_to_char(buf, ch);

    snprintf(buf, sizeof(buf), "Items: |W%d|w / %d", ch->carry_number, can_carry_n(ch));
    snprintf(next_column(buf, SC_COLWIDTH), sizeof(buf), "Weight: |W%d|w / %d\n\r", ch->carry_weight, can_carry_w(ch));
    send_to_char(buf, ch);

    snprintf(buf, sizeof(buf), "Gold: |W%d|w\n\r", (int)ch->gold);
    send_to_char(buf, ch);

    snprintf(buf, sizeof(buf), "Wimpy: |W%d|w", ch->wimpy);
    if (!IS_NPC(ch) && ch->level < LEVEL_HERO)
        snprintf(next_column(buf, SC_COLWIDTH), sizeof(buf), "Score: |W%d|w (|W%d|w to next level)\n\r", (int)ch->exp,
                 (unsigned int)((ch->level + 1) * exp_per_level(ch, ch->pcdata->points) - ch->exp));
    else
        snprintf(next_column(buf, SC_COLWIDTH), sizeof(buf), "Score: |W%d|w\n\r", (int)ch->exp);
    send_to_char(buf, ch);
    send_to_char("\n\r", ch);

    snprintf(buf, sizeof(buf), "Hit: |W%d|w / %d", ch->hit, ch->max_hit);
    snprintf(next_column(buf, SC_COLWIDTH), sizeof(buf), "Mana: |W%d|w / %d", ch->mana, ch->max_mana);
    snprintf(next_column(buf, 2 * SC_COLWIDTH), sizeof(buf), "Move: |W%d|w / %d\n\r", ch->move, ch->max_move);
    send_to_char(buf, ch);

    describe_armour(ch, AC_PIERCE, "piercing");
    describe_armour(ch, AC_BASH, "bashing");
    describe_armour(ch, AC_SLASH, "slashing");
    describe_armour(ch, AC_EXOTIC, "magic");

    if (ch->level >= 15) {
        snprintf(buf, sizeof(buf), "Hit roll: |W%d|w", GET_HITROLL(ch));
        snprintf(next_column(buf, SC_COLWIDTH), sizeof(buf), "Damage roll: |W%d|w\n\r", GET_DAMROLL(ch));
        send_to_char(buf, ch);
    }
    send_to_char("\n\r", ch);

    snprintf(buf, sizeof(buf), "Strength: %d (|W%d|w)", ch->perm_stat[STAT_STR], get_curr_stat(ch, STAT_STR));
    snprintf(next_column(buf, SC_COLWIDTH), sizeof(buf), "Intelligence: %d (|W%d|w)", ch->perm_stat[STAT_INT],
             get_curr_stat(ch, STAT_INT));
    snprintf(next_column(buf, 2 * SC_COLWIDTH), sizeof(buf), "Wisdom: %d (|W%d|w)\n\r", ch->perm_stat[STAT_WIS],
             get_curr_stat(ch, STAT_WIS));
    send_to_char(buf, ch);

    snprintf(buf, sizeof(buf), "Dexterity: %d (|W%d|w)", ch->perm_stat[STAT_DEX], get_curr_stat(ch, STAT_DEX));
    snprintf(next_column(buf, SC_COLWIDTH), sizeof(buf), "Constitution: %d (|W%d|w)\n\r", ch->perm_stat[STAT_CON],
             get_curr_stat(ch, STAT_CON));
    send_to_char(buf, ch);

    snprintf(buf, sizeof(buf), "Practices: |W%d|w", ch->practice);
    snprintf(next_column(buf, SC_COLWIDTH), sizeof(buf), "Training sessions: |W%d|w\n\r", ch->train);
    send_to_char(buf, ch);

    if (ch->level >= 10)
        snprintf(buf, sizeof(buf), "You are %s|w (alignment |W%d|w).\n\r", get_align_description(ch->alignment),
                 ch->alignment);
    else
        snprintf(buf, sizeof(buf), "You are %s|w.\n\r", get_align_description(ch->alignment));
    send_to_char(buf, ch);

    if (!IS_NPC(ch))
        describe_condition(ch);

    if (IS_IMMORTAL(ch)) {
        send_to_char("\n\r", ch);
        snprintf(buf, sizeof(buf), "Holy light: |W%s|w", IS_SET(ch->act, PLR_HOLYLIGHT) ? "on" : "off");
        if (IS_SET(ch->act, PLR_WIZINVIS)) {
            snprintf(next_column(buf, SC_COLWIDTH), sizeof(buf), "Invisible: |W%d|w", ch->invis_level);
        }
        if (IS_SET(ch->act, PLR_PROWL)) {
            snprintf(next_column(buf, SC_COLWIDTH * (IS_SET(ch->act, PLR_WIZINVIS) ? 2 : 1)), sizeof(buf),
                     "Prowl: |W%d|w", ch->invis_level);
        }
        strcat(buf, "\n\r");
        send_to_char(buf, ch);
    }

    if (IS_SET(ch->comm, COMM_AFFECT)) {
        send_to_char("\n\r", ch);
        do_affected(ch);
    }
}

void do_affected(CHAR_DATA *ch) {
    char buf[MAX_STRING_LENGTH];
    AFFECT_DATA *paf;
    int flag = 0;

    send_to_char("You are affected by:\n\r", ch);

    if (ch->affected != NULL) {
        for (paf = ch->affected; paf != NULL; paf = paf->next) {
            if ((paf->type == gsn_sneak) || (paf->type == gsn_ride)) {
                snprintf(buf, sizeof(buf), "Skill: '%s'", skill_table[paf->type].name);
                flag = 1;
            } else {
                snprintf(buf, sizeof(buf), "Spell: '%s'", skill_table[paf->type].name);
                flag = 0;
            }
            send_to_char(buf, ch);

            if (ch->level >= 20) {
                if (flag == 0) {
                    snprintf(buf, sizeof(buf), " modifies %s by %d for %d hours", affect_loc_name(paf->location),
                             paf->modifier, paf->duration);
                    send_to_char(buf, ch);
                } else {
                    snprintf(buf, sizeof(buf), " modifies %s by %d", affect_loc_name(paf->location), paf->modifier);
                    send_to_char(buf, ch);
                }
            }

            send_to_char(".\n\r", ch);
        }
    } else {
        send_to_char("Nothing.\n\r", ch);
    }
}

char *const day_name[] = {"the Moon", "the Bull", "Deception", "Thunder", "Freedom", "the Great Gods", "the Sun"};

char *const month_name[] = {"Winter",
                            "the Winter Wolf",
                            "the Frost Giant",
                            "the Old Forces",
                            "the Grand Struggle",
                            "the Spring",
                            "Nature",
                            "Futility",
                            "the Dragon",
                            "the Sun",
                            "the Heat",
                            "the Battle",
                            "the Dark Shades",
                            "the Shadows",
                            "the Long Shadows",
                            "the Ancient Darkness",
                            "the Great Evil"};

void do_time(CHAR_DATA *ch, char *argument) {
    (void)argument;
    extern char str_boot_time[];
    char buf[MAX_STRING_LENGTH];
    char *suf;
    int day;

    day = time_info.day + 1;

    if (day > 4 && day < 20)
        suf = "th";
    else if (day % 10 == 1)
        suf = "st";
    else if (day % 10 == 2)
        suf = "nd";
    else if (day % 10 == 3)
        suf = "rd";
    else
        suf = "th";

    snprintf(buf, sizeof(buf),
             "It is %d o'clock %s, Day of %s, %d%s the Month of %s.\n\rXania started up at %s\rThe system time is %s\r",

             (time_info.hour % 12 == 0) ? 12 : time_info.hour % 12, time_info.hour >= 12 ? "pm" : "am",
             day_name[day % 7], day, suf, month_name[time_info.month], str_boot_time, (char *)ctime(&current_time));

    send_to_char(buf, ch);

    if (IS_NPC(ch)) {
        if (ch->desc->original)
            ch = ch->desc->original;
        else
            return;
    }

    if (ch->pcdata->houroffset || ch->pcdata->minoffset) {
        time_t ch_timet;
        char buf2[32];
        struct tm *ch_time;

        ch_timet = time(0);

        ch_time = gmtime(&ch_timet);
        ch_time->tm_min += ch->pcdata->minoffset;
        ch_time->tm_hour += ch->pcdata->houroffset;

        ch_time->tm_hour -= (ch_time->tm_min / 60);
        ch_time->tm_min = (ch_time->tm_min % 60);
        if (ch_time->tm_min < 0) {
            ch_time->tm_min += 60;
            ch_time->tm_hour -= 1;
        }
        ch_time->tm_hour = (ch_time->tm_hour % 24);
        if (ch_time->tm_hour < 0)
            ch_time->tm_hour += 24;

        strftime(buf2, 63, "%H:%M:%S", ch_time);

        snprintf(buf, sizeof(buf), "Your local time is %s\n\r", buf2);
        send_to_char(buf, ch);
    }

    return;
}

void do_weather(CHAR_DATA *ch, char *argument) {
    (void)argument;
    char buf[MAX_STRING_LENGTH];

    static char *const sky_look[4] = {"cloudless", "cloudy", "rainy", "lit by flashes of lightning"};

    if (!IS_OUTSIDE(ch)) {
        send_to_char("You can't see the weather indoors.\n\r", ch);
        return;
    }

    snprintf(buf, sizeof(buf), "The sky is %s and %s.\n\r", sky_look[weather_info.sky],
             weather_info.change >= 0 ? "a warm southerly breeze blows" : "a cold northern gust blows");
    send_to_char(buf, ch);
    return;
}

void do_help(CHAR_DATA *ch, char *argument) {
    HELP_DATA *pHelp;
    char argall[MAX_INPUT_LENGTH], argone[MAX_INPUT_LENGTH];

    if (argument[0] == '\0')
        argument = "summary";

    /* this parts handles help a b so that it returns help 'a b' */
    argall[0] = '\0';
    while (argument[0] != '\0') {
        argument = one_argument(argument, argone);
        if (argall[0] != '\0')
            strcat(argall, " ");
        strcat(argall, argone);
    }

    for (pHelp = help_first; pHelp != NULL; pHelp = pHelp->next) {
        if (pHelp->level > get_trust(ch))
            continue;

        if (is_name(argall, pHelp->keyword)) {
            if (pHelp->level >= 0 && str_cmp(argall, "imotd")) {
                send_to_char(pHelp->keyword, ch);
                send_to_char("\n\r", ch);
            }

            /*
             * Strip leading '.' to allow initial blanks.
             */
            if (pHelp->text[0] == '.')
                page_to_char(pHelp->text + 1, ch);
            else
                page_to_char(pHelp->text, ch);
            return;
        }
    }

    send_to_char("No help on that word.\n\r", ch);
    return;
}

/* whois command */
void do_whois(CHAR_DATA *ch, char *argument) {
    char arg[MAX_INPUT_LENGTH];
    char output[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;
    bool found = FALSE;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        send_to_char("You must provide a name.\n\r", ch);
        return;
    }

    output[0] = '\0';

    for (d = descriptor_list; d != NULL; d = d->next) {
        CHAR_DATA *wch;
        char const *class;

        if (d->connected != CON_PLAYING || !can_see(ch, d->character))
            continue;

        wch = (d->original != NULL) ? d->original : d->character;

        if (!can_see(ch, wch))
            continue;

        if (!str_prefix(arg, wch->name)) {
            found = TRUE;

            /* work out the printing */
            class = class_table[wch->class].who_name;
            switch (wch->level) {
            case MAX_LEVEL - 0: class = "|WIMP|w"; break;
            case MAX_LEVEL - 1: class = "|YCRE|w"; break;
            case MAX_LEVEL - 2: class = "|YSUP|w"; break;
            case MAX_LEVEL - 3: class = "|GDEI|w"; break;
            case MAX_LEVEL - 4: class = "|GGOD|w"; break;
            case MAX_LEVEL - 5: class = "|gIMM|w"; break;
            case MAX_LEVEL - 6: class = "|gDEM|w"; break;
            case MAX_LEVEL - 7: class = "ANG"; break;
            case MAX_LEVEL - 8: class = "AVA"; break;
            }

            /* a little formatting */
            snprintf(buf, sizeof(buf), "[%2d %s %s] %s%s%s%s%s", wch->level,
                     wch->race < MAX_PC_RACE ? pc_race_table[wch->race].who_name : "     ", class,
                     (wch->pcdata->pcclan) ? (wch->pcdata->pcclan->clan->whoname) : "",
                     IS_SET(wch->act, PLR_KILLER) ? "(|RKILLER|w) " : "",
                     IS_SET(wch->act, PLR_THIEF) ? "(|RTHIEF|w) " : "", wch->name,
                     IS_NPC(wch) ? "" : wch->pcdata->title);
            strcat(output, buf);
            if (IS_SET(wch->act, PLR_WIZINVIS) && (get_trust(ch) >= LEVEL_IMMORTAL)) {
                snprintf(buf, sizeof(buf), " |g(Wizi at level %d)|w", wch->invis_level);
                strcat(output, buf);
            }
            if (IS_SET(wch->act, PLR_PROWL) && (get_trust(ch) >= LEVEL_IMMORTAL)) {
                snprintf(buf, sizeof(buf), " |g(Prowl level %d)|w", wch->invis_level);
                strcat(output, buf);
            }
            strcat(output, "\n\r");
        }
    }

    if (!found) {
        send_to_char("No one of that name is playing.\n\r", ch);
        return;
    }

    page_to_char(output, ch);
}

/*
 * New 'who' command originally by Alander of Rivers of Mud.
 */
void do_who(CHAR_DATA *ch, char *argument) {
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    char output[4 * MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;
    int iClass;
    int iRace;
    int iClan;
    int iLevelLower;
    int iLevelUpper;
    int nNumber;
    int nMatch;
    int n;
    bool rgfClass[MAX_CLASS];
    bool rgfRace[MAX_PC_RACE];
    bool rgfClan[NUM_CLANS];
    bool fClassRestrict;
    bool fRaceRestrict;
    bool fClanRestrict;
    bool fImmortalOnly;
    char arg[MAX_STRING_LENGTH];

    /*
     * Set default arguments.
     */
    iLevelLower = 0;
    iLevelUpper = MAX_LEVEL;
    fClassRestrict = FALSE;
    fRaceRestrict = FALSE;
    fClanRestrict = FALSE;
    fImmortalOnly = FALSE;
    for (iClass = 0; iClass < MAX_CLASS; iClass++)
        rgfClass[iClass] = FALSE;
    for (iRace = 0; iRace < MAX_PC_RACE; iRace++)
        rgfRace[iRace] = FALSE;
    for (iClan = 0; iClan < NUM_CLANS; iClan++)
        rgfClan[iClan] = FALSE;

    /*
     * Parse arguments.
     */
    nNumber = 0;
    for (;;) {

        argument = one_argument(argument, arg);
        if (arg[0] == '\0')
            break;

        if (is_number(arg)) {
            switch (++nNumber) {
            case 1: iLevelLower = atoi(arg); break;
            case 2: iLevelUpper = atoi(arg); break;
            default: send_to_char("Only two level numbers allowed.\n\r", ch); return;
            }
        } else {

            /*
             * Look for classes to turn on.
             */
            if (arg[0] == 'i') {
                fImmortalOnly = TRUE;
            } else {
                iClass = class_lookup(arg);
                if (iClass == -1) {
                    iRace = race_lookup(arg);
                    if (iRace == 0 || iRace >= MAX_PC_RACE) {
                        /* Check if clan exists */
                        iClan = -1;
                        for (n = 0; n < NUM_CLANS; n++)
                            if (is_name(arg, clantable[n].name))
                                iClan = n;
                        /* Check for NO match on clans */
                        if (iClan == -1) {
                            send_to_char("That's not a valid race, class, or clan.\n\r", ch);
                            return;
                        } else
                        /* It DID match! */
                        {
                            fClanRestrict = TRUE;
                            rgfClan[iClan] = TRUE;
                        }
                    } else {
                        fRaceRestrict = TRUE;
                        rgfRace[iRace] = TRUE;
                    }
                } else {
                    fClassRestrict = TRUE;
                    rgfClass[iClass] = TRUE;
                }
            }
        }
    }

    /*
     * Now show matching chars.
     */
    nMatch = 0;
    buf[0] = '\0';
    output[0] = '\0';
    for (d = descriptor_list; d != NULL; d = d->next) {
        CHAR_DATA *wch;
        char const *class;

        /*
         * Check for match against restrictions.
         * Don't use trust as that exposes trusted mortals.
         */
        if (d->connected != CON_PLAYING || !can_see(ch, d->character))
            continue;
        /* added Faramir 13/8/96 because switched imms were visible to all*/
        if (d->original != NULL)
            if (!can_see(ch, d->original))
                continue;

        wch = (d->original != NULL) ? d->original : d->character;
        if (wch->level < iLevelLower || wch->level > iLevelUpper || (fImmortalOnly && wch->level < LEVEL_HERO)
            || (fClassRestrict && !rgfClass[wch->class]) || (fRaceRestrict && !rgfRace[wch->race]))
            continue;
        if (fClanRestrict) {
            int z, showthem = 0;
            if (!wch->pcdata->pcclan)
                continue;
            for (z = 0; z < NUM_CLANS; z++) {
                if (rgfClan[z] && (clantable[z].clanchar == wch->pcdata->pcclan->clan->clanchar))
                    showthem = 1;
            }
            if (!showthem)
                continue;
        }

        nMatch++;

        /*
         * Figure out what to print for class.
         */
        class = class_table[wch->class].who_name;
        switch (wch->level) {
        default: break; {
            case MAX_LEVEL - 0: class = "|WIMP|w"; break;
            case MAX_LEVEL - 1: class = "|YCRE|w"; break;
            case MAX_LEVEL - 2: class = "|YSUP|w"; break;
            case MAX_LEVEL - 3: class = "|GDEI|w"; break;
            case MAX_LEVEL - 4: class = "|GGOD|w"; break;
            case MAX_LEVEL - 5: class = "|gIMM|w"; break;
            case MAX_LEVEL - 6: class = "|gDEM|w"; break;
            case MAX_LEVEL - 7: class = "ANG"; break;
            case MAX_LEVEL - 8: class = "AVA"; break;
            }
        }

        /*
         * Format it up.
         */
        snprintf(buf, sizeof(buf), "[%3d %s %s] %s%s%s%s%s%s", wch->level,
                 wch->race < MAX_PC_RACE ? pc_race_table[wch->race].who_name : "     ", class,
                 IS_SET(wch->act, PLR_KILLER) ? "(|RKILLER|w) " : "", IS_SET(wch->act, PLR_THIEF) ? "(|RTHIEF|w) " : "",
                 IS_SET(wch->act, PLR_AFK) ? "(|cAFK|w) " : "",
                 (wch->pcdata->pcclan) ? (wch->pcdata->pcclan->clan->whoname) : "", wch->name,
                 IS_NPC(wch) ? "" : wch->pcdata->title);
        strcat(buf, "|w\0");
        strcat(output, buf);
        if (IS_SET(wch->act, PLR_WIZINVIS) && (get_trust(ch) >= LEVEL_IMMORTAL)) {
            snprintf(buf, sizeof(buf), " |g(Wizi at level %d)|w", wch->invis_level);
            strcat(output, buf);
        }
        if (IS_SET(wch->act, PLR_PROWL) && (get_trust(ch) >= LEVEL_IMMORTAL)) {
            snprintf(buf, sizeof(buf), " |g(Prowl level %d)|w", wch->invis_level);
            strcat(output, buf);
        }
        strcat(output, "\n\r");
    }

    snprintf(buf2, sizeof(buf2), "\n\rPlayers found: %d\n\r", nMatch);
    strcat(output, buf2);
    page_to_char(output, ch);
    return;
}

void do_count(CHAR_DATA *ch, char *argument) {
    (void)argument;
    int count;
    DESCRIPTOR_DATA *d;
    char buf[MAX_STRING_LENGTH];

    count = 0;

    for (d = descriptor_list; d != NULL; d = d->next)
        if (d->connected == CON_PLAYING && can_see(ch, d->character))
            count++;

    max_on = UMAX(count, max_on);

    if (max_on == count)
        snprintf(buf, sizeof(buf), "There are %d characters on, the most so far today.\n\r", count);
    else
        snprintf(buf, sizeof(buf), "There are %d characters on, the most on today was %d.\n\r", count, max_on);

    send_to_char(buf, ch);
}

void do_inventory(CHAR_DATA *ch, char *argument) {
    (void)argument;
    send_to_char("You are carrying:\n\r", ch);
    show_list_to_char(ch->carrying, ch, TRUE, TRUE);
    return;
}

void do_equipment(CHAR_DATA *ch, char *argument) {
    (void)argument;
    OBJ_DATA *obj;
    int iWear;
    bool found;
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];

    send_to_char("You are using:\n\r", ch);
    found = FALSE;
    for (iWear = 0; iWear < MAX_WEAR; iWear++) {
        if ((obj = get_eq_char(ch, iWear)) == NULL)
            continue;

        if (obj->wear_string != NULL) {
            snprintf(buf2, sizeof(buf2), "<%s>", obj->wear_string);
            snprintf(buf, sizeof(buf), "%-20s", buf2);
            send_to_char(buf, ch);
        } else {
            send_to_char(where_name[iWear], ch);
        }

        if (can_see_obj(ch, obj)) {
            send_to_char(format_obj_to_char(obj, ch, TRUE), ch);
            send_to_char("\n\r", ch);
        } else {
            send_to_char("something.\n\r", ch);
        }
        found = TRUE;
    }

    if (!found)
        send_to_char("Nothing.\n\r", ch);

    return;
}

void do_compare(CHAR_DATA *ch, char *argument) {
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    OBJ_DATA *obj1;
    OBJ_DATA *obj2;
    int value1;
    int value2;
    char *msg;

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);
    if (arg1[0] == '\0') {
        send_to_char("Compare what to what?\n\r", ch);
        return;
    }

    if ((obj1 = get_obj_carry(ch, arg1)) == NULL) {
        send_to_char("You do not have that item.\n\r", ch);
        return;
    }

    if (arg2[0] == '\0') {
        for (obj2 = ch->carrying; obj2 != NULL; obj2 = obj2->next_content) {
            if (obj2->wear_loc != WEAR_NONE && can_see_obj(ch, obj2) && obj1->item_type == obj2->item_type
                && (obj1->wear_flags & obj2->wear_flags & ~ITEM_TAKE) != 0)
                break;
        }

        if (obj2 == NULL) {
            send_to_char("You aren't wearing anything comparable.\n\r", ch);
            return;
        }
    }

    else if ((obj2 = get_obj_carry(ch, arg2)) == NULL) {
        send_to_char("You do not have that item.\n\r", ch);
        return;
    }

    msg = NULL;
    value1 = 0;
    value2 = 0;

    if (obj1 == obj2) {
        msg = "You compare $p to itself.  It looks about the same.";
    } else if (obj1->item_type != obj2->item_type) {
        msg = "You can't compare $p and $P.";
    } else {
        switch (obj1->item_type) {
        default: msg = "You can't compare $p and $P."; break;

        case ITEM_ARMOR:
            value1 = obj1->value[0] + obj1->value[1] + obj1->value[2];
            value2 = obj2->value[0] + obj2->value[1] + obj2->value[2];
            break;

        case ITEM_WEAPON:
            if (obj1->pIndexData->new_format)
                value1 = (1 + obj1->value[2]) * obj1->value[1];
            else
                value1 = obj1->value[1] + obj1->value[2];

            if (obj2->pIndexData->new_format)
                value2 = (1 + obj2->value[2]) * obj2->value[1];
            else
                value2 = obj2->value[1] + obj2->value[2];
            break;
        }
    }

    if (msg == NULL) {
        if (value1 == value2)
            msg = "$p and $P look about the same.";
        else if (value1 > value2)
            msg = "$p looks better than $P.";
        else
            msg = "$p looks worse than $P.";
    }

    act(msg, ch, obj1, obj2, TO_CHAR);
    return;
}

void do_credits(CHAR_DATA *ch, char *argument) {
    (void)argument;
    do_help(ch, "diku");
    return;
}

void do_where(CHAR_DATA *ch, char *argument) {
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    DESCRIPTOR_DATA *d;
    bool found;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        send_to_char("|cPlayers near you:|w\n\r", ch);
        found = FALSE;
        for (d = descriptor_list; d; d = d->next) {
            if (d->connected == CON_PLAYING && (victim = d->character) != NULL && !IS_NPC(victim)
                && victim->in_room != NULL && victim->in_room->area == ch->in_room->area && can_see(ch, victim)) {
                found = TRUE;
                snprintf(buf, sizeof(buf), "|W%-28s|w %s\n\r", victim->name, victim->in_room->name);
                send_to_char(buf, ch);
            }
        }
        if (!found)
            send_to_char("None\n\r", ch);
    } else {
        found = FALSE;
        for (victim = char_list; victim != NULL; victim = victim->next) {
            if (victim->in_room != NULL && victim->in_room->area == ch->in_room->area && !IS_AFFECTED(victim, AFF_HIDE)
                && !IS_AFFECTED(victim, AFF_SNEAK) && can_see(ch, victim) && is_name(arg, victim->name)) {
                found = TRUE;
                snprintf(buf, sizeof(buf), "%-28s %s\n\r", PERS(victim, ch), victim->in_room->name);
                send_to_char(buf, ch);
                break;
            }
        }
        if (!found)
            act("You didn't find any $T.", ch, NULL, arg, TO_CHAR);
    }

    return;
}

void do_consider(CHAR_DATA *ch, char *argument) {
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    char *msg;
    int diff;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        send_to_char("Consider killing whom?\n\r", ch);
        return;
    }

    if ((victim = get_char_room(ch, arg)) == NULL) {
        send_to_char("They're not here.\n\r", ch);
        return;
    }

    if (is_safe(ch, victim)) {
        send_to_char("Don't even think about it.\n\r", ch);
        return;
    }

    diff = victim->level - ch->level;

    if (diff <= -10)
        msg = "You can kill $N naked and weaponless.";
    else if (diff <= -5)
        msg = "$N is no match for you.";
    else if (diff <= -2)
        msg = "$N looks like an easy kill.";
    else if (diff <= 1)
        msg = "The perfect match!";
    else if (diff <= 4)
        msg = "$N says 'Do you feel lucky, punk?'.";
    else if (diff <= 9)
        msg = "$N laughs at you mercilessly.";
    else
        msg = "|RDeath will thank you for your gift.|w";

    act(msg, ch, NULL, victim, TO_CHAR);
    if (ch->level >= LEVEL_CONSIDER)
        do_mstat(ch, argument);
    return;
}

void set_prompt(CHAR_DATA *ch, char *prompt) {

    if (IS_NPC(ch)) {
        bug("Set_prompt: NPC.", 0);
        return;
    }

    free_string(ch->pcdata->prompt);
    ch->pcdata->prompt = str_dup(prompt);
    return;
}

void set_title(CHAR_DATA *ch, char *title) {
    char buf[MAX_STRING_LENGTH];

    if (IS_NPC(ch)) {
        bug("Set_title: NPC.", 0);
        return;
    }

    if (title[0] != '.' && title[0] != ',' && title[0] != '!' && title[0] != '?') {
        buf[0] = ' ';
        strcpy(buf + 1, title);
    } else {
        strcpy(buf, title);
    }

    free_string(ch->pcdata->title);
    ch->pcdata->title = str_dup(buf);
    return;
}

void do_title(CHAR_DATA *ch, char *argument) {
    if (IS_NPC(ch))
        return;

    if (argument[0] == '\0') {
        send_to_char("Change your title to what?\n\r", ch);
        return;
    }

    if (strlen(argument) > 45)
        argument[45] = '\0';

    smash_tilde(argument);
    set_title(ch, argument);
    send_to_char("Ok.\n\r", ch);
}

void do_description(CHAR_DATA *ch, char *argument) {
    char buf[MAX_STRING_LENGTH];

    if (argument[0] != '\0') {
        buf[0] = '\0';
        smash_tilde(argument);
        if (argument[0] == '+') {
            if (ch->description != NULL)
                strcat(buf, ch->description);
            argument++;
            while (isspace(*argument))
                argument++;
        }
        if (!str_cmp(argument, "-")) {
            int counter;
            int length;

            if (ch->description == NULL) {
                send_to_char("You have no description.\n\r", ch);
                return;
            }
            strcpy(buf, ch->description);
            length = strlen(buf);
            if (length <= 1) {
                send_to_char("There is no body of text to delete.\n\r", ch);
                return;
            }
            if (length == 2) {
                free_string(ch->description);
                ch->description = str_dup("");
                send_to_char("Ok.\n\r", ch);
                return;
            }

            for (counter = length - 3; counter >= 0; counter--) {
                if (counter == 0) {
                    free_string(ch->description);
                    ch->description = str_dup("");
                    send_to_char("Ok.\n\r", ch);
                    return;
                }
                if (buf[counter] == '\n') {
                    free_string(ch->description);
                    buf[counter + 2] = '\0';
                    ch->description = str_dup(buf);
                    send_to_char("Ok.\n\r", ch);
                    return;
                }
            }
        }

        if (strlen(buf) + strlen(argument) >= MAX_STRING_LENGTH - 2) {
            send_to_char("Description too long.\n\r", ch);
            return;
        }

        strcat(buf, argument);
        strcat(buf, "\n\r");
        free_string(ch->description);
        ch->description = str_dup(buf);
    }

    send_to_char("Your description is:\n\r", ch);
    send_to_char(ch->description ? ch->description : "(None).\n\r", ch);
    return;
}

void do_report(CHAR_DATA *ch, char *argument) {
    (void)argument;
    char buf[MAX_INPUT_LENGTH];

    snprintf(buf, sizeof(buf), "You say 'I have %d/%d hp %d/%d mana %d/%d mv %d xp.'\n\r", ch->hit, ch->max_hit,
             ch->mana, ch->max_mana, ch->move, ch->max_move, (int)ch->exp);

    send_to_char(buf, ch);

    snprintf(buf, sizeof(buf), "$n says 'I have %d/%d hp %d/%d mana %d/%d mv %d xp.'", ch->hit, ch->max_hit, ch->mana,
             ch->max_mana, ch->move, ch->max_move, (int)ch->exp);

    act(buf, ch, NULL, NULL, TO_ROOM);

    return;
}

void do_practice(CHAR_DATA *ch, char *argument) {
    char buf[MAX_STRING_LENGTH];
    int sn;

    if (IS_NPC(ch))
        return;

    if (argument[0] == '\0') {
        int col;

        col = 0;
        for (sn = 0; sn < MAX_SKILL; sn++) {
            if (skill_table[sn].name == NULL)
                break;
            if (ch->level < get_skill_level(ch, sn)
                || ch->pcdata->learned[sn] < 1 /* skill is not known (NOT get_skill_learned) */)
                continue;

            snprintf(buf, sizeof(buf), "%-18s %3d%%  ", skill_table[sn].name,
                     ch->pcdata->learned[sn]); // NOT get_skill_learned
            send_to_char(buf, ch);
            if (++col % 3 == 0)
                send_to_char("\n\r", ch);
        }

        if (col % 3 != 0)
            send_to_char("\n\r", ch);

        snprintf(buf, sizeof(buf), "You have %d practice sessions left.\n\r", ch->practice);
        send_to_char(buf, ch);
    } else {
        CHAR_DATA *mob;
        int adept;

        if (!IS_AWAKE(ch)) {
            send_to_char("In your dreams, or what?\n\r", ch);
            return;
        }

        for (mob = ch->in_room->people; mob != NULL; mob = mob->next_in_room) {
            if (IS_NPC(mob) && IS_SET(mob->act, ACT_PRACTICE))
                break;
        }

        if (mob == NULL) {
            send_to_char("You can't do that here.\n\r", ch);
            return;
        }

        if (ch->practice <= 0) {
            send_to_char("You have no practice sessions left.\n\r", ch);
            return;
        }

        if ((sn = skill_lookup(argument)) < 0
            || (!IS_NPC(ch)
                && ((ch->level < get_skill_level(ch, sn) || (ch->pcdata->learned[sn] < 1))))) // NOT get_skill_learned
        {
            send_to_char("You can't practice that.\n\r", ch);
            return;
        }

        adept = IS_NPC(ch) ? 100 : class_table[ch->class].skill_adept;

        if (ch->pcdata->learned[sn] >= adept) // NOT get_skill_learned
        {
            snprintf(buf, sizeof(buf), "You are already learned at %s.\n\r", skill_table[sn].name);
            send_to_char(buf, ch);
        } else {
            ch->practice--;
            if (get_skill_trains(ch, sn) < 0) {
                ch->pcdata->learned[sn] += int_app[get_curr_stat(ch, STAT_INT)].learn / 4;
            } else {
                ch->pcdata->learned[sn] += int_app[get_curr_stat(ch, STAT_INT)].learn / get_skill_difficulty(ch, sn);
            }
            if (ch->pcdata->learned[sn] < adept) // NOT get_skill_learned
            {
                act("You practice $T.", ch, NULL, skill_table[sn].name, TO_CHAR);
                act("$n practices $T.", ch, NULL, skill_table[sn].name, TO_ROOM);
            } else {
                ch->pcdata->learned[sn] = adept;
                act("You are now learned at $T.", ch, NULL, skill_table[sn].name, TO_CHAR);
                act("$n is now learned at $T.", ch, NULL, skill_table[sn].name, TO_ROOM);
            }
        }
    }
    return;
}

/*
 * 'Wimpy' originally by Dionysos.
 */
void do_wimpy(CHAR_DATA *ch, char *argument) {
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    int wimpy;

    one_argument(argument, arg);

    if (arg[0] == '\0')
        wimpy = ch->max_hit / 5;
    else
        wimpy = atoi(arg);

    if (wimpy < 0) {
        send_to_char("Your courage exceeds your wisdom.\n\r", ch);
        return;
    }

    if (wimpy > ch->max_hit / 2) {
        send_to_char("Such cowardice ill becomes you.\n\r", ch);
        return;
    }

    ch->wimpy = wimpy;
    snprintf(buf, sizeof(buf), "Wimpy set to %d hit points.\n\r", wimpy);
    send_to_char(buf, ch);
    return;
}

void do_password(CHAR_DATA *ch, char *argument) {
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char *pArg;
    char *pwdnew;
    char *p;
    char cEnd;

    if (IS_NPC(ch))
        return;

    /*
     * Can't use one_argument here because it smashes case.
     * So we just steal all its code.  Bleagh.
     */
    pArg = arg1;
    while (isspace(*argument))
        argument++;

    cEnd = ' ';
    if (*argument == '\'' || *argument == '"')
        cEnd = *argument++;

    while (*argument != '\0') {
        if (*argument == cEnd) {
            argument++;
            break;
        }
        *pArg++ = *argument++;
    }
    *pArg = '\0';

    pArg = arg2;
    while (isspace(*argument))
        argument++;

    cEnd = ' ';
    if (*argument == '\'' || *argument == '"')
        cEnd = *argument++;

    while (*argument != '\0') {
        if (*argument == cEnd) {
            argument++;
            break;
        }
        *pArg++ = *argument++;
    }
    *pArg = '\0';

    if (arg1[0] == '\0' || arg2[0] == '\0') {
        send_to_char("Syntax: password <old> <new>.\n\r", ch);
        return;
    }

    if ((strlen(ch->pcdata->pwd) > 0) && strcmp(crypt(arg1, ch->pcdata->pwd), ch->pcdata->pwd)) {
        WAIT_STATE(ch, 40);
        send_to_char("Wrong password.  Wait 10 seconds.\n\r", ch);
        return;
    }

    if ((int)strlen(arg2) < 5) {
        send_to_char("New password must be at least five characters long.\n\r", ch);
        return;
    }

    /*
     * No tilde allowed because of player file format.
     */
    pwdnew = crypt(arg2, ch->name);
    for (p = pwdnew; *p != '\0'; p++) {
        if (*p == '~') {
            send_to_char("New password not acceptable, try again.\n\r", ch);
            return;
        }
    }

    free_string(ch->pcdata->pwd);
    ch->pcdata->pwd = str_dup(pwdnew);
    save_char_obj(ch);
    send_to_char("Ok.\n\r", ch);
    return;
}

/* RT configure command SMASHED */

/* MrG Scan command */

void do_scan(CHAR_DATA *ch, char *argument) {
    (void)argument;
    extern char *const dir_name[];
    ROOM_INDEX_DATA *current_place;
    CHAR_DATA *current_person;
    CHAR_DATA *next_person;
    EXIT_DATA *pexit;
    char buf[MAX_STRING_LENGTH];
    int count_num_rooms;
    int num_rooms_scan = UMAX(1, ch->level / 10);
    bool found_anything = FALSE;
    int direction;

    send_to_char("You can see around you :\n\r", ch);
    /* Loop for each point of the compass */

    for (direction = 0; direction < MAX_DIR; direction++) {
        /* No exits in that direction */

        current_place = ch->in_room;

        /* Loop for the distance see-able */

        for (count_num_rooms = 0; count_num_rooms < num_rooms_scan; count_num_rooms++) {

            if ((pexit = current_place->exit[direction]) == NULL || (current_place = pexit->u1.to_room) == NULL
                || !can_see_room(ch, pexit->u1.to_room) || IS_SET(pexit->exit_info, EX_CLOSED))
                break;

            /* This loop goes through each character in a room and says
                        whether or not they are visible */

            for (current_person = current_place->people; current_person != NULL; current_person = next_person) {
                next_person = current_person->next_in_room;
                if (can_see(ch, current_person)) {
                    snprintf(buf, sizeof(buf), "%d %-5s: |W%s|w\n\r", (count_num_rooms + 1),
                             capitalize(dir_name[direction]),
                             (current_person->pcdata == NULL) ? current_person->short_descr : current_person->name);

                    send_to_char(buf, ch);
                    found_anything = TRUE;
                }
            } /* Closes the for_each_char_loop */

        } /* Closes the for_each distance seeable loop */

    } /* closes main loop for each direction */
    if (found_anything == FALSE)
        send_to_char("Nothing of great interest.\n\r", ch);
}

/*
 * OLC:  alist to list all areas
 */

void do_alist(CHAR_DATA *ch, char *argument) {
    (void)argument;
    AREA_DATA *pArea;
    BUFFER *buffer = buffer_create();

    buffer_addline_fmt(buffer, "%3s %-29s %-5s-%5s %-10s %3s %-10s\n\r", "Num", "Area Name", "Lvnum", "Uvnum",
                       "Filename", "Sec", "Builders");

    for (pArea = area_first; pArea; pArea = pArea->next) {
        buffer_addline_fmt(buffer, "%3d %-29.29s %-5d-%5d %-12.12s %d %-10.10s\n\r", pArea->vnum, pArea->areaname,
                           pArea->lvnum, pArea->uvnum, pArea->filename, pArea->security, pArea->builders);
    }
    buffer_send(buffer, ch);
}
