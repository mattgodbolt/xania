/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 ***************************************************************************/

/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                 */
/*  See the header to file: merc.h for original code copyrights          */
/*************************************************************************/
#pragma once

#include "GenericList.hpp"
#include "Materials.hpp"
#include "Types.hpp"
#include "Worn.hpp"

#include "apps/pfu.hpp"
#include <string_view>

struct Char;
enum class Stat;
struct AFFECT_DATA;
struct Object;
struct ObjectIndex;
struct Room;

[[nodiscard]] Char *get_char_room(Char *ch, std::string_view argument);
[[nodiscard]] Char *get_char_world(Char *ch, std::string_view argument);
void extract_char(Char *ch, bool delete_from_world);
[[nodiscard]] int race_lookup(std::string_view name);

int get_weapon_sn(Char *ch);
int get_weapon_skill(Char *ch, int sn);
int get_max_train(Char *ch, Stat stat);
int can_carry_n(Char *ch);
int can_carry_w(Char *ch);
void affect_to_char(Char *ch, const AFFECT_DATA &af);
void affect_to_obj(Object *obj, const AFFECT_DATA &af);
void affect_remove(Char *ch, const AFFECT_DATA &af);
void affect_remove_obj(Object *obj, const AFFECT_DATA &af);
void affect_strip(Char *ch, int sn);
AFFECT_DATA *find_affect(Char *ch, int sn);
void affect_join(Char *ch, const AFFECT_DATA &af);
void char_from_room(Char *ch);
void char_to_room(Char *ch, Room *room);
void obj_to_char(Object *obj, Char *ch);
void obj_from_char(Object *obj);
int apply_ac(Object *obj, const Worn worn, const int type);
Object *get_eq_char(Char *ch, const Worn worn);
void equip_char(Char *ch, Object *obj, const Worn wear);
void unequip_char(Char *ch, Object *obj);
int count_obj_list(ObjectIndex *obj, const GenericList<Object *> &list);
void obj_from_room(Object *obj);
void obj_to_room(Object *obj, Room *room);
void obj_to_obj(Object *obj, Object *obj_to);
void obj_from_obj(Object *obj);
void extract_obj(Object *obj);

/* MRG added */
bool check_sub_issue(Object *obj, Char *ch);

Char *get_mob_by_vnum(sh_int vnum);
Object *get_obj_type(ObjectIndex *objIndexData);
Object *get_obj_list(const Char *ch, std::string_view argument, GenericList<Object *> &list);
Object *get_obj_wear(Char *ch, const char *argument);
Object *get_obj_here(const Char *ch, std::string_view argument);
Object *get_obj_world(Char *ch, std::string_view argument);
Object *create_money(const uint amount, const Logger &logger);
int get_obj_number(Object *obj);
int get_obj_weight(Object *obj);
bool room_is_dark(Room *room);
bool room_is_private(Room *room);
bool can_drop_obj(Char *ch, Object *obj);
