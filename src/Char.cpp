/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "Char.hpp"
#include "AffectFlag.hpp"
#include "ArmourClass.hpp"
#include "BodyPartFlag.hpp"
#include "CharActFlag.hpp"
#include "Classes.hpp"
#include "CommFlag.hpp"
#include "DescriptorList.hpp"
#include "FlagFormat.hpp"
#include "Logging.hpp"
#include "MProg.hpp"
#include "MorphologyFlag.hpp"
#include "Note.hpp"
#include "Object.hpp"
#include "ObjectExtraFlag.hpp"
#include "ObjectIndex.hpp"
#include "ObjectType.hpp"
#include "OffensiveFlag.hpp"
#include "PlayerActFlag.hpp"
#include "Races.hpp"
#include "RoomFlag.hpp"
#include "Sex.hpp"
#include "SkillNumbers.hpp"
#include "TimeInfoData.hpp"
#include "ToleranceFlag.hpp"
#include "Wear.hpp"
#include "act_comm.hpp"
#include "act_obj.hpp"
#include "act_wiz.hpp"
#include "comm.hpp"
#include "common/BitOps.hpp"
#include "db.h"
#include "handler.hpp"
#include "interp.h"
#include "skills.hpp"
#include "string_utils.hpp"

#include <chat/chatlink.h>
#include <fmt/format.h>
#include <range/v3/algorithm/contains.hpp>
#include <range/v3/algorithm/count_if.hpp>
#include <range/v3/algorithm/fill.hpp>
#include <range/v3/view/filter.hpp>
#include <range/v3/view/iota.hpp>
#include <range/v3/view/transform.hpp>

Seconds Char::total_played() const { return std::chrono::duration_cast<Seconds>(current_time - logon + played); }

bool Char::is_npc() const { return check_enum_bit(act, CharActFlag::Npc); }
bool Char::is_warrior() const { return check_enum_bit(act, CharActFlag::Warrior); }
bool Char::is_thief() const { return check_enum_bit(act, CharActFlag::Thief); }
bool Char::is_shopkeeper() const { return is_npc() && mobIndex->shop; }

// Affected by spell bit checks.
bool Char::is_aff_berserk() const { return check_enum_bit(affected_by, AffectFlag::Berserk); }
bool Char::is_aff_blind() const { return check_enum_bit(affected_by, AffectFlag::Blind); }
bool Char::is_aff_calm() const { return check_enum_bit(affected_by, AffectFlag::Calm); }
bool Char::is_aff_charm() const { return check_enum_bit(affected_by, AffectFlag::Charm); }
bool Char::is_aff_curse() const { return check_enum_bit(affected_by, AffectFlag::Curse); }
bool Char::is_aff_detect_evil() const { return check_enum_bit(affected_by, AffectFlag::DetectEvil); }
bool Char::is_aff_detect_magic() const { return check_enum_bit(affected_by, AffectFlag::DetectMagic); }
bool Char::is_aff_detect_hidden() const { return check_enum_bit(affected_by, AffectFlag::DetectHidden); }
bool Char::is_aff_detect_invis() const { return check_enum_bit(affected_by, AffectFlag::DetectInvis); }
bool Char::is_aff_faerie_fire() const { return check_enum_bit(affected_by, AffectFlag::FaerieFire); }
bool Char::is_aff_fly() const { return check_enum_bit(affected_by, AffectFlag::Flying); }
bool Char::is_aff_haste() const { return check_enum_bit(affected_by, AffectFlag::Haste); }
bool Char::is_aff_hide() const { return check_enum_bit(affected_by, AffectFlag::Hide); }
bool Char::is_aff_invisible() const { return check_enum_bit(affected_by, AffectFlag::Invisible); }
bool Char::is_aff_infrared() const { return check_enum_bit(affected_by, AffectFlag::Infrared); }
bool Char::is_aff_lethargy() const { return check_enum_bit(affected_by, AffectFlag::Lethargy); }
bool Char::is_aff_octarine_fire() const { return check_enum_bit(affected_by, AffectFlag::OctarineFire); }
bool Char::is_aff_pass_door() const { return check_enum_bit(affected_by, AffectFlag::PassDoor); }
bool Char::is_aff_plague() const { return check_enum_bit(affected_by, AffectFlag::Plague); }
bool Char::is_aff_poison() const { return check_enum_bit(affected_by, AffectFlag::Poison); }
bool Char::is_aff_protection_evil() const { return check_enum_bit(affected_by, AffectFlag::ProtectionEvil); }
bool Char::is_aff_protection_good() const { return check_enum_bit(affected_by, AffectFlag::ProtectionGood); }
bool Char::is_aff_regeneration() const { return check_enum_bit(affected_by, AffectFlag::Regeneration); }
bool Char::is_aff_sanctuary() const { return check_enum_bit(affected_by, AffectFlag::Sanctuary); }
bool Char::is_aff_sneak() const { return check_enum_bit(affected_by, AffectFlag::Sneak); }
bool Char::is_aff_sleep() const { return check_enum_bit(affected_by, AffectFlag::Sleep); }
bool Char::is_aff_talon() const { return check_enum_bit(affected_by, AffectFlag::Talon); }

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

