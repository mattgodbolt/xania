/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "Object.hpp"
#include "BitsObjectExtra.hpp"
#include "BitsObjectWear.hpp"
#include "BitsWeaponFlag.hpp"
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

bool Object::is_weapon_two_handed() const { return check_bit(value[4], WEAPON_TWO_HANDS); }
