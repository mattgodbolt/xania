/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                    */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  handler.c:  a core module with a huge amount of utility functions    */
/*                                                                       */
/*************************************************************************/

#include "handler.hpp"
#include "AFFECT_DATA.hpp"
#include "AffectFlag.hpp"
#include "Area.hpp"
#include "BodyPartFlag.hpp"
#include "Char.hpp"
#include "CharActFlag.hpp"
#include "Classes.hpp"
#include "CommFlag.hpp"
#include "DamageTolerance.hpp"
#include "DamageType.hpp"
#include "Descriptor.hpp"
#include "Logging.hpp"
#include "Materials.hpp"
#include "MorphologyFlag.hpp"
#include "Object.hpp"
#include "ObjectExtraFlag.hpp"
#include "ObjectIndex.hpp"
#include "ObjectType.hpp"
#include "ObjectWearFlag.hpp"
#include "OffensiveFlag.hpp"
#include "PlayerActFlag.hpp"
#include "Races.hpp"
#include "Room.hpp"
#include "RoomFlag.hpp"
#include "SkillNumbers.hpp"
#include "SpellTarget.hpp"
#include "ToleranceFlag.hpp"
#include "VnumObjects.hpp"
#include "VnumRooms.hpp"
#include "Weapon.hpp"
#include "WeaponFlag.hpp"
#include "Wear.hpp"
#include "WeatherData.hpp"
#include "act_comm.hpp"
#include "act_obj.hpp"
#include "comm.hpp"
#include "common/BitOps.hpp"
#include "db.h"
#include "fight.hpp"
#include "interp.h"
#include "lookup.h"
#include "magic.h"
#include "ride.hpp"
#include "string_utils.hpp"

#include <range/v3/algorithm/count.hpp>

#include <magic_enum.hpp>
#include <range/v3/iterator/operations.hpp>

void spell_poison(int spell_num, int level, Char *ch, const SpellTarget &spell_target);

/*
 * Local functions.
 */
void affect_modify(Char *ch, const AFFECT_DATA &af, bool fAdd);

/* returns race number */
int race_lookup(std::string_view name) {
    for (int race = 0; race_table[race].name != nullptr; race++) {
        if (matches_start(name, race_table[race].name))
            return race;
    }
    return 0;
}

/* returns class number */
int class_lookup(std::string_view name) {
    for (size_t class_num = 0; class_num < MAX_CLASS; class_num++) {
        if (matches_start(name, class_table[class_num].name))
            return class_num;
    }
    return -1;
}

/* for returning skill information */
int get_skill(const Char *ch, int sn) { return ch->get_skill(sn); }

/* for returning weapon information */
int get_weapon_sn(Char *ch) {
    const Object *wield = get_eq_char(ch, Wear::Wield);
    if (wield == nullptr || wield->type != ObjectType::Weapon)
        return gsn_hand_to_hand;
    else {
        const auto opt_weapon_type = Weapons::try_from_integer(wield->value[0]);
        if (opt_weapon_type) {
            switch (*opt_weapon_type) {
            case (Weapon::Sword): return gsn_sword;
            case (Weapon::Dagger): return gsn_dagger;
            case (Weapon::Spear): return gsn_spear;
            case (Weapon::Mace): return gsn_mace;
            case (Weapon::Axe): return gsn_axe;
            case (Weapon::Flail): return gsn_flail;
            case (Weapon::Whip): return gsn_whip;
            case (Weapon::Polearm): return gsn_polearm;
            case (Weapon::Exotic): return -1;
            }
        }
    }
    return -1;
}

int get_weapon_skill(Char *ch, int sn) {
    int skill;

    /* -1 is exotic */
    if (ch->is_npc()) {
        if (sn == -1)
            skill = 3 * ch->level;
        else if (sn == gsn_hand_to_hand)
            skill = 40 + 2 * ch->level;
        else
            skill = 40 + 5 * ch->level / 2;
    }

    else {
        if (sn == -1)
            skill = std::clamp(2 * ch->level, 0, 90);
        else
            skill = ch->pcdata->learned[sn];
    }

    return std::clamp(skill, 0, 100);
}

/*
 * Retrieve a character's age.
 */
int get_age(const Char *ch) {
    using namespace std::chrono;
    return 17 + duration_cast<hours>(ch->total_played()).count() / 20;
}

/* command for retrieving stats */
int get_curr_stat(const Char *ch, Stat stat) { return ch->curr_stat(stat); }

