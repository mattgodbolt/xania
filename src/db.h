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
#include "GenericList.hpp"
#include "MobIndexData.hpp"
#include "buffer.h"

#include <range/v3/view/transform.hpp>

#include <cstdio>
#include <map>
#include <string>

struct MobIndexData;
struct Object;
struct ObjectIndex;
struct ExtraDescription;
struct Room;

/*
 * Mutable global variables.
 */
extern GenericList<Char *> char_list;
extern GenericList<Object *> object_list;
extern bool fBootDb;
extern ObjectIndex *obj_index_hash[MAX_KEY_HASH];
extern int top_obj_index;

void boot_db();
void area_update();
Char *create_mobile(MobIndexData *mobIndex);
void clone_mobile(Char *parent, Char *clone);
Object *create_object(ObjectIndex *objIndex);
void clone_object(Object *parent, Object *clone);
const char *get_extra_descr(std::string_view name, const std::vector<ExtraDescription> &ed);
ObjectIndex *get_obj_index(int vnum);
Room *get_room(int vnum);
char *fread_word(FILE *fp);
void *alloc_mem(int sMem);
void *alloc_perm(int sMem);
void free_mem(void *pMem, int sMem);
char *str_dup(const char *str);
void free_string(char *pstr);
int number_fuzzy(int number);
int number_range(int from, int to);
int number_percent();
int number_bits(int width);
int number_mm();
int dice(int number, int size);
int interpolate(int level, int value_00, int value_32);
bool str_cmp(const char *astr, const char *bstr);
bool str_prefix(const char *astr, const char *bstr);
bool str_suffix(const char *astr, const char *bstr);
void append_file(Char *ch, const char *file, const char *str);
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

MobIndexData *get_mob_index(int vnum);
void add_mob_index(MobIndexData mob_index_data);
const std::map<int, MobIndexData> &all_mob_index_pairs();
inline auto all_mob_indexes() {
    return all_mob_index_pairs()
           | ranges::views::transform([](const auto &p) -> const MobIndexData & { return p.second; });
}
void mprog_read_programs(FILE *fp, MobIndexData *mobIndex);
