/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                    */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  info.c: setinfo and finger functions: by Rohan                       */
/*                                                                       */
/*************************************************************************/

#include "info.hpp"
#include "ArgParser.hpp"
#include "Logging.hpp"
#include "TimeInfoData.hpp"
#include "WrappedFd.hpp"
#include "db.h"
#include "handler.hpp"
#include "interp.h"
#include "merc.h"
#include "save.hpp"
#include "string_utils.hpp"

#include <fmt/format.h>

#include <cstdio>
#include <cstring>
#include <sys/stat.h>
#include <unordered_map>

void do_finger(Char *ch, ArgParser args);
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
FingerInfo *search_info_cache(const Char *ch) { return search_info_cache(ch->name); }

}

/* Rohan's setinfo function */
void do_setinfo(Char *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    int update_show = 0;
    int update_hide = 0;
    char smash_tilded[MAX_INPUT_LENGTH];

    if (argument[0] == '\0') {
        ch->send_line("These are your current info settings:");
        if (ch->pcdata->info_message.empty())
            ch->send_line("Message: Not set.");
        else if (ch->is_set_extra(EXTRA_INFO_MESSAGE))
            ch->send_line("Message: {}.", ch->pcdata->info_message);
        else
            ch->send_line("Message: Withheld.");
        return;
    }
    strncpy(smash_tilded, smash_tilde(argument).c_str(),
            MAX_INPUT_LENGTH - 1); // TODO to minimize changes during refactor
    auto *args = smash_tilded;
    args = one_argument(args, arg);

    if (!strcmp(arg, "message")) {
        if (args[0] == '\0') {
            if (ch->is_set_extra(EXTRA_INFO_MESSAGE)) {
                if (ch->pcdata->info_message.empty())
                    ch->send_line("Your message is currently not set.");
                else
                    ch->send_line("Your message is currently set as: {}.", ch->pcdata->info_message);
            } else
                ch->send_line("Your message is currently being withheld.");
        } else {
            if (strlen(args) > 45)
                args[45] = '\0';
            ch->pcdata->info_message = args;
            ch->set_extra(EXTRA_INFO_MESSAGE);
            ch->send_line("Your message has been set to: {}.", ch->pcdata->info_message);
            /* Update the info if it is in cache */
            if (auto cur = search_info_cache(ch)) {
                cur->info_message = ch->pcdata->info_message;
                cur->i_message = true;
            }
        }
        return;
    }

    if (!strcmp(arg, "show")) {
        if (args[0] == '\0') {
            ch->send_line("You must supply one of the following arguments to 'setinfo show':\n\r    message");
            return;
        } else {
            if (!strcmp(args, "message")) {
                if (ch->pcdata->info_message[0] == '\0')
                    ch->send_to("Your message must be set in order for it to be read by other players.\n\rUse "
                                "'setinfo message <your message>'.\n\r");
                else {
                    ch->set_extra(EXTRA_INFO_MESSAGE);
                    ch->send_line("Players will now be able to read your message when looking at your info.");
                    update_show = EXTRA_INFO_MESSAGE;
                }
                return;
            }
            if (update_show != 0) { // TODO IDE COMPLAINS THIS IS UNREACHABLE
                /* Update the info if it is in cache */
                if (auto cur = search_info_cache(ch)) {
                    switch (update_show) {
                    case EXTRA_INFO_MESSAGE: cur->i_message = true; break;
                    }
                }
            }
            ch->send_line("You must supply one of the following arguments to 'setinfo show':\n\r    message");
            return;
        }
    }

    if (!strcmp(arg, "hide")) {
        if (args[0] == '\0') {
            ch->send_line("You must supply one of the following arguments to 'setinfo hide':\n\r    message");
            return;
        } else {
            if (!strcmp(args, "message")) {
                ch->remove_extra(EXTRA_INFO_MESSAGE);
                ch->send_line("Players will now not be able to read your message when looking at your info.");
                update_hide = EXTRA_INFO_MESSAGE;
            }
            if (update_hide != 0) {
                /* Update the info if it is in cache */
                if (auto cur = search_info_cache(ch)) {
                    switch (update_hide) {
                    case EXTRA_INFO_MESSAGE: cur->i_message = false; break;
                    }
                }
                return;
            } else {
                ch->send_to("You must supply one of the following arguments to 'setinfo hide':\n\r    message\n\r");
                return;
            }
        }
    }

    if (!strcmp(arg, "clear")) {
        ch->pcdata->info_message.clear();
        ch->send_line("Your info details have been cleared.");
        /* Do the same if in cache */
        if (auto cur = search_info_cache(ch)) {
            cur->info_message.clear();
        }
        return;
    }
    ch->send_line("Usage:");
    ch->send_line("setinfo                    Show your current info settings.");
    ch->send_line("setinfo message [Message]  Show/set the message field.");
    ch->send_line("setinfo show message");
    ch->send_line("Set the field as readable by other players when looking at your info.");
    ch->send_line("setinfo hide message");
    ch->send_line("Set the field as non-readable by other players when looking at your info.");
    ch->send_line("setinfo clear              Clear all the fields.");
}