/* command for returning max training score */
int get_max_train(Char *ch, Stat stat) {
    int max;

    if (ch->is_npc() || ch->level > LEVEL_IMMORTAL)
        return MaxStatValue;

    max = pc_race_table[ch->race].max_stats[stat];
    if (class_table[ch->class_num].attr_prime == stat)
        /* if (ch->race == race_lookup("human"))
            max += 3;
         else*/
        max += 2;

    return std::min(max, MaxStatValue);
}

/*
 * Retrieve a character's carry capacity.
 */
int can_carry_n(Char *ch) {
    if (ch->is_pc() && ch->level >= LEVEL_IMMORTAL)
        return 1000;

    if (ch->is_npc() && check_enum_bit(ch->act, CharActFlag::Pet))
        return 4;

    return magic_enum::enum_count<Wear>() + 2 * get_curr_stat(ch, Stat::Dex) + ch->level;
}

/*
 * Retrieve a character's carry capacity.
 */
int can_carry_w(Char *ch) {
    if (ch->is_pc() && ch->level >= LEVEL_IMMORTAL)
        return 1000000;

    if (ch->is_npc() && check_enum_bit(ch->act, CharActFlag::Pet))
        return 1000;

    return str_app[get_curr_stat(ch, Stat::Str)].carry + ch->level * 5 / 2;
}

/*
 * Apply or remove an affect to a character.
 */
void affect_modify(Char *ch, const AFFECT_DATA &af, bool fAdd) {
    if (fAdd) {
        af.apply(*ch);
    } else {
        af.unapply(*ch);
    }

    /*
     * Check for weapon wielding.
     * Guard against recursion (for weapons with affects).
     */
    Object *wield;
    if (ch->is_pc() && (wield = get_eq_char(ch, Wear::Wield)) != nullptr
        && get_obj_weight(wield) > str_app[get_curr_stat(ch, Stat::Str)].wield) {
        static int depth;

        if (depth == 0) {
            depth++;
            act("You drop $p.", ch, wield, nullptr, To::Char);
            act("$n drops $p.", ch, wield, nullptr, To::Room);
            obj_from_char(wield);
            obj_to_room(wield, ch->in_room);
            depth--;
        }
    }
}

/*
 * Give an affect to a char.
 */
void affect_to_char(Char *ch, const AFFECT_DATA &af) {
    auto &paf_new = ch->affected.add(af);
    affect_modify(ch, paf_new, true);
}

/* give an affect to an object */
void affect_to_obj(Object *obj, const AFFECT_DATA &af) { obj->affected.add(af); }

/*
 * Remove an affect from a char.
 */
void affect_remove(Char *ch, const AFFECT_DATA &af) {
    if (ch->affected.empty()) {
        bug("Affect_remove: no affect.");
        return;
    }

    affect_modify(ch, af, false);
    ch->affected.remove(af);
}

void affect_remove_obj(Object *obj, const AFFECT_DATA &af) {
    if (obj->affected.empty()) {
        bug("Affect_remove_object: no affect.");
        return;
    }

    if (obj->carried_by != nullptr && obj->wear_loc != Wear::None)
        affect_modify(obj->carried_by, af, false);

    obj->affected.remove(af);
}

/*
 * Strip all affects of a given sn.
 */
void affect_strip(Char *ch, int sn) {
    // This is O(N²) but N is likely 0 or 1 in all practical cases.
    while (auto *aff = ch->affected.find_by_skill(sn))
        affect_remove(ch, *aff);
}

// Returns the AFFECT_DATA * structure for a char.
// return nullptr if the char isn't affected.
AFFECT_DATA *find_affect(Char *ch, int sn) { return ch->affected.find_by_skill(sn); }

// Add or enhance an affect.
void affect_join(Char *ch, const AFFECT_DATA &af) {
    auto net_af = af;
    if (auto old = ch->affected.find_by_skill(af.type)) {
        net_af.level = (net_af.level + old->level) / 2;
        net_af.duration += old->duration;
        net_af.modifier += old->modifier;
        affect_remove(ch, *old);
    }

    affect_to_char(ch, af);
}

/*
 * Move a char out of a room.
 */
