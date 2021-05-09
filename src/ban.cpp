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

#include "ban.hpp"

#include "comm.hpp"
#include "common/Configuration.hpp"
#include "db.h"
#include "merc.h"
#include "string_utils.hpp"

#include <gsl/gsl_util>
#include <range/v3/algorithm/find_if.hpp>

#include <cstdio>
#include <utility>
#include <vector>

/*
 * Site ban structure.
 */
struct BAN_DATA {
    std::string name;
    int ban_flags = 0;
    int level = 0;
    BAN_DATA() = default;
    BAN_DATA(std::string name, int flags, int level) : name(std::move(name)), ban_flags(flags), level(level) {}
};

std::vector<BAN_DATA> ban_list;

void save_bans() {
    const auto ban_file = Configuration::singleton().ban_file();

    FILE *fp;
    if ((fp = fopen(ban_file.c_str(), "w")) == nullptr) {
        perror(ban_file.c_str());
        return;
    }

    bool found = false;
    for (const auto &ban : ban_list) {
        if (IS_SET(ban.ban_flags, BAN_PERMANENT)) {
            found = true;
            fmt::print(fp, "{} {} {}\n", ban.name, ban.level, print_flags(ban.ban_flags));
        }
    }
    fclose(fp);
    if (!found)
        unlink(ban_file.c_str());
}

void load_bans() {
    ban_list.clear();

    const auto ban_file = Configuration::singleton().ban_file();

    FILE *fp;
    if ((fp = fopen(ban_file.c_str(), "r")) == nullptr)
        return;

    while (!feof(fp)) {
        auto name = std::string(fread_stdstring_word(fp));
        auto level = fread_number(fp);
        auto flags = gsl::narrow<int>(fread_flag(fp));
        fread_to_eol(fp);

        ban_list.emplace_back(BAN_DATA(name, flags, level));
    }
    fclose(fp);
}

bool check_ban(const std::string &host, int type) {
    for (const auto &ban : ban_list) {
        if (!IS_SET(ban.ban_flags, type))
            continue;

        if (IS_SET(ban.ban_flags, BAN_PREFIX) && IS_SET(ban.ban_flags, BAN_SUFFIX) && matches_inside(ban.name, host))
            return true;

        if (IS_SET(ban.ban_flags, BAN_PREFIX) && matches_end(ban.name, host))
            return true;

        if (IS_SET(ban.ban_flags, BAN_SUFFIX) && matches_start(ban.name, host))
            return true;

        if (!IS_SET(ban.ban_flags, BAN_SUFFIX) && !IS_SET(ban.ban_flags, BAN_PREFIX) && matches(ban.name, host))
            return true;
    }

    return false;
}

void ban_site(Char *ch, ArgParser args, bool fPerm) {
    if (args.empty()) {
        if (ban_list.empty()) {
            ch->send_line("No sites banned at this time.");
            return;
        }
        std::string buffer = "Banned sites       Level  Type     Status\n\r";
        for (const auto &ban : ban_list) {
            auto ban_name = fmt::format("{}{}{}", IS_SET(ban.ban_flags, BAN_PREFIX) ? "*" : "", ban.name,
                                        IS_SET(ban.ban_flags, BAN_SUFFIX) ? "*" : "");
            buffer += fmt::format("{:<17}    {:<3}  {:<7}  {}\n\r", ban_name, ban.level,
                                  IS_SET(ban.ban_flags, BAN_NEWBIES)  ? "newbies"
                                  : IS_SET(ban.ban_flags, BAN_PERMIT) ? "permit"
                                  : IS_SET(ban.ban_flags, BAN_ALL)    ? "all"
                                                                      : "",
                                  IS_SET(ban.ban_flags, BAN_PERMANENT) ? "perm" : "temp");
        }
        ch->page_to(buffer);
        return;
    }

    auto name = args.shift();
    auto type_str = args.shift();

    // find out what type of ban
    int type;
    if (type_str.empty() || matches_start(type_str, "all"))
        type = BAN_ALL;
    else if (matches_start(type_str, "newbies"))
        type = BAN_NEWBIES;
    else if (matches_start(type_str, "permit"))
        type = BAN_PERMIT;
    else {
        ch->send_line("Acceptable ban types are 'all', 'newbies', and 'permit'.");
        return;
    }

    bool prefix = false;
    if (name.front() == '*') {
        prefix = true;
        name.remove_prefix(1);
    }

    bool suffix = false;
    if (!name.empty() && name.back() == '*') {
        suffix = true;
        name.remove_suffix(1);
    }

    if (name.empty()) {
        ch->send_line("Ban which site?");
        return;
    }

    if (auto existing_it = ranges::find_if(ban_list, [&name](const BAN_DATA &ban) { return matches(name, ban.name); });
        existing_it != ban_list.end()) {
        if (existing_it->level > ch->get_trust()) {
            ch->send_line("That ban was set by a higher power.");
            return;
        }
        ban_list.erase(existing_it);
    }

    if (prefix)
        SET_BIT(type, BAN_PREFIX);
    if (suffix)
        SET_BIT(type, BAN_SUFFIX);
    if (fPerm)
        SET_BIT(type, BAN_PERMANENT);

    auto &ban = ban_list.emplace_back(std::string(name), type, ch->get_trust());
    save_bans();
    ch->send_line("The host(s) matching '{}{}{}' have been banned.", IS_SET(ban.ban_flags, BAN_PREFIX) ? "*" : "",
                  ban.name, IS_SET(ban.ban_flags, BAN_SUFFIX) ? "*" : "");
}

void do_ban(Char *ch, ArgParser args) { ban_site(ch, std::move(args), false); }

void do_permban(Char *ch, ArgParser args) { ban_site(ch, std::move(args), true); }

void do_allow(Char *ch, ArgParser args) {
    if (args.empty()) {
        ch->send_line("Remove which site from the ban list?");
        return;
    }

    auto site = args.shift();
    if (site.front() == '*')
        site.remove_prefix(1);
    if (!site.empty() && site.back() == '*')
        site.remove_suffix(1);

    if (auto curr = ranges::find_if(ban_list, [&site](const BAN_DATA &ban) { return matches(site, ban.name); });
        curr != ban_list.end()) {
        if (curr->level > ch->get_trust()) {
            ch->send_line("You are not powerful enough to lift that ban.");
            return;
        }

        ch->send_line("Ban on '{}{}{}' lifted.", IS_SET(curr->ban_flags, BAN_PREFIX) ? "*" : "", site,
                      IS_SET(curr->ban_flags, BAN_SUFFIX) ? "*" : "");
        ban_list.erase(curr);
        save_bans();
        return;
    }

    ch->send_line("That site is not banned.");
}