/* MrG finger command - oo-er! */
/* VERY BROKEN and not included :-) */
/* Now updated by Rohan, and hopefully included */
/* Shows info on player as set by setinfo, plus datestamp as to
   when char was last logged on */
namespace {
Char *find_char_by_name(std::string_view name) {
    // Find out if char is logged on (irrespective of whether we can see that char or not - hence I don't use
    // get_char_world, as it only returns chars we can see
    for (auto *wch : char_list)
        if (matches(wch->name, name))
            return wch;
    return nullptr;
}
}
void do_finger(Char *ch, ArgParser args) {
    if (args.empty()) {
        do_setinfo(ch, "");
        return;
    }
    auto name = args.shift();
    if (matches(ch->name, name)) {
        do_setinfo(ch, "");
        return;
    }

    // Disallow file system special characters before calling stat().
    for (auto c : name) {
        if (!isalpha(c)) {
            ch->send_line("Invalid name.");
            return;
        }
    }

    /* Find out if argument is a mob */
    auto *victim = get_char_world(ch, name);

    /* Notice DEATH hack here!!! */
    if (victim != nullptr && victim->is_npc() && !matches(name, "Death")) {
        ch->send_line("Mobs don't have very interesting information to give to you.");
        return;
    }

    victim = find_char_by_name(name);
    struct stat player_file;
    if (!stat(filename_for_player(name).c_str(), &player_file)) {
        /* Player exists in player directory */
        const FingerInfo *cur = search_info_cache(name);
        if (!cur) {
            /* Player info not in cache, proceed to put it in there */
            if (victim && victim->is_pc() && victim->desc) {
                cur = &info_cache
                           .emplace(name, FingerInfo(victim->name, victim->pcdata->info_message,
                                                     victim->desc->login_time(), victim->desc->host(),
                                                     victim->invis_level, victim->is_set_extra(EXTRA_INFO_MESSAGE)))
                           .first->second;
            } else {
                cur = &info_cache.emplace(name, read_char_info(name)).first->second;
            }
        }

        if (cur->info_message.empty())
            ch->send_line("Message: Not set.");
        else if (cur->i_message)
            ch->send_line("Message: {}", cur->info_message);
        else
            ch->send_line("Message: Withheld.");

        /* This is the tricky bit - should the player login time be seen
           by this player or not? */

        if (victim != nullptr && victim->desc != nullptr && victim->is_pc()) {

            /* Player is currently logged in */
            if (victim->invis_level > ch->level && ch->get_trust() < GOD) {
                ch->send_line("It is impossible to determine the last time that {} roamed\n\rthe hills of Xania.",
                              victim->name);
            } else {
                ch->send_line("{} is currently roaming the hills of Xania!", victim->name);
                if (ch->get_trust() >= GOD) {
                    if (victim->desc->host().empty())
                        ch->send_line("It is impossible to determine where {} last logged in from.", victim->name);
                    else {
                        ch->send_line("{} is currently logged in from {}.", cur->name, victim->desc->host());
                    }
                }
            }
        } else {
            /* Player is not currently logged in */
            if (cur->invis_level > ch->level && ch->get_trust() < GOD) {
                ch->send_line("It is impossible to determine the last time that {} roamed\n\rthe hills of Xania.",
                              cur->name);
            } else {

                if (cur->last_login_at[0] == '\0')
                    ch->send_line("It is impossible to determine the last time that {} roamed\n\rthe hills of Xania.",
                                  cur->name);
                else
                    ch->send_line("{} last roamed the hills of Xania on {}.", cur->name, cur->last_login_at);
                if (ch->get_trust() >= GOD) {
                    if (cur->last_login_from[0] == '\0')
                        ch->send_line("It is impossible to determine where {} last logged in from.", cur->name);
                    else
                        ch->send_line("{} last logged in from {}.", cur->name, cur->last_login_from);
                }
            }
        }
    } else {
        ch->send_line("That player does not exist.");
    }
}

void remove_info_for_player(std::string_view player_name) { info_cache.erase(std::string(player_name)); }
void update_info_cache(Char *ch) {
    if (auto cur = search_info_cache(ch)) {
        cur->invis_level = ch->invis_level;

        /* Make sure player isn't link dead. */
        if (ch->desc != nullptr) {
            cur->last_login_at = ch->desc->login_time();
            cur->last_login_from = ch->desc->host();
        } else {
            /* If link dead, we need to grab as much
               info as possible. Death.*/
            cur->last_login_at = formatted_time(current_time);
        }
    }
}

FingerInfo read_char_info(std::string_view player_name) {
    FingerInfo info(player_name);
    if (auto fp = WrappedFd::open(filename_for_player(player_name))) {
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
                info.info_message = fread_stdstring(fp);
            } else if (word == "lastloginfrom") {
                info.last_login_from = fread_stdstring(fp);
            } else if (word == "lastloginat") {
                info.last_login_at = fread_stdstring(fp);
            } else if (word == "name") {
                info.name = fread_stdstring(fp);
            } else {
                fread_to_eol(fp);
            }
        }
    } else {
        bug("Could not open player file '{}' to extract info.", info.name.c_str());
    }
    return info;
}
