/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                    */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  olc_ctrl.c:  Faramir's on line creation manager routines             */
/*                                                                       */
/*************************************************************************/

#if defined(macintosh)
#include <types.h>
#include <time.h>
#else
#if defined(riscos)
#include <time.h>
#include "sys/types.h"
#else
#include <sys/types.h>
#include <sys/time.h>
#endif
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "merc.h"
#include "db.h"
#include "buffer.h"
#include "olc_ctrl.h"

/*** TO DO YET!
 ***
 *** olc_finish();
 *** obj end, undo,
 *** the area file details: vnums, filename, security level etc
 *** the mobile editor shouldn't be hard
 *** update the room editor and add more features
 *** figure out how to set specific resets
 *** check that the possible bug between do_security and olc_initialise is ok
 *** decomment all the commented stuff in merc.h, handler.c, interp.c etc
 *** apply the savearea func and make sure it works
 ***
 *** loads more I can't think of now.
 ***/

extern char str_empty[1];

const char *weapon_class_table[] = {
    "exotic", "sword", "dagger", "spear",   "mace",
    "axe",    "flail", "whip",   "polearm", "$" /* keep at end of list if you know what's good for you */
};

const struct apply_values apply_lookup_table[] = {

    /*
     * Note: the '*' names are like a starred out pwd file: not accessible! :)
     */

    {APPLY_NONE, "none", 0, 0},
    {APPLY_STR, "strength", -5, 5},
    {APPLY_DEX, "dexterity", -5, 5},
    {APPLY_INT, "intelligence", -5, 5},
    {APPLY_WIS, "wisdom", -5, 5},
    {APPLY_CON, "constitution", -5, 5},
    {APPLY_SEX, "sex", -2, 2},
    {APPLY_CLASS, "*", 0, 0},
    {APPLY_LEVEL, "*", 0, 0},
    {APPLY_AGE, "*", 0, 0},
    {APPLY_HEIGHT, "*", 0, 0},
    {APPLY_WEIGHT, "*", 0, 0},
    {APPLY_MANA, "mana", -20, 40},
    {APPLY_HIT, "hp", -20, 40},
    {APPLY_MOVE, "move", -30, 40},
    {APPLY_GOLD, "gold", 0, 4000},
    {APPLY_EXP, "*", 0, 0},
    {APPLY_AC, "armour", -10, 10},
    {APPLY_HITROLL, "hitroll", -5, 5},
    {APPLY_DAMROLL, "damroll", -5, 5},
    {APPLY_SAVING_PARA, "save_para", -10, 10},
    {APPLY_SAVING_ROD, "save_rod", -10, 10},
    {APPLY_SAVING_PETRI, "save_petri", -10, 10},
    {APPLY_SAVING_BREATH, "save_breath", -10, 10},
    {APPLY_SAVING_SPELL, "save_spell", -10, 10},
    {APPLY_MAX, "$", 0, 0}

};

/*
 * Utility for high level Imms/IMPS to give area builders
 * a certain level of access.
 *
 * ALSO MUST ADD: to this function, ability to set the security on
 * any given area file, although the default security value for
 * an area will be actually written into its file
 */

