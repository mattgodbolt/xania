/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "Worn.hpp"

#include <catch2/catch_test_macros.hpp>
#include <fmt/format.h>
#include <range/v3/view/transform.hpp>

TEST_CASE("wear location") {
    SECTION("wearable count") {
        const auto count = WearbleFilter::wearable_count();

        CHECK(count == 19);
    }
    SECTION("wearable") {
        auto wear_names =
            fmt::format("{}", fmt::join(WearbleFilter::wearable() | ranges::views::transform([](const auto i) {
                                            return magic_enum::enum_name<Worn>(i);
                                        }),
                                        ","));
        CHECK(wear_names
              == "Light,FingerL,FingerR,Neck1,Neck2,Body,Head,Legs,Feet,Hands,Arms,Shield,About,Waist,WristL,WristR,"
                 "Wield,Hold,Ears");
    }
}
