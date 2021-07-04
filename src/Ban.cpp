/***************************************************************************
 * ROM 2.4 is copyright 1993-1996 Russ Taylor
 * ROM has been brought to you by the ROM consortium
 *     Russ Taylor (rtaylor@pacinfo.com)
 *     Gabrielle Taylor (gtaylor@pacinfo.com)
 *     Brian Moore (rom@rom.efn.org)
 * By using this code, you have agreed to follow the terms of the
 * ROM license, in the file Rom24/doc/rom.license
 ***************************************************************************/

/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                    */
/*  See the header to file: merc.h for original code copyrights          */
/*  Additional: ROM2.4 copyright as stated at top of file: ban.c         */
/*                                                                       */
/*  ban.c: very useful utilities for keeping certain people out!         */
/*                                                                       */
/*************************************************************************/

#include "Ban.hpp"
#include "Char.hpp"
#include "buffer.h"
#include "comm.hpp"
#include "common/Configuration.hpp"
#include "db.h"
#include "flags.h"
#include "interp.h"
#include "merc.h"
#include "string_utils.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {

struct Ban {
    Ban *next{};
    char *name{};
    int ban_flags{};
    int level{};
};

Ban *ban_list;

}

Ban *new_ban() {
    Ban *res;
    res = (Ban *)(malloc(sizeof(Ban)));
    res->level = 0;
    res->name = nullptr;
    res->next = nullptr;
    res->ban_flags = 0;
    return res;
}

void free_ban(Ban *foo) {
    free_string(foo->name);
    free(foo);
}

void save_bans() {
    Ban *ban;
    FILE *fp;
    bool found = false;
    const auto ban_file = Configuration::singleton().ban_file();

    if ((fp = fopen(ban_file.c_str(), "w")) == nullptr) {
        perror(ban_file.c_str());
    }

    for (ban = ban_list; ban != nullptr; ban = ban->next) {
        if (check_bit(ban->ban_flags, BAN_PERMANENT)) {
            found = true;
            fprintf(fp, "%s %d %s\n", ban->name, ban->level, print_flags(ban->ban_flags));
        }
    }

    fclose(fp);
    if (!found)
        unlink(ban_file.c_str());
}

void load_bans() {
    FILE *fp;
    Ban *ban_last;
    const auto ban_file = Configuration::singleton().ban_file();

    if ((fp = fopen(ban_file.c_str(), "r")) == nullptr)
        return;

    ban_last = nullptr;
    for (;;) {
        Ban *pban;
        if (feof(fp)) {
            fclose(fp);
            return;
        }

        pban = new_ban();

        pban->name = str_dup(fread_word(fp));
        pban->level = fread_number(fp);
        pban->ban_flags = fread_flag(fp);
        pban->next = nullptr;
        fread_to_eol(fp);

        if (ban_list == nullptr)
            ban_list = pban;
        else
            ban_last->next = pban;
        ban_last = pban;
    }
}

bool check_ban(const char *site, int type) {
    Ban *pban;

    char host[MAX_STRING_LENGTH];

    strcpy(host, lower_case(site).c_str()); // TODO horrible.

    for (pban = ban_list; pban != nullptr; pban = pban->next) {
        if (!check_bit(pban->ban_flags, type))
            continue;

        if (check_bit(pban->ban_flags, BAN_PREFIX) && check_bit(pban->ban_flags, BAN_SUFFIX)
            && strstr(pban->name, host) != nullptr)
            return true;

        if (check_bit(pban->ban_flags, BAN_PREFIX) && !str_suffix(pban->name, host))
            return true;

        if (check_bit(pban->ban_flags, BAN_SUFFIX) && !str_prefix(pban->name, host))
            return true;

        if (!check_bit(pban->ban_flags, BAN_SUFFIX) && !check_bit(pban->ban_flags, BAN_PREFIX)
            && !str_cmp(pban->name, host))
            return true;
    }

    return false;
}

