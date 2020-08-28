#include "CHAR_DATA.hpp"

#include "DescriptorList.hpp"
#include "TimeInfoData.hpp"
#include "comm.hpp"
#include "merc.h"
#include "string_utils.hpp"

#include <chat/chatlink.h>
#include <fmt/format.h>
#include <range/v3/algorithm/fill.hpp>

using namespace fmt::literals;

Seconds CHAR_DATA::total_played() const { return std::chrono::duration_cast<Seconds>(current_time - logon + played); }

bool CHAR_DATA::is_npc() const { return IS_SET(act, ACT_IS_NPC); }
bool CHAR_DATA::is_warrior() const { return IS_SET(act, ACT_WARRIOR); }
bool CHAR_DATA::is_thief() const { return IS_SET(act, ACT_THIEF); }
bool CHAR_DATA::is_blind() const { return IS_SET(affected_by, AFF_BLIND); }
bool CHAR_DATA::has_infrared() const { return IS_SET(affected_by, AFF_INFRARED); }
bool CHAR_DATA::is_invisible() const { return IS_SET(affected_by, AFF_INVISIBLE); }
bool CHAR_DATA::has_detect_invis() const { return IS_SET(affected_by, AFF_DETECT_INVIS); }
bool CHAR_DATA::is_sneaking() const { return IS_SET(affected_by, AFF_SNEAK); }
bool CHAR_DATA::is_hiding() const { return IS_SET(affected_by, AFF_HIDE); }
bool CHAR_DATA::is_berserk() const { return IS_SET(affected_by, AFF_BERSERK); }
bool CHAR_DATA::has_detect_hidden() const { return IS_SET(affected_by, AFF_DETECT_HIDDEN); }
bool CHAR_DATA::has_holylight() const { return is_pc() && IS_SET(act, PLR_HOLYLIGHT); }

bool CHAR_DATA::is_wizinvis() const { return is_pc() && IS_SET(act, PLR_WIZINVIS); }
bool CHAR_DATA::is_wizinvis_to(const CHAR_DATA &victim) const {
    return is_wizinvis() && victim.get_trust() < invis_level;
}

bool CHAR_DATA::is_prowlinvis() const { return is_pc() && IS_SET(act, PLR_PROWL); }
bool CHAR_DATA::is_prowlinvis_to(const CHAR_DATA &victim) const {
    return is_prowlinvis() && in_room != victim.in_room && victim.get_trust() < invis_level;
}

bool CHAR_DATA::is_immortal() const { return get_trust() >= LEVEL_IMMORTAL; }
bool CHAR_DATA::is_hero() const { return get_trust() >= LEVEL_IMMORTAL; }

// Retrieve a character's trusted level for permission checking.
int CHAR_DATA::get_trust() const {
    if (desc != nullptr && desc->is_switched() && this != desc->original())
        return desc->original()->get_trust();

    if (trust)
        return trust;

    if (is_npc() && level >= LEVEL_HERO)
        return LEVEL_HERO - 1;

    return level;
}

// True if char can see victim.
bool CHAR_DATA::can_see(const CHAR_DATA &victim) const {
    // We can always see ourself.
    if (this == &victim)
        return true;

    // First handle the trust-level based stuff, which trumps all.
    if (victim.is_wizinvis_to(*this) || victim.is_prowlinvis_to(*this))
        return false;

    // Holylight means you can see everything.
    if (has_holylight())
        return true;

    // NPCs of immortal level can see everything.
    if (is_npc() && is_immortal())
        return true;

    // If you're blind you can't see anything.
    if (is_blind())
        return false;

    // If it's dark, and you haven't got infrared, you can't see that person.
    if (room_is_dark(in_room) && !has_infrared())
        return false;

    // Check invisibility.
    if (victim.is_invisible() && !has_detect_invis())
        return false;

    // Sneak and hide work only for PCs sneaking/hiding against NPCs and vice versa, and for when the victim is not in a
    // fight.
    if (!victim.fighting && victim.is_npc() != is_npc()) {

        if (victim.is_sneaking() && !has_detect_invis()) {
            // Sneak is skill based.
            auto chance = victim.get_skill(gsn_sneak);
            chance += curr_stat(Stat::Dex) * 3 / 2;
            chance -= curr_stat(Stat::Int) * 2;
            chance += level - victim.level * 3 / 2;

            if (number_percent() < chance)
                return false;
        }

        if (victim.is_hiding() && !has_detect_hidden())
            return false;
    }

    return true;
}

