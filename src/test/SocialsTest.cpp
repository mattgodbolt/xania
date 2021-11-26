/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "Socials.hpp"
#include "Char.hpp"
#include "MemFile.hpp"

#include <catch2/catch.hpp>

namespace {
constexpr auto empty = R"(empty
$
$
$
$
$
$
$
$
#)";

constexpr auto malformed = R"(malformed
$
$
$
$
$
$
$
$
)";

constexpr auto hug = R"(hug
Hug who?
$n gives everyone a big hug!
You hug $M.
$n hugs $N.
$n hugs you.
Sorry, friend, I can't see that person here.
You hug yourself.
$n hugs $r in a vain attempt to get friendship.
#)";

constexpr auto hug_and_kiss = R"(hug
Hug who?
$n gives everyone a big hug!
You hug $M.
$n hugs $N.
$n hugs you.
Sorry, friend, I can't see that person here.
You hug yourself.
$n hugs $r in a vain attempt to get friendship.
#

bkiss
You blow kisses to the air.
$n blows kisses at no one in particular.
You blow a kiss at $N and wonder if $E will catch it.
$n blows a kiss at $N ... isn't $e so CUTE?
$n blows a kiss at you and hopes you'll blow one back at $m.
You kiss falls to the ground with no one to go to.
You blow a kiss to yourself... isn't the world beautiful?
$n blows a kiss to $r, obviously very in love.
#
#0
)";

}

TEST_CASE("social load") {
    SECTION("well formed social") {
        test::MemFile text(hug);
        const auto opt_social = Social::load(text.file());

        REQUIRE(opt_social);
        CHECK(opt_social->name() == "hug");
        CHECK(opt_social->char_no_arg() == "Hug who?");
        CHECK(opt_social->others_no_arg() == "$n gives everyone a big hug!");
        CHECK(opt_social->char_found() == "You hug $M.");
        CHECK(opt_social->others_found() == "$n hugs $N.");
        CHECK(opt_social->vict_found() == "$n hugs you.");
        CHECK(opt_social->char_auto() == "You hug yourself.");
        CHECK(opt_social->others_auto() == "$n hugs $r in a vain attempt to get friendship.");
    }
    SECTION("empty social") {
        test::MemFile text(empty);
        const auto opt_social = Social::load(text.file());

        REQUIRE(opt_social);
        CHECK(opt_social->name() == "empty");
        CHECK(opt_social->char_no_arg() == "");
        CHECK(opt_social->others_no_arg() == "");
        CHECK(opt_social->char_found() == "");
        CHECK(opt_social->others_found() == "");
        CHECK(opt_social->vict_found() == "");
        CHECK(opt_social->char_auto() == "");
        CHECK(opt_social->others_auto() == "");
    }
    SECTION("unterminated social") {
        test::MemFile text(malformed);
        const auto opt_social = Social::load(text.file());

        REQUIRE(!opt_social);
    }
    SECTION("end of socials") {
        test::MemFile text(R"(
#0
)");
        const auto opt_social = Social::load(text.file());

        REQUIRE(!opt_social);
    }
}

TEST_CASE("socials") {
    test::MemFile text(hug_and_kiss);
    Socials socials;
    socials.load(text.file());
    SECTION("loaded count") { CHECK(socials.count() == 2); }
    SECTION("find by full name") {
        const auto *social = socials.find("bkiss");

        REQUIRE(social);
        CHECK(social->char_no_arg() == "You blow kisses to the air.");
    }
    SECTION("find by prefix") {
        const auto *social = socials.find("bk");

        REQUIRE(social);
        CHECK(social->char_no_arg() == "You blow kisses to the air.");
    }
    SECTION("find by prefix upper case") {
        const auto *social = socials.find("BK");

        REQUIRE(social);
        CHECK(social->char_no_arg() == "You blow kisses to the air.");
    }
    SECTION("find unknown social") {
        const auto *social = socials.find("punch");

        REQUIRE(!social);
    }
    SECTION("find unknown social") {
        const auto *social = socials.find("punch");

        REQUIRE(!social);
    }
    SECTION("show table") {
        Char bob{};
        Descriptor bob_desc(1);
        bob.desc = &bob_desc;
        bob_desc.character(&bob);
        bob.pcdata = std::make_unique<PcData>();

        socials.show_table(&bob);

        CHECK(bob_desc.buffered_output() == "\n\rbkiss       hug\n\r");
    }
}