void do_security(CHAR_DATA *ch, char *argument) {

    char buf[MAX_STRING_LENGTH], p[MAX_INPUT_LENGTH], l[MAX_INPUT_LENGTH];
    CHAR_DATA *player;
    int security = 0;

    if (!olc_user_authority(ch)) {
        send_to_char("OLC: sorry, only Faramir can use OLC commands atm.\n\r", ch);
        return;
    }
    argument = one_argument(argument, p);
    one_argument(argument, l);
    if ((p[0] == '\0') || (l[0] == '\0')) {
        send_to_char("Syntax: security <player> <security level 1-5>\n\r", ch);
        return;
    }
    player = (get_char_world(ch, p));
    if (player == NULL) {
        send_to_char("They're not here.\n\r", ch);
        return;
    }
    if (get_trust(ch) < get_trust(player)) {
        send_to_char("You failed.\n\r", ch);
        return;
    }
    security = atoi(l);
    if (security < OLC_SECURITY_NONE || security > OLC_SECURITY_IMPLEMENTOR) {
        send_to_char("Security level must be between 0 and 5 inclusive.\n\r", ch);
        return;
    }
    if ((security > OLC_SECURITY_CREATOR) && (get_trust(player) < IMPLEMENTOR)) {
        send_to_char("Only implementors have security access level 5.\n\r", ch);
        return;
    }
    if ((player->pcdata->olc == NULL) && security == 0) {
        snprintf(buf, sizeof(buf), "OLC: %s already has no security access!\n\r", player->name);
        send_to_char(buf, ch);
        return;
    }
    if ((player->pcdata->olc == NULL) && security > 0)
        olc_initialise(ch);

    if ((player->pcdata->olc->security == OLC_SECURITY_NONE) && security == 0) {
        snprintf(buf, sizeof(buf), "OLC: %s already has no security access!\n\r", player->name);
        send_to_char(buf, ch);
        return;
    }
    switch (security) {

    case 0:
        snprintf(buf, sizeof(buf), "OLC: %s now has no security access.\n\r", player->name);
        send_to_char(buf, ch);
        send_to_char("OLC: your security access has been removed.\n\r", player);
        player->pcdata->olc->security = 0;
        break;
    default:
        snprintf(buf, sizeof(buf), "OLC: security access %d", security);
        send_to_char(buf, player);
        send_to_char(" has been granted to you.\n\r", player);
        send_to_char(buf, ch);
        send_to_char(" given.\n\r", ch);
        player->pcdata->olc->security = security;
    }
    return;
}

bool olc_string_isalpha(CHAR_DATA *ch, char *string, int len) {

    int count;
    for (count = 0; count <= len; count++) {
        if (!isalpha(string[count]))
            return FALSE;
    }
    return TRUE;
}

