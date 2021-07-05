/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "Char.hpp"
#include "ArmourClass.hpp"
#include "BitsAffect.hpp"
#include "BitsCharAct.hpp"
#include "BitsCharOffensive.hpp"
#include "BitsObjectExtra.hpp"
#include "BitsRoomState.hpp"
#include "Classes.hpp"
#include "DescriptorList.hpp"
#include "Note.hpp"
#include "Object.hpp"
#include "ObjectIndex.hpp"
#include "ObjectType.hpp"
#include "Races.hpp"
#include "Sex.hpp"
#include "SkillNumbers.hpp"
#include "TimeInfoData.hpp"
#include "WearLocation.hpp"
#include "act_comm.hpp"
#include "comm.hpp"
#include "common/urange.hpp"
#include "db.h"
#include "handler.hpp"
#include "interp.h"
#include "merc.h"
#include "mob_prog.hpp"
#include "skills.hpp"
#include "string_utils.hpp"

#include <chat/chatlink.h>
#include <fmt/format.h>
#include <range/v3/algorithm/contains.hpp>
#include <range/v3/algorithm/count_if.hpp>
#include <range/v3/algorithm/fill.hpp>
#include <range/v3/view/transform.hpp>

Seconds Char::total_played() const { return std::chrono::duration_cast<Seconds>(current_time - logon + played); }

bool Char::is_npc() const { return check_bit(act, ACT_IS_NPC); }
bool Char::is_warrior() const { return check_bit(act, ACT_WARRIOR); }
bool Char::is_thief() const { return check_bit(act, ACT_THIEF); }
bool Char::is_blind() const { return check_bit(affected_by, AFF_BLIND); }
bool Char::has_infrared() const { return check_bit(affected_by, AFF_INFRARED); }
bool Char::is_invisible() const { return check_bit(affected_by, AFF_INVISIBLE); }
bool Char::has_detect_invis() const { return check_bit(affected_by, AFF_DETECT_INVIS); }
bool Char::is_sneaking() const { return check_bit(affected_by, AFF_SNEAK); }
bool Char::is_hiding() const { return check_bit(affected_by, AFF_HIDE); }
bool Char::is_berserk() const { return check_bit(affected_by, AFF_BERSERK); }
bool Char::is_shopkeeper() const { return is_npc() && pIndexData->shop; }
bool Char::has_detect_hidden() const { return check_bit(affected_by, AFF_DETECT_HIDDEN); }
bool Char::has_holylight() const { return is_pc() && check_bit(act, PLR_HOLYLIGHT); }

bool Char::is_pos_dead() const { return position == Position::Type::Dead; }
bool Char::is_pos_dying() const { return position < Position::Type::Stunned; }
bool Char::is_pos_stunned_or_dying() const { return position < Position::Type::Sleeping; }
bool Char::is_pos_sleeping() const { return position == Position::Type::Sleeping; }
bool Char::is_pos_relaxing() const {
    return position == Position::Type::Sleeping || position == Position::Type::Resting
           || position == Position::Type::Sitting;
}
bool Char::is_pos_awake() const { return position > Position::Type::Sleeping; }
bool Char::is_pos_fighting() const { return position == Position::Type::Fighting; }
bool Char::is_pos_preoccupied() const { return position < Position::Type::Standing; }
bool Char::is_pos_standing() const { return position == Position::Type::Standing; }

bool Char::is_wizinvis() const { return is_pc() && check_bit(act, PLR_WIZINVIS); }
bool Char::is_wizinvis_to(const Char &victim) const { return is_wizinvis() && victim.get_trust() < invis_level; }

bool Char::is_prowlinvis() const { return is_pc() && check_bit(act, PLR_PROWL); }
bool Char::is_prowlinvis_to(const Char &victim) const {
    return is_prowlinvis() && in_room != victim.in_room && victim.get_trust() < invis_level;
}

bool Char::is_immortal() const { return get_trust() >= LEVEL_IMMORTAL; }
bool Char::is_hero() const { return get_trust() >= LEVEL_HERO; }

// Retrieve a character's trusted level for permission checking.
int Char::get_trust() const {
    if (desc != nullptr && desc->is_switched() && this != desc->original())
        return desc->original()->get_trust();

    if (trust)
        return trust;

    if (is_npc() && level >= LEVEL_HERO)
        return LEVEL_HERO - 1;

    return level;
}

