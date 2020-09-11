/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                    */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  db.h: file in/out, area database handler                             */
/*                                                                       */
/*************************************************************************/

#pragma once

#include "Constants.hpp"
#include "buffer.h"

#include <cstdio>
#include <string>

struct MOB_INDEX_DATA;
struct OBJ_INDEX_DATA;

extern bool fBootDb;
extern int newmobs;
extern int newobjs;
extern MOB_INDEX_DATA *mob_index_hash[MAX_KEY_HASH];
extern OBJ_INDEX_DATA *obj_index_hash[MAX_KEY_HASH];
extern int top_mob_index;
extern int top_obj_index;
extern int top_affect;

extern int top_vnum_room;
extern int top_vnum_mob;
extern int top_vnum_obj;

/* from db2.c */

extern int social_count;

char fread_letter(FILE *fp);
int fread_number(FILE *fp);
int fread_spnumber(FILE *fp);
long fread_flag(FILE *fp);
BUFFER *fread_string_tobuffer(FILE *fp);
char *fread_string(FILE *fp);
std::string fread_stdstring(FILE *fp);
char *fread_string_eol(FILE *fp);
void fread_to_eol(FILE *fp);
std::string fread_stdstring_eol(FILE *fp);
