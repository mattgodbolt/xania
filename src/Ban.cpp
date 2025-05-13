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
#include "Act.hpp"
#include "ArgParser.hpp"
#include "Char.hpp"
#include "FlagFormat.hpp"
#include "Interpreter.hpp"
#include "common/BitOps.hpp"
#include "common/Configuration.hpp"
#include "db.h"
#include "string_utils.hpp"

#include <range/v3/algorithm/find_if.hpp>
#include <range/v3/algorithm/remove_if.hpp>

void do_ban(Char *ch, ArgParser args) { ch->mud_.bans().ban_site(ch, args, false); }

void do_permban(Char *ch, ArgParser args) { ch->mud_.bans().ban_site(ch, args, true); }

void do_allow(Char *ch, ArgParser args) { ch->mud_.bans().allow_site(ch, args); }

bool Bans::check_ban(std::string_view site, const BanFlag ban_flag) const { return check_ban(site, to_int(ban_flag)); }

bool Bans::check_ban(std::string_view site, const int type_bits) const {
    const auto host = lower_case(site);
    for (auto &&ban : bans_) {
        if (!check_bit(ban->ban_flags_, type_bits))
            continue;

        if (check_enum_bit(ban->ban_flags_, BanFlag::Prefix) && check_enum_bit(ban->ban_flags_, BanFlag::Suffix)
            && matches_inside(ban->site_, host))
            return true;

        if (check_enum_bit(ban->ban_flags_, BanFlag::Prefix) && matches_end(ban->site_, host))
            return true;

        if (check_enum_bit(ban->ban_flags_, BanFlag::Suffix) && matches_start(ban->site_, host))
            return true;

        if (!check_enum_bit(ban->ban_flags_, BanFlag::Suffix) && !check_enum_bit(ban->ban_flags_, BanFlag::Prefix)
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
        set_enum_bit(ban_type_flag, BanFlag::Prefix);
    }
    if (site_arg[site_arg.size() - 1] == '*') {
        set_enum_bit(ban_type_flag, BanFlag::Suffix);
    }
    if (is_permanent) {
        set_enum_bit(ban_type_flag, BanFlag::Permanent);
    }
    const auto site = replace_strings(site_arg, "*", "");
    if (site.empty()) {
        ch->send_line("Ban which site?");
        return false;
    }

    const auto ban_type_arg = args.shift();
    /* find out what type of ban */
    if (ban_type_arg.empty() || matches(ban_type_arg, "all"))
        set_enum_bit(ban_type_flag, BanFlag::All);
    else if (matches(ban_type_arg, "newbies"))
        set_enum_bit(ban_type_flag, BanFlag::Newbies);
    else if (matches(ban_type_arg, "permit"))
        set_enum_bit(ban_type_flag, BanFlag::Permit);
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

size_t Bans::load(const Logger &logger) {
    FILE *fp;
    if ((fp = dependencies_->open_read()) == nullptr) {
        return 0;
    }
    for (;;) {
        auto c = getc(fp);
        if (c == EOF)
            break;
        else
            ungetc(c, fp);
        // This isn't resilient against malformed ban lines.
        const std::string name = fread_word(fp);
        const int level = fread_number(fp, logger);
        const int flags = fread_flag(fp, logger);
        bans_.emplace_back(std::make_unique<const Ban>(name, level, flags));
        fread_to_eol(fp);
    }
    dependencies_->close(fp);
    return bans_.size();
}

void Bans::save() {
    FILE *fp;
    bool found = false;
    if ((fp = dependencies_->open_write()) == nullptr) {
        perror("Error opening ban file for write");
    }
    for (auto &&ban : bans_) {
        if (check_enum_bit(ban->ban_flags_, BanFlag::Permanent)) {
            found = true;
            fmt::print(fp, "{} {} {}\n", ban->site_, ban->level_, serialize_flags(ban->ban_flags_));
        }
    }
    dependencies_->close(fp);
    if (!found)
        dependencies_->unlink();
}

void Bans::list(Char *ch) {
    if (bans_.empty()) {
        ch->send_line("No sites banned at this time.");
        return;
    }
    ch->send_line(" No.  Banned sites       Level  Type     Status");
    auto index = 0;
    for (auto &&ban : bans_) {
        const auto ban_flags_ = fmt::format("{}{}{}", check_enum_bit(ban->ban_flags_, BanFlag::Prefix) ? "*" : "",
                                            ban->site_, check_enum_bit(ban->ban_flags_, BanFlag::Suffix) ? "*" : "");
        const auto ban_line = fmt::format("{:>3})  {:<17}    {:<3}  {:<7}  {}", index++, ban_flags_, ban->level_,
                                          check_enum_bit(ban->ban_flags_, BanFlag::Newbies)  ? "newbies"
                                          : check_enum_bit(ban->ban_flags_, BanFlag::Permit) ? "permit"
                                          : check_enum_bit(ban->ban_flags_, BanFlag::All)    ? "all"
                                                                                             : "",
                                          check_enum_bit(ban->ban_flags_, BanFlag::Permanent) ? "perm" : "temp");
        ch->send_line(ban_line);
    }
}

class DependenciesImpl : public Bans::Dependencies {
public:
    DependenciesImpl(std::string ban_file);
    FILE *open_read();
    FILE *open_write();
    void close(FILE *fp);
    void unlink();

private:
    FILE *open_file(const char *mode);
    std::string ban_file_;
};

DependenciesImpl::DependenciesImpl(std::string ban_file) : Dependencies() { ban_file_ = ban_file; }

FILE *DependenciesImpl::open_read() { return open_file("r"); }

FILE *DependenciesImpl::open_write() { return open_file("w"); }

FILE *DependenciesImpl::open_file(const char *mode) { return fopen(ban_file_.c_str(), mode); }

void DependenciesImpl::close(FILE *fp) { fclose(fp); }

void DependenciesImpl::unlink() { ::unlink(ban_file_.c_str()); }

Bans::Bans(std::string ban_file) : dependencies_{std::make_unique<DependenciesImpl>(ban_file)} {}

Bans::Bans(std::unique_ptr<Dependencies> dependencies) : dependencies_{std::move(dependencies)} {}
