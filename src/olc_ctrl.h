/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                    */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  olc_ctrl.h:  Faramir's on line creation manager routines             */
/*                                                                       */
/*************************************************************************/

#pragma once

#include "merc.h"

/*
 * Okay - the defined functions we have are ...
 */

DECLARE_DO_FUN(do_object);
DECLARE_DO_FUN(do_clipboard);

#define GET_HOLD 16385
#define GET_WIELD 8193
#define GET_BODY 9

#define FNAME_LEN 30

#if defined(MSDOS)
#define FNAME_LEN 8
#endif

/*
 * Maxima
 */

#define OLC_MAX_OBJECT_LEVEL 120
#define OLC_MAX_MOBILE_LEVEL 120

#define OLC_SECURITY_NONE 0
#define OLC_SECURITY_MINIMUM 1
#define OLC_SECURITY_NORMAL 2
#define OLC_SECURITY_QUEST 3
#define OLC_SECURITY_CREATOR 4
#define OLC_SECURITY_IMPLEMENTOR 5

extern bool obj_check(CHAR_DATA *ch, OBJ_DATA *obj);
extern void find_limits(AREA_DATA *area, int *lower, int *higher);
extern int create_new_room(int lower, int higher);
extern int _delete_obj(int vnum);

/*
 * OLC additional stuff for the object editor
 */

extern const char *weapon_class_table[];

struct apply_values {
    int value;
    char *name;
    int minimum;
    int maximum;
};

/*
 * the functions
 */

/* administrative */

void do_security(CHAR_DATA *ch, char *argument);
char *olc_stripcolour(char *attack_type);
void do_edit(CHAR_DATA *ch, char *argument);
void olc_initialise(CHAR_DATA *ch);
void olc_cleanstrings(CHAR_DATA *ch, bool fObj, bool fMob, bool fRoom);
void olc_slookup_obj(CHAR_DATA *ch);
bool olc_check_quest_list(CHAR_DATA *ch, char *argument);
bool olc_user_authority(CHAR_DATA *ch);
bool olc_string_isalpha(CHAR_DATA *ch, char *string, int len);

/* clip utils */

void clip_add(CHAR_DATA *ch, char *mes);
void clip_sub(CHAR_DATA *ch);
void do_clipboard(CHAR_DATA *ch, char *argument);

/* the object editor */

void do_object(CHAR_DATA *ch, char *argument);
void olc_parse_obj(CHAR_DATA *ch, char *argument);
void olc_edit_obj(CHAR_DATA *ch, char *argument);
bool olc_can_edit_obj(CHAR_DATA *ch, char *argument);
bool olc_can_create_obj(CHAR_DATA *ch, char *argument);
bool olc_create_obj(CHAR_DATA *ch, int vnum, char *type, int level);
void olc_destroy_obj(CHAR_DATA *ch);
void olc_slookup_obj(CHAR_DATA *ch);
void olc_magical_obj(CHAR_DATA *ch, char *argument);
void olc_change_spells_obj(CHAR_DATA *ch, char *argument, int initial, int max_count);
void olc_magical_listvals_obj(CHAR_DATA *ch, OBJ_INDEX_DATA *obj, bool stf_wnd);
void olc_material_obj(CHAR_DATA *ch, char *argument);
void olc_list_materials_obj(CHAR_DATA *ch);
bool olc_material_search_obj(CHAR_DATA *ch, char *argument, int count);
void olc_cost_obj(CHAR_DATA *ch, char *argument);
void olc_flag_obj(CHAR_DATA *ch, char *argument);
void olc_wear_obj(CHAR_DATA *ch, char *argument);
void olc_weapon_obj(CHAR_DATA *ch, char *argument);
void olc_attack_obj(CHAR_DATA *ch, char *argument);
void olc_weapon_flags_obj(CHAR_DATA *ch, char *argument);
void olc_weapon_type_obj(CHAR_DATA *ch, char *argument);
bool olc_weapon_type_search_obj(CHAR_DATA *ch, char *weapon, int count);
void olc_weight_obj(CHAR_DATA *ch, char *argument);
void olc_string_obj(CHAR_DATA *ch, char *argument);
void olc_string_name_obj(CHAR_DATA *ch, char *argument);
void olc_string_short_obj(CHAR_DATA *ch, char *argument);
void olc_string_long_obj(CHAR_DATA *ch, char *argument);
void olc_string_extd_obj(CHAR_DATA *ch, char *argument);
void olc_apply_obj(CHAR_DATA *ch, char *argument);
bool olc_check_apply_range(CHAR_DATA *ch, int aff_loc, int amount);
void olc_list_apply_table_obj(CHAR_DATA *ch);
void olc_display_obj_vars(CHAR_DATA *ch, int info, OBJ_INDEX_DATA *obj);
void olc_add_affect_obj(CHAR_DATA *ch, int aff_loc, int amount);
void olc_remove_affect_obj(CHAR_DATA *ch, int aff_loc);

/* the room editor */

/* control */
void do_room(CHAR_DATA *ch, char *argument);
void olc_parse_room(CHAR_DATA *ch, char *argument);
void olc_edit_room(CHAR_DATA *ch, char *argument);
bool olc_can_edit_room(CHAR_DATA *ch, char *argument);
bool olc_can_create_room(CHAR_DATA *ch, char *argument);
void olc_create_room(CHAR_DATA *ch, int vnum);
int olc_make_new_room(int vnum);
void olc_destroy_room(CHAR_DATA *ch, char *argument);
bool olc_erase_room(int vnum);

/* editing */
void olc_string_room(CHAR_DATA *ch, char *argument);
void olc_room_string_name(CHAR_DATA *ch, char *argument);
void olc_room_string_description(CHAR_DATA *ch, char *argument);
void olc_room_string_extras(CHAR_DATA *ch, char *argument);

/* information */
void olc_display_room_vars(CHAR_DATA *ch, int info);
