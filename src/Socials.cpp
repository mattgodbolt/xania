/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "Socials.hpp"
#include "Columner.hpp"
#include "Logging.hpp"
#include "db.h"
#include "string_utils.hpp"
#include <range/v3/algorithm/find_if.hpp>
#include <range/v3/algorithm/for_each.hpp>
#include <range/v3/view/transform.hpp>

std::optional<Social> Social::load(FILE *fp) {
    const auto next_phrase = [](auto *fp) -> std::string {
        const auto phrase = fread_string_eol(fp);
        // The phrase can be empty, indicated by $ in which case the social is empty and won't be used.
        if (matches(phrase, "$")) {
            return "";
        } else {
            return phrase;
        }
    };
    const auto name = fread_word(fp);
    if (matches(name, "#0"))
        return std::nullopt;
    const auto char_no_arg{next_phrase(fp)};
    const auto others_no_arg{next_phrase(fp)};
    const auto char_found{next_phrase(fp)};
    const auto others_found{next_phrase(fp)};
    const auto vict_found{next_phrase(fp)};
    // The char_not_found phrase is not currently used so skip it.
    next_phrase(fp);
    const auto char_auto{next_phrase(fp)};
    const auto others_auto{next_phrase(fp)};
    const auto terminator{next_phrase(fp)};
    if (terminator != "#") {
        bug("loading socials: expected social terminator (#) but instead got {}", terminator);
        return std::nullopt;
    }
    return Social{name, char_no_arg, others_no_arg, char_found, others_found, vict_found, char_auto, others_auto};
}

const Social *Socials::find(std::string_view name) const noexcept {
    const auto name_matches = [&name](const auto &social) { return matches_start(name, social.name_); };
    const auto it = ranges::find_if(socials_, name_matches);
    if (it != socials_.end()) {
        return &*it;
    } else {
        return nullptr;
    }
}

/* snarf a socials file */
void Socials::load(FILE *fp) {
    for (;;) {
        if (const auto opt_social = Social::load(fp)) {
            socials_.push_back(std::move(*opt_social));
        } else {
            return;
        }
    }
}

size_t Socials::count() const noexcept { return socials_.size(); }

const Social &Socials::random() const noexcept { return socials_.at(number_range(0, (socials_.size() - 1))); }

void Socials::show_table(Char *ch) const noexcept {
    Columner col(*ch, 6, 12);
    ranges::for_each(socials_ | ranges::view::transform(&Social::name_), [&col](const auto &name) { col.add(name); });
}

Socials &Socials::singleton() {
    static Socials singleton;
    return singleton;
}

void do_socials(Char *ch) { Socials::singleton().show_table(ch); }
