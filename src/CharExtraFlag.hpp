/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

#include <magic_enum/magic_enum.hpp>

// The Char Extra flags feature was originally used to store binary state of
// various character settings. It reserved 64 bits and encoded their
// on/off state with a 1 and 0 in the Char's pfile. Most of the bits have never
// actually been used for any features, although the pfile save/load code always
// processes the full 64 bits. Be aware there *could* be legacy player files where
// these bits are enabled so consider that if you decide to repurpose them. And
// write some migration code, probably within `pfu`.
enum class CharExtraFlag {
    WiznetOn,
    WiznetDebug,
    WiznetMort,
    WiznetImm,
    WiznetBug,
    Permit,
    WiznetTick,
    Unused7,
    Unused8,
    Unused9,
    Unused10,
    InfoMessage,
    Unused12,
    Unused13,
    TipWizard,
    Unused15,
    TipAdvanced,
    Unused17,
    Unused18,
    Unused19,
    Unused20,
    Unused21,
    Unused22,
    Unused23,
    Unused24,
    Unused25,
    Unused26,
    Unused27,
    Unused28,
    Unused29,
    Unused30,
    Unused31,
    Unused32,
    Unused33,
    Unused34,
    Unused35,
    Unused36,
    Unused37,
    Unused38,
    Unused39,
    Unused40,
    Unused41,
    Unused42,
    Unused43,
    Unused44,
    Unused45,
    Unused46,
    Unused47,
    Unused48,
    Unused49,
    Unused50,
    Unused51,
    Unused52,
    Unused53,
    Unused54,
    Unused55,
    Unused56,
    Unused57,
    Unused58,
    Unused59,
    Unused60,
    Unused61,
    Unused62,
    Unused63
};

[[nodiscard]] constexpr auto to_int(const CharExtraFlag flag) noexcept {
    return magic_enum::enum_integer<CharExtraFlag>(flag);
}

namespace CharExtraFlags {

constexpr std::array<std::string_view, magic_enum::enum_count<CharExtraFlag>()> AllExtraFlags = {{
    // clang-format off
        "wnet",      "wn_debug",  "wn_mort",   "wn_imm",  "wn_bug",
        "permit",    "wn_tick",   "",          "",        "",
        "",          "info_mes",  "",          "",        "tip_wiz",
        "",          "tip_adv",   "",          "",        "",
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

}