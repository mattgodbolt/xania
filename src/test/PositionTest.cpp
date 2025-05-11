#include "Position.hpp"
#include "Char.hpp"
#include "DescriptorList.hpp"

#include "MemFile.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators_range.hpp>

#include "MockMud.hpp"

namespace {

test::MockMud mock_mud{};
DescriptorList descriptors{};
Logger logger{descriptors};

}

TEST_CASE("Operators") {
    SECTION("assign from enum and equals") {
        Position pos;

        pos = Position::Type::Mortal;

        CHECK(pos == Position::Type::Mortal);
        CHECK(pos != Position::Type::Stunned);
    }
    SECTION("less") {
        Position pos(Position::Type::Fighting);

        CHECK(pos < Position::Type::Standing);
    }
    SECTION("less or equal to") {
        Position pos(Position::Type::Fighting);

        CHECK(pos <= Position::Type::Fighting);
        CHECK(pos <= Position::Type::Standing);
    }
}
TEST_CASE("constructor") {
    SECTION("default") {
        Position pos;

        CHECK(pos == Position::Type::Standing);
    }
    SECTION("with type") {
        Position pos(Position::Type::Stunned);

        CHECK(pos == Position::Type::Stunned);
    }
}
TEST_CASE("accessors") {
    SECTION("dead") {
        Position dead(Position::Type::Dead);

        CHECK(dead.name() == "Dead");
        CHECK(dead.short_description() == "dead");
        CHECK(dead.bad_position_msg() == "Lie still; you are DEAD.");
        CHECK(dead.present_progressive_verb() == "|RDEAD|w!!");
        CHECK(dead.type() == Position::Type::Dead);
        CHECK(dead.integer() == 0);
    }
    SECTION("mort") {
        Position mort(Position::Type::Mortal);

        CHECK(mort.name() == "Mortal");
        CHECK(mort.short_description() == "mortally wounded");
        CHECK(mort.bad_position_msg() == "You are hurt far too bad for that.");
        CHECK(mort.present_progressive_verb() == "|Rmortally wounded|w.");
        CHECK(mort.type() == Position::Type::Mortal);
        CHECK(mort.integer() == 1);
    }
    SECTION("incap") {
        Position incap(Position::Type::Incap);

        CHECK(incap.name() == "Incap");
        CHECK(incap.short_description() == "incapacitated");
        CHECK(incap.bad_position_msg() == "You are hurt far too bad for that.");
        CHECK(incap.present_progressive_verb() == "|rincapacitated|w.");
        CHECK(incap.type() == Position::Type::Incap);
        CHECK(incap.integer() == 2);
    }
    SECTION("stunned") {
        Position stunned(Position::Type::Stunned);

        CHECK(stunned.name() == "Stunned");
        CHECK(stunned.short_description() == "stunned");
        CHECK(stunned.bad_position_msg() == "You are too stunned to do that.");
        CHECK(stunned.present_progressive_verb() == "|rlying here stunned|w.");
        CHECK(stunned.type() == Position::Type::Stunned);
        CHECK(stunned.integer() == 3);
    }
    SECTION("sleeping") {
        Position sleeping(Position::Type::Sleeping);

        CHECK(sleeping.name() == "Sleeping");
        CHECK(sleeping.short_description() == "sleeping");
        CHECK(sleeping.bad_position_msg() == "In your dreams, or what?");
        CHECK(sleeping.present_progressive_verb() == "sleeping here.");
        CHECK(sleeping.type() == Position::Type::Sleeping);
        CHECK(sleeping.integer() == 4);
    }
    SECTION("resting") {
        Position resting(Position::Type::Resting);

        CHECK(resting.name() == "Resting");
        CHECK(resting.short_description() == "resting");
        CHECK(resting.bad_position_msg() == "You're resting. Try standing up first!");
        CHECK(resting.present_progressive_verb() == "resting here.");
        CHECK(resting.type() == Position::Type::Resting);
        CHECK(resting.integer() == 5);
    }
    SECTION("sitting") {
        Position sitting(Position::Type::Sitting);

        CHECK(sitting.name() == "Sitting");
        CHECK(sitting.short_description() == "sitting");
        CHECK(sitting.bad_position_msg() == "Better stand up first.");
        CHECK(sitting.present_progressive_verb() == "sitting here.");
        CHECK(sitting.type() == Position::Type::Sitting);
        CHECK(sitting.integer() == 6);
    }
    SECTION("fighting") {
        Position fighting(Position::Type::Fighting);

        CHECK(fighting.name() == "Fighting");
        CHECK(fighting.short_description() == "fighting");
        CHECK(fighting.bad_position_msg() == "No way!  You are still fighting!");
        CHECK(fighting.present_progressive_verb() == "here, fighting");
        CHECK(fighting.type() == Position::Type::Fighting);
        CHECK(fighting.integer() == 7);
    }
    SECTION("standing") {
        Position standing(Position::Type::Standing);

        CHECK(standing.name() == "Standing");
        CHECK(standing.short_description() == "standing");
        CHECK(standing.bad_position_msg() == "You're standing.");
        CHECK(standing.present_progressive_verb() == "here");
        CHECK(standing.type() == Position::Type::Standing);
        CHECK(standing.integer() == 8);
    }
}
TEST_CASE("factories") {
    ALLOW_CALL(mock_mud, descriptors()).LR_RETURN(descriptors);
    SECTION("try from name substring") {
        const auto opt_pos = Position::try_from_name("sleep");

        CHECK(opt_pos);
        CHECK(*opt_pos == Position::Type::Sleeping);
    }
    SECTION("try from name full string") {
        const auto opt_pos = Position::try_from_name("sleeping");

        CHECK(opt_pos);
        CHECK(*opt_pos == Position::Type::Sleeping);
    }
    SECTION("try from name case insensitive") {
        const auto opt_pos = Position::try_from_name("SLEEPING");

        CHECK(opt_pos);
        CHECK(*opt_pos == Position::Type::Sleeping);
    }
    SECTION("try from name not found") {
        const auto opt_pos = Position::try_from_name("slleeping");

        CHECK(!opt_pos);
    }
    SECTION("from word ok") {
        test::MemFile file("sleeping ");

        const auto pos = Position::read_from_word(file.file(), logger);

        CHECK(pos == Position::Type::Sleeping);
    }
    SECTION("from word failure default") {
        test::MemFile file("slleeping ");

        const auto pos = Position::read_from_word(file.file(), logger);

        CHECK(pos == Position::Type::Standing);
    }
    SECTION("from number ok") {
        test::MemFile file("7 ");

        const auto pos = Position::read_from_number(file.file(), logger);

        CHECK(pos == Position::Type::Fighting);
    }
    SECTION("from number failure default") {
        test::MemFile file("9 ");

        const auto pos = Position::read_from_number(file.file(), logger);

        CHECK(pos == Position::Type::Standing);
    }
}
TEST_CASE("Char position accessors") {
    Char ch{mock_mud};
    SECTION("is pos dead") {
        ch.position = Position(Position::Type::Dead);

        CHECK(ch.is_pos_dead());
    }
    SECTION("is pos dying") {
        auto valid_pos = GENERATE(Position::Type::Dead, Position::Type::Mortal, Position::Type::Incap);
        SECTION("true") {
            ch.position = Position(valid_pos);

            CHECK(ch.is_pos_dying());
        }
        auto invalid_pos = GENERATE(Position::Type::Stunned, Position::Type::Sleeping);
        SECTION("false") {
            ch.position = Position(invalid_pos);

            CHECK(!ch.is_pos_dying());
        }
    }
    SECTION("is pos stunned or dying") {
        auto valid_pos =
            GENERATE(Position::Type::Dead, Position::Type::Mortal, Position::Type::Incap, Position::Type::Stunned);
        SECTION("true") {
            ch.position = Position(valid_pos);

            CHECK(ch.is_pos_stunned_or_dying());
        }
        SECTION("false") {
            ch.position = Position(Position::Type::Sleeping);

            CHECK(!ch.is_pos_stunned_or_dying());
        }
    }
    SECTION("is pos relaxing") {
        auto valid_pos = GENERATE(Position::Type::Sleeping, Position::Type::Resting, Position::Type::Sitting);
        SECTION("true") {
            ch.position = Position(valid_pos);

            CHECK(ch.is_pos_relaxing());
        }
        auto invalid_pos = GENERATE(Position::Type::Stunned, Position::Type::Fighting, Position::Type::Standing);
        SECTION("false") {
            ch.position = Position(invalid_pos);

            CHECK(!ch.is_pos_relaxing());
        }
    }
    SECTION("is pos awake") {
        auto valid_pos = GENERATE(Position::Type::Resting, Position::Type::Sitting, Position::Type::Fighting,
                                  Position::Type::Standing);
        SECTION("true") {
            ch.position = Position(valid_pos);

            CHECK(ch.is_pos_awake());
        }
        auto invalid_pos = GENERATE(Position::Type::Sleeping);
        SECTION("false") {
            ch.position = Position(invalid_pos);

            CHECK(!ch.is_pos_awake());
        }
    }
    SECTION("is pos fighting") {
        auto valid_pos = GENERATE(Position::Type::Fighting);
        SECTION("true") {
            ch.position = Position(valid_pos);

            CHECK(ch.is_pos_fighting());
        }
        auto invalid_pos = GENERATE(Position::Type::Sitting, Position::Type::Standing);
        SECTION("false") {
            ch.position = Position(invalid_pos);

            CHECK(!ch.is_pos_fighting());
        }
    }
    SECTION("is pos preoccupied") {
        auto valid_pos =
            GENERATE(Position::Type::Dead, Position::Type::Mortal, Position::Type::Incap, Position::Type::Sleeping,
                     Position::Type::Resting, Position::Type::Sitting, Position::Type::Fighting);
        SECTION("true") {
            ch.position = Position(valid_pos);

            CHECK(ch.is_pos_preoccupied());
        }
        auto invalid_pos = GENERATE(Position::Type::Standing);
        SECTION("false") {
            ch.position = Position(invalid_pos);

            CHECK(!ch.is_pos_preoccupied());
        }
    }
    SECTION("is pos standing") {
        SECTION("true") {
            ch.position = Position(Position::Type::Standing);

            CHECK(ch.is_pos_standing());
        }
        auto invalid_pos = GENERATE(Position::Type::Resting, Position::Type::Fighting);
        SECTION("false") {
            ch.position = Position(invalid_pos);

            CHECK(!ch.is_pos_standing());
        }
    }
}
