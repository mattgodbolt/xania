/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "Object.hpp"
#include "Char.hpp"
#include "FlagFormat.hpp"
#include "Lights.hpp"
#include "Logging.hpp"
#include "ObjectExtraFlag.hpp"
#include "ObjectIndex.hpp"
#include "ObjectType.hpp"
#include "ObjectWearFlag.hpp"
#include "WeaponFlag.hpp"
#include "common/BitOps.hpp"
#include "handler.hpp"
#include "string_utils.hpp"

#include <magic_enum.hpp>

Room *get_room(int vnum);

Object *Object::create(ObjectIndex *obj_idx, GenericList<std::unique_ptr<Object>> &object_list) {
    if (obj_idx == nullptr) {
        bug("Object::create: null ObjectIndex.");
        return nullptr;
    }
    auto obj = std::make_unique<Object>(obj_idx);
    auto *raw_obj = obj.get();
    object_list.add_front(std::move(obj));
    return raw_obj;
}

Object *Object::clone(const Object *source, GenericList<std::unique_ptr<Object>> &object_list) {
    if (!source) {
        return nullptr;
    }
    Object *target = Object::create(source->objIndex, object_list);
    if (!target) {
        return nullptr;
    }
    target->name = source->name;
    target->short_descr = source->short_descr;
    target->description = source->description;
    target->type = source->type;
    target->extra_flags = source->extra_flags;
    target->wear_flags = source->wear_flags;
    target->weight = source->weight;
    target->cost = source->cost;
    target->level = source->level;
    target->condition = source->condition;
    target->material = source->material;
    target->timer = source->timer;
    target->value = source->value;
    target->enchanted = source->enchanted;
    target->extra_descr = source->extra_descr;
    for (auto &af : source->affected)
        affect_to_obj(target, af);
    return target;
}

Object::Object(ObjectIndex *obj_idx)
    : objIndex(obj_idx), in_room(nullptr), enchanted(false), owner(""), name(obj_idx->name),
      short_descr(obj_idx->short_descr), description(obj_idx->description), type(obj_idx->type),
      extra_flags(obj_idx->extra_flags), wear_flags(obj_idx->wear_flags), wear_string(obj_idx->wear_string),
      wear_loc(Wear::None), weight(obj_idx->weight), cost(obj_idx->cost), level(obj_idx->level),
      condition(obj_idx->condition), material(obj_idx->material), timer(0), value(obj_idx->value), destination(nullptr)

{
    switch (type) {
    case ObjectType::Light:
        if (value[2] == Lights::ObjectValues::EndlessMarker)
            value[2] = Lights::ObjectValues::Endless;
        break;
    case ObjectType::Portal:
        // TheMoog 1/10/2k : fixes up portal objects - value[0] of a portal
        // if non-zero is looked up and then destination set accordingly.
        if (value[0] != 0) {
            destination = get_room(value[0]);
            if (!destination)
                bug("Couldn't find room index {} for a portal (vnum {})", value[0], objIndex->vnum);
            value[0] = 0; // Prevent finding the vnum in the obj
        }
        break;
    default: break;
    }
    objIndex->count++;
}

Object::~Object() { objIndex->count--; }

std::string Object::type_name() const { return lower_case(magic_enum::enum_name<ObjectType>(type)); }

