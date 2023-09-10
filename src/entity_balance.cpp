/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                 */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/

#include "AFFECT_DATA.hpp"
#include "Char.hpp"
#include "Logging.hpp"
#include "Object.hpp"
#include "ObjectIndex.hpp"
#include "ObjectType.hpp"
#include "ObjectWearFlag.hpp"
#include "WeaponFlag.hpp"
#include "common/BitOps.hpp"
#include "db.h"
#include "handler.hpp"
#include "string_utils.hpp"

#include <fmt/format.h>
#include <range/v3/numeric/accumulate.hpp>
#include <range/v3/view/transform.hpp>

namespace {

void objectbug(std::string_view str, ObjectIndex *obj) {
    log_string("obj> {} (#{}): {}", obj->short_descr, obj->vnum, str);
}

/* report_object, takes an object_index_data obj and returns the 'worth' of an
   object in points. */
int report_object(const Object *object) {
    int averagedam, allowedaverage;
    ObjectIndex *obj = object->objIndex;
    auto value =
        ranges::accumulate(obj->affected | ranges::views::transform(&AFFECT_DATA::worth), AFFECT_DATA::Value{});

    /* Weapons are allowed 1 hit and 1 dam for each point */

    auto worth =
        value.worth + (obj->type == ObjectType::Weapon ? (value.hit + value.damage) / 2 : value.hit + value.damage);

    /* Object specific routines */
    switch (obj->type) {

    case ObjectType::Weapon:
        /* Calculate the damage allowed and actual */
        allowedaverage = (object->level / 2) + 4;
        if (check_enum_bit(obj->value[4], WeaponFlag::TwoHands)
            && check_enum_bit(obj->wear_flags, ObjectWearFlag::TwoHands))
            allowedaverage += std::max(1, (allowedaverage) / 20);
        averagedam = (obj->value[1] * obj->value[2] + obj->value[1]) / 2;
        if ((averagedam > allowedaverage)) {
            objectbug("average damage too high", obj);
        }
        /* Add to worth for each weapon type */
        if (check_enum_bit(obj->value[4], WeaponFlag::Flaming))
            worth++;
        if (check_enum_bit(obj->value[4], WeaponFlag::Frost))
            worth++;
        if (check_enum_bit(obj->value[4], WeaponFlag::Vampiric))
            worth++;
        if (check_enum_bit(obj->value[4], WeaponFlag::Sharp))
            worth++;
        if (check_enum_bit(obj->value[4], WeaponFlag::Vorpal))
            worth++;
        break;

    case ObjectType::Potion:
    case ObjectType::Pill:
    case ObjectType::Scroll:
    case ObjectType::Bomb:
    case ObjectType::Staff:
        if ((obj->value[4] > (object->level + (std::max(5, obj->level / 10)))))
            objectbug("level of spell too high", obj);
        break;

    default: break;
    }

    if (worth > ((obj->level / 10) + 1)) {
        objectbug(fmt::format("points too high: has {} points (max should be {})", worth, ((obj->level / 10) + 1)),
                  obj);
    }
    return worth;
}

}

void do_immworth(Char *ch, ArgParser args) {
    const auto *obj = get_obj_world(ch, args.shift());
    if (!obj) {
        ch->send_line("No object found.");
        return;
    }

    const auto worth = report_object(obj);
    const auto shouldbe = ((obj->level / 10) + 1);
    if (worth == shouldbe) {
        ch->send_line("Object '{}' has {} point(s) - exactly right.", obj->objIndex->short_descr, worth);
    } else if (worth > shouldbe) {
        ch->send_line("Object '{}' has {} point(s), {} points |Rtoo high|w.", obj->objIndex->short_descr, worth,
                      worth - shouldbe);
    } else {
        ch->send_line("Object '{}' has {} point(s), within the {} point maximum.", obj->objIndex->short_descr, worth,
                      shouldbe);
    }
}
