/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                    */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  db.h: file in/out, area database handler                             */
/*                                                                       */
/*************************************************************************/

#pragma once

#include "MobIndexData.hpp"
#include "ObjectIndex.hpp"
#include "Room.hpp"

#include <range/v3/view/transform.hpp>

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

struct AreaList;
struct Object;
struct ObjectIndex;
struct ExtraDescription;
struct Mud;

/*
 * Mutable global variables.
 * TODO kill
 */
extern std::vector<std::unique_ptr<Char>> char_list;
extern std::vector<Object *> reapable_objects;
extern std::vector<std::unique_ptr<Object>> object_list;
extern std::vector<Char *> reapable_chars;

extern bool fBootDb; // TODO kill

// Initializes the in-memory database from the area files.
void boot_db(Mud &mud);

void area_update(const AreaList &areas);
Char *create_mobile(MobIndexData *mobIndex, Mud &mud);
void clone_mobile(Char *parent, Char *clone);
void clone_object(Object *parent, Object *clone);
ObjectIndex *get_obj_index(int vnum, const Logger &logger);

const std::map<int, ObjectIndex> &all_object_index_pairs();

inline auto all_object_indexes() {
    return all_object_index_pairs()
           | ranges::views::transform([](const auto &p) -> const ObjectIndex & { return p.second; });
}

const std::map<int, Room> &all_room_pairs();

inline auto all_rooms() {
    return all_room_pairs() | ranges::views::transform([](const auto &p) -> const Room & { return p.second; });
}

Room *get_room(int vnum, const Logger &logger);
std::optional<std::string> try_fread_word(FILE *fp);
std::string fread_word(FILE *fp);
int number_fuzzy(int number);
int number_range(int from, int to);
int number_percent();
int number_bits(int width);
int number_mm();
int dice(int number, int size);
int interpolate(int level, int value_00, int value_32);
bool append_file(const std::string &file, std::string_view text);
char fread_letter(FILE *fp);
int fread_number(FILE *fp, const Logger &logger);
int fread_spnumber(FILE *fp, const Logger &logger);
std::optional<int> try_fread_spnumber(FILE *fp, const Logger &logger);
long fread_flag(FILE *fp, const Logger &logger);
std::string fread_string(FILE *fp);
void fread_to_eol(FILE *fp);
std::string fread_string_eol(FILE *fp);
void reset_room(Room *room, Mud &mud);

MobIndexData *get_mob_index(int vnum, const Logger &logger);
void add_mob_index(MobIndexData mob_index_data, const Logger &logger);

const std::map<int, MobIndexData> &all_mob_index_pairs();

inline auto all_mob_indexes() {
    return all_mob_index_pairs()
           | ranges::views::transform([](const auto &p) -> const MobIndexData & { return p.second; });
}
