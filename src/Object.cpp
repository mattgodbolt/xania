/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "Object.hpp"
#include "BitsObjectExtra.hpp"
#include "ObjectType.hpp"
#include "ObjectWearFlag.hpp"
#include "WeaponFlag.hpp"
#include "common/BitOps.hpp"
#include "string_utils.hpp"

#include <magic_enum.hpp>

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

bool Object::is_anti_evil() const { return check_bit(extra_flags, ITEM_ANTI_EVIL); }
bool Object::is_anti_good() const { return check_bit(extra_flags, ITEM_ANTI_GOOD); }
bool Object::is_anti_neutral() const { return check_bit(extra_flags, ITEM_ANTI_NEUTRAL); }
bool Object::is_blessed() const { return check_bit(extra_flags, ITEM_BLESS); }
bool Object::is_dark() const { return check_bit(extra_flags, ITEM_DARK); }
bool Object::is_evil() const { return check_bit(extra_flags, ITEM_EVIL); }
bool Object::is_glowing() const { return check_bit(extra_flags, ITEM_GLOW); }
bool Object::is_humming() const { return check_bit(extra_flags, ITEM_HUM); }
bool Object::is_inventory() const { return check_bit(extra_flags, ITEM_INVENTORY); }
bool Object::is_invisible() const { return check_bit(extra_flags, ITEM_INVIS); }
bool Object::is_lock() const { return check_bit(extra_flags, ITEM_LOCK); }
bool Object::is_magic() const { return check_bit(extra_flags, ITEM_MAGIC); }
bool Object::is_no_drop() const { return check_bit(extra_flags, ITEM_NODROP); }
bool Object::is_no_locate() const { return check_bit(extra_flags, ITEM_NO_LOCATE); }
bool Object::is_no_purge() const { return check_bit(extra_flags, ITEM_NOPURGE); }
bool Object::is_no_remove() const { return check_bit(extra_flags, ITEM_NOREMOVE); }
bool Object::is_protect_container() const { return check_bit(extra_flags, ITEM_PROTECT_CONTAINER); }
bool Object::is_rot_death() const { return check_bit(extra_flags, ITEM_ROT_DEATH); }
bool Object::is_summon_corpse() const { return check_bit(extra_flags, ITEM_SUMMON_CORPSE); }
bool Object::is_unique() const { return check_bit(extra_flags, ITEM_UNIQUE); }
bool Object::is_vis_death() const { return check_bit(extra_flags, ITEM_VIS_DEATH); }

bool Object::is_weapon_two_handed() const { return check_enum_bit(value[4], WeaponFlag::TwoHands); }
