/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "Object.hpp"
#include "AffectList.hpp"
#include "ExtraDescription.hpp"
#include "Lights.hpp"
#include "Materials.hpp"
#include "ObjectExtraFlag.hpp"
#include "ObjectIndex.hpp"
#include "ObjectType.hpp"
#include "ObjectWearFlag.hpp"
#include "Room.hpp"
#include "VnumRooms.hpp"
#include "Wear.hpp"

#include <catch2/catch_test_macros.hpp>
#include <vector>

namespace {

unsigned int hum = to_int(ObjectExtraFlag::Hum);
unsigned int hold = to_int(ObjectWearFlag::Hold);

ObjectIndex make_obj_idx() {
    std::vector<ExtraDescription> extra_descr{};
    extra_descr.emplace_back(ExtraDescription{"key", "val"});
    AFFECT_DATA affect;
    affect.level = 1;
    AffectList affected;
    affected.add(affect);
    ObjectIndex obj_idx{// clang-format off
        .extra_descr{extra_descr},
        .name{"name"},
        .short_descr{"short descr"},
        .description{"description"},
        .vnum{123},
        .reset_num{0},
        .material{Material::Type::Paper},
        .type{ObjectType::Map},
        .extra_flags{hum},
        .wear_flags{hold},
        .wear_string{"wear string"},
        .level{1},
        .condition{90},
        .count{0},
        .weight{12},
        .cost{13},
        .value{{0, 1, 2, 3, 4}}
    }; // clang-format on
    obj_idx.affected.add(affect);
    return obj_idx;
}

void validate_object(Object &obj, ObjectIndex &obj_idx) {
    // Although ObjectIndex and Object both have an extra_descr vector, the extra_descr from the template
    // does not get copied to Object by its constructor.  When a Char looks at an Object, try_get_obj_descr() will
    // examine both the instance and its template for extended descriptions. The 'set obj ed' imm command can be
    // used to customize object extended descriptions.
    CHECK(obj.objIndex == &obj_idx);
    CHECK(obj.extra_descr.empty());
    CHECK(obj.affected.empty());
    CHECK(obj.in_room == nullptr);
    CHECK_FALSE(obj.enchanted);
    CHECK(obj.owner == "");
    CHECK(obj.name == "name");
    CHECK(obj.short_descr == "short descr");
    CHECK(obj.description == "description");
    CHECK(obj.type == ObjectType::Map);
    CHECK(obj.extra_flags == hum);
    CHECK(obj.wear_flags == hold);
    CHECK(obj.wear_string == "wear string");
    CHECK(obj.wear_loc == Wear::None);
    CHECK(obj.weight == 12);
    CHECK(obj.cost == 13);
    CHECK(obj.level == 1);
    CHECK(obj.condition == 90);
    CHECK(obj.material == Material::Type::Paper);
    CHECK(obj.timer == 0);
    CHECK(obj.value == std::array{0, 1, 2, 3, 4});
    CHECK(obj.destination == nullptr);
}

TEST_CASE("Construction") {
    SECTION("Prototype counter increment and decrement") {
        ObjectIndex obj_idx1{};
        ObjectIndex obj_idx2{};
        CHECK(obj_idx1.count == 0);
        CHECK(obj_idx2.count == 0);
        {
            Object obj{&obj_idx1};
            CHECK(obj_idx1.count == 1);
            CHECK(obj_idx2.count == 0);
        }
        CHECK(obj_idx1.count == 0);
        CHECK(obj_idx2.count == 0);
    }
    SECTION("Prototype fields cloned") {
        ObjectIndex obj_idx = make_obj_idx();

        SECTION("Using Object constructor") {
            Object obj{&obj_idx};
            validate_object(obj, obj_idx);
        }
        SECTION("Using Object::create") {
            GenericList<std::unique_ptr<Object>> object_list;
            const auto object_count = object_list.size();
            Object *obj = Object::create(&obj_idx, object_list);
            validate_object(*obj, obj_idx);
            CHECK(object_list.size() == object_count + 1);
        }
    }
    SECTION("Object::create with null ObjectIndex") {
        GenericList<std::unique_ptr<Object>> object_list;
        Object *obj = Object::create(nullptr, object_list);
        CHECK(obj == nullptr);
    }
    SECTION("Lights") {
        SECTION("endless light") {
            ObjectIndex obj_idx{.type{ObjectType::Light}, .value{0, 0, Lights::ObjectValues::EndlessMarker, 0, 0}};
            Object obj{&obj_idx};
            CHECK(obj.value[2] == Lights::ObjectValues::Endless);
        }
        SECTION("transient light") {
            ObjectIndex obj_idx{.type{ObjectType::Light}, .value{0, 0, 1, 0, 0}};
            Object obj{&obj_idx};
            CHECK(obj.value[2] == 1);
        }
    }
    SECTION("Portals") {
        SECTION("with destination") {
            ObjectIndex obj_idx{.type{ObjectType::Portal}, .value{Rooms::Limbo, 0, 0, 0, 0}};
            Object obj{&obj_idx};
            CHECK(obj.destination->vnum == 2);
            CHECK(obj.value[0] == 0);
        }
        SECTION("without destination") {
            ObjectIndex obj_idx{.type{ObjectType::Portal}, .value{0, 0, 0, 0, 0}};
            Object obj{&obj_idx};
            CHECK(obj.destination == nullptr);
            CHECK(obj.value[0] == 0);
        }
    }
}

TEST_CASE("Clone") {
    SECTION("simple") {
        GenericList<std::unique_ptr<Object>> object_list;
        ObjectIndex obj_idx = make_obj_idx();
        Object *source = Object::create(&obj_idx, object_list);
        Object *clone = Object::clone(source, object_list);
        validate_object(*clone, obj_idx);
    }
    SECTION("container shallow clone") {
        GenericList<std::unique_ptr<Object>> object_list;
        ObjectIndex obj_idx = make_obj_idx();
        Object *container = Object::create(&obj_idx, object_list);
        Object *contained = Object::create(&obj_idx, object_list);
        container->contains.add_front(contained);

        Object *clone = Object::clone(container, object_list);

        validate_object(*clone, obj_idx);
        CHECK(clone->contains.empty());
    }
}

}