void do_edit(CHAR_DATA *ch, char *argument) {

    char arg1[MAX_INPUT_LENGTH];

    char filename[FNAME_LEN], areaname[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];

    if ((ch->pcdata->olc == NULL) || (ch->pcdata->olc->security == OLC_SECURITY_NONE)) {
        send_to_char("Huh?\n\r", ch);
        return;
    }
    if (!olc_user_authority(ch)) {
        send_to_char("OLC: sorry, only Faramir can use OLC commands atm.\n\r", ch);
        return;
    }
    if (IS_SET(ch->comm, COMM_OLC_MODE)) {
        send_to_char("OLC: you are already in OLC mode!\n\r", ch);
        return;
    }
    if (argument[0] == '\0') {
        send_to_char("Syntax: edit [current||new||quest] {filename} {'areaname'}\n\r", ch);
        send_to_char("OLC:    new filenames are automatically given .are suffix.\n\r", ch);
        send_to_char("        you must provide filename and areaname for new areas only.\n\r", ch);
        send_to_char("        quest areas require only an areaname argument.\n\r", ch);
        return;
    }
    argument = one_argument(argument, arg1);
    if (!str_cmp(arg1, "current")) {
        /*
         * put in a check for area file names here.
         * they should be loaded into the mud on boot up
         *
         * must then check the builder's security level with that of the area file
         */
        send_to_char("OLC: ok, entering OLC mode ...\n\r", ch);
        olc_initialise(ch);
        SET_BIT(ch->comm, COMM_OLC_MODE);
        /*
         * must then set the master, room, mob and obj vnum ranges,
         * as well as areaname, filename
         * in the ch->pcdata->olc settings
         * can't do that until I know where that info will be stored
         * in the area file/area_data
         */
        return;
    }
    if (!str_cmp(arg1, "new")) {
        argument = one_argument(argument, arg1);
        if (arg1[0] == '\0') {
            send_to_char("Syntax: edit new <filename> <'areaname'>\n\r", ch);
            return;
        }
        if (strlen(arg1) > FNAME_LEN) {
            send_to_char("OLC: filename too long.\n\r", ch);
            return;
        }
        if (!olc_string_isalpha(ch, arg1, strlen(arg1) - 1)) {
            send_to_char("OLC: filename must consist only of letters.\n\r", ch);
            return;
        }
        strcat(filename, arg1);
        argument = one_argument(argument, arg1);
        if (arg1[0] == '\0') {
            send_to_char("OLC: you must provide a name for your area.\n\r", ch);
            send_to_char("     enclose it in '' single quotes.\n\r", ch);
            return;
        }
        snprintf(areaname, sizeof(areaname), arg1);

        olc_initialise(ch);
        ch->pcdata->olc->areaname = str_dup(areaname);
        ch->pcdata->olc->filename = str_dup(filename);
        snprintf(buf, sizeof(buf), "OLC: editing area:%s\n\r     file: %s\n\r", areaname, filename);
        send_to_char(buf, ch);
        SET_BIT(ch->comm, COMM_OLC_MODE);
        return;
    }
    if (!str_cmp(arg1, "quest")) {
        if (ch->pcdata->olc->security < OLC_SECURITY_QUEST) {
            send_to_char("OLC: you do not have sufficient security access to build a questing area.\n\r", ch);
            return;
        }
        one_argument(argument, arg1);
        if (arg1[0] == '\0') {
            send_to_char("Syntax: edit quest 'areaname'\n\r", ch);
            return;
        }
        if (!olc_check_quest_list(ch, arg1)) {
            /* TO DO: routine for                           */
            /* seeing what they are trying to edit          */
            /* perhaps builders can lock other builders out */
            send_to_char("OLC: you have been denied access to that area.\n\r", ch);
            return;
        }

        send_to_char("OLC: ok, entering quest OLC mode ...\n\r", ch);
        olc_initialise(ch);
        ch->pcdata->olc->areaname = str_dup(arg1);
        SET_BIT(ch->comm, COMM_OLC_MODE);
        return;
    }
    send_to_char("OLC:    unrecognised edit command.\n\r", ch);
    send_to_char("Syntax: edit [current||new||quest] {filename} {'areaname'}\n\r", ch);
    return;
}

/*
 * used to alloc mem if a builder doesn't have an OLC struct initialised.
 * potential clash here with do_security and do_edit because you can only
 * enter OLC mode with edit command if you have at least OLC_SECURITY_MINIMUM,
 * and this can only be set by do_security -> this now checks the ch->pcdata->olc
 * and uses this function to alloc the memory if needs be
 */

void olc_initialise(CHAR_DATA *ch) {

    OLC_DATA *new_olc_struct;

    if (ch->pcdata->olc)
        return;

    new_olc_struct = (OLC_DATA *)alloc_mem(sizeof(OLC_DATA));

    new_olc_struct->security = OLC_SECURITY_MINIMUM;
    /* if do_security called this */
    /* may be upped immediately!  */
    new_olc_struct->areaname = &str_empty[0];
    new_olc_struct->filename = &str_empty[0];
    new_olc_struct->current_obj = (OBJ_INDEX_DATA *)alloc_mem(sizeof(OBJ_INDEX_DATA));
    new_olc_struct->current_obj->extra_descr = (EXTRA_DESCR_DATA *)alloc_mem(sizeof(EXTRA_DESCR_DATA));
    new_olc_struct->current_mob = (MOB_INDEX_DATA *)alloc_mem(sizeof(MOB_INDEX_DATA));
    new_olc_struct->current_room = (ROOM_INDEX_DATA *)alloc_mem(sizeof(ROOM_INDEX_DATA));
    new_olc_struct->current_room->extra_descr = (EXTRA_DESCR_DATA *)alloc_mem(sizeof(EXTRA_DESCR_DATA));
    new_olc_struct->obj_template = (OBJ_INDEX_DATA *)alloc_mem(sizeof(OBJ_INDEX_DATA));
    new_olc_struct->obj_template->extra_descr = (EXTRA_DESCR_DATA *)alloc_mem(sizeof(EXTRA_DESCR_DATA));
    new_olc_struct->mob_template = (MOB_INDEX_DATA *)alloc_mem(sizeof(MOB_INDEX_DATA));
    new_olc_struct->room_template = (ROOM_INDEX_DATA *)alloc_mem(sizeof(ROOM_INDEX_DATA));
    new_olc_struct->room_template->extra_descr = (EXTRA_DESCR_DATA *)alloc_mem(sizeof(EXTRA_DESCR_DATA));
    new_olc_struct->master_vnums[0] = 0;
    new_olc_struct->master_vnums[1] = 0;
    new_olc_struct->obj_vnums[0] = 0;
    new_olc_struct->obj_vnums[1] = 0;
    new_olc_struct->mob_vnums[0] = 0;
    new_olc_struct->mob_vnums[1] = 0;
    new_olc_struct->room_vnums[0] = 0;
    new_olc_struct->room_vnums[1] = 0;

    ch->pcdata->olc = new_olc_struct;
    olc_cleanstrings(ch, TRUE, TRUE, TRUE);
    return;
}

