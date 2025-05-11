#include "DamageMessages.hpp"
#include "Char.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators_range.hpp>
#include <tuple>

#include "MockMud.hpp"

namespace {

test::MockMud mock_mud{};

}

TEST_CASE("impact verbs") {
    SECTION("expected impact verbs") {
        using std::make_tuple;
        int proportion;
        std::string_view singular, plural;
        std::tie(proportion, singular, plural) = GENERATE(table<int, std::string_view, std::string_view>({
            // clang-format off
            make_tuple(0, "miss", "misses"),
            make_tuple(1, "scrape", "scrapes"),
            make_tuple(2, "scratch", "scratches"),
            make_tuple(3, "bruise", "bruises"),
            make_tuple(4, "graze", "grazes"),
            make_tuple(5, "hit", "hits"),
            make_tuple(6, "cut", "cuts"),
            make_tuple(7, "injure", "injures"),
            make_tuple(8, "wound", "wounds"),
            make_tuple(10, "split", "splits"),
            make_tuple(12, "gash", "gashes"),
            make_tuple(14, "shred", "shreds"),
            make_tuple(16, "maul", "mauls"),
            make_tuple(18, "decimate", "decimates"),
            make_tuple(22, "eviscerate", "eviscerates"),
            make_tuple(26, "devastate", "devastates"),
            make_tuple(30, "maim", "maims"),
            make_tuple(35, "MUTILATE", "MUTILATES"),
            make_tuple(40, "DISEMBOWEL", "DISEMBOWELS"),
            make_tuple(45, "DISMEMBER", "DISMEMBERS"),
            make_tuple(50, "MASSACRE", "MASSACRES"),
            make_tuple(56, "MANGLE", "MANGLES"),
            make_tuple(62, "DEMOLISH", "DEMOLISHES"),
            make_tuple(70, "DEVASTATE", "DEVASTATES"),
            make_tuple(80, "OBLITERATE", "OBLITERATES"),
            make_tuple(90, "ANNIHILATE", "ANNIHILATES"),
            make_tuple(120, "ERADICATE", "ERADICATES"),
            make_tuple(121, "does |RUNSPEAKABLE|w things to", "does |RUNSPEAKABLE|w things to") // clang-format on
        }));
        SECTION("created using basic damage type") {
            const auto verb = ImpactVerbs::create(true, proportion, DamageType::Bash);

            CHECK(verb.singular() == singular);
            CHECK(verb.plural() == plural);
        }
    }
    SECTION("pluralise the y") {
        SECTION("holy pacifies") {
            const auto verb = ImpactVerbs::create(true, 16, DamageType::Holy);

            CHECK(verb.singular() == "pacify");
            CHECK(verb.plural() == "pacifies");
        }
        SECTION("holy crucifies") {
            const auto verb = ImpactVerbs::create(true, 45, DamageType::Holy);

            CHECK(verb.singular() == "CRUCIFY");
            CHECK(verb.plural() == "CRUCIFIES");
        }
    }
    SECTION("max damage without an attack verb") {
        const auto verb = ImpactVerbs::create(false, 121, DamageType::Bash);

        CHECK(verb.singular() == "do |RUNSPEAKABLE|w things to");
        CHECK(verb.plural() == "does |RUNSPEAKABLE|w things to");
    }
}
TEST_CASE("damage messages") {
    Char ch{mock_mud};
    ch.level = 2;
    Char victim{mock_mud};
    victim.level = 2;
    victim.max_hit = 100; // So that the damage amounts are also their proportion.
    AttackType atk_type;
    InjuredPart injured_part{"left calf", std::nullopt, std::nullopt};
    KnuthRng rng(0xdeadbeef);

    using std::make_tuple;
    int damage;
    std::string_view to_char, to_victim, to_room;
    SECTION("no attack verb") {
        atk_type = Attacks::at(0);
        std::tie(damage, to_char, to_victim, to_room) =
            GENERATE(table<int, std::string_view, std::string_view, std::string_view>(
                {// clang-format off
            make_tuple(0, "You |wmiss|w $N's left calf.|w", "$n |wmisses|w your left calf.|w", "$n |wmisses|w $N's left calf.|w"),
            make_tuple(1, "You |wscrape|w $N's left calf.|w", "$n |wscrapes|w your left calf.|w", "$n |wscrapes|w $N's left calf.|w"),
            make_tuple(2, "You |wscratch|w $N's left calf.|w", "$n |wscratches|w your left calf.|w", "$n |wscratches|w $N's left calf.|w"),
            make_tuple(3, "You |wbruise|w $N's left calf.|w", "$n |wbruises|w your left calf.|w", "$n |wbruises|w $N's left calf.|w"),
            make_tuple(4, "You |wgraze|w $N's left calf.|w", "$n |wgrazes|w your left calf.|w", "$n |wgrazes|w $N's left calf.|w"),
            make_tuple(5, "You |whit|w $N's left calf.|w", "$n |whits|w your left calf.|w", "$n |whits|w $N's left calf.|w"),
            make_tuple(6, "You |wcut|w $N's left calf.|w", "$n |wcuts|w your left calf.|w", "$n |wcuts|w $N's left calf.|w"),
            make_tuple(7, "You |winjure|w $N's left calf.|w", "$n |winjures|w your left calf.|w", "$n |winjures|w $N's left calf.|w"),
            make_tuple(8, "You |wwound|w $N's left calf.|w", "$n |wwounds|w your left calf.|w", "$n |wwounds|w $N's left calf.|w"),
            make_tuple(10, "You |wsplit|w $N's left calf.|w", "$n |wsplits|w your left calf.|w", "$n |wsplits|w $N's left calf.|w"),
            make_tuple(12, "You |wgash|w $N's left calf.|w", "$n |wgashes|w your left calf.|w", "$n |wgashes|w $N's left calf.|w"),
            make_tuple(14, "You |wshred|w $N's left calf.|w", "$n |wshreds|w your left calf.|w", "$n |wshreds|w $N's left calf.|w"),
            make_tuple(16, "You |Gmaul|w $N's left calf.|w", "$n |Gmauls|w your left calf.|w", "$n |Gmauls|w $N's left calf.|w"),
            make_tuple(18, "You |Gdecimate|w $N's left calf.|w", "$n |Gdecimates|w your left calf.|w", "$n |Gdecimates|w $N's left calf.|w"),
            make_tuple(22, "You |Geviscerate|w $N's left calf!|w", "$n |Geviscerates|w your left calf!|w", "$n |Geviscerates|w $N's left calf!|w"),
            make_tuple(26, "You |Gdevastate|w $N's left calf!|w", "$n |Gdevastates|w your left calf!|w", "$n |Gdevastates|w $N's left calf!|w"),
            make_tuple(30, "You |Gmaim|w $N's left calf!|w", "$n |Gmaims|w your left calf!|w", "$n |Gmaims|w $N's left calf!|w"),
            make_tuple(35, "You |YMUTILATE|w $N's left calf!|w", "$n |YMUTILATES|w your left calf!|w", "$n |YMUTILATES|w $N's left calf!|w"),
            make_tuple(62, "You |W***  DEMOLISH  ***|w $N's left calf!|w", "$n |W***  DEMOLISHES  ***|w your left calf!|w", "$n |W***  DEMOLISHES  ***|w $N's left calf!|w"),
            make_tuple(81, "You |R<<<  ANNIHILATE  >>>|w $N's left calf!|w", "$n |R<<<  ANNIHILATES  >>>|w your left calf!|w", "$n |R<<<  ANNIHILATES  >>>|w $N's left calf!|w"),
            make_tuple(121, "You |wdo |RUNSPEAKABLE|w things to|w $N's left calf!|w", "$n |wdoes |RUNSPEAKABLE|w things to|w your left calf!|w", "$n |wdoes |RUNSPEAKABLE|w things to|w $N's left calf!|w")
        })); // clang-format on

        SECTION("to char, victim and room") {
            DamageContext context{damage, atk_type, DamageType::Bash, false, injured_part};
            const auto dmg_messages = DamageMessages::create(&ch, &victim, context, rng);

            CHECK(dmg_messages.to_char() == to_char);
            CHECK(dmg_messages.to_victim() == to_victim);
            CHECK(dmg_messages.to_room() == to_room);
        }
    }
    SECTION("attacking self and no attack verb") {
        atk_type = Attacks::at(0);
        std::tie(damage, to_char, to_room) = GENERATE(table<int, std::string_view, std::string_view>(
            {// clang-format off
            make_tuple(0, "You |wmiss|w your own left calf.|w", "$n |wmisses|w $s left calf.|w"),
            make_tuple(1, "You |wscrape|w your own left calf.|w", "$n |wscrapes|w $s left calf.|w")
        })); // clang-format on

        SECTION("to char and room") {
            DamageContext context{damage, atk_type, DamageType::Bash, false, injured_part};
            const auto dmg_messages = DamageMessages::create(&victim, &victim, context, rng);

            CHECK(dmg_messages.to_char() == to_char);
            CHECK(dmg_messages.to_room() == to_room);
        }
    }
    SECTION("with slice verb") {
        atk_type = Attacks::at(1); // the slice attack type
        std::tie(damage, to_char, to_victim, to_room) =
            GENERATE(table<int, std::string_view, std::string_view, std::string_view>(
                {// clang-format off
            make_tuple(0, "Your slice |wmisses|w $N's left calf.|w", "$n's slice |wmisses|w your left calf.|w", "$n's slice |wmisses|w $N's left calf.|w"),
            make_tuple(1, "Your slice |wscrapes|w $N's left calf.|w", "$n's slice |wscrapes|w your left calf.|w", "$n's slice |wscrapes|w $N's left calf.|w"),
            make_tuple(121, "Your slice |wdoes |RUNSPEAKABLE|w things to|w $N's left calf!|w", "$n's slice |wdoes |RUNSPEAKABLE|w things to|w your left calf!|w", "$n's slice |wdoes |RUNSPEAKABLE|w things to|w $N's left calf!|w")
        })); // clang-format on

        SECTION("to char, victim and room") {
            DamageContext context{damage, atk_type, DamageType::Slash, false, injured_part};
            const auto dmg_messages = DamageMessages::create(&ch, &victim, context, rng);

            CHECK(dmg_messages.to_char() == to_char);
            CHECK(dmg_messages.to_victim() == to_victim);
            CHECK(dmg_messages.to_room() == to_room);
        }
    }
    SECTION("attacking self with slice verb") {
        atk_type = Attacks::at(1);
        std::tie(damage, to_char, to_room) = GENERATE(table<int, std::string_view, std::string_view>(
            {// clang-format off
            make_tuple(0, "Your slice |wmisses|w you.|w", "$n's slice |wmisses|w $m.|w"),
            make_tuple(1, "Your slice |wscrapes|w you.|w", "$n's slice |wscrapes|w $m.|w"),
            make_tuple(121, "Your slice |wdoes |RUNSPEAKABLE|w things to|w you!|w", "$n's slice |wdoes |RUNSPEAKABLE|w things to|w $m!|w")
        })); // clang-format on

        SECTION("to char and room") {
            DamageContext context{damage, atk_type, DamageType::Slash, false, injured_part};
            const auto dmg_messages = DamageMessages::create(&victim, &victim, context, rng);

            CHECK(dmg_messages.to_char() == to_char);
            CHECK(dmg_messages.to_room() == to_room);
        }
    }
    SECTION("victim is immune") {
        atk_type = Attacks::at(1);
        std::tie(damage, to_char, to_victim, to_room) =
            GENERATE(table<int, std::string_view, std::string_view, std::string_view>(
                {// clang-format off
            make_tuple(0, "$N is |Wunaffected|w by your slice!|w", "$n's slice is powerless against you.|w", "$N is |Wunaffected|w by $n's slice.|w")
        })); // clang-format on

        SECTION("to char, victim and room") {
            DamageContext context{damage, atk_type, DamageType::Slash, true, injured_part};
            const auto dmg_messages = DamageMessages::create(&ch, &victim, context, rng);

            CHECK(dmg_messages.to_char() == to_char);
            CHECK(dmg_messages.to_victim() == to_victim);
            CHECK(dmg_messages.to_room() == to_room);
        }
    }
    SECTION("attacking self and immune") {
        atk_type = Attacks::at(1);
        std::tie(damage, to_char, to_room) = GENERATE(table<int, std::string_view, std::string_view>(
            {// clang-format off
            make_tuple(0, "Luckily, you are immune to that.|w", "$n is |Wunaffected|w by $s own slice.|w")
        })); // clang-format on

        SECTION("to char and room") {
            DamageContext context{damage, atk_type, DamageType::Slash, true, injured_part};
            const auto dmg_messages = DamageMessages::create(&victim, &victim, context, rng);

            CHECK(dmg_messages.to_char() == to_char);
            CHECK(dmg_messages.to_room() == to_room);
        }
    }
}

