/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2026 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "ObjectComparison.hpp"
#include "AFFECT_DATA.hpp"
#include "AffectLocation.hpp"
#include "DescriptorList.hpp"
#include "Logging.hpp"
#include "Object.hpp"
#include "ObjectIndex.hpp"
#include "ObjectType.hpp"
#include "ObjectWearFlag.hpp"
#include "common/BitOps.hpp"

#include <array>
#include <catch2/catch_test_macros.hpp>
#include <initializer_list>
#include <string_view>
#include <utility>
#include <vector>

using namespace std::literals;

namespace {

DescriptorList descriptors{};
Logger logger{descriptors};

const auto finger = to_int(ObjectWearFlag::Take) | to_int(ObjectWearFlag::Finger);
const auto neck = to_int(ObjectWearFlag::Take) | to_int(ObjectWearFlag::Neck);
const auto wield = to_int(ObjectWearFlag::Take) | to_int(ObjectWearFlag::Wield);
const auto body = to_int(ObjectWearFlag::Take) | to_int(ObjectWearFlag::Body);

ObjectIndex make_index(ObjectType type, unsigned int wear_flags, std::array<int, 5> value,
                       std::initializer_list<std::pair<AffectLocation, int>> affects) {
    ObjectIndex index{.type = type, .wear_flags = wear_flags, .value = value};
    for (const auto &[location, modifier] : affects) {
        AFFECT_DATA af;
        af.location = location;
        af.modifier = static_cast<sh_int>(modifier);
        index.affected.add_at_end(af);
    }
    return index;
}

std::vector<std::string_view> compare(const Object &a, const Object &b) {
    return ObjectComparison::compare(a, b).lines;
}

}

// The examples from #144, with the items' values and affects from the area files.
TEST_CASE("compare weapons") {
    // a very sharp pin (16018): 4d5, +2 hitroll, +1 damroll, -3 str, -15 hp
    auto pin_index = make_index(ObjectType::Weapon, wield, {0, 4, 5, 0, 0},
                                {{AffectLocation::Str, -3},
                                 {AffectLocation::Hit, -15},
                                 {AffectLocation::Hitroll, 2},
                                 {AffectLocation::Damroll, 1}});
    // a long elven dagger (1603): 5d4, +2 hitroll
    auto dagger_index = make_index(ObjectType::Weapon, wield, {0, 5, 4, 0, 0}, {{AffectLocation::Hitroll, 2}});
    Object pin{&pin_index, logger};
    Object dagger{&dagger_index, logger};

    SECTION("damroll counts in the verdict: the pin's +1 outweighs half a point of dice") {
        CHECK(ObjectComparison::weapon_score(pin) == 26); // (1 + 5) * 4 + 2 * 1: twice 13 a hit
        CHECK(ObjectComparison::weapon_score(dagger) == 25); // (1 + 4) * 5: twice 12.5
        CHECK(compare(pin, dagger)
              == std::vector{"$p looks better than $P."sv, "$p would make you weaker."sv,
                             "$p would make you less healthy."sv});
    }
    SECTION("the other way round") {
        CHECK(compare(dagger, pin)
              == std::vector{"$p looks worse than $P."sv, "$p would make you stronger."sv,
                             "$p would make you healthier."sv});
    }
    SECTION("hitroll gets its own line") {
        auto plain_index = make_index(ObjectType::Weapon, wield, {0, 5, 4, 0, 0}, {});
        Object plain{&plain_index, logger};
        CHECK(compare(dagger, plain)
              == std::vector{"$p and $P look about the same."sv, "$p would help you hit more often."sv});
    }
    SECTION("damroll doesn't get a line of its own") {
        auto bonus_index = make_index(ObjectType::Weapon, wield, {0, 5, 4, 0, 0},
                                      {{AffectLocation::Hitroll, 2}, {AffectLocation::Damroll, 3}});
        Object bonus{&bonus_index, logger};
        CHECK(compare(bonus, dagger) == std::vector{"$p looks better than $P."sv});
    }
    SECTION("an enchanted object's own affects replace its index's") {
        dagger.enchanted = true; // as enchant weapon leaves it, having copied the index's affects (here, none)
        CHECK(ObjectComparison::total_modifier(dagger, AffectLocation::Hitroll) == 0);
        CHECK(compare(dagger, pin)
              == std::vector{"$p looks worse than $P."sv, "$p would make you miss more often."sv,
                             "$p would make you stronger."sv, "$p would make you healthier."sv});
    }
}