bool Char::has_holylight() const { return is_pc() && check_enum_bit(act, PlayerActFlag::PlrHolyLight); }
bool Char::is_wizinvis() const { return is_pc() && check_enum_bit(act, PlayerActFlag::PlrWizInvis); }
bool Char::is_wizinvis_to(const Char &victim) const { return is_wizinvis() && victim.get_trust() < invis_level; }

bool Char::is_prowlinvis() const { return is_pc() && check_enum_bit(act, PlayerActFlag::PlrProwl); }
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
    if (is_aff_blind())
        return false;

    // If it's dark, and you haven't got infrared, you can't see that person.
    if (room_is_dark(in_room) && !is_aff_infrared())
        return false;

    // Check invisibility.
    if (victim.is_aff_invisible() && !is_aff_detect_invis())
        return false;

    // Sneak and hide work only for PCs sneaking/hiding against NPCs and vice versa, and for when the victim is not in a
    // fight.
    if (!victim.fighting && victim.is_npc() != is_npc()) {

        if (victim.is_aff_sneak() && !is_aff_detect_invis()) {
            // Sneak is skill based.
            auto chance = victim.get_skill(gsn_sneak);
            chance += curr_stat(Stat::Dex) * 3 / 2;
            chance -= curr_stat(Stat::Int) * 2;
            chance += level - victim.level * 3 / 2;

            if (number_percent() < chance)
                return false;
        }

        if (victim.is_aff_hide() && !is_aff_detect_hidden())
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
    return std::clamp(static_cast<sh_int>(perm_stat[stat] + mod_stat[stat]), 3_s, max_stat(stat));
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

bool Char::has_affect_bit(int affect_bit) const { return check_bit(affected_by, affect_bit); }

bool Char::is_outside() const { return in_room && in_room->is_outside(); }

bool Char::is_inside() const { return in_room && in_room->is_inside(); }

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

        else if (skill_number == gsn_backstab && check_enum_bit(off_flags, OffensiveFlag::Backstab))
            skill = std::max(30, level / 3);

        else if (skill_number == gsn_hand_to_hand)
            skill = 40 + 2 * level;

        else if (skill_number == gsn_trip && check_enum_bit(off_flags, OffensiveFlag::Trip))
            skill = std::max(30, level / 3);

        else if (skill_number == gsn_bash && check_enum_bit(off_flags, OffensiveFlag::Bash))
            skill = std::max(30, level / 3);

        else if (skill_number == gsn_disarm
                 && (check_enum_bit(off_flags, OffensiveFlag::Disarm) || is_warrior() || is_thief()))
            skill = std::max(30, level / 3);

        else if (skill_number == gsn_berserk && check_enum_bit(off_flags, OffensiveFlag::Berserk))
            skill = std::max(30, level / 3);

        else if (skill_number == gsn_sword || skill_number == gsn_dagger || skill_number == gsn_spear
                 || skill_number == gsn_mace || skill_number == gsn_axe || skill_number == gsn_flail
                 || skill_number == gsn_whip || skill_number == gsn_polearm)
            skill = 40 + 5 * level / 2;

        else
            skill = 0;
    }

    if (is_aff_berserk())
        skill /= 1.5;

    if (is_affected_by(gsn_insanity))
        skill -= 10;

    return std::clamp(skill, 0, 100);
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

bool Char::is_comm_brief() const { return is_pc() && check_enum_bit(comm, CommFlag::Brief); }
bool Char::should_autoexit() const { return is_pc() && check_enum_bit(act, PlayerActFlag::PlrAutoExit); }

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
    return find_filtered_obj(argument, [](const Object &obj) { return obj.wear_loc == Wear::None; });
}

