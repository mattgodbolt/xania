/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                    */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  info.c: setinfo and finger functions: by Rohan                       */
/*                                                                       */
/*************************************************************************/

#include "info.hpp"
#include "Descriptor.hpp"
#include "TimeInfoData.hpp"
#include "WrappedFd.hpp"
#include "buffer.h"
#include "comm.hpp"
#include "merc.h"
#include "string_utils.hpp"

#include <fmt/format.h>

#include <cstdio>
#include <cstring>
#include <ctime>
#include <dirent.h>
#include <unordered_map>

using namespace fmt::literals;

void do_finger(CHAR_DATA *ch, char *arg);
FingerInfo read_char_info(std::string_view player_name);

// For finger info.
namespace {

// Annoyingly C++ means this doesn't work all that well for string_view lookups. We have to construct a std::string
// and search by that. I really don't think that's an issue in this case.
std::unordered_map<std::string, FingerInfo> info_cache;

FingerInfo *search_info_cache(std::string_view name) {
    if (auto find_it = info_cache.find(std::string(name)); find_it != info_cache.end())
        return &find_it->second;
    return nullptr;
}
FingerInfo *search_info_cache(const CHAR_DATA *ch) { return search_info_cache(ch->name); }

}

// TODO(#108) is this used?
KNOWN_PLAYERS *player_list = nullptr;

void load_player_list() {
    /* Procedure roughly nicked from emacs info :) */
    KNOWN_PLAYERS *current_pos = player_list;
    DIR *dp;
    struct dirent *ep;
    dp = opendir(PLAYER_DIR);
    if (dp != nullptr) {
        while ((ep = (readdir(dp)))) {
            if (player_list == nullptr) {
                player_list = static_cast<KNOWN_PLAYERS *>(alloc_mem(sizeof(KNOWN_PLAYERS)));
                player_list->next = nullptr;
                player_list->name = str_dup(ep->d_name);
                current_pos = player_list;
            } else {
                current_pos->next = static_cast<KNOWN_PLAYERS *>(alloc_mem(sizeof(KNOWN_PLAYERS)));
                current_pos = current_pos->next;
                current_pos->next = nullptr;
                current_pos->name = str_dup(ep->d_name);
            }
        }
        closedir(dp);
        log_string("Player list loaded.");
    } else
        bug("Couldn't open the player directory for reading file entries.");
}