void char_from_room(Char *ch) {
    Object *obj;

    if (ch->in_room == nullptr) {
        bug("Char_from_room: nullptr.");
        return;
    }

    if (ch->is_pc())
        ch->in_room->area->player_left();

    if ((obj = get_eq_char(ch, Wear::Light)) != nullptr && obj->type == ObjectType::Light && obj->value[2] != 0
        && ch->in_room->light > 0)
        --ch->in_room->light;

    if (!ch->in_room->people.remove(ch))
        bug("Char_from_room: ch not found.");

    /* MOBProgs - check to tell mobs we've left the room removed from HERE*/

    ch->in_room = nullptr;
}

/*
 * Move a char into a room.
 */
void char_to_room(Char *ch, Room *room) {
    if (room == nullptr) {
        bug("Char_to_room: nullptr.");
        return;
    }

    ch->in_room = room;
    room->people.add_front(ch);

    if (ch->is_pc())
        ch->in_room->area->player_entered();

    if (auto *obj = get_eq_char(ch, Wear::Light); obj && obj->type == ObjectType::Light && obj->value[2] != 0)
        ++ch->in_room->light;

    if (ch->is_aff_plague()) {
        auto *existing_plague = ch->affected.find_by_skill(gsn_plague);
        if (!existing_plague) {
            clear_enum_bit(ch->affected_by, AffectFlag::Plague);
            return;
        }

        if (existing_plague->level == 1)
            return;

        AFFECT_DATA plague;
        plague.type = gsn_plague;
        plague.level = existing_plague->level - 1;
        plague.duration = number_range(1, 2 * plague.level);
        plague.location = AffectLocation::Str;
        plague.modifier = -5;
        plague.bitvector = to_int(AffectFlag::Plague);

        for (auto *vch : ch->in_room->people) {
            const int save = [&]() -> int {
                switch (check_damage_tolerance(vch, DamageType::Disease)) {
                default:
                case DamageTolerance::None: return existing_plague->level - 4;
                case DamageTolerance::Immune: return 0;
                case DamageTolerance::Resistant: return existing_plague->level - 8;
                case DamageTolerance::Vulnerable: return existing_plague->level;
                }
            }();

            if (save != 0 && !saves_spell(save, vch) && vch->is_mortal() && !vch->is_aff_plague()
                && number_bits(6) == 0) {
                vch->send_line("You feel hot and feverish.");
                act("$n shivers and looks very ill.", vch);
                affect_join(vch, plague);
            }
        }
    }
}

/*
 * Give an obj to a char.
 */
void obj_to_char(Object *obj, Char *ch) {
    ch->carrying.add_front(obj);
    obj->carried_by = ch;
    obj->in_room = nullptr;
    obj->in_obj = nullptr;
    ch->carry_number += get_obj_number(obj);
    ch->carry_weight += get_obj_weight(obj);
}

/*
 * Take an obj from its character.
 */
void obj_from_char(Object *obj) {
    Char *ch;

    if ((ch = obj->carried_by) == nullptr) {
        bug("Obj_from_char: null ch.");
        return;
    }

    if (obj->wear_loc != Wear::None)
        unequip_char(ch, obj);

    if (!ch->carrying.remove(obj))
        bug("Obj_from_char: obj not in list.");

    obj->carried_by = nullptr;
    ch->carry_number -= get_obj_number(obj);
    ch->carry_weight -= get_obj_weight(obj);
}

/*
 * Find the ac value of an obj, including position effect.
 */
int apply_ac(Object *obj, const Wear wear, const int type) {
    if (obj->type != ObjectType::Armor)
        return 0;

    switch (wear) {
    case Wear::Body:
    case Wear::Head:
    case Wear::Legs:
    case Wear::Feet:
    case Wear::Hands:
    case Wear::Arms:
    case Wear::Shield:
    case Wear::FingerL:
    case Wear::FingerR:
    case Wear::Neck1:
    case Wear::Neck2:
    case Wear::About:
    case Wear::Waist:
    case Wear::WristL:
    case Wear::WristR:
    case Wear::Hold:
    case Wear::Ears: return obj->value[type];
    default: return 0;
    }
}

/*
 * Find a piece of eq on a character.
 */
Object *get_eq_char(Char *ch, const Wear wear) {
    if (ch == nullptr)
        return nullptr;

    for (auto *obj : ch->carrying) {
        if (obj->wear_loc == wear)
            return obj;
    }

    return nullptr;
}

/**
 * If a character is vulnerable to the material of an object
 * then poison them and inform the room. The poison effect doesn't
 * stack with existing poison, that would be pretty nasty.
 */