Object *Char::find_worn(std::string_view argument) const {
    return find_filtered_obj(argument, [](const Object &obj) { return obj.wear_loc != Wear::None; });
}

bool Char::can_see(const Object &object) const {
    if (has_holylight())
        return true;

    if (check_enum_bit(object.extra_flags, ObjectExtraFlag::VisDeath))
        return false;

    if (is_aff_blind() && object.type != ObjectType::Potion)
        return false;

    if (object.type == ObjectType::Light && object.value[2] != 0)
        return true;

    if (check_enum_bit(object.extra_flags, ObjectExtraFlag::Invis) && !is_aff_detect_invis())
        return false;

    if (check_enum_bit(object.extra_flags, ObjectExtraFlag::Glow))
        return true;

    if (room_is_dark(in_room) && !is_aff_infrared())
        return false;

    return true;
}

bool Char::can_see(const Room &room) const {
    if (check_enum_bit(room.room_flags, RoomFlag::ImpOnly) && get_trust() < MAX_LEVEL)
        return false;

    if (check_enum_bit(room.room_flags, RoomFlag::GodsOnly) && !is_immortal())
        return false;

    if (check_enum_bit(room.room_flags, RoomFlag::HeroesOnly) && !is_hero())
        return false;

    if (check_enum_bit(room.room_flags, RoomFlag::NewbiesOnly) && level > 5 && !is_immortal())
        return false;

    return true;
}

int Char::num_active_ = 0;

Char::Char()
    : name{}, logon(current_time), position(Position::Type::Standing) /*todo once not in merc.h put in header*/ {
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
        if (!check_enum_bit(victim.comm, CommFlag::Quiet))
            ::act("|W$n yells '$t|W'|w", this, exclamation, &victim, To::Vict);
    }
}

void Char::say(std::string_view message) {
    ::act("$n|w says '$T|w'", this, nullptr, message, To::Room);
    ::act("You say '$T|w'", this, nullptr, message, To::Char);
    chatperformtoroom(message, this);
    MProg::speech_trigger(message, this);
}

bool Char::is_player_killer() const noexcept { return is_pc() && check_enum_bit(act, PlayerActFlag::PlrKiller); }
bool Char::is_player_thief() const noexcept { return is_pc() && check_enum_bit(act, PlayerActFlag::PlrThief); }

bool Char::is_set_extra(const CharExtraFlag flag) const noexcept {
    const auto index = to_int(flag);
    return is_pc() && extra_flags[index / 32u] & (1u << (index & 31u));
}

void Char::set_extra(const CharExtraFlag flag) noexcept {
    if (is_npc())
        return;
    const auto index = to_int(flag);
    extra_flags[index / 32u] |= (1u << (index & 31u));
}

bool Char::toggle_extra(const CharExtraFlag flag) noexcept {
    if (is_set_extra(flag)) {
        remove_extra(flag);
        return false;
    }
    set_extra(flag);
    return true;
}

void Char::remove_extra(const CharExtraFlag flag) noexcept {
    if (is_npc())
        return;
    extra_flags[to_int(flag) / 32] &= ~(1u << (to_int(flag) & 31u));
}

int Char::get_hitroll() const noexcept { return hitroll + str_app[curr_stat(Stat::Str)].tohit; }
int Char::get_damroll() const noexcept { return damroll + str_app[curr_stat(Stat::Str)].todam; }

void Char::set_not_afk() {
    if (!check_enum_bit(act, PlayerActFlag::PlrAfk))
        return;
    send_line("|cYour keyboard welcomes you back!|w");
    send_line("|cYou are no longer marked as being afk.|w");
    ::act("|W$n's|w keyboard has welcomed $m back!", this, nullptr, nullptr, To::Room, MobTrig::Yes,
          Position::Type::Dead);
    ::act("|W$n|w is no longer afk.", this, nullptr, nullptr, To::Room, MobTrig::Yes, Position::Type::Dead);
    announce("|W###|w (|cAFK|w) $N has returned to $S keyboard.", this);
    clear_enum_bit(act, PlayerActFlag::PlrAfk);
}