bool Object::is_holdable() const { return check_enum_bit(wear_flags, ObjectWearFlag::Hold); }
bool Object::is_takeable() const { return check_enum_bit(wear_flags, ObjectWearFlag::Take); }
bool Object::is_two_handed() const { return check_enum_bit(wear_flags, ObjectWearFlag::TwoHands); }
bool Object::is_wieldable() const { return check_enum_bit(wear_flags, ObjectWearFlag::Wield); }
bool Object::is_wear_about() const { return check_enum_bit(wear_flags, ObjectWearFlag::About); }
bool Object::is_wear_arms() const { return check_enum_bit(wear_flags, ObjectWearFlag::Arms); }
bool Object::is_wear_body() const { return check_enum_bit(wear_flags, ObjectWearFlag::Body); }
bool Object::is_wear_ears() const { return check_enum_bit(wear_flags, ObjectWearFlag::Ears); }
bool Object::is_wear_feet() const { return check_enum_bit(wear_flags, ObjectWearFlag::Feet); }
bool Object::is_wear_finger() const { return check_enum_bit(wear_flags, ObjectWearFlag::Finger); }
bool Object::is_wear_hands() const { return check_enum_bit(wear_flags, ObjectWearFlag::Hands); }
bool Object::is_wear_head() const { return check_enum_bit(wear_flags, ObjectWearFlag::Head); }
bool Object::is_wear_legs() const { return check_enum_bit(wear_flags, ObjectWearFlag::Legs); }
bool Object::is_wear_neck() const { return check_enum_bit(wear_flags, ObjectWearFlag::Neck); }
bool Object::is_wear_shield() const { return check_enum_bit(wear_flags, ObjectWearFlag::Shield); }
bool Object::is_wear_waist() const { return check_enum_bit(wear_flags, ObjectWearFlag::Waist); }
bool Object::is_wear_wrist() const { return check_enum_bit(wear_flags, ObjectWearFlag::Wrist); }

bool Object::is_anti_evil() const { return check_enum_bit(extra_flags, ObjectExtraFlag::AntiEvil); }
bool Object::is_anti_good() const { return check_enum_bit(extra_flags, ObjectExtraFlag::AntiGood); }
bool Object::is_anti_neutral() const { return check_enum_bit(extra_flags, ObjectExtraFlag::AntiNeutral); }
bool Object::is_blessed() const { return check_enum_bit(extra_flags, ObjectExtraFlag::Bless); }
bool Object::is_dark() const { return check_enum_bit(extra_flags, ObjectExtraFlag::Dark); }
bool Object::is_evil() const { return check_enum_bit(extra_flags, ObjectExtraFlag::Evil); }
bool Object::is_glowing() const { return check_enum_bit(extra_flags, ObjectExtraFlag::Glow); }
bool Object::is_humming() const { return check_enum_bit(extra_flags, ObjectExtraFlag::Hum); }
bool Object::is_inventory() const { return check_enum_bit(extra_flags, ObjectExtraFlag::Inventory); }
bool Object::is_invisible() const { return check_enum_bit(extra_flags, ObjectExtraFlag::Invis); }
bool Object::is_lock() const { return check_enum_bit(extra_flags, ObjectExtraFlag::Lock); }
bool Object::is_magic() const { return check_enum_bit(extra_flags, ObjectExtraFlag::Magic); }
bool Object::is_no_drop() const { return check_enum_bit(extra_flags, ObjectExtraFlag::NoDrop); }
bool Object::is_no_locate() const { return check_enum_bit(extra_flags, ObjectExtraFlag::NoLocate); }
bool Object::is_no_purge() const { return check_enum_bit(extra_flags, ObjectExtraFlag::NoPurge); }
bool Object::is_no_remove() const { return check_enum_bit(extra_flags, ObjectExtraFlag::NoRemove); }
bool Object::is_protect_container() const { return check_enum_bit(extra_flags, ObjectExtraFlag::ProtectContainer); }
bool Object::is_rot_death() const { return check_enum_bit(extra_flags, ObjectExtraFlag::RotDeath); }
bool Object::is_summon_corpse() const { return check_enum_bit(extra_flags, ObjectExtraFlag::SummonCorpse); }
bool Object::is_unique() const { return check_enum_bit(extra_flags, ObjectExtraFlag::Unique); }
bool Object::is_vis_death() const { return check_enum_bit(extra_flags, ObjectExtraFlag::VisDeath); }

bool Object::is_weapon_two_handed() const { return check_enum_bit(value[4], WeaponFlag::TwoHands); }