void enforce_material_vulnerability(Char *ch, Object *obj) {
    if (check_material_vulnerability(ch, obj)) {
        act("As you equip $p it burns you, causing you to shriek in pain!", ch, obj, nullptr, To::Char);
        act("$n shrieks in pain!", ch, obj, nullptr, To::Room);
        if (!ch->is_aff_poison()) {
            int p_sn = skill_lookup("poison");
            auto spell_target = SpellTarget(ch);
            spell_poison(p_sn, ch->level, ch, spell_target);
        }
    }
}

/*
 * Equip a char with an obj.
 */
void equip_char(Char *ch, Object *obj, const Wear wear) {
    if (get_eq_char(ch, wear) != nullptr) {
        bug("Equip_char: {} #{} already equipped in slot {}.", ch->name, (ch->is_npc() ? ch->mobIndex->vnum : 0), wear);
        return;
    }

    if ((obj->is_anti_evil() && ch->is_evil()) || (obj->is_anti_good() && ch->is_good())
        || (obj->is_anti_neutral() && ch->is_neutral())) {
        /*
         * Thanks to Morgenes for the bug fix here!
         */
        act("You are zapped by $p and drop it.", ch, obj, nullptr, To::Char);
        act("$n is zapped by $p and drops it.", ch, obj, nullptr, To::Room);
        obj_from_char(obj);
        obj_to_room(obj, ch->in_room);
        return;
    }

    enforce_material_vulnerability(ch, obj);

    for (int i = 0; i < 4; i++)
        ch->armor[i] -= apply_ac(obj, wear, i);
    obj->wear_loc = wear;

    if (!obj->enchanted)
        for (auto &af : obj->objIndex->affected)
            affect_modify(ch, af, true);
    for (auto &af : obj->affected)
        affect_modify(ch, af, true);

    if (obj->type == ObjectType::Light && obj->value[2] != 0 && ch->in_room != nullptr)
        ++ch->in_room->light;
}

/*
 * Unequip a char with an obj.
 */
void unequip_char(Char *ch, Object *obj) {
    if (obj->wear_loc == Wear::None) {
        bug("Unequip_char: already unequipped.");
        return;
    }

    for (int i = 0; i < 4; i++)
        ch->armor[i] += apply_ac(obj, obj->wear_loc, i);
    obj->wear_loc = Wear::None;

    if (!obj->enchanted)
        for (auto &af : obj->objIndex->affected)
            affect_modify(ch, af, false);
    for (auto &af : obj->affected)
        affect_modify(ch, af, false);

    if (obj->type == ObjectType::Light && obj->value[2] != 0 && ch->in_room != nullptr && ch->in_room->light > 0)
        --ch->in_room->light;
}

/*
 * Count occurrences of an obj in a list.
 */
int count_obj_list(ObjectIndex *objIndex, const GenericList<Object *> &list) {
    return ranges::count(list, objIndex, [](auto *obj) { return obj->objIndex; });
}

/*
 * Move an obj out of a room.
 */
void obj_from_room(Object *obj) {
    if (!obj->in_room) {
        bug("obj_from_room: nullptr.");
        return;
    }

    if (!obj->in_room->contents.remove(obj)) {
        bug("Obj_from_room: obj not found.");
        return;
    }

    obj->in_room = nullptr;
}

bool check_sub_issue(Object *obj, Char *ch) {
    int vnum = obj->objIndex->vnum;
    if (((vnum >= 3700) && (vnum <= 3713)) || (vnum == 3716) || (vnum == 3717)) {
        act("$n drops the $p. It disappears in a puff of acrid smoke.", ch, obj, nullptr, To::Room);
        act("$p disappears in a puff of acrid smoke.", ch, obj, nullptr, To::Char);
        return true;
    }
    return false;
}

/*
 * Move an obj into a room.
 */
void obj_to_room(Object *obj, Room *room) {
    room->contents.add_front(obj);
    obj->in_room = room;
    obj->carried_by = nullptr;
    obj->in_obj = nullptr;
}

/*
 * Move an object into an object.
 */
