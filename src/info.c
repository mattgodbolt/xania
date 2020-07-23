/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                    */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  info.c: setinfo and finger functions: by Rohan                       */
/*                                                                       */
/*************************************************************************/

#if defined(macintosh)
#include <types.h>
#else
#if defined(riscos)
#include "sys/types.h"
#include <time.h>
#else
#include <sys/time.h>
#include <sys/types.h>
#endif
#endif
#include "buffer.h"
#include "merc.h"
#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

DECLARE_DO_FUN(do_setinfo);
DECLARE_DO_FUN(do_finger);
void read_char_info(FINGER_INFO *info);

#if defined(KEY)
#undef KEY
#endif

#define KEY(literal, field, value)                                                                                     \
    if (!str_cmp(word, literal)) {                                                                                     \
        field = value;                                                                                                 \
        fMatch = TRUE;                                                                                                 \
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
                player_list->name = strdup(ep->d_name);
                current_pos = player_list;
            } else {
                current_pos->next = alloc_mem(sizeof(KNOWN_PLAYERS));
                current_pos = current_pos->next;
                current_pos->next = NULL;
                current_pos->name = strdup(ep->d_name);
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
    bool info_found = FALSE;
    int update_show = 0;
    int update_hide = 0;

    if (argument[0] == '\0') {
        send_to_char("These are your current info settings:\n\r", ch);
        if (ch->pcdata->info_name[0] == '\0')
            snprintf(buf, sizeof(buf), "Real name: Not set.\n\r");
        else if (is_set_extra(ch, EXTRA_INFO_NAME))
            snprintf(buf, sizeof(buf), "Real name: %s.\n\r", ch->pcdata->info_name);
        else
            snprintf(buf, sizeof(buf), "Real name: Withheld.\n\r");
        send_to_char(buf, ch);
        if (ch->pcdata->info_email[0] == '\0')
            snprintf(buf, sizeof(buf), "Email: Not set.\n\r");
        else if (is_set_extra(ch, EXTRA_INFO_EMAIL))
            snprintf(buf, sizeof(buf), "Email: %s\n\r", ch->pcdata->info_email);
        else
            snprintf(buf, sizeof(buf), "Email: Withheld.\n\r");
        send_to_char(buf, ch);
        if (ch->pcdata->info_url[0] == '\0')
            snprintf(buf, sizeof(buf), "URL: Not set.\n\r");
        else if (is_set_extra(ch, EXTRA_INFO_URL))
            snprintf(buf, sizeof(buf), "URL: %s\n\r", ch->pcdata->info_url);
        else
            snprintf(buf, sizeof(buf), "URL: Withheld.\n\r");
        send_to_char(buf, ch);
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

    if (!strcmp(arg, "name")) {
        if (argument[0] == '\0') {
            if (is_set_extra(ch, EXTRA_INFO_NAME)) {
                if (ch->pcdata->info_name[0] == '\0')
                    snprintf(buf, sizeof(buf), "Your real name is currently not set.\n\r");
                else
                    snprintf(buf, sizeof(buf), "Your real name is currently set as: %s.\n\r", ch->pcdata->info_name);
            } else
                snprintf(buf, sizeof(buf), "Your name is currently being withheld.\n\r");
            send_to_char(buf, ch);
        } else {
            free_string(ch->pcdata->info_name);
            if (strlen(argument) > 45)
                argument[45] = '\0';
            ch->pcdata->info_name = strdup(argument);
            snprintf(buf, sizeof(buf), "Your real name has been set to: %s.\n\r", ch->pcdata->info_name);
            set_extra(ch, EXTRA_INFO_NAME);
            send_to_char(buf, ch);
            /* Update the info if it is in cache */
            cur = info_cache;
            while (cur != NULL && info_found == FALSE) {
                if (!strcmp(cur->name, ch->name))
                    info_found = TRUE;
                else
                    cur = cur->next;
            }
            if (info_found == TRUE) {
                /*send_to_char ("Player info has been found in cache.\n\r", ch);*/
                free_string(cur->info_name);
                cur->info_name = strdup(ch->pcdata->info_name);
                cur->i_name = TRUE;
            }
        }
        return;
    }

    if (!strcmp(arg, "email")) {
        if (argument[0] == '\0') {
            if (is_set_extra(ch, EXTRA_INFO_EMAIL)) {
                if (ch->pcdata->info_email[0] == '\0')
                    snprintf(buf, sizeof(buf), "Your email address is currently not set.\n\r");
                else
                    snprintf(buf, sizeof(buf), "Your email address is currently set as: %s.\n\r",
                             ch->pcdata->info_email);
            } else
                snprintf(buf, sizeof(buf), "Your email address is currently being withheld.\n\r");
            send_to_char(buf, ch);
        } else {
            free_string(ch->pcdata->info_email);
            if (strlen(argument) > 45)
                argument[45] = '\0';
            ch->pcdata->info_email = strdup(argument);
            snprintf(buf, sizeof(buf), "Your email address has been set to: %s.\n\r", ch->pcdata->info_email);
            set_extra(ch, EXTRA_INFO_EMAIL);
            send_to_char(buf, ch);
            /* Update the info if it is in cache */
            cur = info_cache;
            while (cur != NULL && info_found == FALSE) {
                if (!strcmp(cur->name, ch->name))
                    info_found = TRUE;
                else
                    cur = cur->next;
            }
            if (info_found == TRUE) {
                /*send_to_char ("Player info has been found in cache.\n\r", ch);*/
                free_string(cur->info_email);
                cur->info_email = strdup(ch->pcdata->info_email);
                cur->i_email = TRUE;
            }
        }
        return;
    }

    if (!strcmp(arg, "url")) {
        if (argument[0] == '\0') {
            if (is_set_extra(ch, EXTRA_INFO_URL)) {
                if (ch->pcdata->info_url[0] == '\0')
                    snprintf(buf, sizeof(buf), "Your URL address is currently not set.\n\r");
                else
                    snprintf(buf, sizeof(buf), "Your URL address is currently set as: %s.\n\r", ch->pcdata->info_url);
            } else
                snprintf(buf, sizeof(buf), "Your URL address is currently being withheld.\n\r");
            send_to_char(buf, ch);
        } else {
            free_string(ch->pcdata->info_url);
            if (strlen(argument) > 45)
                argument[45] = '\0';
            ch->pcdata->info_url = strdup(argument);
            snprintf(buf, sizeof(buf), "Your URL address has been set to: %s.\n\r", ch->pcdata->info_url);
            set_extra(ch, EXTRA_INFO_URL);
            send_to_char(buf, ch);
            /* Update the info if it is in cache */
            cur = info_cache;
            while (cur != NULL && info_found == FALSE) {
                if (!strcmp(cur->name, ch->name))
                    info_found = TRUE;
                else
                    cur = cur->next;
            }
            if (info_found == TRUE) {
                /*send_to_char ("Player info has been found in cache.\n\r", ch);*/
                free_string(cur->info_url);
                cur->info_url = strdup(ch->pcdata->info_url);
                cur->i_url = TRUE;
            }
        }
        return;
    }

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
            ch->pcdata->info_message = strdup(argument);
            snprintf(buf, sizeof(buf), "Your message has been set to: %s.\n\r", ch->pcdata->info_message);
            set_extra(ch, EXTRA_INFO_MESSAGE);
            send_to_char(buf, ch);
            /* Update the info if it is in cache */
            cur = info_cache;
            while (cur != NULL && info_found == FALSE) {
                if (!strcmp(cur->name, ch->name))
                    info_found = TRUE;
                else
                    cur = cur->next;
            }
            if (info_found == TRUE) {
                /*send_to_char ("Player info has been found in cache.\n\r", ch);*/
                free_string(cur->info_message);
                cur->info_message = strdup(ch->pcdata->info_message);
                cur->i_message = TRUE;
            }
        }
        return;
    }

    if (!strcmp(arg, "show")) {
        if (argument[0] == '\0') {
            send_to_char(
                "You must supply one of the following arguments to 'setinfo show':\n\rname email url message\n\r", ch);
            return;
        } else {
            if (!strcmp(argument, "name")) {
                if (ch->pcdata->info_name[0] == '\0')
                    send_to_char("Your name must be set in order for it to be read by other players.\n\rUse 'setinfo "
                                 "name <your name>'.\n\r",
                                 ch);
                else {
                    set_extra(ch, EXTRA_INFO_NAME);
                    send_to_char("Players will now be able to read your real name when looking at your info.\n\r", ch);
                    update_show = EXTRA_INFO_NAME;
                }
                return;
            }
            if (!strcmp(argument, "email")) {
                if (ch->pcdata->info_email[0] == '\0')
                    send_to_char("Your email address must be set in order for it to be read by other players.\n\rUse "
                                 "'setinfo email <your email address>'.\n\r",
                                 ch);
                else {
                    set_extra(ch, EXTRA_INFO_EMAIL);
                    send_to_char("Players will now be able to read your email address when looking at your info.\n\r",
                                 ch);
                    update_show = EXTRA_INFO_EMAIL;
                }
                return;
            }
            if (!strcmp(argument, "url")) {
                if (ch->pcdata->info_url[0] == '\0')
                    send_to_char("Your URL address must be set in order for it to be read by other players.\n\rUse "
                                 "'setinfo url <your URL address>'.\n\r",
                                 ch);
                else {
                    set_extra(ch, EXTRA_INFO_URL);
                    send_to_char("Players will now be able to read your URL address when looking at your info.\n\r",
                                 ch);
                    update_show = EXTRA_INFO_URL;
                }
                return;
            }
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
                while (cur != NULL && info_found == FALSE) {
                    if (!strcmp(cur->name, ch->name))
                        info_found = TRUE;
                    else
                        cur = cur->next;
                }
                if (info_found == TRUE) {
                    /*send_to_char ("Player info has been found in cache.\n\r", ch);*/
                    switch (update_show) {
                    case EXTRA_INFO_NAME: cur->i_name = TRUE; break;
                    case EXTRA_INFO_EMAIL: cur->i_email = TRUE; break;
                    case EXTRA_INFO_URL: cur->i_url = TRUE; break;
                    case EXTRA_INFO_MESSAGE: cur->i_message = TRUE; break;
                    }
                }
            }
            send_to_char(
                "You must supply one of the following arguments to 'setinfo show':\n\rname email url message\n\r", ch);
            return;
        }
    }

    if (!strcmp(arg, "hide")) {
        if (argument[0] == '\0') {
            send_to_char(
                "You must supply one of the following arguments to 'setinfo hide':\n\rname email url message\n\r", ch);
            return;
        } else {
            if (!strcmp(argument, "name")) {
                remove_extra(ch, EXTRA_INFO_NAME);
                send_to_char("Players will now not be able to read your real name when looking at your info.\n\r", ch);
                update_hide = EXTRA_INFO_NAME;
            }
            if (!strcmp(argument, "email")) {
                remove_extra(ch, EXTRA_INFO_EMAIL);
                send_to_char(
                    "Players will now not be able to read your email address when looking at your\n\rinfo.\n\r", ch);
                update_hide = EXTRA_INFO_EMAIL;
            }
            if (!strcmp(argument, "url")) {
                remove_extra(ch, EXTRA_INFO_URL);
                send_to_char("Players will now not be able to read your URL address when looking at your\n\rinfo.\n\r",
                             ch);
                update_hide = EXTRA_INFO_URL;
            }
            if (!strcmp(argument, "message")) {
                remove_extra(ch, EXTRA_INFO_MESSAGE);
                send_to_char("Players will now not be able to read your message when looking at your info.\n\r", ch);
                update_hide = EXTRA_INFO_MESSAGE;
            }
            if (update_hide != 0) {
                /* Update the info if it is in cache */
                cur = info_cache;
                while (cur != NULL && info_found == FALSE) {
                    if (!strcmp(cur->name, ch->name))
                        info_found = TRUE;
                    else
                        cur = cur->next;
                }
                if (info_found == TRUE) {
                    /*send_to_char ("Player info has been found in cache.\n\r", ch);*/
                    switch (update_hide) {
                    case EXTRA_INFO_NAME: cur->i_name = FALSE; break;
                    case EXTRA_INFO_EMAIL: cur->i_email = FALSE; break;
                    case EXTRA_INFO_URL: cur->i_url = FALSE; break;
                    case EXTRA_INFO_MESSAGE: cur->i_message = FALSE; break;
                    }
                }
                return;
            } else {
                send_to_char(
                    "You must supply one of the following arguments to 'setinfo hide':\n\rname email url message\n\r",
                    ch);
                return;
            }
        }
    }

    if (!strcmp(arg, "clear")) {
        free_string(ch->pcdata->info_name);
        free_string(ch->pcdata->info_email);
        free_string(ch->pcdata->info_url);
        free_string(ch->pcdata->info_message);
        ch->pcdata->info_name = strdup("");
        ch->pcdata->info_email = strdup("");
        ch->pcdata->info_url = strdup("");
        ch->pcdata->info_message = strdup("");
        send_to_char("Your info details have been cleared.\n\r", ch);
        /* Do the same if in cache */
        cur = info_cache;
        while (cur != NULL && info_found == FALSE) {
            if (!strcmp(cur->name, ch->name))
                info_found = TRUE;
            else
                cur = cur->next;
        }
        if (info_found == TRUE) {
            /*send_to_char ("Player info has been found in cache.\n\r", ch);*/
            free_string(cur->info_name);
            free_string(cur->info_email);
            free_string(cur->info_url);
            free_string(cur->info_message);
            cur->info_name = strdup("");
            cur->info_email = strdup("");
            cur->info_url = strdup("");
            cur->info_message = strdup("");
        }
        return;
    }
    send_to_char("Usage:\n\r", ch);
    send_to_char("setinfo                    Show your current info settings.\n\r", ch);
    send_to_char("setinfo name [Name]        Show/set the name field.\n\r", ch);
    send_to_char("setinfo email [Email]      Show/set the email field.\n\r", ch);
    send_to_char("setinfo url [URL]          Show/set the url field.\n\r", ch);
    send_to_char("setinfo message [Message]  Show/set the message field.\n\r", ch);
    send_to_char("setinfo show name||email||url||message\n\r", ch);
    send_to_char("Set the field as readable by other players when looking at your info.\n\r", ch);
    send_to_char("setinfo hide name||email||url||message\n\r", ch);
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
    bool player_found = FALSE;
    bool info_found = FALSE;
    CHAR_DATA *wch = char_list;
    bool char_found = FALSE;

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
        while (wch != NULL && char_found == FALSE) {
            if (!strcmp(wch->name, argument))
                char_found = TRUE;
            else
                wch = wch->next;
        }

        if (char_found == TRUE)
            victim = wch;
        else
            victim = NULL;

        while (cursor != NULL && player_found == FALSE) {
            if (!strcmp(cursor->name, argument))
                player_found = TRUE;
            cursor = cursor->next;
        }

        if (player_found == TRUE) {
            /* Player exists in player directory */
            /*	  send_to_char ("Player found.\n\r", ch);*/
            cur = info_cache;

            while (cur != NULL && info_found == FALSE) {
                if (!strcmp(cur->name, argument))
                    info_found = TRUE;
                else
                    cur = cur->next;
            }

            if (info_found == TRUE) {
                /* Player info is in cache. Do nothing (I know, not very good
                   programming technique, but who cares :) */
                /*send_to_char ("Player info has been found in cache.\n\r", ch);*/
            } else {
                /* Player info not in cache, proceed to put it in there */
                /*send_to_char ("Player info is not in cache.\n\r", ch);*/
                cur = alloc_mem(sizeof(FINGER_INFO));
                cur->name = strdup(argument);
                cur->next = info_cache;
                info_cache = cur;

                if (victim != NULL && !IS_NPC(victim)) {
                    if (victim->desc == NULL) {
                        /* Character is switched???? Load from file as
                           it crashes otherwise - can it be changed? */
                        /*log_string ("Desc is null.");*/
                        cur->info_name = strdup("");
                        cur->info_email = strdup("");
                        cur->info_url = strdup("");
                        cur->info_message = strdup("");
                        cur->last_login_from = strdup("");
                        cur->last_login_at = strdup("");
                        read_char_info(cur);
                    } else {
                        /* Player is currently logged in, so get info from loaded
                           char and put it in cache */
                        /*send_to_char ("Player is currently logged in.\n\r", ch);*/
                        cur->info_name = strdup(victim->pcdata->info_name);
                        cur->info_email = strdup(victim->pcdata->info_email);
                        cur->info_url = strdup(victim->pcdata->info_url);
                        cur->info_message = strdup(victim->pcdata->info_message);
                        cur->invis_level = victim->invis_level;
                        cur->last_login_at = strdup(victim->desc->logintime);
                        cur->last_login_from = strdup(victim->desc->host);

                        if (is_set_extra(victim, EXTRA_INFO_NAME))
                            cur->i_name = TRUE;
                        else
                            cur->i_name = FALSE;

                        if (is_set_extra(victim, EXTRA_INFO_EMAIL))
                            cur->i_email = TRUE;
                        else
                            cur->i_email = FALSE;

                        if (is_set_extra(victim, EXTRA_INFO_URL))
                            cur->i_url = TRUE;
                        else
                            cur->i_url = FALSE;

                        if (is_set_extra(victim, EXTRA_INFO_MESSAGE))
                            cur->i_message = TRUE;
                        else
                            cur->i_message = FALSE;
                    }
                } else {
                    /* Player is not logged in, so need to load it in.
                       initialise everything to nothing first to avoid
                       null pointers (because I had several crashes if
                       I didn't do this) */
                    /*send_to_char ("Player is not currently logged in.\n\r", ch);*/

                    cur->info_name = strdup("");
                    cur->info_email = strdup("");
                    cur->info_url = strdup("");
                    cur->info_message = strdup("");
                    cur->last_login_from = strdup("");
                    cur->last_login_at = strdup("");
                    read_char_info(cur);
                }
            }

            /* Give the char the info */
            if (cur->info_name[0] == '\0')
                snprintf(buf, sizeof(buf), "Real name: Not set.\n\r");
            else if (cur->i_name)
                snprintf(buf, sizeof(buf), "Real name: %s.\n\r", cur->info_name);
            else
                snprintf(buf, sizeof(buf), "Real name: Withheld.\n\r");
            send_to_char(buf, ch);

            if (cur->info_email[0] == '\0')
                snprintf(buf, sizeof(buf), "Email: Not set.\n\r");
            else if (cur->i_email)
                snprintf(buf, sizeof(buf), "Email: %s\n\r", cur->info_email);
            else
                snprintf(buf, sizeof(buf), "Email: Withheld.\n\r");
            send_to_char(buf, ch);

            if (cur->info_url[0] == '\0')
                snprintf(buf, sizeof(buf), "URL: Not set.\n\r");
            else if (cur->i_url)
                snprintf(buf, sizeof(buf), "URL: %s\n\r", cur->info_url);
            else
                snprintf(buf, sizeof(buf), "URL: Withheld.\n\r");
            send_to_char(buf, ch);

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
                        else
                            snprintf(buf, sizeof(buf), "%s is currently logged in from %s.\n\r", cur->name,
                                     victim->desc->host);
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
    char buf[MAX_STRING_LENGTH];
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
            fMatch = FALSE;

            switch (UPPER(word[0])) {
            case 'E':
                if (!strcmp("End", word))
                    return;
                if (!str_cmp(word, "ExtraBits")) {
                    line = fread_string(fp);
                    if (line[EXTRA_INFO_NAME] == '1')
                        info->i_name = TRUE;
                    else
                        info->i_name = FALSE;
                    if (line[EXTRA_INFO_EMAIL] == '1')
                        info->i_email = TRUE;
                    else
                        info->i_email = FALSE;
                    if (line[EXTRA_INFO_URL] == '1')
                        info->i_url = TRUE;
                    else
                        info->i_url = FALSE;
                    if (line[EXTRA_INFO_MESSAGE] == '1')
                        info->i_message = TRUE;
                    else
                        info->i_message = FALSE;
                    fread_to_eol(fp);
                    fMatch = TRUE;
                    break;
                }
                break;

            case 'I':
                KEY("InvisLevel", info->invis_level, fread_number(fp));
                KEY("Invi", info->invis_level, fread_number(fp));
                KEY("Info_name", info->info_name, fread_string(fp));
                KEY("Info_email", info->info_email, fread_string(fp));
                KEY("Info_message", info->info_message, fread_string(fp));
                KEY("Info_url", info->info_url, fread_string(fp));
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
        snprintf(buf, sizeof(buf), "Could not open player file '%s' to extract info.", info->name);
        bug(buf);
    }
}
