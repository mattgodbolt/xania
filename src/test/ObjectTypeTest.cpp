/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "ObjectType.hpp"

#include "DescriptorList.hpp"
#include "Object.hpp"
#include "ObjectIndex.hpp"

#include <catch2/catch_test_macros.hpp>
#include <vector>

#include "MockMud.hpp"

namespace {

test::MockMud mock_mud{};
DescriptorList descriptors{};
Logger logger{descriptors};

}

TEST_CASE("object type name") {
    ObjectIndex obj_index{.type = ObjectType::Armor};
    Object obj{&obj_index, logger};
    SECTION("armor") { CHECK(obj.type_name() == "armor"); }
}
TEST_CASE("object index type name") {
    ObjectIndex obj_index{.type = ObjectType::Armor};
    SECTION("armor") { CHECK(obj_index.type_name() == "armor"); }
}
TEST_CASE("object type try from integer") {
    SECTION("valid") {
        const auto result = ObjectTypes::try_from_integer(30);

        CHECK(result == ObjectType::Portal);
    }
    SECTION("invalid") {
        const auto result = ObjectTypes::try_from_integer(31);

        CHECK(!result);
    }
}
TEST_CASE("object type lookup with default") {
    ALLOW_CALL(mock_mud, descriptors()).LR_RETURN(descriptors);
    SECTION("valid") {
        const auto result = ObjectTypes::lookup_with_default("armor", logger);

        CHECK(result == ObjectType::Armor);
    }
    SECTION("invalid and defaulted") {
        const auto result = ObjectTypes::lookup_with_default("armorr", logger);

        CHECK(result == ObjectType::Light);
    }
}
TEST_CASE("object type try lookup") {
    SECTION("valid") {
        const auto result = ObjectTypes::try_lookup("armor");

        CHECK(result == ObjectType::Armor);
    }
    SECTION("substring valid") {
        const auto result = ObjectTypes::try_lookup("armo");

        CHECK(result == ObjectType::Armor);
    }
    SECTION("case insensitive valid") {
        const auto result = ObjectTypes::try_lookup("ARMOR");

        CHECK(result == ObjectType::Armor);
    }
    SECTION("invalid") {
        const auto result = ObjectTypes::try_lookup("armorr");

        CHECK(!result);
    }
}
TEST_CASE("object type sorted type names") {
    SECTION("expected entries") {
        const auto expected = std::vector<std::string>{
            "armor",     "boat",   "bomb",   "clothing", "container", "drink",     "food",     "fountain",
            "furniture", "key",    "light",  "map",      "money",     "npccorpse", "pccorpse", "pill",
            "portal",    "potion", "scroll", "staff",    "trash",     "treasure",  "wand",     "weapon"};

        const auto result = ObjectTypes::sorted_type_names();

        CHECK(result == expected);
    }
}