void CHAR_DATA::send_to(std::string_view txt) const {
    if (txt.empty() || desc == nullptr || !desc->person())
        return;
    desc->write(colourise_mud_string(desc->person()->pcdata->colour, txt));
}

sh_int CHAR_DATA::curr_stat(Stat stat) const { return URANGE(3, perm_stat[stat] + mod_stat[stat], max_stat(stat)); }

sh_int CHAR_DATA::max_stat(Stat stat) const {
    if (is_npc() || level > LEVEL_IMMORTAL)
        return 25;

    return UMIN(pc_race_table[race].max_stats[stat] + (class_table[class_num].attr_prime == stat ? 6 : 4), 25);
}

bool CHAR_DATA::is_affected_by(int skill_number) const {
    for (auto paf = affected; paf != nullptr; paf = paf->next)
        if (paf->type == skill_number)
            return true;
    return false;
}

int CHAR_DATA::get_skill(int skill_number) const {
    int skill;

    if (skill_number < -1 || skill_number > MAX_SKILL) {
        bug("Bad sn %d in get_skill.", skill_number);
        return 0;
    }

    if (skill_number == -1) {
        // -1 is shorthand for level based skills
        skill = level * 5 / 2;
    } else if (is_pc()) {
        // Lookup PC skills.
        if (level < get_skill_level(this, skill_number))
            skill = 0;
        else
            skill = pcdata->learned[skill_number];
    } else {
        // Derive mob skills.

        if (skill_number == gsn_sneak)
            skill = level * 2 + 20;

        if (skill_number == gsn_second_attack && (is_warrior() || is_thief()))
            skill = 10 + 3 * level;

        else if (skill_number == gsn_third_attack && is_warrior())
            skill = 4 * level - 40;

        else if (skill_number == gsn_hand_to_hand)
            skill = 40 + 2 * level;

        else if (skill_number == gsn_trip && IS_SET(off_flags, OFF_TRIP))
            skill = 10 + 3 * level;

        else if (skill_number == gsn_bash && IS_SET(off_flags, OFF_BASH))
            skill = 10 + 3 * level;

        else if (skill_number == gsn_disarm && (IS_SET(off_flags, OFF_DISARM) || is_warrior() || is_thief()))
            skill = 20 + 3 * level;

        else if (skill_number == gsn_berserk && IS_SET(off_flags, OFF_BERSERK))
            skill = 3 * level;

        else if (skill_number == gsn_sword || skill_number == gsn_dagger || skill_number == gsn_spear
                 || skill_number == gsn_mace || skill_number == gsn_axe || skill_number == gsn_flail
                 || skill_number == gsn_whip || skill_number == gsn_polearm)
            skill = 40 + 5 * level / 2;

        else
            skill = 0;
    }

    if (is_berserk())
        skill -= level / 2;

    if (is_affected_by(gsn_insanity))
        skill -= 10;

    return URANGE(0, skill, 100);
}

void CHAR_DATA::set_title(std::string title) {
    if (is_npc()) {
        bug("set_title: NPC.");
        return;
    }

    if (!title.empty() && !ispunct(title[0]))
        pcdata->title = " {}"_format(title);
    else
        pcdata->title = std::move(title);
}

void CHAR_DATA::page_to(std::string_view txt) const {
    if (desc)
        desc->page_to(txt);
}

PCCLAN *CHAR_DATA::pc_clan() { return is_pc() && pcdata->pcclan ? &pcdata->pcclan.value() : nullptr; }
const PCCLAN *CHAR_DATA::pc_clan() const { return is_pc() && pcdata->pcclan ? &pcdata->pcclan.value() : nullptr; }

const CLAN *CHAR_DATA::clan() const { return pc_clan() ? pc_clan()->clan : nullptr; }

