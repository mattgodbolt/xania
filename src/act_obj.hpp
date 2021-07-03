/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

#include "GenericList.hpp"

class Char;
struct Object;

bool can_loot(const Char *ch, const Object *obj);
void get_obj(Char *ch, Object *obj, Object *container);
bool obj_move_violates_uniqueness(Char *source_char, Char *dest_char, Object *moving_obj, Object *obj_to);
bool obj_move_violates_uniqueness(Char *source_char, Char *dest_char, Object *moving_obj,
                                  GenericList<Object *> &objs_to);
int check_material_vulnerability(Char *ch, Object *object);
