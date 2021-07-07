/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "Object.hpp"
#include "BitsObjectWear.hpp"
#include "ObjectType.hpp"
#include "common/BitOps.hpp"
#include "string_utils.hpp"

#include <magic_enum.hpp>

std::string Object::type_name() const { return lower_case(magic_enum::enum_name<ObjectType>(type)); }

bool Object::is_holdable() const { return check_bit(wear_flags, ITEM_HOLD); }
bool Object::is_takeable() const { return check_bit(wear_flags, ITEM_TAKE); }
bool Object::is_two_handed() const { return check_bit(wear_flags, ITEM_TWO_HANDS); }
bool Object::is_wieldable() const { return check_bit(wear_flags, ITEM_WIELD); }
bool Object::is_wear_about() const { return check_bit(wear_flags, ITEM_WEAR_ABOUT); }
bool Object::is_wear_arms() const { return check_bit(wear_flags, ITEM_WEAR_ARMS); }
bool Object::is_wear_body() const { return check_bit(wear_flags, ITEM_WEAR_BODY); }
bool Object::is_wear_ears() const { return check_bit(wear_flags, ITEM_WEAR_EARS); }
bool Object::is_wear_feet() const { return check_bit(wear_flags, ITEM_WEAR_FEET); }
bool Object::is_wear_finger() const { return check_bit(wear_flags, ITEM_WEAR_FINGER); }
bool Object::is_wear_hands() const { return check_bit(wear_flags, ITEM_WEAR_HANDS); }
bool Object::is_wear_head() const { return check_bit(wear_flags, ITEM_WEAR_HEAD); }
bool Object::is_wear_legs() const { return check_bit(wear_flags, ITEM_WEAR_LEGS); }
bool Object::is_wear_neck() const { return check_bit(wear_flags, ITEM_WEAR_NECK); }
bool Object::is_wear_shield() const { return check_bit(wear_flags, ITEM_WEAR_SHIELD); }
bool Object::is_wear_waist() const { return check_bit(wear_flags, ITEM_WEAR_WAIST); }
bool Object::is_wear_wrist() const { return check_bit(wear_flags, ITEM_WEAR_WRIST); }
