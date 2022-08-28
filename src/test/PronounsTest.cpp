/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "Pronouns.hpp"
#include "ArgParser.hpp"
#include "Char.hpp"
#include "Sex.hpp"

#include <catch2/catch_test_macros.hpp>

namespace {
Pronouns exp_male{"his", "him", "he", "himself"};
Pronouns exp_female{"her", "her", "she", "herself"};
Pronouns exp_neutral{"their", "them", "they", "themself"};
Pronouns exp_uncustomized{"", "", "", ""};
Pronouns exp_customized{"zer", "hir", "ze", "hirself"};
};

TEST_CASE("pronouns for") {
    Char player{};
    player.pcdata = std::make_unique<PcData>();
    SECTION("male standard no custom") {
        player.sex = Sex::male();
        const auto custom = pronouns_for(player).first;
        auto expected_cust = std::tie(exp_uncustomized.possessive, exp_uncustomized.objective,
                                      exp_uncustomized.subjective, exp_uncustomized.reflexive);
        auto actual_cust = std::tie(custom.possessive, custom.objective, custom.subjective, custom.reflexive);
        REQUIRE(expected_cust == actual_cust);

        const auto standard = pronouns_for(player).second;
        auto expected_std = std::tie(exp_male.possessive, exp_male.objective, exp_male.subjective, exp_male.reflexive);
        auto actual_std = std::tie(standard.possessive, standard.objective, standard.subjective, standard.reflexive);
        REQUIRE(expected_std == actual_std);
    }
    SECTION("female standard") {
        player.sex = Sex::female();
        const auto standard = pronouns_for(player).second;
        auto expected_std =
            std::tie(exp_female.possessive, exp_female.objective, exp_female.subjective, exp_female.reflexive);
        auto actual_std = std::tie(standard.possessive, standard.objective, standard.subjective, standard.reflexive);
        REQUIRE(expected_std == actual_std);
    }
    SECTION("neutral standard") {
        player.sex = Sex::neutral();
        const auto standard = pronouns_for(player).second;
        auto expected_std =
            std::tie(exp_neutral.possessive, exp_neutral.objective, exp_neutral.subjective, exp_neutral.reflexive);
        auto actual_std = std::tie(standard.possessive, standard.objective, standard.subjective, standard.reflexive);
        REQUIRE(expected_std == actual_std);
    }
    SECTION("male custom") {
        player.sex = Sex::male();
        player.pcdata.get()->pronouns.possessive = "zer";
        player.pcdata.get()->pronouns.objective = "hir";
        player.pcdata.get()->pronouns.subjective = "ze";
        player.pcdata.get()->pronouns.reflexive = "hirself";
        const auto custom = pronouns_for(player).first;
        auto expected_cust = std::tie(exp_customized.possessive, exp_customized.objective, exp_customized.subjective,
                                      exp_customized.reflexive);
        auto actual_cust = std::tie(custom.possessive, custom.objective, custom.subjective, custom.reflexive);
        REQUIRE(expected_cust == actual_cust);
    }
}
TEST_CASE("pronoun shortcuts") {
    Char player{};
    player.pcdata = std::make_unique<PcData>();
    SECTION("no custom") {
        player.sex = Sex::female();
        SECTION("possessive") {
            const auto &result = possessive(player);
            REQUIRE("her" == result);
        }
        SECTION("objective") {
            const auto &result = objective(player);
            REQUIRE("her" == result);
        }
        SECTION("subjective") {
            const auto &result = subjective(player);
            REQUIRE("she" == result);
        }
        SECTION("reflexive") {
            const auto &result = reflexive(player);
            REQUIRE("herself" == result);
        }
    }
    SECTION("custom") {
        player.sex = Sex::male();
        player.pcdata.get()->pronouns.possessive = "zer";
        player.pcdata.get()->pronouns.objective = "hir";
        player.pcdata.get()->pronouns.subjective = "ze";
        player.pcdata.get()->pronouns.reflexive = "hirself";
        SECTION("possessive") {
            const auto &result = possessive(player);
            REQUIRE("zer" == result);
        }
        SECTION("objective") {
            const auto &result = objective(player);
            REQUIRE("hir" == result);
        }
        SECTION("subjective") {
            const auto &result = subjective(player);
            REQUIRE("ze" == result);
        }
        SECTION("reflexive") {
            const auto &result = reflexive(player);
            REQUIRE("hirself" == result);
        }
    }
}

TEST_CASE("parse pronouns") {
    SECTION("no args") {
        ArgParser args("");
        auto res = parse_pronouns(args);
        REQUIRE(res.state == PronounParseState::EmptyArgs);
    }
    SECTION("three args missing") {
        ArgParser args("a");
        auto res = parse_pronouns(args);
        REQUIRE(res.state == PronounParseState::MissingArgs);
        REQUIRE(res.parsed.possessive == "a");
    }
    SECTION("two args missing") {
        ArgParser args("a b");
        auto res = parse_pronouns(args);
        REQUIRE(res.state == PronounParseState::MissingArgs);
        REQUIRE(res.parsed.possessive == "a");
        REQUIRE(res.parsed.objective == "b");
    }
    SECTION("one arg missing") {
        ArgParser args("a b c");
        auto res = parse_pronouns(args);
        REQUIRE(res.state == PronounParseState::MissingArgs);
        REQUIRE(res.parsed.possessive == "a");
        REQUIRE(res.parsed.objective == "b");
        REQUIRE(res.parsed.subjective == "c");
    }
    SECTION("args ok") {
        ArgParser args("a b c d");
        auto res = parse_pronouns(args);
        REQUIRE(res.state == PronounParseState::Ok);
        REQUIRE(res.parsed.possessive == "a");
        REQUIRE(res.parsed.objective == "b");
        REQUIRE(res.parsed.subjective == "c");
        REQUIRE(res.parsed.reflexive == "d");
    }
    // because the pronouns are saved to the pfile and tildes are field terminators.
    SECTION("smash tildes") {
        ArgParser args("a~ b~ c~ d~");
        auto res = parse_pronouns(args);
        REQUIRE(res.state == PronounParseState::Ok);
        REQUIRE(res.parsed.possessive == "a-");
        REQUIRE(res.parsed.objective == "b-");
        REQUIRE(res.parsed.subjective == "c-");
        REQUIRE(res.parsed.reflexive == "d-");
    }
}
