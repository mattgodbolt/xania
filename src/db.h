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
#include "MobIndexData.hpp"
#include "buffer.h"

#include <range/v3/view/transform.hpp>

#include <cstdio>
#include <map>
#include <string>

struct MobIndexData;
struct OBJ_INDEX_DATA;

extern bool fBootDb;
extern int newobjs;
extern OBJ_INDEX_DATA *obj_index_hash[MAX_KEY_HASH];
extern int top_obj_index;
extern int top_affect;

extern int top_vnum_room;
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
char *fread_word(FILE *fp);
inline std::string fread_stdstring_word(FILE *fp) { return std::string(fread_word(fp)); }

MobIndexData *get_mob_index(int vnum);
void add_mob_index(MobIndexData mob_index_data);
const std::map<int, MobIndexData> &all_mob_index_pairs();
inline auto all_mob_indexes() {
    return all_mob_index_pairs()
           | ranges::views::transform([](const auto &p) -> const MobIndexData & { return p.second; });
}
void mprog_read_programs(FILE *fp, MobIndexData *pMobIndex);

char *print_flags(int value);