bool CHAR_DATA::is_comm_brief() const { return is_pc() && IS_SET(comm, COMM_BRIEF); }
bool CHAR_DATA::should_autoexit() const { return is_pc() && IS_SET(act, PLR_AUTOEXIT); }

template <typename Func>
OBJ_DATA *CHAR_DATA::find_filtered_obj(std::string_view argument, Func filter) const {
    auto &&[number, arg] = number_argument(argument);
    int count = 0;
    for (auto *obj = carrying; obj != nullptr; obj = obj->next_content) {
        if (can_see(*obj) && is_name(arg, obj->name) && filter(*obj)) {
            if (++count == number)
                return obj;
        }
    }

    return nullptr;
}

OBJ_DATA *CHAR_DATA::find_in_inventory(std::string_view argument) const {
    return find_filtered_obj(argument, [](const OBJ_DATA &obj) { return obj.wear_loc == WEAR_NONE; });
}

OBJ_DATA *CHAR_DATA::find_worn(std::string_view argument) const {
    return find_filtered_obj(argument, [](const OBJ_DATA &obj) { return obj.wear_loc != WEAR_NONE; });
}

bool CHAR_DATA::can_see(const OBJ_DATA &object) const {
    if (has_holylight())
        return true;

    if (IS_SET(object.extra_flags, ITEM_VIS_DEATH))
        return false;

    if (is_blind() && object.item_type != ITEM_POTION)
        return false;

    if (object.item_type == ITEM_LIGHT && object.value[2] != 0)
        return true;

    if (IS_SET(object.extra_flags, ITEM_INVIS) && !has_detect_invis())
        return false;

    if (IS_SET(object.extra_flags, ITEM_GLOW))
        return true;

    if (room_is_dark(in_room) && !has_infrared())
        return false;

    return true;
}

bool CHAR_DATA::can_see(const ROOM_INDEX_DATA &room) const {
    if (IS_SET(room.room_flags, ROOM_IMP_ONLY) && get_trust() < MAX_LEVEL)
        return false;

    if (IS_SET(room.room_flags, ROOM_GODS_ONLY) && !is_immortal())
        return false;

    if (IS_SET(room.room_flags, ROOM_HEROES_ONLY) && !is_hero())
        return false;

    if (IS_SET(room.room_flags, ROOM_NEWBIES_ONLY) && level > 5 && !is_immortal())
        return false;

    return true;
}

int CHAR_DATA::num_active_ = 0;

extern char str_empty[]; // Soon, to die...

CHAR_DATA::CHAR_DATA()
    : name(str_empty), logon(current_time), position(POS_STANDING) /*todo once not in merc.h put in header*/ {
    ranges::fill(armor, 100);
    ranges::fill(perm_stat, 13);
    ++num_active_;
}

CHAR_DATA::~CHAR_DATA() {
    --num_active_;

    while (carrying)
        extract_obj(carrying);

    while (affected)
        affect_remove(this, affected);
}

void CHAR_DATA::yell(std::string_view exclamation) const {
    ::act("|WYou yell '$t|W'|w", this, exclamation, nullptr, To::Char);
    for (auto &victim :
         descriptors().all_but(*this) | DescriptorFilter::same_area(*this) | DescriptorFilter::to_character()) {
        if (!IS_SET(victim.comm, COMM_QUIET))
            ::act("|W$n yells '$t|W'|w", this, exclamation, &victim, To::Vict);
    }
}

void CHAR_DATA::say(std::string_view message) {
    ::act("$n|w says '$T|w'", this, nullptr, message, To::Room);
    ::act("You say '$T|w'", this, nullptr, message, To::Char);
    chatperformtoroom(message, this);
    // TODO: one day we will make this take string_views. but for now:
    auto as_std = std::string(message);
    /* Merc-2.2 MOBProgs - Faramir 31/8/1998 */
    mprog_speech_trigger(as_std.c_str(), this);
}

bool CHAR_DATA::is_player_killer() const noexcept { return is_pc() && IS_SET(act, PLR_KILLER); }
bool CHAR_DATA::is_player_thief() const noexcept { return is_pc() && IS_SET(act, PLR_THIEF); }