/* Rohan's setinfo function */
void do_setinfo(CHAR_DATA *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    int update_show = 0;
    int update_hide = 0;
    char smash_tilded[MAX_INPUT_LENGTH];

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
    strncpy(smash_tilded, smash_tilde(argument).c_str(),
            MAX_INPUT_LENGTH - 1); // TODO to minimize changes during refactor
    auto *args = smash_tilded;
    args = one_argument(args, arg);

    if (!strcmp(arg, "message")) {
        if (args[0] == '\0') {
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
            if (strlen(args) > 45)
                args[45] = '\0';
            ch->pcdata->info_message = str_dup(args);
            snprintf(buf, sizeof(buf), "Your message has been set to: %s.\n\r", ch->pcdata->info_message);
            set_extra(ch, EXTRA_INFO_MESSAGE);
            send_to_char(buf, ch);
            /* Update the info if it is in cache */
            if (auto cur = search_info_cache(ch)) {
                /*send_to_char ("Player info has been found in cache.\n\r", ch);*/
                cur->info_message = ch->pcdata->info_message;
                cur->i_message = true;
            }
        }
        return;
    }

    if (!strcmp(arg, "show")) {
        if (args[0] == '\0') {
            send_to_char("You must supply one of the following arguments to 'setinfo show':\n\r    message\n\r", ch);
            return;
        } else {
            if (!strcmp(args, "message")) {
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
            if (update_show != 0) { // TODO IDE COMPLAINS THIS IS UNREACHABLE
                /* Update the info if it is in cache */
                if (auto cur = search_info_cache(ch)) {
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
        if (args[0] == '\0') {
            send_to_char("You must supply one of the following arguments to 'setinfo hide':\n\r    message\n\r", ch);
            return;
        } else {
            if (!strcmp(args, "message")) {
                remove_extra(ch, EXTRA_INFO_MESSAGE);
                send_to_char("Players will now not be able to read your message when looking at your info.\n\r", ch);
                update_hide = EXTRA_INFO_MESSAGE;
            }
            if (update_hide != 0) {
                /* Update the info if it is in cache */
                if (auto cur = search_info_cache(ch)) {
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
        if (auto cur = search_info_cache(ch)) {
            /*send_to_char ("Player info has been found in cache.\n\r", ch);*/
            cur->info_message.clear();
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

void do_finger(CHAR_DATA *ch, const char *argument) {
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim = nullptr;
    KNOWN_PLAYERS *cursor = player_list;
    bool player_found = false;
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
        if (victim != nullptr && IS_NPC(victim) && strcmp(argument, "Death")) {
            send_to_char("Mobs don't have very interesting information to give to you.\n\r", ch);
            return;
        }

        /* Find out if char is logged on (irrespective of whether we can
           see that char or not - hence I don't use get_char_world, as
           it only returns chars we can see */
        while (wch != nullptr && char_found == false) {
            if (!strcmp(wch->name, argument))
                char_found = true;
            else
                wch = wch->next;
        }

        if (char_found == true)
            victim = wch;
        else
            victim = nullptr;

        while (cursor != nullptr && player_found == false) {
            if (!strcmp(cursor->name, argument))
                player_found = true;
            cursor = cursor->next;
        }

        if (player_found == true) {
            /* Player exists in player directory */
            /*   send_to_char ("Player found.\n\r", ch);*/
            const FingerInfo *cur = search_info_cache(argument);
            if (!cur) {
                /* Player info not in cache, proceed to put it in there */
                /*send_to_char ("Player info is not in cache.\n\r", ch);*/
                if (victim && !IS_NPC(victim) && victim->desc) {
                    cur = &info_cache
                               .emplace(argument,
                                        FingerInfo(victim->name, victim->pcdata->info_message,
                                                   victim->desc->login_time(), victim->desc->host(),
                                                   victim->invis_level, is_set_extra(victim, EXTRA_INFO_MESSAGE)))
                               .first->second;
                } else {
                    cur = &info_cache.emplace(argument, read_char_info(argument)).first->second;
                }
            }

            if (cur->info_message.empty())
                snprintf(buf, sizeof(buf), "Message: Not set.\n\r");
            else if (cur->i_message)
                snprintf(buf, sizeof(buf), "Message: %s\n\r", cur->info_message.c_str());
            else
                snprintf(buf, sizeof(buf), "Message: Withheld.\n\r");
            send_to_char(buf, ch);

            /* This is the tricky bit - should the player login time be seen
               by this player or not? */

            if (victim != nullptr && victim->desc != nullptr && !IS_NPC(victim)) {

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
                        if (victim->desc->host().empty())
                            snprintf(buf, sizeof(buf),
                                     "It is impossible to determine where %s last logged in from.\n\r", victim->name);
                        else {
                            snprintf(buf, sizeof(buf), "%s is currently logged in from %s.\n\r", cur->name.c_str(),
                                     victim->desc->host().c_str());
                        }
                        send_to_char(buf, ch);
                    }
                }
            } else {
                /* Player is not currently logged in */
                if (cur->invis_level > ch->level && get_trust(ch) < 96) {

                    snprintf(buf, sizeof(buf),
                             "It is impossible to determine the last time that %s roamed\n\rthe hills of Xania.\n\r",
                             cur->name.c_str());
                    send_to_char(buf, ch);
                } else {

                    if (cur->last_login_at[0] == '\0')
                        snprintf(
                            buf, sizeof(buf),
                            "It is impossible to determine the last time that %s roamed\n\rthe hills of Xania.\n\r",
                            cur->name.c_str());
                    else
                        snprintf(buf, sizeof(buf), "%s last roamed the hills of Xania on %s", cur->name.c_str(),
                                 cur->last_login_at.c_str());

                    send_to_char(buf, ch);

                    if (get_trust(ch) >= 96) {
                        if (cur->last_login_from[0] == '\0')
                            snprintf(buf, sizeof(buf),
                                     "It is impossible to determine where %s last logged in from.\n\r",
                                     cur->name.c_str());
                        else
                            snprintf(buf, sizeof(buf), "%s last logged in from %s.\n\r", cur->name.c_str(),
                                     cur->last_login_from.c_str());

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

void remove_info_for_player(std::string_view player_name) { info_cache.erase(std::string(player_name)); }
void update_info_cache(CHAR_DATA *ch) {
    if (auto cur = search_info_cache(ch)) {
        cur->invis_level = ch->invis_level;

        /* Make sure player isn't link dead. */
        if (ch->desc != nullptr) {
            cur->last_login_at = ch->desc->login_time();
            cur->last_login_from = ch->desc->host();
        } else {
            /* If link dead, we need to grab as much
               info as possible. Death.*/
            cur->last_login_at = "{}"_format(secs_only(current_time));
        }
    }
}

FingerInfo read_char_info(std::string_view player_name) {
    FingerInfo info(player_name);
    if (auto fp = WrappedFd::open("{}{}"_format(PLAYER_DIR, player_name))) {
        for (;;) {
            const std::string word = lower_case(fread_word(fp));
            if (feof(fp))
                break;
            if (word == "end") {
                return info;
            } else if (word == "extrabits") {
                const char *line = fread_string(fp);
                info.i_message = line[EXTRA_INFO_MESSAGE] == '1';
                fread_to_eol(fp);
            } else if (word == "invislevel" || word == "invis") {
                info.invis_level = fread_number(fp);
            } else if (word == "info_message") {
                info.info_message = fread_string(fp);
            } else if (word == "lastloginfrom") {
                info.last_login_from = fread_string(fp);
            } else if (word == "lastloginat") {
                info.last_login_at = fread_string(fp);
            } else if (word == "name") {
                info.name = fread_string(fp);
            } else {
                fread_to_eol(fp);
            }
        }
    } else {
        bug("Could not open player file '%s' to extract info.", info.name.c_str());
    }
    return info;
}