TEST_CASE("compare armor") {
    auto plate_index = make_index(ObjectType::Armor, body, {5, 5, 5, 0, 0}, {});
    Object plate{&plate_index, logger};
    SECTION("armor class vs magic counts") {
        auto robe_index = make_index(ObjectType::Armor, body, {4, 4, 4, 4, 0}, {});
        Object robe{&robe_index, logger};
        CHECK(compare(robe, plate) == std::vector{"$p looks better than $P."sv}); // 16 against 15
    }
    SECTION("an AC affect counts once for each of the four armor classes") {
        auto mail_index = make_index(ObjectType::Armor, body, {4, 4, 4, 0, 0}, {{AffectLocation::Ac, -1}});
        Object mail{&mail_index, logger};
        CHECK(ObjectComparison::armor_score(mail) == 16);
        CHECK(compare(mail, plate) == std::vector{"$p looks better than $P."sv});
    }
    SECTION("armor's other bonuses get lines, damroll included") {
        auto spiked_index = make_index(ObjectType::Armor, body, {5, 5, 5, 0, 0},
                                       {{AffectLocation::Damroll, 1}, {AffectLocation::SavingPara, -2}});
        Object spiked{&spiked_index, logger};
        CHECK(compare(spiked, plate)
              == std::vector{"$p and $P look about the same."sv, "$p would guard you better against magic."sv,
                             "$p would make you hit harder."sv});
    }
}

TEST_CASE("compare things worn in the same place") {
    // a ring of protection (3043, treasure): -2 AC, -1 save vs spell
    auto protection_index = make_index(ObjectType::Treasure, finger, {0, 0, 0, 0, 0},
                                       {{AffectLocation::Ac, -2}, {AffectLocation::SavingSpell, -1}});
    // a pink ice ring (6601, treasure): +1 str, +4 hp
    auto pink_index =
        make_index(ObjectType::Treasure, finger, {0, 0, 0, 0, 0}, {{AffectLocation::Str, 1}, {AffectLocation::Hit, 4}});
    Object protection{&protection_index, logger};
    Object pink{&pink_index, logger};

    SECTION("rings get the stat lines, with no verdict") {
        CHECK(compare(protection, pink)
              == std::vector{"$p would protect you better than $P."sv, "$p would guard you better against magic."sv,
                             "$p would make you weaker."sv, "$p would make you less healthy."sv});
    }
    SECTION("all the saving throws are one") {
        auto rod_index = make_index(ObjectType::Treasure, finger, {0, 0, 0, 0, 0},
                                    {{AffectLocation::Ac, -2}, {AffectLocation::SavingRod, -1}});
        Object rod{&rod_index, logger};
        CHECK(compare(protection, rod) == std::vector{"$p and $P look about the same."sv});
    }
    SECTION("an armor ring's own armor class counts") {
        auto band_index = make_index(ObjectType::Armor, finger, {1, 1, 1, 1, 0}, {});
        Object band{&band_index, logger};
        CHECK(compare(band, pink)
              == std::vector{"$p would protect you better than $P."sv, "$p would make you weaker."sv,
                             "$p would make you less healthy."sv});
    }
    SECTION("not worn in the same place") {
        auto amulet_index = make_index(ObjectType::Treasure, neck, {0, 0, 0, 0, 0}, {{AffectLocation::Str, 1}});
        Object amulet{&amulet_index, logger};
        CHECK(compare(amulet, pink) == std::vector{"You can't compare $p and $P."sv});
    }
    SECTION("an object with itself") {
        CHECK(compare(pink, pink) == std::vector{"You compare $p to itself.  It looks about the same."sv});
    }
}

TEST_CASE("a weapon and something else can't be compared") {
    auto sword_index = make_index(ObjectType::Weapon, wield, {0, 2, 6, 0, 0}, {});
    auto club_index = make_index(ObjectType::Treasure, wield, {0, 0, 0, 0, 0}, {{AffectLocation::Str, 1}});
    Object sword{&sword_index, logger};
    Object club{&club_index, logger};
    CHECK(compare(sword, club) == std::vector{"You can't compare $p and $P."sv});
}
