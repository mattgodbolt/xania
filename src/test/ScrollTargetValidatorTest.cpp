/************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                     */
/*  (C) 2021 Xania Development Team                                     */
/*  See the header to file: merc.h for original code copyrights         */
/************************************************************************/
#include "ScrollTargetValidator.hpp"
#include "ObjectIndex.hpp"
#include "ObjectType.hpp"
#include "Target.hpp"

#include <catch2/catch_test_macros.hpp>
#include <range/v3/algorithm/fill_n.hpp>

namespace {

const auto EmptyIndex = -1;
const auto FireballIndex = 0;
const auto EnchantIndex = 1;
const auto ForgeIndex = 2;
const auto FaerieFogIndex = 3;
const auto DetectEvilIndex = 4;

skill_type test_skill_table[5] = {
    {
        "fireball",
        nullptr, // normally spell_func
        nullptr,
        "fireball",
        "!Fireball!",
        nullptr,
        nullptr,
        0,
        Target::CharOffensive,
        Position::Type::Fighting,
        FireballIndex,
        15,
        12,
        {22, 60, 60, 60},
        {1, 1, 2, 2},
    },
    {
        "enchant weapon",
        nullptr, // normally spell_func
        nullptr,
        "",
        "!Enchant Weapon!",
        nullptr,
        nullptr,
        0,
        Target::ObjectInventory,
        Position::Type::Standing,
        EnchantIndex,
        100,
        24,
        {17, 60, 60, 60},
        {1, 1, 2, 2},
    },
    {
        "forge", // An idea for a new spell!
        nullptr, // normally spell_func
        nullptr,
        "",
        "!Forge!",
        nullptr,
        nullptr,
        0,
        Target::CharObject,
        Position::Type::Standing,
        ForgeIndex,
        100,
        24,
        {17, 60, 60, 60},
        {1, 1, 2, 2},
    },
    {
        "faerie fog",
        nullptr, // normally spell_func
        nullptr,
        "faerie fog",
        "!Faerie Fog!",
        nullptr,
        nullptr,
        0,
        Target::Ignore,
        Position::Type::Standing,
        FaerieFogIndex,
        12,
        12,
        {14, 21, 16, 24},
        {1, 1, 2, 2},
    },
    {
        "detect evil",
        nullptr, // normall spell_func
        nullptr,
        "",
        "The red in your vision disappears.",
        nullptr,
        "$n's sensitivity to evil fades.",
        0,
        Target::CharSelf,
        Position::Type::Standing,
        DetectEvilIndex,
        5,
        12,
        {12, 4, 60, 60},
        {1, 1, 2, 2},
    },
};

}

TEST_CASE("scroll target validator") {
    const auto validator = ScrollTargetValidator(test_skill_table);
    ObjectIndex obj_index;
    obj_index.type = ObjectType::Scroll;
    // The spell slot indexes default to have no spell
    ranges::fill_n(&obj_index.value[1], 3, EmptyIndex);
    SECTION("valid one spell index 1") {
        obj_index.value[1] = FireballIndex;

        const auto result = validator.validate(obj_index);

        CHECK(!result.error_message);
    }
    SECTION("valid one spell index 2") {
        obj_index.value[2] = FireballIndex;

        const auto result = validator.validate(obj_index);

        CHECK(!result.error_message);
    }
    SECTION("valid one spell index 3") {
        obj_index.value[3] = FireballIndex;

        const auto result = validator.validate(obj_index);

        CHECK(!result.error_message);
    }
    SECTION("valid two CharOffensive index 1-2") {
        obj_index.value[1] = FireballIndex;
        obj_index.value[2] = FireballIndex;

        const auto result = validator.validate(obj_index);

        CHECK(!result.error_message);
    }
    SECTION("valid two CharOffensive 2-3") {
        obj_index.value[2] = FireballIndex;
        obj_index.value[3] = FireballIndex;

        const auto result = validator.validate(obj_index);

        CHECK(!result.error_message);
    }
    SECTION("valid CharOffensive-Ignore 1-3") {
        obj_index.value[1] = FireballIndex;
        obj_index.value[3] = FireballIndex;

        const auto result = validator.validate(obj_index);

        CHECK(!result.error_message);
    }
    SECTION("invalid CharOffensive-ObjectInventory 1-2") {
        obj_index.value[1] = FireballIndex;
        obj_index.value[2] = EnchantIndex;

        const auto result = validator.validate(obj_index);

        CHECK(*result.error_message == "Incompatible target type for slot1 and slot2 in #0");
    }
    SECTION("invalid CharOffensive-ObjectInventory 2-1") {
        obj_index.value[2] = FireballIndex;
        obj_index.value[1] = EnchantIndex;

        const auto result = validator.validate(obj_index);

        CHECK(*result.error_message == "Incompatible target type for slot1 and slot2 in #0");
    }
    SECTION("invalid CharObject-CharSelf 1-3") {
        obj_index.value[1] = ForgeIndex;
        obj_index.value[3] = DetectEvilIndex;

        const auto result = validator.validate(obj_index);

        CHECK(*result.error_message == "Incompatible target type for slot1 and slot3 in #0");
    }
    SECTION("invalid CharObject-CharSelf 3-2") {
        obj_index.value[3] = ForgeIndex;
        obj_index.value[2] = DetectEvilIndex;

        const auto result = validator.validate(obj_index);

        CHECK(*result.error_message == "Incompatible target type for slot2 and slot3 in #0");
    }
    SECTION("valid Ignore-CharOffensive 1-2") {
        obj_index.value[1] = FaerieFogIndex;
        obj_index.value[2] = FireballIndex;

        const auto result = validator.validate(obj_index);

        CHECK(!result.error_message);
    }
    SECTION("valid not a scroll") {
        obj_index.type = ObjectType::Wand;
        obj_index.value[1] = FireballIndex;
        obj_index.value[3] = EnchantIndex;

        const auto result = validator.validate(obj_index);

        CHECK(!result.error_message);
    }
}
