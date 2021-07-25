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
/*  (C) 1995-2021 Xania Development Team                                 */
/*  See the header to file: merc.h for original code copyrights          */
/*  Additional: ROM2.4 copyright as stated at top of file: ban.c         */
/*                                                                       */
/*  ban.c: very useful utilities for keeping certain people out!         */
/*                                                                       */
/*************************************************************************/

#include "Ban.hpp"
#include "ArgParser.hpp"
#include "Char.hpp"
#include "Flag.hpp"
#include "comm.hpp"
#include "common/BitOps.hpp"
#include "common/Configuration.hpp"
#include "db.h"
#include "interp.h"
#include "string_utils.hpp"

#include <range/v3/algorithm/find_if.hpp>
#include <range/v3/algorithm/remove_if.hpp>

void do_ban(Char *ch, ArgParser args) { Bans::singleton().ban_site(ch, args, false); }

void do_permban(Char *ch, ArgParser args) { Bans::singleton().ban_site(ch, args, true); }

void do_allow(Char *ch, ArgParser args) { Bans::singleton().allow_site(ch, args); }

Bans::Bans(Dependencies &dependencies) : dependencies_{dependencies}, bans_{} {}

bool Bans::check_ban(std::string_view site, const int type_bits) const {
    const auto host = lower_case(site);
    for (auto &&ban : bans_) {
        if (!check_bit(ban->ban_flags_, type_bits))
            continue;

        if (check_bit(ban->ban_flags_, BAN_PREFIX) && check_bit(ban->ban_flags_, BAN_SUFFIX)
            && matches_inside(ban->site_, host))
            return true;

        if (check_bit(ban->ban_flags_, BAN_PREFIX)
            && !str_suffix(ban->site_.c_str(), host.c_str())) // FIXME: we need a matches_end()
            return true;

        if (check_bit(ban->ban_flags_, BAN_SUFFIX) && matches_start(ban->site_, host))
            return true;

        if (!check_bit(ban->ban_flags_, BAN_SUFFIX) && !check_bit(ban->ban_flags_, BAN_PREFIX)
            && matches(ban->site_, host))
            return true;
    }
    return false;
}

bool Bans::ban_site(Char *ch, ArgParser args, const bool is_permanent) {
    if (args.empty()) {
        list(ch);
        return false;
    }
    int ban_type_flag{0};
    const std::string site_arg{args.shift()};
    if (site_arg[0] == '*') {
        set_bit(ban_type_flag, BAN_PREFIX);
    }
    if (site_arg[site_arg.size() - 1] == '*') {
        set_bit(ban_type_flag, BAN_SUFFIX);
    }
    if (is_permanent) {
        set_bit(ban_type_flag, BAN_PERMANENT);
    }
    const auto site = replace_strings(site_arg, "*", "");
    if (site.empty()) {
        ch->send_line("Ban which site?");
        return false;
    }

    const auto ban_type_arg = args.shift();
    /* find out what type of ban */
    if (ban_type_arg.empty() || matches(ban_type_arg, "all"))
        set_bit(ban_type_flag, BAN_ALL);
    else if (matches(ban_type_arg, "newbies"))
        set_bit(ban_type_flag, BAN_NEWBIES);
    else if (matches(ban_type_arg, "permit"))
        set_bit(ban_type_flag, BAN_PERMIT);
    else {
        ch->send_line("Acceptable ban types are 'all', 'newbies', and 'permit'.");
        return false;
    }
    if (auto existing = ranges::find_if(
            bans_,
            [&site, &ch](const auto &ban) { return matches(site, ban->site_) && ban->level_ > ch->get_trust(); });
        existing != bans_.end()) {
        ch->send_line("That ban was set by a higher power.");
        return false;
    }
    // Clear out any existing ban matching the new one.
    bans_.erase(ranges::remove_if(bans_, [&site](auto &&ban) { return matches(site, ban->site_); }), bans_.end());

    bans_.emplace_back(std::make_unique<const Ban>(site, ch->get_trust(), ban_type_flag));
    save();
    ch->send_line("Bans updated.");
    return true;
}

