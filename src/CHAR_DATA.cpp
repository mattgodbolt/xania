#include "CHAR_DATA.hpp"

#include "TimeInfoData.hpp"
#include "merc.h"
#include "string_utils.hpp"

#include <fmt/format.h>

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
