#include "PcNutrition.hpp"
#include "Char.hpp"

#include <catch2/catch.hpp>

TEST_CASE("Default nutrition states") {
    SECTION("sober") {
        auto sober = PcNutrition::sober();

        CHECK(sober.get() == 0);
        CHECK(!sober.is_satisfied());
        CHECK(!sober.is_unhealthy());
        CHECK(sober.unhealthy_adjective() == "drunk");
    }
    SECTION("fed") {
        auto fed = PcNutrition::fed();

        CHECK(fed.get() == 40);
        CHECK(!fed.is_satisfied());
        CHECK(!fed.is_unhealthy());
        CHECK(fed.unhealthy_adjective() == "hungry");
    }
    SECTION("hydrated") {
        auto hydrated = PcNutrition::hydrated();

        CHECK(hydrated.get() == 40);
        CHECK(!hydrated.is_satisfied());
        CHECK(!hydrated.is_unhealthy());
        CHECK(hydrated.unhealthy_adjective() == "thirsty");
    }
}
TEST_CASE("nutrition setter") {
    SECTION("upper bound") {
        auto pcn = PcNutrition::hydrated();

        pcn.set(49);

        CHECK(pcn.get() == 48);
    }
    SECTION("lower bound") {
        auto pcn = PcNutrition::hydrated();

        pcn.set(-1);

        CHECK(pcn.get() == 0);
    }
}
TEST_CASE("Char nutrition") {
    Char ch{};
    ch.pcdata = std::make_unique<PcData>();
    SECTION("describe nutrition") {
        SECTION("default") {
            const auto message = ch.describe_nutrition();

            CHECK(!message);
        }
        SECTION("drunk") {
            ch.pcdata->inebriation.set(40);

            const auto message = ch.describe_nutrition();

            CHECK(*message == "|wYou are |Wdrunk|w|W|w|W|w.");
        }
        SECTION("hungry") {
            ch.pcdata->hunger.set(0);

            const auto message = ch.describe_nutrition();

            CHECK(*message == "|wYou are |W|w|Whungry|w|W|w.");
        }
        SECTION("thirsty") {
            ch.pcdata->thirst.set(0);

            const auto message = ch.describe_nutrition();

            CHECK(*message == "|wYou are |W|w|W|w|Wthirsty|w.");
        }
        SECTION("drunk and hungry") {
            ch.pcdata->inebriation.set(40);
            ch.pcdata->hunger.set(0);

            const auto message = ch.describe_nutrition();

            CHECK(*message == "|wYou are |Wdrunk|w and |Whungry|w|W|w.");
        }
        SECTION("drunk, hungry and thirsty") {
            ch.pcdata->inebriation.set(40);
            ch.pcdata->hunger.set(0);
            ch.pcdata->thirst.set(0);

            const auto message = ch.describe_nutrition();

            CHECK(*message == "|wYou are |Wdrunk|w, |Whungry|w and |Wthirsty|w.");
        }
    }
    SECTION("report nutrition") {
        SECTION("drunk") {
            ch.pcdata->inebriation.set(11);
            ch.pcdata->hunger.set(12);
            ch.pcdata->thirst.set(13);

            const auto message = ch.report_nutrition();

            CHECK(*message == "Thirst: 13  Hunger: 12  Inebriation: 11");
        }
    }
    SECTION("health checks") {
        SECTION("is not inebriated") {
            ch.pcdata->inebriation.set(10);

            CHECK(!ch.is_inebriated());
        }
        SECTION("is inebriated") {
            ch.pcdata->inebriation.set(11);

            CHECK(ch.is_inebriated());
        }
        SECTION("is not hungry") {
            ch.pcdata->hunger.set(41);

            CHECK(!ch.is_hungry());
        }
        SECTION("is hungry") {
            ch.pcdata->hunger.set(40);

            CHECK(ch.is_hungry());
            CHECK(!ch.is_starving());
        }
        SECTION("is not starving") {
            ch.pcdata->hunger.set(1);

            CHECK(!ch.is_starving());
        }
        SECTION("is starving") {
            ch.pcdata->hunger.set(0);

            CHECK(ch.is_starving());
        }
        SECTION("is not thirsty") {
            ch.pcdata->thirst.set(41);

            CHECK(!ch.is_thirsty());
        }
        SECTION("is thirsty") {
            ch.pcdata->thirst.set(40);

            CHECK(ch.is_thirsty());
        }
        SECTION("is not parched") {
            ch.pcdata->thirst.set(1);

            CHECK(!ch.is_parched());
        }
        SECTION("is parched") {
            ch.pcdata->thirst.set(0);

            CHECK(ch.is_parched());
        }
    }
    SECTION("delta nutrition") {
        SECTION("inebriation sober") {
            ch.pcdata->inebriation.set(1);

            const auto msg = ch.delta_inebriation(-1);
            CHECK(*msg == "You are sober.");
            CHECK(ch.pcdata->inebriation.get() == 0);
        }
        SECTION("inebriation no satisfaction message") {
            ch.pcdata->inebriation.set(9);

            const auto msg = ch.delta_inebriation(1);
            CHECK(!msg);
            CHECK(ch.pcdata->inebriation.get() == 10);
        }
        SECTION("inebriation satisfied") {
            ch.pcdata->inebriation.set(9);

            const auto msg = ch.delta_inebriation(2);
            CHECK(*msg == "You feel drunk.");
            CHECK(ch.pcdata->inebriation.get() == 11);
        }
        SECTION("hunger hungry") {
            ch.pcdata->hunger.set(1);

            const auto msg = ch.delta_hunger(-1);
            CHECK(*msg == "You are hungry.");
            CHECK(ch.pcdata->hunger.get() == 0);
        }
        SECTION("hunger no satisfaction message") {
            ch.pcdata->hunger.set(39);

            const auto msg = ch.delta_hunger(1);
            CHECK(!msg);
            CHECK(ch.pcdata->hunger.get() == 40);
        }
        SECTION("hunger satisfied") {
            ch.pcdata->hunger.set(39);

            const auto msg = ch.delta_hunger(2);
            CHECK(*msg == "You feel well nourished.");
            CHECK(ch.pcdata->hunger.get() == 41);
        }
        SECTION("thirst thirsty") {
            ch.pcdata->thirst.set(1);

            const auto msg = ch.delta_thirst(-1);
            CHECK(*msg == "You are thirsty.");
            CHECK(ch.pcdata->thirst.get() == 0);
        }
        SECTION("thirst no satisfaction message") {
            ch.pcdata->thirst.set(39);

            const auto msg = ch.delta_thirst(1);
            CHECK(!msg);
            CHECK(ch.pcdata->thirst.get() == 40);
        }
        SECTION("thirst satisfied") {
            ch.pcdata->thirst.set(39);

            const auto msg = ch.delta_thirst(2);
            CHECK(*msg == "Your thirst is quenched.");
            CHECK(ch.pcdata->thirst.get() == 41);
        }
    }
}