void obj_to_obj(Object *obj, Object *obj_to) {
    obj_to->contains.add_front(obj);
    obj->in_obj = obj_to;
    obj->in_room = nullptr;
    obj->carried_by = nullptr;
    if (obj_to->objIndex->vnum == Objects::Pit)
        obj->cost = 0;

    for (; obj_to != nullptr; obj_to = obj_to->in_obj) {
        if (obj_to->carried_by != nullptr) {
            obj_to->carried_by->carry_number += get_obj_number(obj);
            obj_to->carried_by->carry_weight += get_obj_weight(obj);
        }
    }
}

/*
 * Move an object out of an object.
 */
void obj_from_obj(Object *obj) {
    Object *obj_from;

    if ((obj_from = obj->in_obj) == nullptr) {
        bug("Obj_from_obj: null obj_from.");
        return;
    }

    if (!obj_from->contains.remove(obj)) {
        bug("Obj_from_obj: obj not found.");
        return;
    }
    obj->in_obj = nullptr;

    for (; obj_from != nullptr; obj_from = obj_from->in_obj) {
        if (obj_from->carried_by != nullptr) {
            obj_from->carried_by->carry_number -= get_obj_number(obj);
            obj_from->carried_by->carry_weight -= get_obj_weight(obj);
        }
    }
}

/*
 * Extract an obj from the world.
 */
void extract_obj(Object *obj) {
    if (obj->in_room != nullptr)
        obj_from_room(obj);
    else if (obj->carried_by != nullptr)
        obj_from_char(obj);
    else if (obj->in_obj != nullptr)
        obj_from_obj(obj);

    for (auto *obj_content : obj->contains)
        extract_obj(obj_content);

    if (!object_list.remove(obj)) {
        bug("Extract_obj: obj {} not found.", obj->objIndex->vnum);
        return;
    }

    --obj->objIndex->count;
    delete obj;
}

std::vector<std::unique_ptr<Char>> chars_to_reap;
void reap_old_chars() { chars_to_reap.clear(); }

/*
 * Extract a char from the world.
 */
void extract_char(Char *ch, bool delete_from_world) {
    if (ch->in_room == nullptr) {
        bug("Extract_char: nullptr.");
        return;
    }

    nuke_pets(ch);
    ch->pet = nullptr; /* just in case */

    if (ch->ridden_by != nullptr) {
        thrown_off(ch->ridden_by, ch);
    }

    if (delete_from_world)
        die_follower(ch);

    stop_fighting(ch, true);

    for (auto *obj : ch->carrying)
        extract_obj(obj);

    char_from_room(ch);

    if (!delete_from_world) {
        char_to_room(ch, get_room(Rooms::MidgaardAltar));
        return;
    }

    if (ch->is_npc())
        --ch->mobIndex->count;

    if (ch->desc != nullptr && ch->desc->is_switched()) {
        do_return(ch);
    }

    for (auto *wch : char_list)
        if (wch->reply == ch)
            wch->reply = nullptr;

    if (!char_list.remove(ch)) {
        bug("Extract_char: char not found.");
        return;
    }

    if (ch->desc)
        ch->desc->character(nullptr);
    chars_to_reap.emplace_back(ch);
}

// Find a char in the room.
Char *get_char_room(Char *ch, std::string_view argument) {
    auto &&[number, arg] = number_argument(argument);
    if (matches(arg, "self"))
        return ch;
    int count = 0;
    for (auto *rch : ch->in_room->people) {
        if (!can_see(ch, rch) || !is_name(arg, rch->name))
            continue;
        if (++count == number)
            return rch;
    }

    return nullptr;
}

// Find a char in the world.
Char *get_char_world(Char *ch, std::string_view argument) {
    if (auto *wch = get_char_room(ch, argument))
        return wch;

    auto &&[number, arg] = number_argument(argument);
    int count = 0;
    for (auto *wch : char_list) {
        if (!wch->in_room || !ch->can_see(*wch) || !is_name(arg, wch->name))
            continue;
        if (++count == number)
            return wch;
    }

    return nullptr;
}

/* find a MOB by vnum in the world, returning its Char * */
Char *get_mob_by_vnum(sh_int vnum) {
    for (auto *current : char_list)
        if (current->mobIndex && current->mobIndex->vnum == vnum)
            return current;

    return nullptr;
}

/*
 * Find some object with a given index data.
 * Used by area-reset 'P' command.
 */
Object *get_obj_type(ObjectIndex *objIndex) {
    for (auto *obj : object_list)
        if (obj->objIndex == objIndex)
            return obj;

    return nullptr;
}

/*
 * Find an obj in a list.
 */
