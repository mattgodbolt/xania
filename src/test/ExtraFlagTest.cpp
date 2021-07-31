/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2021 Xania Development Team                                 */
/*  See the header to file: merc.h for original code copyrights          */
/*************************************************************************/

#include "Char.hpp"
#include "ExtraFlags.hpp"

#include <catch2/catch.hpp>

TEST_CASE("format set extra flags") {
    Char ch{};
    Descriptor ch_desc(0);
    ch.desc = &ch_desc;
    ch_desc.character(&ch);
    ch.pcdata = std::make_unique<PcData>();
    SECTION("no flags set") {
        const auto result = format_set_extra_flags(&ch);

        CHECK(result == "");
    }
    SECTION("all flags set") {
        for (auto i = 0; i < 3; i++)
            ch.extra_flags[i] = ~(0ul);
        const auto result = format_set_extra_flags(&ch);

        CHECK(result
              == "wnet wn_debug wn_mort wn_imm wn_bug permit wn_tick info_name info_email info_mes info_url tip_std "
                 "tip_olc tip_adv");
    }
}
