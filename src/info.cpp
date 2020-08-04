/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                    */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  info.c: setinfo and finger functions: by Rohan                       */
/*                                                                       */
/*************************************************************************/

#include "buffer.h"
#include "merc.h"
#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>

void do_setinfo(CHAR_DATA *ch, char *arg);
void do_finger(CHAR_DATA *ch, char *arg);
void read_char_info(FINGER_INFO *info);

#if defined(KEY)
#undef KEY
#endif

#define KEY(literal, field, value)                                                                                     \
    if (!str_cmp(word, literal)) {                                                                                     \
        field = value;                                                                                                 \
        fMatch = true;                                                                                                 \
        break;                                                                                                         \
    }

/* For finger info */
FINGER_INFO *info_cache = NULL;
KNOWN_PLAYERS *player_list = NULL;

void load_player_list() {
    /* Procedure roughly nicked from emacs info :) */
    KNOWN_PLAYERS *current_pos = player_list;
    DIR *dp;
    struct dirent *ep;
    dp = opendir("../player");
    if (dp != NULL) {
        while ((ep = (readdir(dp)))) {
            if (player_list == NULL) {
                player_list = alloc_mem(sizeof(KNOWN_PLAYERS));
                player_list->next = NULL;
                player_list->name = str_dup(ep->d_name);
                current_pos = player_list;
            } else {
                current_pos->next = alloc_mem(sizeof(KNOWN_PLAYERS));
                current_pos = current_pos->next;
                current_pos->next = NULL;
                current_pos->name = str_dup(ep->d_name);
            }
        }
        closedir(dp);
        log_string("Player list loaded.");
    } else
        bug("Couldn't open the player directory for reading file entries.");
}

