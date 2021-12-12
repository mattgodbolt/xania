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
#include "ObjectIndex.hpp"
#include "Room.hpp"

#include <range/v3/view/transform.hpp>

#include <map>
#include <optional>
#include <string>

struct Object;
struct ObjectIndex;
struct ExtraDescription;

/*
 * Mutable global variables.
 */
extern GenericList<Char *> char_list;
extern GenericList<Object *> object_list;
extern bool fBootDb;

// Initializes the in-memory database from the area files.
void boot_db();
// On shutdown, deletes the Chars & Objects owned by char_list and object_list.
void delete_globals_on_shutdown();

void area_update();
Char *create_mobile(MobIndexData *mobIndex);
void clone_mobile(Char *parent, Char *clone);
Object *create_object(ObjectIndex *objIndex);
void clone_object(Object *parent, Object *clone);
ObjectIndex *get_obj_index(int vnum);

const std::map<int, ObjectIndex> &all_object_index_pairs();

inline auto all_object_indexes() {
    return all_object_index_pairs()
           | ranges::views::transform([](const auto &p) -> const ObjectIndex & { return p.second; });
}

const std::map<int, Room> &all_room_pairs();

inline auto all_rooms() {
    return all_room_pairs() | ranges::views::transform([](const auto &p) -> const Room & { return p.second; });
}

Room *get_room(int vnum);
std::optional<std::string> try_fread_word(FILE *fp);
std::string fread_word(FILE *fp);
int number_fuzzy(int number);
int number_range(int from, int to);
int number_percent();
int number_bits(int width);
int number_mm();
int dice(int number, int size);
int interpolate(int level, int value_00, int value_32);
bool str_cmp(const char *astr, const char *bstr);
bool append_file(std::string file, std::string_view text);
char fread_letter(FILE *fp);
int fread_number(FILE *fp);
int fread_spnumber(FILE *fp);
std::optional<int> try_fread_spnumber(FILE *fp);
long fread_flag(FILE *fp);
std::string fread_string(FILE *fp);
void fread_to_eol(FILE *fp);
std::string fread_string_eol(FILE *fp);
void reset_room(Room *room);

MobIndexData *get_mob_index(int vnum);
void add_mob_index(MobIndexData mob_index_data);

const std::map<int, MobIndexData> &all_mob_index_pairs();

inline auto all_mob_indexes() {
    return all_mob_index_pairs()
           | ranges::views::transform([](const auto &p) -> const MobIndexData & { return p.second; });
}