bool Bans::allow_site(Char *ch, ArgParser args) {
    if (const auto opt_index = args.try_shift_number()) {
        const size_t index = static_cast<size_t>(*opt_index);
        if (index >= bans_.size()) {
            ch->send_line("Please specify a value between 0 and {}.", bans_.size() - 1);
            return false;
        }
        bans_.erase(bans_.begin() + index);
        save();
        ch->send_line("Bans updated.");
        return true;
    } else {
        ch->send_line("Please specify the number of the ban you wish to lift.");
        return false;
    }
}

size_t Bans::load() {
    FILE *fp;
    if ((fp = dependencies_.open_read()) == nullptr) {
        return 0;
    }
    auto count = 0;
    for (;;) {
        auto c = getc(fp);
        if (c == EOF)
            break;
        else
            ungetc(c, fp);
        // This isn't resilient against malformed ban lines.
        const std::string name = fread_word(fp);
        const int level = fread_number(fp);
        const int flags = fread_flag(fp);
        bans_.emplace_back(std::make_unique<const Ban>(name, level, flags));
        count++;
        fread_to_eol(fp);
    }
    dependencies_.close(fp);
    return bans_.size();
}

void Bans::save() {
    FILE *fp;
    bool found = false;
    if ((fp = dependencies_.open_write()) == nullptr) {
        perror("Error opening ban file for write");
    }
    for (auto &&ban : bans_) {
        if (check_bit(ban->ban_flags_, BAN_PERMANENT)) {
            found = true;
            fmt::print(fp, "{} {} {}\n", ban->site_, ban->level_, serialize_flags(ban->ban_flags_));
        }
    }
    dependencies_.close(fp);
    if (!found)
        dependencies_.unlink();
}

void Bans::list(Char *ch) {
    if (bans_.empty()) {
        ch->send_line("No sites banned at this time.");
        return;
    }
    ch->send_line(" No.  Banned sites       Level  Type     Status");
    auto index = 0;
    for (auto &&ban : bans_) {
        const auto ban_flags_ = fmt::format("{}{}{}", check_bit(ban->ban_flags_, BAN_PREFIX) ? "*" : "", ban->site_,
                                            check_bit(ban->ban_flags_, BAN_SUFFIX) ? "*" : "");
        const auto ban_line = fmt::format(
            "{:>3})  {:<17}    {:<3}  {:<7}  {}", index++, ban_flags_, ban->level_,
            check_bit(ban->ban_flags_, BAN_NEWBIES)
                ? "newbies"
                : check_bit(ban->ban_flags_, BAN_PERMIT) ? "permit" : check_bit(ban->ban_flags_, BAN_ALL) ? "all" : "",
            check_bit(ban->ban_flags_, BAN_PERMANENT) ? "perm" : "temp");
        ch->send_line(ban_line);
    }
}

class DependenciesImpl : public Bans::Dependencies {
public:
    DependenciesImpl();
    FILE *open_read();
    FILE *open_write();
    void close(FILE *fp);
    void unlink();

private:
    FILE *open_file(const char *mode);
    std::string ban_file_;
};

DependenciesImpl::DependenciesImpl() { ban_file_ = Configuration::singleton().ban_file(); }

FILE *DependenciesImpl::open_read() { return open_file("r"); }

FILE *DependenciesImpl::open_write() { return open_file("w"); }

FILE *DependenciesImpl::open_file(const char *mode) { return fopen(ban_file_.c_str(), mode); }

void DependenciesImpl::close(FILE *fp) { fclose(fp); }

void DependenciesImpl::unlink() { ::unlink(ban_file_.c_str()); }

Bans &Bans::singleton() {
    static DependenciesImpl dependencies;
    static Bans singleton(dependencies);
    return singleton;
}