// Extends the tests above to include the damage amount which appears for players level 20+
TEST_CASE("damage messages with damage amount") {
    Char ch{mock_mud};
    ch.level = 20;
    Char victim{mock_mud};
    victim.level = 20;
    victim.max_hit = 100;
    AttackType atk_type;
    InjuredPart injured_part{"left calf", std::nullopt, std::nullopt};
    KnuthRng rng(0xdeadbeef);

    using std::make_tuple;
    int damage;
    std::string_view to_char, to_victim, to_room;

    SECTION("with slice verb") {
        atk_type = Attacks::at(1); // the slice attack type
        std::tie(damage, to_char, to_victim, to_room) =
            GENERATE(table<int, std::string_view, std::string_view, std::string_view>(
                {// clang-format off
            make_tuple(25, "Your slice |Gmaims|w $N's left calf!|w (25)", "$n's slice |Gmaims|w your left calf!|w (25)", "$n's slice |Gmaims|w $N's left calf!|w")
        })); // clang-format on

        SECTION("to char, victim and room") {
            DamageContext context{damage, atk_type, DamageType::Slash, false, injured_part};
            const auto dmg_messages = DamageMessages::create(&ch, &victim, context, rng);

            CHECK(dmg_messages.to_char() == to_char);
            CHECK(dmg_messages.to_victim() == to_victim);
            CHECK(dmg_messages.to_room() == to_room);
        }
    }
}