void olc_cleanstrings(CHAR_DATA *ch, bool fObj, bool fMob, bool fRoom) {

    if (fObj) {

        ch->pcdata->olc->current_obj->name = &str_empty[0];
        ch->pcdata->olc->current_obj->name = &str_empty[0];
        ch->pcdata->olc->current_obj->name = &str_empty[0];
        ch->pcdata->olc->current_obj->extra_descr->keyword = &str_empty[0];
        ch->pcdata->olc->current_obj->extra_descr->description = &str_empty[0];
        ch->pcdata->olc->obj_template->name = &str_empty[0];
        ch->pcdata->olc->obj_template->name = &str_empty[0];
        ch->pcdata->olc->obj_template->name = &str_empty[0];
        ch->pcdata->olc->current_obj->extra_descr->keyword = &str_empty[0];
        ch->pcdata->olc->current_obj->extra_descr->description = &str_empty[0];
    }
    if (fMob) {
        ch->pcdata->olc->current_mob->player_name = &str_empty[0];
        ch->pcdata->olc->current_mob->short_descr = &str_empty[0];
        ch->pcdata->olc->current_mob->long_descr = &str_empty[0];
        ch->pcdata->olc->current_mob->description = &str_empty[0];
        ch->pcdata->olc->mob_template->player_name = &str_empty[0];
        ch->pcdata->olc->mob_template->short_descr = &str_empty[0];
        ch->pcdata->olc->mob_template->long_descr = &str_empty[0];
        ch->pcdata->olc->mob_template->description = &str_empty[0];
    }
    if (fRoom) {
        ch->pcdata->olc->current_room->name = &str_empty[0];
        ch->pcdata->olc->current_room->description = &str_empty[0];
        ch->pcdata->olc->current_room->extra_descr->keyword = &str_empty[0];
        ch->pcdata->olc->current_room->extra_descr->description = &str_empty[0];
        ch->pcdata->olc->room_template->name = &str_empty[0];
        ch->pcdata->olc->room_template->description = &str_empty[0];
        ch->pcdata->olc->room_template->extra_descr->keyword = &str_empty[0];
        ch->pcdata->olc->room_template->extra_descr->description = &str_empty[0];
    }
    return;
}

void olc_slookup_obj(CHAR_DATA *ch) {

    BUFFER *buffer;
    char text[MAX_STRING_LENGTH];
    int sn, count = 0;

    buffer = new_buf();
    send_to_char("Spells available.\n\r", ch);
    for (sn = 0; sn < MAX_SKILL; sn++) {
        if (skill_table[sn].name == NULL)
            continue;
        if (skill_table[sn].slot == 0) /* don't print skills! they only */
            continue; /* need to know all the spells   */
        if (count == 5) {
            count = 0;
            snprintf(text, sizeof(text), "\n\r");
            add_buf(buffer, text);
        }
        snprintf(text, sizeof(text), "'%s' ", skill_table[sn].name);
        count++;
        add_buf(buffer, text);
    }
    buf_to_char(buffer, ch);
    return;
}

