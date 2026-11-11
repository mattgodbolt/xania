/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2026 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "Area.hpp"
#include "Char.hpp"
#include "Descriptor.hpp"
#include "DescriptorList.hpp"
#include "MemFile.hpp"
#include "Room.hpp"
#include "db.h"
#include "handler.hpp"

#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <catch2/trompeloeil.hpp>

#include "MockMud.hpp"

namespace {

test::MockMud mock_mud{};
DescriptorList descriptors{};
Logger logger{descriptors};

}

TEST_CASE("extract_char detaches the character from its descriptor both ways") {
    ALLOW_CALL(mock_mud, logger()).LR_RETURN(logger);
    test::MemFile area_file(R"(ignored~
Test~
{ 1 50} Builder A test area~
100 199
)");
    auto area = Area::parse(1, area_file.file(), "test.are", mock_mud);
    Room room{};
    room.area = &area;
    Char player{mock_mud};
    player.name = "Lyra";
    player.level = 10;
    player.pcdata = std::make_unique<PcData>();
    player.in_room = &room;
    room.people.add_back(&player);
    Descriptor desc{0, mock_mud};
    player.desc = &desc;
    desc.character(&player);

    SECTION("leaving the world (quit, purge)") {
        extract_char(&player, true);

        CHECK(desc.character() == nullptr);
        // The character stays in char_list until collect_all_garbage() at the end of the pulse, after the closed
        // descriptor has been freed: it must not still point at it, or char_update() reads freed memory (#2).
        CHECK(player.desc == nullptr);
        CHECK(std::ranges::find(reapable_chars, &player) != reapable_chars.end());
    }
    SECTION("staying in the world (a player's death) keeps the connection") {
        extract_char(&player, false);

        CHECK(player.desc == &desc);
        CHECK(desc.character() == &player);
    }
    // Don't leave a pointer to this stack Char for a later collect_all_garbage().
    std::erase(reapable_chars, &player);
    if (player.in_room)
        player.in_room->people.remove(&player);
    player.in_room = nullptr;
}