// True if char can see victim.
bool Char::can_see(const Char &victim) const {
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

void Char::send_to(std::string_view txt) const {
    if (txt.empty() || desc == nullptr || !desc->person())
        return;
    desc->write(colourise_mud_string(desc->person()->pcdata->colour, txt));
}

sh_int Char::curr_stat(Stat stat) const {
    return urange(3_s, static_cast<sh_int>(perm_stat[stat] + mod_stat[stat]), max_stat(stat));
}

sh_int Char::max_stat(Stat stat) const {
    if (is_npc() || level > LEVEL_IMMORTAL)
        return MaxStatValue;

    return std::min(pc_race_table[race].max_stats[stat] + (class_table[class_num].attr_prime == stat ? 6 : 4),
                    MaxStatValue);
}

sh_int Char::get_armour_class(const ArmourClass ac_slot) const {
    return armor[magic_enum::enum_integer<ArmourClass>(ac_slot)]
           + (is_pos_awake() ? dex_app[curr_stat(Stat::Dex)].defensive : 0);
}

bool Char::is_affected_by(int skill_number) const { return affected.find_by_skill(skill_number); }

int Char::get_skill(int skill_number) const {
    int skill;

    if (skill_number < -1 || skill_number > MAX_SKILL) {
        bug("Bad sn {} in get_skill.", skill_number);
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

        else if (skill_number == gsn_recall)
            skill = 40 + level;

        else if (skill_number == gsn_second_attack && (is_warrior() || is_thief()))
            // i.e. level 100 NPC: 50% skill (and thus chance) of 2nd attack happening.
            skill = std::max(25, level / 2);

        else if (skill_number == gsn_third_attack && is_warrior())
            // i.e. a level 100 NPC only has a 70% skill in 3rd attack (minus other
            // modifiers like Berserk), which means a 70% chance of it happening,
            // and only if their 2nd attack succeeded.
            skill = level - 30;

        else if (skill_number == gsn_parry || skill_number == gsn_dodge || skill_number == gsn_shield_block)
            // Defensive melee skills: Warrior/Thief NPCs are a little better at them than Mage/Cleric.
            skill = 15 + (level / (is_warrior() || is_thief() ? 2 : 3));

        else if (skill_number == gsn_backstab && check_bit(off_flags, OFF_BACKSTAB))
            skill = std::max(30, level / 3);

        else if (skill_number == gsn_hand_to_hand)
            skill = 40 + 2 * level;

        else if (skill_number == gsn_trip && check_bit(off_flags, OFF_TRIP))
            skill = std::max(30, level / 3);

        else if (skill_number == gsn_bash && check_bit(off_flags, OFF_BASH))
            skill = std::max(30, level / 3);

        else if (skill_number == gsn_disarm && (check_bit(off_flags, OFF_DISARM) || is_warrior() || is_thief()))
            skill = std::max(30, level / 3);

        else if (skill_number == gsn_berserk && check_bit(off_flags, OFF_BERSERK))
            skill = std::max(30, level / 3);

        else if (skill_number == gsn_sword || skill_number == gsn_dagger || skill_number == gsn_spear
                 || skill_number == gsn_mace || skill_number == gsn_axe || skill_number == gsn_flail
                 || skill_number == gsn_whip || skill_number == gsn_polearm)
            skill = 40 + 5 * level / 2;

        else
            skill = 0;
    }

    if (is_berserk())
        skill /= 1.5;

    if (is_affected_by(gsn_insanity))
        skill -= 10;

    return urange(0, skill, 100);
}

void Char::set_title(std::string title) {
    if (is_npc()) {
        bug("set_title: NPC.");
        return;
    }

    if (!title.empty() && !ispunct(title[0]))
        pcdata->title = fmt::format(" {}", title);
    else
        pcdata->title = std::move(title);
}

void Char::page_to(std::string_view txt) const {
    if (desc)
        desc->page_to(txt);
}

void Char::wait_state(const sh_int npulse) { wait = std::max(wait, npulse); }

PcClan *Char::pc_clan() { return is_pc() && pcdata->pcclan ? &pcdata->pcclan.value() : nullptr; }
const PcClan *Char::pc_clan() const { return is_pc() && pcdata->pcclan ? &pcdata->pcclan.value() : nullptr; }

const Clan *Char::clan() const { return pc_clan() ? &pc_clan()->clan : nullptr; }

bool Char::is_comm_brief() const { return is_pc() && check_bit(comm, COMM_BRIEF); }
bool Char::should_autoexit() const { return is_pc() && check_bit(act, PLR_AUTOEXIT); }

template <typename Func>
Object *Char::find_filtered_obj(std::string_view argument, Func filter) const {
    auto &&[number, arg] = number_argument(argument);
    int count = 0;
    for (auto *obj : carrying) {
        if (can_see(*obj) && is_name(arg, obj->name) && filter(*obj)) {
            if (++count == number)
                return obj;
        }
    }

    return nullptr;
}

Object *Char::find_in_inventory(std::string_view argument) const {
    return find_filtered_obj(argument, [](const Object &obj) { return obj.wear_loc == WEAR_NONE; });
}

Object *Char::find_worn(std::string_view argument) const {
    return find_filtered_obj(argument, [](const Object &obj) { return obj.wear_loc != WEAR_NONE; });
}

bool Char::can_see(const Object &object) const {
    if (has_holylight())
        return true;

    if (check_bit(object.extra_flags, ITEM_VIS_DEATH))
        return false;

    if (is_blind() && object.type != ObjectType::Potion)
        return false;

    if (object.type == ObjectType::Light && object.value[2] != 0)
        return true;

    if (check_bit(object.extra_flags, ITEM_INVIS) && !has_detect_invis())
        return false;

    if (check_bit(object.extra_flags, ITEM_GLOW))
        return true;

    if (room_is_dark(in_room) && !has_infrared())
        return false;

    return true;
}

bool Char::can_see(const Room &room) const {
    if (check_bit(room.room_flags, ROOM_IMP_ONLY) && get_trust() < MAX_LEVEL)
        return false;

    if (check_bit(room.room_flags, ROOM_GODS_ONLY) && !is_immortal())
        return false;

    if (check_bit(room.room_flags, ROOM_HEROES_ONLY) && !is_hero())
        return false;

    if (check_bit(room.room_flags, ROOM_NEWBIES_ONLY) && level > 5 && !is_immortal())
        return false;

    return true;
}

int Char::num_active_ = 0;

extern char str_empty[]; // Soon, to die...

Char::Char()
    : name(str_empty), logon(current_time),
      position(Position::Type::Standing) /*todo once not in merc.h put in header*/ {
    ranges::fill(armor, -1); // #216 -1 armour is the new normal
    ranges::fill(perm_stat, 13);
    ++num_active_;
}

Char::~Char() {
    --num_active_;

    for (auto *obj : carrying)
        extract_obj(obj);

    for (auto &af : affected)
        affect_remove(this, af);
}

void Char::yell(std::string_view exclamation) const {
    ::act("|WYou yell '$t|W'|w", this, exclamation, nullptr, To::Char);
    for (auto &victim :
         descriptors().all_but(*this) | DescriptorFilter::same_area(*this) | DescriptorFilter::to_character()) {
        if (!check_bit(victim.comm, COMM_QUIET))
            ::act("|W$n yells '$t|W'|w", this, exclamation, &victim, To::Vict);
    }
}

void Char::say(std::string_view message) {
    ::act("$n|w says '$T|w'", this, nullptr, message, To::Room);
    ::act("You say '$T|w'", this, nullptr, message, To::Char);
    chatperformtoroom(message, this);
    // TODO: one day we will make this take string_views. but for now:
    auto as_std = std::string(message);
    mprog_speech_trigger(as_std.c_str(), this);
}

bool Char::is_player_killer() const noexcept { return is_pc() && check_bit(act, PLR_KILLER); }
bool Char::is_player_thief() const noexcept { return is_pc() && check_bit(act, PLR_THIEF); }
bool Char::has_detect_magic() const { return check_bit(affected_by, AFF_DETECT_MAGIC); }
bool Char::has_detect_evil() const { return check_bit(affected_by, AFF_DETECT_EVIL); }

void Char::set_extra(unsigned int flag) noexcept {
    if (is_npc())
        return;
    extra_flags[flag / 32] |= (1u << (flag & 31u));
}
void Char::remove_extra(unsigned int flag) noexcept {
    if (is_npc())
        return;
    extra_flags[flag / 32] &= ~(1u << (flag & 31u));
}

int Char::get_hitroll() const noexcept { return hitroll + str_app[curr_stat(Stat::Str)].tohit; }
int Char::get_damroll() const noexcept { return damroll + str_app[curr_stat(Stat::Str)].todam; }

void Char::set_not_afk() {
    if (!check_bit(act, PLR_AFK))
        return;
    send_line("|cYour keyboard welcomes you back!|w");
    send_line("|cYou are no longer marked as being afk.|w");
    ::act("|W$n's|w keyboard has welcomed $m back!", this, nullptr, nullptr, To::Room, Position::Type::Dead);
    ::act("|W$n|w is no longer afk.", this, nullptr, nullptr, To::Room, Position::Type::Dead);
    announce("|W###|w (|cAFK|w) $N has returned to $S keyboard.", this);
    clear_bit(act, PLR_AFK);
}

void Char::set_afk(std::string_view afk_message) {
    pcdata->afk = afk_message;
    set_bit(act, PLR_AFK);
    ::act(fmt::format("|cYou notify the mud that you are {}|c.|w", afk_message), this, nullptr, nullptr, To::Char,
          Position::Type::Dead);
    ::act(fmt::format("|W$n|w is {}|w.", afk_message), this, nullptr, nullptr, To::Room, Position::Type::Dead);
    announce(fmt::format("|W###|w (|cAFK|w) $N is {}|w.", afk_message), this);
}

bool Char::has_boat() const noexcept {
    return is_immortal() || ranges::contains(carrying, ObjectType::Boat, &Object::type);
}

bool Char::carrying_object_vnum(int vnum) const noexcept {
    return ranges::contains(carrying | ranges::views::transform(&Object::objIndex), vnum, &ObjectIndex::vnum);
}

size_t Char::num_group_members_in_room() const noexcept {
    return ranges::count_if(in_room->people, [&](auto *gch) { return is_same_group(gch, this); });
}

std::optional<std::string_view> Char::delta_inebriation(const sh_int delta) noexcept {
    return delta_nutrition(
        [this]() -> auto & { return pcdata->inebriation; }, delta);
}

std::optional<std::string_view> Char::delta_hunger(const sh_int delta) noexcept {
    return delta_nutrition(
        [this]() -> auto & { return pcdata->hunger; }, delta);
}

std::optional<std::string_view> Char::delta_thirst(const sh_int delta) noexcept {
    return delta_nutrition(
        [this]() -> auto & { return pcdata->thirst; }, delta);
}

std::optional<std::string_view> Char::delta_nutrition(const auto supplier, const sh_int delta) noexcept {
    // Skip NPCs, they don't have a pcdata and nutrition isn't relevant.
    if (delta == 0 || is_npc() || is_immortal()) {
        return std::nullopt;
    }
    return supplier().apply_delta(delta);
}

std::optional<std::string> Char::describe_nutrition() const {
    if (is_npc()) {
        return std::nullopt;
    }
    const auto drunk = pcdata->inebriation.is_unhealthy();
    const auto hungry = pcdata->hunger.is_unhealthy();
    const auto thirsty = pcdata->thirst.is_unhealthy();
    static std::array delimiters = {"", " and ", ", "};
    if (!drunk && !hungry && !thirsty)
        return std::nullopt;

    const auto message =
        fmt::format("|wYou are |W{}|w{}|W{}|w{}|W{}|w.", drunk ? pcdata->inebriation.unhealthy_adjective() : "",
                    drunk ? delimiters[hungry + thirsty] : "", hungry ? pcdata->hunger.unhealthy_adjective() : "",
                    (thirsty && hungry) ? " and " : "", thirsty ? pcdata->thirst.unhealthy_adjective() : "");
    return message;
}

std::optional<std::string> Char::report_nutrition() const {
    if (is_npc()) {
        return std::nullopt;
    }
    const auto message = fmt::format("Thirst: {}  Hunger: {}  Inebriation: {}", pcdata->thirst.get(),
                                     pcdata->hunger.get(), pcdata->inebriation.get());
    return message;
}

bool Char::is_inebriated() const noexcept { return is_pc() && pcdata->inebriation.is_unhealthy(); }

bool Char::is_hungry() const noexcept { return is_pc() && !pcdata->hunger.is_satisfied(); }

bool Char::is_starving() const noexcept { return is_pc() && pcdata->hunger.is_unhealthy(); }

bool Char::is_thirsty() const noexcept { return is_pc() && !pcdata->thirst.is_satisfied(); }

bool Char::is_parched() const noexcept { return is_pc() && pcdata->thirst.is_unhealthy(); }