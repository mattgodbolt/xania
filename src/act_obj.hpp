/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

#include "GenericList.hpp"

class Char;
struct OBJ_DATA;

bool can_loot(const Char *ch, const OBJ_DATA *obj);
void get_obj(Char *ch, OBJ_DATA *obj, OBJ_DATA *container);
bool obj_move_violates_uniqueness(Char *source_char, Char *dest_char, OBJ_DATA *moving_obj, OBJ_DATA *obj_to);
bool obj_move_violates_uniqueness(Char *source_char, Char *dest_char, OBJ_DATA *moving_obj,
                                  GenericList<OBJ_DATA *> &objs_to);
int check_material_vulnerability(Char *ch, OBJ_DATA *object);