/* Rohan's setinfo function */
void do_setinfo(CHAR_DATA *ch, char *argument) {
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    FINGER_INFO *cur;
    bool info_found = false;
    int update_show = 0;
    int update_hide = 0;

    if (argument[0] == '\0') {
        send_to_char("These are your current info settings:\n\r", ch);
        if (ch->pcdata->info_message[0] == '\0')
            snprintf(buf, sizeof(buf), "Message: Not set.\n\r");
        else if (is_set_extra(ch, EXTRA_INFO_MESSAGE))
            snprintf(buf, sizeof(buf), "Message: %s\n\r", ch->pcdata->info_message);
        else
            snprintf(buf, sizeof(buf), "Message: Withheld.\n\r");
        send_to_char(buf, ch);
        return;
    }
    argument = one_argument(argument, arg);

    smash_tilde(arg);

    if (!strcmp(arg, "message")) {
        if (argument[0] == '\0') {
            if (is_set_extra(ch, EXTRA_INFO_MESSAGE)) {
                if (ch->pcdata->info_message[0] == '\0')
                    snprintf(buf, sizeof(buf), "Your message is currently not set.\n\r");
                else
                    snprintf(buf, sizeof(buf), "Your message is currently set as: %s.\n\r", ch->pcdata->info_message);
            } else
                snprintf(buf, sizeof(buf), "Your message is currently being withheld.\n\r");
            send_to_char(buf, ch);
        } else {
            free_string(ch->pcdata->info_message);
            if (strlen(argument) > 45)
                argument[45] = '\0';
            ch->pcdata->info_message = str_dup(argument);
            snprintf(buf, sizeof(buf), "Your message has been set to: %s.\n\r", ch->pcdata->info_message);
            set_extra(ch, EXTRA_INFO_MESSAGE);
            send_to_char(buf, ch);
            /* Update the info if it is in cache */
            cur = info_cache;
            while (cur != NULL && info_found == false) {
                if (!strcmp(cur->name, ch->name))
                    info_found = true;
                else
                    cur = cur->next;
            }
            if (info_found == true) {
                /*send_to_char ("Player info has been found in cache.\n\r", ch);*/
                free_string(cur->info_message);
                cur->info_message = str_dup(ch->pcdata->info_message);
                cur->i_message = true;
            }
        }
        return;
    }

    if (!strcmp(arg, "show")) {
        if (argument[0] == '\0') {
            send_to_char("You must supply one of the following arguments to 'setinfo show':\n\r    message\n\r", ch);
            return;
        } else {
            if (!strcmp(argument, "message")) {
                if (ch->pcdata->info_message[0] == '\0')
                    send_to_char("Your message must be set in order for it to be read by other players.\n\rUse "
                                 "'setinfo message <your message>'.\n\r",
                                 ch);
                else {
                    set_extra(ch, EXTRA_INFO_MESSAGE);
                    send_to_char("Players will now be able to read your message when looking at your info.\n\r", ch);
                    update_show = EXTRA_INFO_MESSAGE;
                }
                return;
            }
            if (update_show != 0) {
                /* Update the info if it is in cache */
                cur = info_cache;
                while (cur != NULL && info_found == false) {
                    if (!strcmp(cur->name, ch->name))
                        info_found = true;
                    else
                        cur = cur->next;
                }
                if (info_found == true) {
                    /*send_to_char ("Player info has been found in cache.\n\r", ch);*/
                    switch (update_show) {
                    case EXTRA_INFO_MESSAGE: cur->i_message = true; break;
                    }
                }
            }
            send_to_char("You must supply one of the following arguments to 'setinfo show':\n\r    message\n\r", ch);
            return;
        }
    }

    if (!strcmp(arg, "hide")) {
        if (argument[0] == '\0') {
            send_to_char("You must supply one of the following arguments to 'setinfo hide':\n\r    message\n\r", ch);
            return;
        } else {
            if (!strcmp(argument, "message")) {
                remove_extra(ch, EXTRA_INFO_MESSAGE);
                send_to_char("Players will now not be able to read your message when looking at your info.\n\r", ch);
                update_hide = EXTRA_INFO_MESSAGE;
            }
            if (update_hide != 0) {
                /* Update the info if it is in cache */
                cur = info_cache;
                while (cur != NULL && info_found == false) {
                    if (!strcmp(cur->name, ch->name))
                        info_found = true;
                    else
                        cur = cur->next;
                }
                if (info_found == true) {
                    /*send_to_char ("Player info has been found in cache.\n\r", ch);*/
                    switch (update_hide) {
                    case EXTRA_INFO_MESSAGE: cur->i_message = false; break;
                    }
                }
                return;
            } else {
                send_to_char("You must supply one of the following arguments to 'setinfo hide':\n\r    message\n\r",
                             ch);
                return;
            }
        }
    }

    if (!strcmp(arg, "clear")) {
        free_string(ch->pcdata->info_message);
        ch->pcdata->info_message = str_dup("");
        send_to_char("Your info details have been cleared.\n\r", ch);
        /* Do the same if in cache */
        cur = info_cache;
        while (cur != NULL && info_found == false) {
            if (!strcmp(cur->name, ch->name))
                info_found = true;
            else
                cur = cur->next;
        }
        if (info_found == true) {
            /*send_to_char ("Player info has been found in cache.\n\r", ch);*/
            free_string(cur->info_message);
            cur->info_message = str_dup("");
        }
        return;
    }
    send_to_char("Usage:\n\r", ch);
    send_to_char("setinfo                    Show your current info settings.\n\r", ch);
    send_to_char("setinfo message [Message]  Show/set the message field.\n\r", ch);
    send_to_char("setinfo show message\n\r", ch);
    send_to_char("Set the field as readable by other players when looking at your info.\n\r", ch);
    send_to_char("setinfo hide message\n\r", ch);
    send_to_char("Set the field as non-readable by other players when looking at your info.\n\r", ch);
    send_to_char("setinfo clear              Clear all the fields.\n\r", ch);
}

/* MrG finger command - oo-er! */
/* VERY BROKEN and not included :-) */
/* Now updated by Rohan, and hopefully included */
/* Shows info on player as set by setinfo, plus datestamp as to
   when char was last logged on */

