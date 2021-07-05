/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "ObjectType.hpp"
#include "ExtraDescription.hpp"
#include "Object.hpp"
#include "ObjectIndex.hpp"

#include <catch2/catch.hpp>

TEST_CASE("object type name") {
    Object obj{};
    obj.type = ObjectType::Armor;
    SECTION("armor") { CHECK(obj.type_name() == "armor"); }
}
TEST_CASE("object index type name") {
    ObjectIndex obj_index{};
    obj_index.type = ObjectType::Armor;
    SECTION("armor") { CHECK(obj_index.type_name() == "armor"); }
}
TEST_CASE("try from ordinal") {
    SECTION("valid") {
        const auto result = ObjectTypes::try_from_ordinal(30);

        CHECK(result == ObjectType::Portal);
    }
    SECTION("invalid") {
        const auto result = ObjectTypes::try_from_ordinal(31);

        CHECK(!result);
    }
}
TEST_CASE("lookup with default") {
    SECTION("valid") {
        const auto result = ObjectTypes::lookup_with_default("armor");

        CHECK(result == ObjectType::Armor);
    }
    SECTION("invalid and defaulted") {
        const auto result = ObjectTypes::lookup_with_default("armorr");

        CHECK(result == ObjectType::Light);
    }
}
TEST_CASE("try lookup") {
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
TEST_CASE("list type names") {
    SECTION("formatted string") {
        const auto result = ObjectTypes::list_type_names();

        CHECK(result
              == "	scroll wand staff weapon treasure armor potion \n\r\tclothing furniture trash container drink "
                 "key food \n\r\tmoney boat npccorpse pccorpse fountain pill map \n\r\tbomb portal \n\r");
    }
}
