#include "Sex.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Sex types") {
    auto default_sex = Sex();
    auto neutral = Sex(Sex::Type::neutral);
    auto male = Sex(Sex::Type::male);
    auto female = Sex(Sex::Type::female);
    SECTION("neutral type is default") { CHECK(default_sex.type() == Sex::Type::neutral); }
    SECTION("neutral type") { CHECK(neutral.type() == Sex::Type::neutral); }
    SECTION("male type") { CHECK(male.type() == Sex::Type::male); }
    SECTION("female type") { CHECK(female.type() == Sex::Type::female); }
    SECTION("is neutral") { CHECK(neutral.is_neutral()); }
    SECTION("is male") { CHECK(male.is_male()); }
    SECTION("is female") { CHECK(female.is_female()); }
    SECTION("neutral name") { CHECK(neutral.name() == "neutral"); }
    SECTION("male name") { CHECK(male.name() == "male"); }
    SECTION("female name") { CHECK(female.name() == "female"); }
    SECTION("neutral int") { CHECK(neutral.integer() == 0); }
    SECTION("male int") { CHECK(male.integer() == 1); }
    SECTION("female int") { CHECK(female.integer() == 2); }
    SECTION("equals") { CHECK(default_sex == neutral); }
    SECTION("not equals") { CHECK(male != neutral); }
    // Some of these may look a bit weird, but AFFECT_DATA::modify() accepts signed ints,
    // and whether the argument is positive or negative, the Sex attribute must
    // wrap around. This is to verify the modulo arithmetic works.
    SECTION("modulo arithmetic") {
        SECTION("plus equals with wrap around") {
            CHECK((neutral += 1) == male);
            CHECK((neutral += 1) == female);
            CHECK((neutral += 1) == neutral);
            CHECK((neutral += 1) == male);
            CHECK((neutral += 1) == female);
            CHECK((neutral += 1) == neutral);
        }
        SECTION("plus equals 2") { CHECK((neutral += 2) == female); }
        SECTION("plus equals 3") { CHECK((neutral += 3) == neutral); }
        SECTION("neutral plus 1") { CHECK((neutral + 1) == male); }
        SECTION("neutral plus 2") { CHECK((neutral + 2) == female); }
        SECTION("neutral plus 3") { CHECK((neutral + 2) == neutral); }
        SECTION("male plus 2") { CHECK((male + 2) == neutral); }
        SECTION("neutral plus -1") {
            const auto mod = -1;
            CHECK((neutral + mod) == female);
        }
        SECTION("neutral plus -2") {
            const auto mod = -2;
            CHECK((neutral + mod) == male);
        }
        SECTION("neutral plus -3") {
            const auto mod = -3;
            CHECK((neutral + mod) == neutral);
        }
    }
}
TEST_CASE("Sex statics") {
    SECTION("factories") {
        SECTION("neutral") {
            auto sex = Sex::neutral();
            CHECK(sex.is_neutral());
        }
        SECTION("male") {
            auto sex = Sex::male();
            CHECK(sex.is_male());
        }
        SECTION("female") {
            auto sex = Sex::female();
            CHECK(sex.is_female());
        }
    }
    SECTION("try from name") {
        SECTION("valid") {
            auto sex = Sex::try_from_name("male");
            CHECK(sex->is_male());
        }
        SECTION("invalid") {
            auto sex = Sex::try_from_name("emale");
            CHECK(!sex);
        }
    }
    SECTION("try from integer") {
        SECTION("neutral") {
            auto sex = Sex::try_from_integer(0);
            CHECK(sex->is_neutral());
        }
        SECTION("male") {
            auto sex = Sex::try_from_integer(1);
            CHECK(sex->is_male());
        }
        SECTION("female") {
            auto sex = Sex::try_from_integer(2);
            CHECK(sex->is_female());
        }
        SECTION("invalid") {
            auto sex = Sex::try_from_integer(3);
            CHECK(!sex);
        }
    }
    SECTION("try from char") {
        SECTION("neutral") {
            auto sex = Sex::try_from_char('n');
            CHECK(sex->is_neutral());
        }
        SECTION("male") {
            auto sex = Sex::try_from_char('m');
            CHECK(sex->is_male());
        }
        SECTION("female") {
            auto sex = Sex::try_from_char('f');
            CHECK(sex->is_female());
        }
        SECTION("null invalid") {
            auto sex = Sex::try_from_char('\0');
            CHECK(!sex);
        }
        SECTION("char invalid") {
            auto sex = Sex::try_from_char('e');
            CHECK(!sex);
        }
    }
    SECTION("names csv") {
        auto names_csv = Sex::names_csv();
        CHECK(names_csv == "neutral, male, female");
    }
}
