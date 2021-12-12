/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "Wear.hpp"

#include <catch2/catch.hpp>
#include <fmt/format.h>
#include <ranges>

TEST_CASE("wear location") {
    SECTION("wearable count") {
        const auto count = WearFilter::wearable_count();

        CHECK(count == 19);
    }
    SECTION("wearable") {
        auto wear_names = fmt::format("{}", fmt::join(WearFilter::wearable() | std::views::transform([](const auto i) {
                                                          return magic_enum::enum_name<Wear>(i);
                                                      }),
                                                      ","));
        CHECK(wear_names
              == "Light,FingerL,FingerR,Neck1,Neck2,Body,Head,Legs,Feet,Hands,Arms,Shield,About,Waist,WristL,WristR,"
                 "Wield,Hold,Ears");
    }
}