void do_finger(CHAR_DATA *ch, char *argument) {
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim = NULL;
    KNOWN_PLAYERS *cursor = player_list;
    FINGER_INFO *cur;
    bool player_found = false;
    bool info_found = false;
    CHAR_DATA *wch = char_list;
    bool char_found = false;

    if (argument[0] == '\0' || !strcmp(ch->name, capitalize(argument))) {
        do_setinfo(ch, "");
        return;
    } else {
        argument = capitalize(argument);

        /* Find out if argument is a mob */
        victim = get_char_world(ch, argument);

        /* Notice DEATH hack here!!! */
        if (victim != NULL && IS_NPC(victim) && strcmp(argument, "Death")) {
            send_to_char("Mobs don't have very interesting information to give to you.\n\r", ch);
            return;
        }

        /* Find out if char is logged on (irrespective of whether we can
           see that char or not - hence I don't use get_char_world, as
           it only returns chars we can see */
        while (wch != NULL && char_found == false) {
            if (!strcmp(wch->name, argument))
                char_found = true;
            else
                wch = wch->next;
        }

        if (char_found == true)
            victim = wch;
        else
            victim = NULL;

        while (cursor != NULL && player_found == false) {
            if (!strcmp(cursor->name, argument))
                player_found = true;
            cursor = cursor->next;
        }

        if (player_found == true) {
            /* Player exists in player directory */
            /*	  send_to_char ("Player found.\n\r", ch);*/
            cur = info_cache;

            while (cur != NULL && info_found == false) {
                if (!strcmp(cur->name, argument))
                    info_found = true;
                else
                    cur = cur->next;
            }

            if (info_found == true) {
                /* Player info is in cache. Do nothing (I know, not very good
                   programming technique, but who cares :) */
                /*send_to_char ("Player info has been found in cache.\n\r", ch);*/
            } else {
                /* Player info not in cache, proceed to put it in there */
                /*send_to_char ("Player info is not in cache.\n\r", ch);*/
                cur = alloc_mem(sizeof(FINGER_INFO));
                cur->name = str_dup(argument);
                cur->next = info_cache;
                info_cache = cur;

                if (victim != NULL && !IS_NPC(victim)) {
                    if (victim->desc == NULL) {
                        /* Character is switched???? Load from file as
                           it crashes otherwise - can it be changed? */
                        /*log_string ("Desc is null.");*/
                        cur->info_message = str_dup("");
                        cur->last_login_from = str_dup("");
                        cur->last_login_at = str_dup("");
                        read_char_info(cur);
                    } else {
                        /* Player is currently logged in, so get info from loaded
                           char and put it in cache */
                        /*send_to_char ("Player is currently logged in.\n\r", ch);*/
                        cur->info_message = str_dup(victim->pcdata->info_message);
                        cur->invis_level = victim->invis_level;
                        cur->last_login_at = str_dup(victim->desc->logintime);
                        cur->last_login_from = str_dup(victim->desc->host);

                        if (is_set_extra(victim, EXTRA_INFO_MESSAGE))
                            cur->i_message = true;
                        else
                            cur->i_message = false;
                    }
                } else {
                    /* Player is not logged in, so need to load it in.
                       initialise everything to nothing first to avoid
                       null pointers (because I had several crashes if
                       I didn't do this) */
                    /*send_to_char ("Player is not currently logged in.\n\r", ch);*/

                    cur->info_message = str_dup("");
                    cur->last_login_from = str_dup("");
                    cur->last_login_at = str_dup("");
                    read_char_info(cur);
                }
            }

            if (cur->info_message[0] == '\0')
                snprintf(buf, sizeof(buf), "Message: Not set.\n\r");
            else if (cur->i_message)
                snprintf(buf, sizeof(buf), "Message: %s\n\r", cur->info_message);
            else
                snprintf(buf, sizeof(buf), "Message: Withheld.\n\r");
            send_to_char(buf, ch);

            /* This is the tricky bit - should the player login time be seen
               by this player or not? */

            if (victim != NULL && victim->desc != NULL && !IS_NPC(victim)) {

                /* Player is currently logged in */
                if (victim->invis_level > ch->level && get_trust(ch) < 96) {

                    snprintf(buf, sizeof(buf),
                             "It is impossible to determine the last time that %s roamed\n\rthe hills of Xania.\n\r",
                             victim->name);
                    send_to_char(buf, ch);
                } else {
                    snprintf(buf, sizeof(buf), "%s is currently roaming the hills of Xania!\n\r", victim->name);
                    send_to_char(buf, ch);
                    if (get_trust(ch) >= 96) {
                        if (victim->desc->host[0] == '\0')
                            snprintf(buf, sizeof(buf),
                                     "It is impossible to determine where %s last logged in from.\n\r", victim->name);
                        else {
                            char hostbuf[MAX_MASKED_HOSTNAME];
                            snprintf(buf, sizeof(buf), "%s is currently logged in from %s.\n\r", cur->name,
                                     get_masked_hostname(hostbuf, victim->desc->host));
                        }
                        send_to_char(buf, ch);
                    }
                }
            } else {
                /* Player is not currently logged in */
                if (cur->invis_level > ch->level && get_trust(ch) < 96) {

                    snprintf(buf, sizeof(buf),
                             "It is impossible to determine the last time that %s roamed\n\rthe hills of Xania.\n\r",
                             cur->name);
                    send_to_char(buf, ch);
                } else {

                    if (cur->last_login_at[0] == '\0')
                        snprintf(
                            buf, sizeof(buf),
                            "It is impossible to determine the last time that %s roamed\n\rthe hills of Xania.\n\r",
                            cur->name);
                    else
                        snprintf(buf, sizeof(buf), "%s last roamed the hills of Xania on %s", cur->name,
                                 cur->last_login_at);

                    send_to_char(buf, ch);

                    if (get_trust(ch) >= 96) {
                        if (cur->last_login_from[0] == '\0')
                            snprintf(buf, sizeof(buf),
                                     "It is impossible to determine where %s last logged in from.\n\r", cur->name);
                        else
                            snprintf(buf, sizeof(buf), "%s last logged in from %s.\n\r", cur->name,
                                     cur->last_login_from);

                        send_to_char(buf, ch);
                    }
                }
            }
            return;
        } else {
            send_to_char("That player does not exist.\n\r", ch);
            return;
        }
    }
}

