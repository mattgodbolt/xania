/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2026 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "ArgParser.hpp"
#include "Char.hpp"
#include "Descriptor.hpp"
#include "DescriptorList.hpp"
#include "Object.hpp"
#include "ObjectExtraFlag.hpp"
#include "ObjectIndex.hpp"
#include "ObjectType.hpp"
#include "ObjectWearFlag.hpp"
#include "Room.hpp"
#include "common/BitOps.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <catch2/trompeloeil.hpp>

#include "MockMud.hpp"

extern void do_get(Char *ch, ArgParser args);

using Catch::Matchers::ContainsSubstring;

namespace {

test::MockMud mock_mud{};
DescriptorList descriptors{};
Logger logger{descriptors};

// An item that can be picked up, called name.
struct Item {
    ObjectIndex index;
    Object obj;
    Item(const ObjectType type, const std::string &name) : index{.type = type}, obj{&index, logger} {
        obj.type = type;
        obj.name = name;
        obj.short_descr = "a " + name;
        set_enum_bit(obj.wear_flags, ObjectWearFlag::Take);
    }
};

// A player standing in an (inside, so lit) room, with a descriptor to read what they're told.
struct Scene {
    Room room{};
    Char player{mock_mud};
    Descriptor desc{0, mock_mud};
    Scene() {
        player.name = "Lyra";
        player.level = 20;
        player.pcdata = std::make_unique<PcData>();
        player.in_room = &room;
        room.people.add_back(&player);
        player.desc = &desc;
        desc.character(&player);
    }
    ~Scene() {
        // The items live on the test's stack: don't let ~Char extract them into the world's garbage.
        player.carrying.clear();
        room.people.clear();
    }
    void in_room(Item &item) {
        room.contents.add_back(&item.obj);
        item.obj.in_room = &room;
    }
    [[nodiscard]] std::string told() const { return desc.buffered_output(); }
};

void put_in(Item &item, Item &container) {
    container.obj.contains.add_back(&item.obj);
    item.obj.in_obj = &container.obj;
}

}

TEST_CASE("get all and get all.<name> in a room") {
    ALLOW_CALL(mock_mud, logger()).LR_RETURN(logger);
    Item sword{ObjectType::Weapon, "sword"};
    Scene scene;

    // Each of the first three read past the end of the argument before the fix: debug builds aborted on the
    // libstdc++ bounds assertion, release builds read whatever byte followed it.
    SECTION("get all in an empty room") {
        do_get(&scene.player, ArgParser("all"));
        CHECK(scene.told() == "\n\rI see nothing here.\n\r");
    }
    SECTION("get all.<name> in an empty room") {
        do_get(&scene.player, ArgParser("all.sword"));
        CHECK(scene.told() == "\n\rI see no sword here.\n\r");
    }
    SECTION("get all. (no name) in an empty room") {
        do_get(&scene.player, ArgParser("all."));
        CHECK_THAT(scene.told(), ContainsSubstring("I see no"));
    }
    SECTION("get all picks everything up") {
        scene.in_room(sword);
        do_get(&scene.player, ArgParser("all"));
        CHECK(sword.obj.carried_by == &scene.player);
        CHECK(scene.room.contents.empty());
    }
    SECTION("get all.<name> picks up what's called that") {
        scene.in_room(sword);
        do_get(&scene.player, ArgParser("all.sword"));
        CHECK(sword.obj.carried_by == &scene.player);
    }
}

TEST_CASE("get all from an NPC corpse leaves undroppable items unless named") {
    ALLOW_CALL(mock_mud, logger()).LR_RETURN(logger);
    Item corpse{ObjectType::Npccorpse, "corpse"};
    corpse.index.vnum = 10; // anything but the donation pit
    Item bread{ObjectType::Food, "bread"};
    Item ring{ObjectType::Treasure, "ring"};
    set_enum_bit(ring.obj.extra_flags, ObjectExtraFlag::NoDrop);
    Item axe{ObjectType::Weapon, "axe"};
    set_enum_bit(axe.obj.extra_flags, ObjectExtraFlag::NoRemove);
    put_in(bread, corpse);
    put_in(ring, corpse);
    put_in(axe, corpse);
    Scene scene;
    scene.in_room(corpse);

    SECTION("get all corpse (autoloot sends the same) takes the bread, refrains from the nodrop and noremove items") {
        // Before the fix, release builds took all three: the guard got ' ' (the byte after "all"), not a NUL.
        do_get(&scene.player, ArgParser("all corpse"));
        CHECK(bread.obj.carried_by == &scene.player);
        CHECK(ring.obj.in_obj == &corpse.obj);
        CHECK(axe.obj.in_obj == &corpse.obj);
        CHECK_THAT(scene.told(), ContainsSubstring("You refrain from looting a ring"));
        CHECK_THAT(scene.told(), ContainsSubstring("You refrain from looting a axe"));
    }
    SECTION("get all.<name> corpse takes it: the player asked for it by name") {
        do_get(&scene.player, ArgParser("all.ring corpse"));
        CHECK(ring.obj.carried_by == &scene.player);
        CHECK(bread.obj.in_obj == &corpse.obj);
    }
    SECTION("get <name> corpse takes it") {
        do_get(&scene.player, ArgParser("ring corpse"));
        CHECK(ring.obj.carried_by == &scene.player);
    }
    SECTION("get all.<name> corpse with a name shorter than 4 characters") {
        // The old code read arg1[3] after removing "all.", so short names read past the end as well.
        do_get(&scene.player, ArgParser("all.bo corpse"));
        CHECK_THAT(scene.told(), ContainsSubstring("I see nothing like that in the corpse."));
    }
    SECTION("the guard is only for NPC corpses: get all from a container takes everything") {
        corpse.obj.type = ObjectType::Container;
        do_get(&scene.player, ArgParser("all corpse"));
        CHECK(ring.obj.carried_by == &scene.player);
        CHECK(axe.obj.carried_by == &scene.player);
    }
}