void Char::set_afk(std::string_view afk_message) {
    pcdata->afk = afk_message;
    set_enum_bit(act, PlayerActFlag::PlrAfk);
    ::act(fmt::format("|cYou notify the mud that you are {}|c.|w", afk_message), this, nullptr, nullptr, To::Char,
          MobTrig::Yes, Position::Type::Dead);
    ::act(fmt::format("|W$n|w is {}|w.", afk_message), this, nullptr, nullptr, To::Room, MobTrig::Yes,
          Position::Type::Dead);
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

void Char::try_give_item_to(Object *object, Char *victim) {
    if (object->wear_loc != Wear::None) {
        send_line("You must remove it first.");
        return;
    }

    if (!can_drop_obj(this, object)) {
        send_line("You can't let go of it.");
        return;
    }

    if (obj_move_violates_uniqueness(this, victim, object, victim->carrying)) {
        ::act(deity_name + " has forbidden $N from possessing more than one $p.", this, object, victim, To::Char);
        return;
    }

    if (victim->carry_number + get_obj_number(object) > can_carry_n(victim)) {
        ::act("$N has $S hands full.", this, nullptr, victim, To::Char);
        return;
    }

    if (victim->carry_weight + get_obj_weight(object) > can_carry_w(victim)) {
        ::act("$N can't carry that much weight.", this, nullptr, victim, To::Char);
        return;
    }

    if (!can_see_obj(victim, object)) {
        ::act("$N can't see it.", this, nullptr, victim, To::Char);
        return;
    }

    obj_from_char(object);
    obj_to_char(object, victim);

    ::act("$n gives $p to $N.", this, object, victim, To::NotVict, MobTrig::No);
    ::act("$n gives you $p.", this, object, victim, To::Vict, MobTrig::No);
    ::act("You give $p to $N.", this, object, victim, To::Char, MobTrig::No);

    handle_corpse_summoner(this, victim, object);
    MProg::give_trigger(victim, this, object);
}

std::string_view Char::describe(const Char &to_describe) const noexcept {
    if (!can_see(to_describe))
        return "someone"sv;
    return to_describe.is_npc() ? to_describe.short_descr : to_describe.name;
}

std::string Char::format_extra_flags() const noexcept {
    namespace rv = ranges::views;
    const auto is_set = [this](const auto i) { return is_set_extra(i); };
    const auto to_name = [](const auto i) { return CharExtraFlags::AllExtraFlags[to_int(i)]; };
    const auto not_empty = [](const auto name) { return !name.empty(); };
    auto names =
        magic_enum::enum_values<CharExtraFlag>() | rv::filter(is_set) | rv::transform(to_name) | rv::filter(not_empty);
    return fmt::format("{}", fmt::join(names, " "));
}

std::string Char::serialize_extra_flags() const noexcept {
    namespace rv = ranges::views;
    const auto to_binary = [this](const auto extra_flag) { return is_set_extra(extra_flag) ? "1" : "0"; };
    return fmt::format("{}", fmt::join(magic_enum::enum_values<CharExtraFlag>() | rv::transform(to_binary), ""));
}

std::string Char::format_affect_flags() const noexcept { return AffectFlags::format(affected_by); };

std::string Char::format_act_flags() const noexcept {
    return is_npc() ? format_set_flags(CharActFlags::AllCharActFlags, act)
                    : format_set_flags(PlayerActFlags::AllPlayerActFlags, act);
}

std::string Char::format_comm_flags() const noexcept { return format_set_flags(CommFlags::AllCommFlags, comm); };

std::string Char::format_offensive_flags() const noexcept {
    return format_set_flags(OffensiveFlags::AllOffensiveFlags, off_flags);
}

std::string Char::format_immune_flags() const noexcept {
    return format_set_flags(ToleranceFlags::AllToleranceFlags, imm_flags);
}

std::string Char::format_resist_flags() const noexcept {
    return format_set_flags(ToleranceFlags::AllToleranceFlags, res_flags);
}

std::string Char::format_vuln_flags() const noexcept {
    return format_set_flags(ToleranceFlags::AllToleranceFlags, vuln_flags);
}

std::string Char::format_morphology_flags() const noexcept {
    return format_set_flags(MorphologyFlags::AllMorphologyFlags, morphology);
}

std::string Char::format_body_part_flags() const noexcept {
    return format_set_flags(BodyPartFlags::AllBodyPartFlags, parts);
}

void Char::apply_skill_points(const int skill_num, const sh_int change) {
    if (is_pc()) {
        pcdata->learned[skill_num] = std::clamp(static_cast<sh_int>(pcdata->learned[skill_num] + change), 1_s, 100_s);
    }
}
