#include "MobIndexData.hpp"

#include "CatchFormatters.hpp"
#include "merc.h"
#include "string_utils.hpp"

#include <catch2/catch.hpp>

#include "MemFile.hpp"

using namespace std::literals;

TEST_CASE("loading mobs") {
    SECTION("should notice the end of mobs") {
        test::MemFile empty(R"(#0
)");
        CHECK(MobIndexData::from_file(empty.file()) == std::nullopt);
    }
    SECTION("should load an example mob") {
        test::MemFile orc(R"mob(
#13000
evil orc shaman~
the orc shaman~
An orc magic-user stands here, practising dark arts.
~
This orc is more intelligent than most - he has grasped some of the more crude
magics, but the dark froth around the corners of his mouth lay testimony to his
insanity.  He wears a crude tatter of a robe, inscribed with unholy sigils of
evil.  He perhaps was a student of the evil orc mage Uruk.
~
orc~
FGHR DEH -950 0
42 22 10d6+1770 10d6+460 10d4+19 blast
-2 -2 -2 -2
FJNQ ABQ CL N
stand stand male 200
0 0 medium 0

#0
)mob");
        auto mob = MobIndexData::from_file(orc.file());
        REQUIRE(mob);
        CHECK(mob->player_name == "evil orc shaman");
        CHECK(mob->short_descr == "the orc shaman");
        CHECK(mob->long_descr == "An orc magic-user stands here, practising dark arts.\n\r");
        CHECK(matches_start("This orc is more intelligent", mob->description));
        CHECK(mob->description.back() == '\r');
        CHECK(race_table[mob->race].name == "orc"sv);
        CHECK(mob->alignment == -950);
        // TODO: this may need adjusting if we change the loader
        CHECK(mob->level == 42);
        CHECK(mob->hitroll == 22);
        CHECK(mob->hit == Dice(10, 6, 1770));
        CHECK(mob->mana == Dice(10, 6, 460));
        CHECK(mob->damage == Dice(10, 4, 19));
        CHECK(attack_table[mob->dam_type].name == "blast"sv);
        CHECK(mob->ac == std::array<sh_int, 4>{-20, -20, -20, -20});
        CHECK(mob->sex == SEX_MALE);
        // TODO: check flags
        CHECK(mob->default_pos == POS_STANDING);
        CHECK(mob->start_pos == POS_STANDING);
        CHECK(mob->size == SIZE_MEDIUM);
        CHECK(mob->material == 0);
    }
}