Object *get_obj_list(const Char *ch, std::string_view argument, GenericList<Object *> &list) {
    auto &&[number, arg] = number_argument(argument);
    int count = 0;
    for (auto *obj : list) {
        if (can_see_obj(ch, obj) && is_name(arg, obj->name)) {
            if (++count == number)
                return obj;
        }
    }

    return nullptr;
}

/*
 * Find an obj in the room or in inventory.
 */
Object *get_obj_here(const Char *ch, std::string_view argument) {
    if (auto *obj = get_obj_list(ch, argument, ch->in_room->contents))
        return obj;

    if (auto *obj = ch->find_in_inventory(argument))
        return obj;

    if (auto *obj = ch->find_worn(argument))
        return obj;

    return nullptr;
}

/* Written by Wandera & Death */
Object *get_object(sh_int vnum) {
    if (ObjectIndex *objIndex = get_obj_index(vnum); objIndex != nullptr) {
        return create_object(objIndex);
    }
    return nullptr;
}

/*
 * Find an obj in the world.
 */
Object *get_obj_world(Char *ch, std::string_view argument) {
    if (auto *obj = get_obj_here(ch, argument))
        return obj;

    auto &&[number, arg] = number_argument(argument);
    int count = 0;
    for (auto *obj : object_list) {
        if (can_see_obj(ch, obj) && is_name(arg, obj->name)) {
            if (++count == number)
                return obj;
        }
    }

    return nullptr;
}

/*
 * Create a 'money' obj.
 */
Object *create_money(int amount) {
    if (amount <= 0) {
        bug("Create_money: zero or negative money {}.", amount);
        amount = 1;
    }

    if (amount == 1) {
        return create_object(get_obj_index(Objects::MoneyOne));
    }

    auto *obj = create_object(get_obj_index(Objects::MoneySome));
    obj->short_descr = fmt::sprintf(obj->short_descr, amount);
    obj->value[0] = amount;
    obj->cost = amount;

    return obj;
}

/*
 * Return # of objects which an object counts as.
 * Thanks to Tony Chamberlain for the correct recursive code here.
 */
int get_obj_number(Object *obj) {
    int number;

    if (obj->type == ObjectType::Container || obj->type == ObjectType::Money)
        number = 0;
    else
        number = 1;

    for (auto *content : obj->contains)
        number += get_obj_number(content);

    return number;
}

/*
 * Return weight of an object, including weight of contents.
 */
int get_obj_weight(Object *obj) {
    int weight;

    weight = obj->weight;
    for (auto *content : obj->contains)
        weight += get_obj_weight(content);

    return weight;
}

/*
 * True if room is dark.
 */
bool room_is_dark(Room *room) {
    if (room->light > 0)
        return false;

    if (check_enum_bit(room->room_flags, RoomFlag::Dark))
        return true;

    if (room->sector_type == SectorType::Inside || room->sector_type == SectorType::City)
        return false;

    if (weather_info.is_dark())
        return true;

    return false;
}

/*
 * True if room is private.
 */
bool room_is_private(Room *room) {
    auto count = ranges::distance(room->people);

    if (check_enum_bit(room->room_flags, RoomFlag::Private) && count >= 2)
        return true;

    if (check_enum_bit(room->room_flags, RoomFlag::Solitary) && count >= 1)
        return true;

    if (check_enum_bit(room->room_flags, RoomFlag::ImpOnly))
        return true;

    return false;
}

/* visibility on a room -- for entering and exits */
bool can_see_room(const Char *ch, const Room *room) {
    // TODO remove
    return ch->can_see(*room);
}

bool can_see(const Char *ch, const Char *victim) {
    /* without this block, poor code involving descriptors would
       crash the mud - Fara 13/8/96 */
    // MRG notes, not seen in any logs thus far, candidate for deletion.
    if (victim == nullptr) {
        bug("can_see: victim is nullptr");
        return false;
    }
    return ch->can_see(*victim);
}

/*
 * True if char can see obj.
 */
bool can_see_obj(const Char *ch, const Object *obj) {
    // TODO remove
    return ch->can_see(*obj);
}

/*
 * True if char can drop obj.
 */
bool can_drop_obj(Char *ch, Object *obj) {
    if (!check_enum_bit(obj->extra_flags, ObjectExtraFlag::NoDrop))
        return true;

    if (ch->is_pc() && ch->level >= LEVEL_IMMORTAL)
        return true;

    return false;
}