void ban_site(Char *ch, const char *argument, bool fPerm) {
    char buf[MAX_STRING_LENGTH];
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char *name;
    BUFFER *buffer;
    Ban *pban, *prev;
    bool prefix = false, suffix = false;
    int type;

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if (arg1[0] == '\0') {
        if (ban_list == nullptr) {
            ch->send_line("No sites banned at this time.");
            return;
        }
        buffer = buffer_create();

        buffer_addline(buffer, "Banned sites       Level  Type     Status\n\r");
        for (pban = ban_list; pban != nullptr; pban = pban->next) {
            snprintf(buf, sizeof(buf), "%s%s%s", check_bit(pban->ban_flags, BAN_PREFIX) ? "*" : "", pban->name,
                     check_bit(pban->ban_flags, BAN_SUFFIX) ? "*" : "");
            buffer_addline_fmt(buffer, "%-17s    %-3d  %-7s  %s\n\r", buf, pban->level,
                               check_bit(pban->ban_flags, BAN_NEWBIES)
                                   ? "newbies"
                                   : check_bit(pban->ban_flags, BAN_PERMIT)
                                         ? "permit"
                                         : check_bit(pban->ban_flags, BAN_ALL) ? "all" : "",
                               check_bit(pban->ban_flags, BAN_PERMANENT) ? "perm" : "temp");
        }
        page_to_char(buffer_string(buffer), ch);
        buffer_destroy(buffer);
        return;
    }

    /* find out what type of ban */
    if (arg2[0] == '\0' || !str_prefix(arg2, "all"))
        type = BAN_ALL;
    else if (!str_prefix(arg2, "newbies"))
        type = BAN_NEWBIES;
    else if (!str_prefix(arg2, "permit"))
        type = BAN_PERMIT;
    else {
        ch->send_line("Acceptable ban types are 'all', 'newbies', and 'permit'.");
        return;
    }

    name = arg1;

    if (name[0] == '*') {
        prefix = true;
        name++;
    }

    if (name[strlen(name) - 1] == '*') {
        suffix = true;
        name[strlen(name) - 1] = '\0';
    }

    if (strlen(name) == 0) {
        ch->send_line("Ban which site?");
        return;
    }

    prev = nullptr;
    for (pban = ban_list; pban != nullptr; prev = pban, pban = pban->next) {
        if (!str_cmp(name, pban->name)) {
            if (pban->level > ch->get_trust()) {
                ch->send_line("That ban was set by a higher power.");
                return;
            } else {
                if (prev == nullptr)
                    ban_list = pban->next;
                else
                    prev->next = pban->next;
                free_ban(pban);
            }
        }
    }

    pban = new_ban();
    pban->name = str_dup(name);
    pban->level = ch->get_trust();

    /* set ban type */
    pban->ban_flags = type;

    if (prefix)
        set_bit(pban->ban_flags, BAN_PREFIX);
    if (suffix)
        set_bit(pban->ban_flags, BAN_SUFFIX);
    if (fPerm)
        set_bit(pban->ban_flags, BAN_PERMANENT);

    pban->next = ban_list;
    ban_list = pban;
    save_bans();
    snprintf(buf, sizeof(buf), "The host(s) matching '%s%s%s' have been banned.\n\r",
             check_bit(pban->ban_flags, BAN_PREFIX) ? "*" : "", pban->name,
             check_bit(pban->ban_flags, BAN_SUFFIX) ? "*" : "");
    ch->send_to(buf);
}

void do_ban(Char *ch, const char *argument) { ban_site(ch, argument, false); }

void do_permban(Char *ch, const char *argument) { ban_site(ch, argument, true); }

void do_allow(Char *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH], *aargh = arg;
    char buf[MAX_STRING_LENGTH];
    Ban *prev;
    Ban *curr;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        ch->send_line("Remove which site from the ban list?");
        return;
    }

    if (arg[0] == '*')
        aargh++;

    if (aargh[strlen(aargh)] == '*')
        aargh[strlen(aargh)] = '\0';

    prev = nullptr;
    for (curr = ban_list; curr != nullptr; prev = curr, curr = curr->next) {
        if (!str_cmp(aargh, curr->name)) {
            if (curr->level > ch->get_trust()) {
                ch->send_line("You are not powerful enough to lift that ban.");
                return;
            }
            if (prev == nullptr)
                ban_list = ban_list->next;
            else
                prev->next = curr->next;

            snprintf(buf, sizeof(buf), "Ban on '%s%s%s' lifted.\n\r", check_bit(curr->ban_flags, BAN_PREFIX) ? "*" : "",
                     aargh, check_bit(curr->ban_flags, BAN_SUFFIX) ? "*" : "");
            free_ban(curr);
            ch->send_to(buf);
            save_bans();
            return;
        }
    }

    ch->send_line("That site is not banned.");
}