void read_char_info(FINGER_INFO *info) {
    FILE *fp;
    char strsave[MAX_INPUT_LENGTH];
    char *word;
    char *line;
    bool fMatch;

    snprintf(strsave, sizeof(strsave), "%s%s", PLAYER_DIR, info->name);
    if ((fp = fopen(strsave, "r")) != NULL) {
        /*      log_string ("Player file open");*/
        for (;;) {
            word = fread_word(fp);
            if (feof(fp))
                return;
            fMatch = false;

            switch (UPPER(word[0])) {
            case 'E':
                if (!strcmp("End", word))
                    return;
                if (!str_cmp(word, "ExtraBits")) {
                    line = fread_string(fp);
                    if (line[EXTRA_INFO_MESSAGE] == '1')
                        info->i_message = true;
                    else
                        info->i_message = false;
                    fread_to_eol(fp);
                    fMatch = true;
                    break;
                }
                break;

            case 'I':
                KEY("InvisLevel", info->invis_level, fread_number(fp));
                KEY("Invi", info->invis_level, fread_number(fp));
                KEY("Info_message", info->info_message, fread_string(fp));
                break;
            case 'L':
                KEY("LastLoginFrom", info->last_login_from, fread_string(fp));
                KEY("LastLoginAt", info->last_login_at, fread_string(fp));
                break;
            }
            if (!fMatch) {
                fread_to_eol(fp);
            }
        }

        fclose(fp);
    } else {
        bug("Could not open player file '%s' to extract info.", info->name);
    }
}