char *olc_stripcolour(char *attack_type) {

    static char buf[MAX_STRING_LENGTH];

    if (attack_type[0] == '|') {
        strcpy(buf, attack_type + 2);
        buf[strlen(attack_type) - 4] = '\0';
        return (char *)&buf;
    } else
        return attack_type;
}

bool olc_check_quest_list(CHAR_DATA *ch, char *argument) { return TRUE; }

bool olc_user_authority(CHAR_DATA *ch) {

    if (!IS_NPC(ch)) {
        if (!str_prefix(ch->name, "Faramir"))
            return TRUE;
    }
    return FALSE;
}

/*
 * TM's original clipboard utils modified into Xania's OLC
 * format.
 */

void clip_add(CHAR_DATA *ch, char *mes) {

    char old_mes_buf[2 * MAX_STRING_LENGTH], *old_mes;
    if (ch->clipboard == NULL) {
        old_mes = strcpy(old_mes_buf, mes);
    } else {
        if ((strlen(ch->clipboard) + strlen(mes)) >= (2 * MAX_STRING_LENGTH)) {
            send_to_char("OLC: clipboard has become too long!\n\r", ch);
            return;
        }
        old_mes = strcpy(old_mes_buf, ch->clipboard);
        strcat(old_mes_buf, mes);
        free_string(ch->clipboard);
    }
    strcat(old_mes_buf, "\n\r");
    ch->clipboard = str_dup(old_mes);
    send_to_char("Ok.\n\r", ch);
}

void clip_sub(CHAR_DATA *ch) {

    char old_mes_buf[MAX_STRING_LENGTH], *old, *newline, *previous;
    if (ch->clipboard == NULL) {
        send_to_char("OLC: your clipboard is already clear.\n\r", ch);
        return;
    }
    old = newline = previous = old_mes_buf;
    strcpy(old_mes_buf, ch->clipboard);
    free_string(ch->clipboard);
    ch->clipboard = NULL;
    for (; *old; old++) {
        if (*old == '\n') {
            previous = newline;
            newline = old;
        }
    }
    *previous = '\0';
    if (strlen(old_mes_buf) > 0) {
        strcat(old_mes_buf, "\n\r");
        ch->clipboard = str_dup(old_mes_buf);
    }
    send_to_char("Ok.\n\r", ch);
    return;
}

void do_clipboard(CHAR_DATA *ch, char *argument) {
    char com[MAX_STRING_LENGTH];

    argument = one_argument(argument, &com);

    if (!str_cmp(com, "+")) {
        clip_add(ch, argument);
        return;
    }
    if (!str_cmp(com, "-")) {
        clip_sub(ch);
        return;
    }
    if (!str_prefix(com, "show")) {
        if (ch->clipboard) {
            send_to_char("OLC: your clipboard:|c\n\r", ch);
            send_to_char(ch->clipboard, ch);
            send_to_char("|w", ch);
        } else {
            send_to_char("OLC: your clipboard is empty.\n\r", ch);
        }
        return;
    }
    if (!str_prefix(com, "clear")) {
        if (ch->clipboard) {
            send_to_char("OLC: clipboard cleared.\n\r", ch);
            free_string(ch->clipboard);
            ch->clipboard = NULL;
        } else {
            send_to_char("OLC: you have no clipboard to erase!\n\r", ch);
        }
        return;
    }
    if (!str_prefix(com, "copyroom")) {
        if (ch->clipboard)
            free_string(ch->clipboard);
        ch->clipboard = str_dup(ch->in_room->description);
        send_to_char("OLC: room description copied to your clipboard.\n\r", ch);
        return;
    }
    send_to_char("Syntax: clipboard [+||-||show||clear||fromdesc] ...", ch);
    return;
}
