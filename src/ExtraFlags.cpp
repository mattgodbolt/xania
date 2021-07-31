/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "ExtraFlags.hpp"
#include "Char.hpp"
#include <array>
#include <fmt/format.h>
#include <range/v3/view/filter.hpp>
#include <range/v3/view/iota.hpp>
#include <range/v3/view/transform.hpp>
#include <string>

std::string format_set_extra_flags(const Char *ch) {
    /*
     * Extra flag names.
     * The info_ flags are unused, except info_mes.
     */
    const static std::array<std::string_view, MAX_EXTRA_FLAGS> flagname_extra = {{
        // clang-format off
        "wnet",      "wn_debug",  "wn_mort",   "wn_imm",  "wn_bug",
        "permit",    "wn_tick",   "",          "",        "info_name",
        "info_email","info_mes", "info_url",   "",        "tip_std",
        "tip_olc",   "tip_adv",   "",          "",        "",           
        "",          "",          "",          "",        "",
        "",          "",          "",          "",        "",
        "",          "",          "",          "",        "",
        "",          "",          "",          "",        "",
        "",          "",          "",          "",        "",
        "",          "",          "",          "",        "",
        "",          "",          "",          "",        "",
        "",          "",          "",          "",        "",
        "",          "",          "",          ""
        // clang-format on
    }};
    namespace rv = ranges::views;
    return fmt::format("{}", fmt::join(rv::iota(0u, flagname_extra.size()) | rv::filter([&ch](const auto i) {
                                           return ch->is_set_extra(i) && !flagname_extra[i].empty();
                                       }) | rv::transform([](const auto i) { return flagname_extra[i]; }),
                                       " "));
}