/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                 */
/*  See the header to file: merc.h for original code copyrights          */
/*************************************************************************/

#include "act_wiz.hpp"
#include "AFFECT_DATA.hpp"
#include "AffectFlag.hpp"
#include "Area.hpp"
#include "ArmourClass.hpp"
#include "Char.hpp"
#include "CharActFlag.hpp"
#include "Classes.hpp"
#include "Columner.hpp"
#include "CommFlag.hpp"
#include "DamageType.hpp"
#include "Descriptor.hpp"
#include "DescriptorList.hpp"
#include "Exit.hpp"
#include "FlagFormat.hpp"
#include "Logging.hpp"
#include "MobIndexData.hpp"
#include "Object.hpp"
#include "ObjectExtraFlag.hpp"
#include "ObjectIndex.hpp"
#include "ObjectType.hpp"
#include "ObjectWearFlag.hpp"
#include "PlayerActFlag.hpp"
#include "PracticeTabulator.hpp"
#include "Races.hpp"
#include "SkillNumbers.hpp"
#include "SkillTables.hpp"
#include "SkillsTabulator.hpp"
#include "TimeInfoData.hpp"
#include "VnumObjects.hpp"
#include "VnumRooms.hpp"
#include "Weapon.hpp"
#include "Wear.hpp"
#include "act_obj.hpp"
#include "challenge.hpp"
#include "comm.hpp"
#include "common/BitOps.hpp"
#include "common/Configuration.hpp"
#include "db.h"
#include "fight.hpp"
#include "handler.hpp"
#include "interp.h"
#include "lookup.h"
#include "magic.h"
#include "mob_prog.hpp"
#include "ride.hpp"
#include "save.hpp"
#include "skills.hpp"
#include "string_utils.hpp"
#include "update.hpp"

#include <fmt/format.h>
#include <magic_enum.hpp>
#include <range/v3/numeric/accumulate.hpp>
#include <range/v3/view/concat.hpp>
#include <range/v3/view/transform.hpp>

// Mutable global: imms can change it using the sacname command.
std::string deity_name = "Etaine";

// Log-all switch.
// Mutable global: imms can toggle it using the log command.
bool fLogAll = false;

void do_mskills(Char *ch, ArgParser args);
void do_maffects(Char *ch, ArgParser args);
void do_mpracs(Char *ch, ArgParser args);
void do_minfo(Char *ch, ArgParser args);
void do_mspells(Char *ch, ArgParser args);
SpecialFunc spec_lookup(const char *name);

/*
 * Local functions.
 */
Room *find_location(Char *ch, std::string_view arg);

/* Permits or denies a player from playing the Mud from a PERMIT banned site */
void do_permit(Char *ch, const char *argument) {
    Char *victim;
    int flag = 1;
    if (ch->is_npc())
        return;
    if (argument[0] == '-') {
        argument++;
        flag = 0;
    }
    if (argument[0] == '+')
        argument++;
    victim = get_char_room(ch, argument);
    if (victim == nullptr || victim->is_npc()) {
        ch->send_line("Permit whom?");
        return;
    }
    if (flag) {
        victim->set_extra(CharExtraFlag::Permit);
    } else {
        victim->remove_extra(CharExtraFlag::Permit);
    }
    ch->send_line("PERMIT flag {} for {}.", flag ? "set" : "removed", victim->name);
}

/* equips a character */
void do_outfit(Char *ch) {
    if (ch->level > 5 || ch->is_npc()) {
        ch->send_line("Find it yourself!");
        return;
    }

    if (!get_eq_char(ch, Wear::Light)) {
        auto *obj = create_object(get_obj_index(Objects::SchoolBanner));
        obj->cost = 0;
        obj_to_char(obj, ch);
        equip_char(ch, obj, Wear::Light);
    }

    if (!get_eq_char(ch, Wear::Body)) {
        auto *obj = create_object(get_obj_index(Objects::SchoolVest));
        obj->cost = 0;
        obj_to_char(obj, ch);
        equip_char(ch, obj, Wear::Body);
    }

    if (!get_eq_char(ch, Wear::Shield)) {
        auto *obj = create_object(get_obj_index(Objects::SchoolShield));
        obj->cost = 0;
        obj_to_char(obj, ch);
        equip_char(ch, obj, Wear::Shield);
    }

    if (!get_eq_char(ch, Wear::Wield)) {
        auto *obj = create_object(get_obj_index(class_table[ch->class_num].weapon));
        obj_to_char(obj, ch);
        equip_char(ch, obj, Wear::Wield);
    }

    ch->send_line("You have been equipped by {}.", deity_name);
}

/* RT nochannels command, for those spammers */
void do_nochannels(Char *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    Char *victim;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        ch->send_line("Nochannel whom?");
        return;
    }

    if ((victim = get_char_world(ch, arg)) == nullptr) {
        ch->send_line("They aren't here.");
        return;
    }

    if (victim->get_trust() >= ch->get_trust()) {
        ch->send_line("You failed.");
        return;
    }

    if (check_enum_bit(victim->comm, CommFlag::NoChannels)) {
        clear_enum_bit(victim->comm, CommFlag::NoChannels);
        victim->send_line("The gods have restored your channel privileges.");
        ch->send_line("NOCHANNELS removed.");
    } else {
        set_enum_bit(victim->comm, CommFlag::NoChannels);
        victim->send_line("The gods have revoked your channel privileges.");
        ch->send_line("NOCHANNELS set.");
    }
}

void do_bamfin(Char *ch, const char *argument) {
    if (ch = ch->player(); !ch)
        return;
    auto bamfin = smash_tilde(argument);

    if (bamfin.empty()) {
        ch->send_line("Your poofin is {}", ch->pcdata->bamfin);
        return;
    }

    if (!matches_inside(ch->name, bamfin)) {
        ch->send_line("You must include your name.");
        return;
    }

    ch->pcdata->bamfin = bamfin;
    ch->send_line("Your poofin is now {}", ch->pcdata->bamfin);
}

void do_bamfout(Char *ch, const char *argument) {
    if (ch = ch->player(); !ch)
        return;
    auto bamfout = smash_tilde(argument);

    if (bamfout.empty()) {
        ch->send_line("Your poofout is {}", ch->pcdata->bamfout);
        return;
    }

    if (!matches_inside(ch->name, bamfout)) {
        ch->send_line("You must include your name.");
        return;
    }

    ch->pcdata->bamfout = bamfout;
    ch->send_line("Your poofout is now {}", ch->pcdata->bamfout);
}

void do_deny(Char *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    Char *victim;

    one_argument(argument, arg);
    if (arg[0] == '\0') {
        ch->send_line("Deny whom?");
        return;
    }

    if ((victim = get_char_world(ch, arg)) == nullptr) {
        ch->send_line("They aren't here.");
        return;
    }

    if (victim->is_npc()) {
        ch->send_line("Not on NPC's.");
        return;
    }

    if (victim->get_trust() >= ch->get_trust()) {
        ch->send_line("You failed.");
        return;
    }

    set_enum_bit(victim->act, PlayerActFlag::PlrDeny);
    victim->send_line("You are denied access!");
    ch->send_line("OK.");
    save_char_obj(victim);
    do_quit(victim);
}

void do_disconnect(Char *ch, const char *argument) {
    char buf[MAX_INPUT_LENGTH];
    one_argument(argument, buf);
    if (buf[0] == '\0') {
        ch->send_line("Usage:");
        ch->send_line("   disconnect <player name>");
        ch->send_line("   disconnect <socket number>");
        return;
    }
    const std::string_view argsv = buf;
    if (is_number(argsv)) {
        const uint32_t channel_num = parse_number(argsv);
        for (auto &d : descriptors().all()) {
            if (d.channel() == channel_num) {
                d.close();
                ch->send_line("Ok.");
                return;
            }
        }
        ch->send_line("Couldn't find a matching descriptor.");
        return;
    } else {
        for (auto &d : descriptors().all()) {
            if (d.character() && matches(d.character()->name, argsv)) {
                d.close();
                ch->send_line("Ok.");
                return;
            }
        }
        ch->send_line("Player not found!");
        return;
    }
}

void do_pardon(Char *ch, const char *argument) {
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    Char *victim;

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if (arg1[0] == '\0' || arg2[0] == '\0') {
        ch->send_line("Syntax: pardon <character> <killer|thief>.");
        return;
    }

    if ((victim = get_char_world(ch, arg1)) == nullptr) {
        ch->send_line("They aren't here.");
        return;
    }

    if (victim->is_npc()) {
        ch->send_line("Not on NPC's.");
        return;
    }

    if (!str_cmp(arg2, "killer")) {
        if (check_enum_bit(victim->act, PlayerActFlag::PlrKiller)) {
            clear_enum_bit(victim->act, PlayerActFlag::PlrKiller);
            ch->send_line("Killer flag removed.");
            victim->send_line("You are no longer a KILLER.");
        }
        return;
    }

    if (!str_cmp(arg2, "thief")) {
        if (check_enum_bit(victim->act, PlayerActFlag::PlrThief)) {
            clear_enum_bit(victim->act, PlayerActFlag::PlrThief);
            ch->send_line("Thief flag removed.");
            victim->send_line("You are no longer a THIEF.");
        }
        return;
    }

    ch->send_line("Syntax: pardon <character> <killer|thief>.");
}

void do_echo(Char *ch, std::string_view argument) {
    if (argument.empty()) {
        ch->send_line("Global echo what?");
        return;
    }

    for (auto &victim : descriptors().playing() | DescriptorFilter::to_character()) {
        victim.send_line("{}{}", victim.get_trust() >= ch->get_trust() ? "global> " : "", argument);
    }
}

void do_recho(Char *ch, const char *argument) {
    if (argument[0] == '\0') {
        ch->send_line("Local echo what?");
        return;
    }

    for (auto &victim : descriptors().playing() | DescriptorFilter::same_room(*ch) | DescriptorFilter::to_character()) {
        victim.send_to(fmt::format(
            "{}{}\n\r",
            victim.get_trust() >= ch->get_trust() && ch->in_room->vnum != Rooms::ChallengeGallery ? "local> " : "",
            argument));
    }
}

void do_zecho(Char *ch, const char *argument) {
    if (argument[0] == '\0') {
        ch->send_line("Zone echo what?");
        return;
    }

    for (auto &victim : descriptors().playing() | DescriptorFilter::same_area(*ch) | DescriptorFilter::to_character()) {
        victim.send_line("{}{}", victim.get_trust() >= ch->get_trust() ? "zone> " : "", argument);
    }
}

void do_pecho(Char *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    Char *victim;

    argument = one_argument(argument, arg);

    if (argument[0] == '\0' || arg[0] == '\0') {
        ch->send_line("Personal echo what?");
        return;
    }

    if ((victim = get_char_world(ch, arg)) == nullptr) {
        ch->send_line("Target not found.");
        return;
    }

    if (victim->get_trust() >= ch->get_trust() && ch->get_trust() != MAX_LEVEL)
        victim->send_to("personal> ");

    victim->send_line("{}", argument);
    ch->send_line("personal> {}", argument);
}

Room *find_location(Char *ch, std::string_view arg) {
    Char *victim;
    Object *obj;

    if (is_number(arg))
        return get_room(parse_number(arg));

    if (matches(arg, "here"))
        return ch->in_room;

    if ((victim = get_char_world(ch, arg)) != nullptr)
        return victim->in_room;

    if ((obj = get_obj_world(ch, arg)) != nullptr)
        return obj->in_room;

    return nullptr;
}

void do_transfer(Char *ch, ArgParser args) {
    if (args.empty()) {
        ch->send_line("Transfer whom (and where)?");
        return;
    }

    auto whom = args.shift();
    auto where = args.shift();
    if (matches(whom, "all")) {
        for (auto &victim :
             descriptors().all_visible_to(*ch) | DescriptorFilter::except(*ch) | DescriptorFilter::to_character()) {
            if (victim.in_room != nullptr)
                do_transfer(ch, ArgParser(fmt::format("{} {}", victim.name, where)));
        }
        return;
    }

    /*
     * Thanks to Grodyn for the optional location parameter.
     */
    Room *location{};
    if (where.empty()) {
        location = ch->in_room;
    } else {
        if ((location = find_location(ch, where)) == nullptr) {
            ch->send_line("No such location.");
            return;
        }

        if (room_is_private(location) && (ch->get_trust() < IMPLEMENTOR)) {
            ch->send_line("That room is private right now.");
            return;
        }
    }

    auto *victim = get_char_world(ch, whom);
    if (!victim) {
        ch->send_line("They aren't here.");
        return;
    }
    transfer(ch, victim, location);
}

void transfer(const Char *imm, Char *victim, Room *location) {
    if (victim->in_room == nullptr) {
        imm->send_line("They are in limbo.");
        return;
    }

    if (victim->fighting != nullptr)
        stop_fighting(victim, true);
    if (victim->riding != nullptr)
        unride_char(victim, victim->riding);
    act("$n disappears in a mushroom cloud.", victim);
    char_from_room(victim);
    char_to_room(victim, location);
    act("$n arrives from a puff of smoke.", victim);
    if (imm != victim)
        act("$n has transferred you.", imm, nullptr, victim, To::Vict);
    look_auto(victim);
    imm->send_line("Ok.");
}

void do_wizlock(Char *ch) {
    extern bool wizlock;
    wizlock = !wizlock;

    if (wizlock)
        ch->send_line("Game wizlocked.");
    else
        ch->send_line("Game un-wizlocked.");
}

/* RT anti-newbie code */

void do_newlock(Char *ch) {
    extern bool newlock;
    newlock = !newlock;

    if (newlock)
        ch->send_line("New characters have been locked out.");
    else
        ch->send_line("Newlock removed.");
}

void do_at(Char *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    Room *location;
    Room *original;

    argument = one_argument(argument, arg);

    if (arg[0] == '\0' || argument[0] == '\0') {
        ch->send_line("At where what?");
        return;
    }

    if ((location = find_location(ch, arg)) == nullptr) {
        ch->send_line("No such location.");
        return;
    }
    if ((ch->in_room != nullptr) && location == ch->in_room) {
        ch->send_line("But that's in here.......");
        return;
    }
    if (room_is_private(location) && (ch->get_trust() < IMPLEMENTOR)) {
        ch->send_line("That room is private right now.");
        return;
    }
    original = ch->in_room;
    char_from_room(ch);
    char_to_room(ch, location);
    interpret(ch, argument);

    /*
     * See if 'ch' still exists before continuing!
     * Handles 'at XXXX quit' case.
     */
    for (auto *wch : char_list) {
        if (wch == ch) {
            char_from_room(ch);
            char_to_room(ch, original);
            break;
        }
    }
}

void do_goto(Char *ch, const char *argument) {
    if (argument[0] == '\0') {
        ch->send_line("Goto where?");
        return;
    }

    Room *location;
    if ((location = find_location(ch, argument)) == nullptr) {
        ch->send_line("No such location.");
        return;
    }

    if (room_is_private(location) && (ch->get_trust() < IMPLEMENTOR)) {
        ch->send_line("That room is private right now.");
        return;
    }

    if (ch->fighting != nullptr)
        stop_fighting(ch, true);

    // We don't user ch->player()'s bamfin/bamfout to avoid exposing which IMM is controlling a switched mob.
    for (auto *rch : ch->in_room->people) {
        if (rch->get_trust() >= ch->invis_level) {
            if (ch->pcdata != nullptr && !ch->pcdata->bamfout.empty())
                act("$t", ch, ch->pcdata->bamfout, rch, To::Vict);
            else
                act("$n leaves in a swirling mist.", ch, nullptr, rch, To::Vict);
        }
    }

    char_from_room(ch);
    char_to_room(ch, location);
    if (ch->pet) {
        char_from_room(ch->pet);
        char_to_room(ch->pet, location);
    }

    for (auto *rch : ch->in_room->people) {
        if (rch->get_trust() >= ch->invis_level) {
            if (ch->pcdata != nullptr && !ch->pcdata->bamfin.empty())
                act("$t", ch, ch->pcdata->bamfin, rch, To::Vict);
            else
                act("$n appears in a swirling mist.", ch, nullptr, rch, To::Vict);
        }
    }

    look_auto(ch);
}

/* RT to replace the 3 stat commands */

void do_stat(Char *ch, ArgParser args) {
    const auto arg = args.shift();
    if (arg.empty()) {
        ch->send_line("Syntax:");
        ch->send_line("  stat obj <name>");
        ch->send_line("  stat mob <name>");
        ch->send_line("  stat room <number>");
        ch->send_line("  stat <skills/spells/info/pracs/affects> <name>");
        ch->send_line("  stat prog <mobprog>");
        return;
    }

    if (matches(arg, "room")) {
        do_rstat(ch, args);
        return;
    }

    if (matches(arg, "obj")) {
        do_ostat(ch, args);
        return;
    }
    if (matches(arg, "char") || matches(arg, "mob")) {
        do_mstat(ch, args.remaining());
        return;
    }
    if (matches(arg, "skills")) {
        do_mskills(ch, args);
        return;
    }
    if (matches(arg, "affects")) {
        do_maffects(ch, args);
        return;
    }
    if (matches(arg, "pracs")) {
        do_mpracs(ch, args);
        return;
    }
    if (matches(arg, "info")) {
        do_minfo(ch, args);
        return;
    }
    if (matches(arg, "spells")) {
        do_mspells(ch, args);
        return;
    }
    if (matches(arg, "prog")) {
        do_mpstat(ch, args);
        return;
    }
}

void do_rstat(Char *ch, ArgParser args) {
    const auto argument = args.shift();
    auto *location = argument.empty() ? ch->in_room : find_location(ch, argument);
    if (location == nullptr) {
        ch->send_line("No such location.");
        return;
    }

    if (ch->in_room != location && room_is_private(location) && ch->get_trust() < IMPLEMENTOR) {
        ch->send_line("That room is private right now.");
        return;
    }

    ch->send_line("Name: '{}'.", location->name);
    ch->send_line("Area: '{}'.'", location->area->description());

    ch->send_line("Vnum: {}.  Sector: {} ({}).  Light: {}.", location->vnum, to_string(location->sector_type),
                  static_cast<int>(location->sector_type), location->light);
    ch->send_to("Flags: ");
    ch->send_line(format_set_flags(Room::AllFlags, ch, location->room_flags));
    ch->send_line("Description:");
    ch->send_to(location->description);

    if (!location->extra_descr.empty()) {
        ch->send_line("Extra description keywords: '{}'.",
                      fmt::join(location->extra_descr | ranges::view::transform(&ExtraDescription::keyword), " "));
    }

    ch->send_to("Characters:");
    for (auto *rch : location->people) {
        if (can_see(ch, rch)) {
            ch->send_to(" ");
            ch->send_to(ArgParser(rch->name).shift());
        }
    }
    ch->send_line(".");

    ch->send_line("Objects:    {}.", fmt::join(location->contents | ranges::views::transform([](auto *obj) {
                                                   return std::string(ArgParser(obj->name).shift());
                                               }),
                                               " "));

    for (auto direction : all_directions) {
        if (auto &exit = location->exits[direction]) {
            ch->send_to(fmt::format("Door: {}.  To: {}.  Key: {}.  Exit flags: {}.\n\rKeyword: '{}'.  Description: {}",
                                    to_string(direction), (exit->u1.to_room == nullptr ? -1 : exit->u1.to_room->vnum),
                                    exit->key, exit->exit_info, exit->keyword,
                                    exit->description[0] != '\0' ? exit->description : "(none).\n\r"));
        }
    }
}

void do_ostat(Char *ch, ArgParser args) {
    Object *obj;
    ObjectIndex *objIndex;
    if (args.empty()) {
        ch->send_line("Stat what?");
        return;
    }
    if (const auto opt_number = args.try_shift_number(); opt_number) {
        const auto vnum = *opt_number;
        if ((objIndex = get_obj_index(vnum)) == nullptr) {
            ch->send_line("Nothing like that in hell, earth, or heaven.");
            return;
        }
        ch->send_line("Template of object:");
        ch->send_line("Name(s): {}", objIndex->name);

        ch->send_line("Vnum: {}  Type: {}", objIndex->vnum, objIndex->type_name());

        ch->send_line("Short description: {}", objIndex->short_descr);
        ch->send_line("Long description: {}", objIndex->description);

        ch->send_line("Wear bits: {}", wear_bit_name(objIndex->wear_flags));
        ch->send_line("Extra bits: {}", format_set_flags(Object::AllExtraFlags, ch, objIndex->extra_flags));

        ch->send_line("Wear string: {}", objIndex->wear_string);

        ch->send_line("Weight: {}", objIndex->weight);

        ch->send_line("Level: {}  Cost: {}  Condition: {}", objIndex->level, objIndex->cost, objIndex->condition);

        ch->send_line("Values: {}", fmt::join(objIndex->value, " "));
        ch->send_line("Please load this object if you need to know more about it.");
        return;
    }
    const auto argument = args.shift();
    if (argument.length() == 1) {
        ch->send_line("Please be more specific.");
        return;
    }

    if ((obj = get_obj_world(ch, argument)) == nullptr) {
        ch->send_line("Nothing like that in hell, earth, or heaven.");
        return;
    }

    ch->send_line("Name(s): {}", obj->name);

    ch->send_line("Vnum: {}  Type: {}  Resets: {}", obj->objIndex->vnum, obj->type_name(), obj->objIndex->reset_num);

    ch->send_line("Short description: {}", obj->short_descr);
    ch->send_line("Long description: {}", obj->description);

    ch->send_line("Wear bits: {}", wear_bit_name(obj->wear_flags));
    ch->send_line("Extra bits: {}", format_set_flags(Object::AllExtraFlags, ch, obj->extra_flags));

    ch->send_line("Wear string: {}", obj->wear_string);

    ch->send_line("Number: {}/{}  Weight: {}/{}", 1, get_obj_number(obj), obj->weight, get_obj_weight(obj));

    ch->send_line("Level: {}  Cost: {}  Condition: {}  Timer: {}", obj->level, obj->cost, obj->condition, obj->timer);

    ch->send_to(fmt::format(
        "In room: {}  In object: {}  Carried by: {}  Wear_loc: {}\n\r",
        obj->in_room == nullptr ? 0 : obj->in_room->vnum, obj->in_obj == nullptr ? "(none)" : obj->in_obj->short_descr,
        obj->carried_by == nullptr ? "(none)" : can_see(ch, obj->carried_by) ? obj->carried_by->name : "someone",
        obj->wear_loc));

    ch->send_line("Values: {}", fmt::join(obj->value, " "));

    /* now give out vital statistics as per identify */

    switch (obj->type) {
    case ObjectType::Scroll:
    case ObjectType::Potion:
    case ObjectType::Pill:
    case ObjectType::Bomb:
        ch->send_to("Level {} spells of:", obj->value[0]);

        if (obj->value[1] >= 0 && obj->value[1] < MAX_SKILL)
            ch->send_to(" '{}'", skill_table[obj->value[1]].name);

        if (obj->value[2] >= 0 && obj->value[2] < MAX_SKILL)
            ch->send_to(" '{}'", skill_table[obj->value[2]].name);

        if (obj->value[3] >= 0 && obj->value[3] < MAX_SKILL)
            ch->send_to(" '{}'", skill_table[obj->value[3]].name);

        if ((obj->value[4] >= 0 && obj->value[4] < MAX_SKILL) && obj->type == ObjectType::Bomb)
            ch->send_to(" '{}'", skill_table[obj->value[4]].name);

        ch->send_line(".");
        break;

    case ObjectType::Wand:
    case ObjectType::Staff:
        ch->send_to("Has {}({}) charges of level {}", obj->value[1], obj->value[2], obj->value[0]);

        if (obj->value[3] >= 0 && obj->value[3] < MAX_SKILL)
            ch->send_to(" '{}'", skill_table[obj->value[3]].name);

        ch->send_line(".");
        break;

    case ObjectType::Weapon: {
        ch->send_line("Weapon type is {}", Weapons::name_from_integer(obj->value[0]));
        ch->send_line("Damage is {}d{} (average {})", obj->value[1], obj->value[2],
                      (1 + obj->value[2]) * obj->value[1] / 2);

        if (obj->value[4]) {
            ch->send_line("Weapons flags: {}", weapon_bit_name(obj->value[4]));
        }
        const auto *atk_type = Attacks::at(obj->value[3]);
        if (!atk_type) {
            bug("Invalid attack type {} in object #{}.", obj->value[3], obj->objIndex->vnum);
        } else {
            const auto damage_type = atk_type->damage_type;
            ch->send_line("Damage type is {}.", magic_enum::enum_name<DamageType>(damage_type));
        }
        break;
    }
    case ObjectType::Armor:
        ch->send_line("Armor class is {} pierce, {} bash, {} slash, and {} vs. magic", obj->value[0], obj->value[1],
                      obj->value[2], obj->value[3]);
        break;

    case ObjectType::Portal: ch->send_line("Portal to {} ({}).", obj->destination->name, obj->destination->vnum); break;

    default:; // TODO #260 support Light objects.
    }

    if (!obj->extra_descr.empty() || !obj->objIndex->extra_descr.empty()) {
        ch->send_line("Extra description keywords: '{}'",
                      fmt::join(ranges::view::concat(obj->extra_descr, obj->objIndex->extra_descr)
                                    | ranges::view::transform(&ExtraDescription::keyword),
                                " "));
    }

    for (auto &af : obj->affected)
        ch->send_line("Affects {}.", af.describe_item_effect(true));

    if (!obj->enchanted)
        for (auto &af : obj->objIndex->affected)
            ch->send_line("Affects {}.", af.describe_item_effect(true));
}

void do_mskills(Char *ch, ArgParser args) {
    Char *victim;
    const auto argument = args.shift();
    if (argument.empty()) {
        ch->send_line("Stat skills whom?");
        return;
    }

    if ((victim = get_char_world(ch, argument)) == nullptr) {
        ch->send_line("They aren't here.");
        return;
    }

    if (victim->is_npc()) {
        ch->send_line("You can only view the skills of players.");
        return;
    }

    ch->send_line("Skill table for {}:", victim->name);
    const auto tabulator = SkillsTabulator::skills(ch, victim);
    if (!tabulator.tabulate()) {
        ch->send_line("They know no skills.");
        return;
    }
}

void do_mspells(Char *ch, ArgParser args) {
    Char *victim;
    const auto arg = args.shift();
    if (arg.empty()) {
        ch->send_line("Stat spells whom?");
        return;
    }

    if ((victim = get_char_world(ch, arg)) == nullptr) {
        ch->send_line("They aren't here.");
        return;
    }

    if (victim->is_npc()) {
        ch->send_line("You can only view the spells of players.");
        return;
    }

    ch->send_line("Spell list for {}:", victim->name);
    const auto tabulator = SkillsTabulator::spells(ch, victim);
    if (!tabulator.tabulate()) {
        ch->send_line("They know no spells.");
        return;
    }
}

void do_maffects(Char *ch, ArgParser args) {
    const auto arg = args.shift();
    if (arg.empty()) {
        ch->send_line("Stat affects whom?");
        return;
    }

    Char *victim = get_char_world(ch, arg);
    if (!victim) {
        ch->send_line("They aren't here.");
        return;
    }

    ch->send_line("Affect list for {}:", victim->name);

    if (victim->affected.empty()) {
        ch->send_line("Nothing.");
        return;
    }
    for (auto &af : victim->affected)
        ch->send_to(fmt::format("{}: '{}'{}.\n\r", af.is_skill() ? "Skill" : "Spell", skill_table[af.type].name,
                                af.describe_char_effect(true)));
}

/* Corrected 28/8/96 by Oshea to give correct list of spells/skills. */
void do_mpracs(Char *ch, ArgParser args) {
    Char *victim;
    const auto arg = args.shift();
    if (arg.empty()) {
        ch->send_line("Stat pracs whom?");
        return;
    }

    if ((victim = get_char_world(ch, arg)) == nullptr) {
        ch->send_line("They aren't here.");
        return;
    }

    if (victim->is_npc())
        return;

    ch->send_line("Practice list for {}:", victim->name);
    PracticeTabulator::tabulate(ch, victim);
}

// Show a victim's skill groups and creation points.
void do_minfo(Char *ch, ArgParser args) {
    Char *victim;
    const auto arg = args.shift();
    if (arg.empty()) {
        ch->send_line("Stat info whom?");
        return;
    }

    if ((victim = get_char_world(ch, arg)) == nullptr) {
        ch->send_line("They aren't here.");
        return;
    }

    if (victim->is_npc()) {
        ch->send_line("You can only view the info of players.");
        return;
    }

    ch->send_line("Info list for {}:", victim->name);

    Columner col3(*ch, 3);
    for (auto gn = 0; gn < MAX_GROUP; gn++) {
        if (group_table[gn].name == nullptr)
            break;
        if (victim->pcdata->group_known[gn]) {
            col3.add(group_table[gn].name);
        }
    }
    ch->send_line("Creation points: {}", victim->pcdata->points);
}

void do_mstat(Char *ch, std::string_view argument) {
    Char *victim;
    if (argument.empty()) {
        ch->send_line("Stat what?");
        return;
    }
    if (argument.length() < 2) {
        ch->send_line("Please be more specific.");
        return;
    }

    if ((victim = get_char_world(ch, argument)) == nullptr) {
        ch->send_line("They aren't here.");
        return;
    }

    ch->send_line("Name: {}     Clan: {}     Rank: {}.", victim->name, victim->clan() ? victim->clan()->name : "(none)",
                  victim->pc_clan() ? victim->pc_clan()->level_name() : "(none)");

    ch->send_line("Vnum: {}  Format: {}  Race: {}  Sex: {}  Room: {}", victim->is_npc() ? victim->mobIndex->vnum : 0,
                  victim->is_npc() ? "npc" : "pc", race_table[victim->race].name,
                  std::string(victim->sex.name()).c_str(), victim->in_room == nullptr ? 0 : victim->in_room->vnum);

    if (victim->is_npc()) {
        ch->send_line("Count: {}  Killed: {}", victim->mobIndex->count, victim->mobIndex->killed);
    }

    ch->send_line("Str: {}({})  Int: {}({})  Wis: {}({})  Dex: {}({})  Con: {}({})", victim->perm_stat[Stat::Str],
                  get_curr_stat(victim, Stat::Str), victim->perm_stat[Stat::Int], get_curr_stat(victim, Stat::Int),
                  victim->perm_stat[Stat::Wis], get_curr_stat(victim, Stat::Wis), victim->perm_stat[Stat::Dex],
                  get_curr_stat(victim, Stat::Dex), victim->perm_stat[Stat::Con], get_curr_stat(victim, Stat::Con));

    ch->send_line("Hp: {}/{}  Mana: {}/{}  Move: {}/{}  Practices: {}", victim->hit, victim->max_hit, victim->mana,
                  victim->max_mana, victim->move, victim->max_move, ch->is_npc() ? 0 : victim->practice);

    ch->send_line("Lv: {}  Class: {}  Align: {}  Gold: {}d  Exp: {}", victim->level,
                  victim->is_npc() ? "mobile" : class_table[victim->class_num].name, victim->alignment, victim->gold,
                  victim->exp);

    ch->send_line("Armor: pierce: {}  bash: {}  slash: {}  magic: {}", victim->get_armour_class(ArmourClass::Pierce),
                  victim->get_armour_class(ArmourClass::Bash), victim->get_armour_class(ArmourClass::Slash),
                  victim->get_armour_class(ArmourClass::Exotic));

    ch->send_line("Hit: {}  Dam: {}  Saves: {}  Position: {}  Wimpy: {}", victim->get_hitroll(), victim->get_damroll(),
                  victim->saving_throw, victim->position.name(), victim->wimpy);

    if (victim->is_npc()) {
        const auto *atk_type = Attacks::at(victim->attack_type);
        if (!atk_type) {
            bug("Invalid attack type {} in mob #{}.", victim->attack_type, victim->mobIndex->vnum);
        } else {
            ch->send_line("Damage: {}  Message: {}", victim->damage, atk_type->verb);
        }
    }
    ch->send_line("Fighting: {}", victim->fighting ? victim->fighting->name : "(none)");

    ch->send_to(
        fmt::format("Sentient 'victim': {}\n\r", victim->sentient_victim.empty() ? "(none)" : victim->sentient_victim));

    if (const auto opt_nutrition = victim->report_nutrition()) {
        ch->send_line(*opt_nutrition);
    }

    ch->send_line("Carry number: {}  Carry weight: {}", victim->carry_number, victim->carry_weight);

    if (victim->is_pc()) {
        using namespace std::chrono;
        ch->send_line("Age: {}  Played: {}  Last Level: {}  Timer: {}", get_age(victim),
                      duration_cast<hours>(victim->total_played()).count(), victim->pcdata->last_level, victim->timer);
    }

    ch->send_line("Act: {}", (char *)act_bit_name(victim->act));

    if (victim->is_pc()) {
        ch->send_line("Extra: {}", ch->format_extra_flags());
    }

    if (victim->comm) {
        ch->send_line("Comm: {}", (char *)comm_bit_name(victim->comm));
    }

    if (victim->is_npc() && victim->off_flags) {
        ch->send_line("Offense: {}", (char *)off_bit_name(victim->off_flags));
    }

    if (victim->imm_flags) {
        ch->send_line("Immune: {}", (char *)imm_bit_name(victim->imm_flags));
    }

    if (victim->res_flags) {
        ch->send_line("Resist: {}", (char *)imm_bit_name(victim->res_flags));
    }

    if (victim->vuln_flags) {
        ch->send_line("Vulnerable: {}", (char *)imm_bit_name(victim->vuln_flags));
    }

    ch->send_line("Form: {}\n\rParts: {}", morphology_bit_name(victim->morphology),
                  (char *)body_part_bit_name(victim->parts));

    if (victim->affected_by) {
        ch->send_line("Affected by {}", format_set_flags(Char::AllAffectFlags, victim->affected_by));
    }

    ch->send_line(fmt::format("Master: {}  Leader: {}  Pet: {}", victim->master ? victim->master->name : "(none)",
                              victim->leader ? victim->leader->name : "(none)",
                              victim->pet ? victim->pet->name : "(none)"));

    ch->send_line(fmt::format("Riding: {}  Ridden by: {}", victim->riding ? victim->riding->name : "(none)",
                              victim->ridden_by ? victim->ridden_by->name : "(none)"));

    ch->send_line(fmt::format("Short description: {}\n\rLong  description: {}", victim->short_descr,
                              victim->long_descr.empty() ? "(none)" : victim->long_descr));

    if (victim->is_npc() && victim->spec_fun)
        ch->send_line("Mobile has special procedure.");

    if (victim->is_npc() && victim->mobIndex->progtypes) {
        ch->send_line("Mobile has MOBPROG: view with \"stat prog '{}'\"", victim->name);
    }

    for (const auto &af : victim->affected)
        ch->send_line(fmt::format("{}: '{}'{}.", af.is_skill() ? "Skill" : "Spell", skill_table[af.type].name,
                                  af.describe_char_effect(true)));
    ch->send_line("");
}

/* ofind and mfind replaced with vnum, vnum skill also added */

void do_mfind(Char *ch, std::string_view argument) {
    if (argument.empty()) {
        ch->send_line("Find whom?");
        return;
    }
    auto buffer = ranges::accumulate(all_mob_indexes() | ranges::views::filter([&](const auto &mob) {
                                         return is_name(argument, mob.player_name);
                                     }) | ranges::views::transform([](const auto &mob) {
                                         return fmt::format("[{:5}] {}\n\r", mob.vnum, mob.short_descr);
                                     }),
                                     std::string());

    if (!buffer.empty())
        ch->page_to(buffer);
    else
        ch->send_line("No mobiles by that name.");
}

void do_ofind(Char *ch, std::string_view argument) {
    if (argument.empty()) {
        ch->send_line("Find what?");
        return;
    }
    auto buffer = ranges::accumulate(all_object_indexes() | ranges::views::filter([&](const auto &obj_index) {
                                         return is_name(argument, obj_index.name);
                                     }) | ranges::views::transform([](const auto &obj_index) {
                                         return fmt::format("[{:5}] {}\n\r", obj_index.vnum, obj_index.short_descr);
                                     }),
                                     std::string());
    if (!buffer.empty())
        ch->page_to(buffer);
    else
        ch->send_line("No objects by that name.");
}

void do_vnum(Char *ch, ArgParser args) {
    if (args.empty()) {
        ch->send_line("Syntax:");
        ch->send_line("  vnum obj <name>");
        ch->send_line("  vnum mob <name>");
        ch->send_line("  vnum skill <skill or spell>");
        return;
    }
    const auto arg1 = args.shift();
    const auto arg2 = args.shift();
    if (matches(arg1, "obj")) {
        do_ofind(ch, arg2);
        return;
    }
    if (matches(arg1, "mob") || matches(arg1, "char")) {
        do_mfind(ch, arg2);
        return;
    }
    if (matches(arg1, "skill") || matches(arg1, "spell")) {
        do_slookup(ch, arg2);
        return;
    }
    /* do both */
    do_mfind(ch, arg1);
    do_ofind(ch, arg1);
}

void do_mwhere(Char *ch, const char *argument) {
    bool find_pc = false;
    if (argument[0] == '\0') {
        find_pc = true;
    } else if (strlen(argument) < 2) {
        ch->send_line("Please be more specific.");
        return;
    }

    bool found = false;
    int number = 0;
    std::string buffer;
    for (auto *victim : char_list) {
        if ((victim->is_npc() && victim->in_room != nullptr && is_name(argument, victim->name) && !find_pc)
            || (victim->is_pc() && find_pc && can_see(ch, victim))) {
            found = true;
            number++;
            buffer += fmt::format("{:3} [{:5}] {:<28} [{:5}] {:>20}\n\r", number, find_pc ? 0 : victim->mobIndex->vnum,
                                  victim->short_name(), victim->in_room->vnum, victim->in_room->name);
        }
    }
    ch->page_to(buffer);

    if (!found)
        act("You didn't find any $T.", ch, nullptr, argument, To::Char); // Poor Arthur.
}

void do_reboo(Char *ch) { ch->send_line("If you want to REBOOT, spell it out."); }

void do_reboot(Char *ch) {
    extern bool merc_down;

    if (!check_enum_bit(ch->act, PlayerActFlag::PlrWizInvis)) {
        do_echo(ch, fmt::format("Reboot by {}.", ch->name));
    }
    do_force(ch, "all save");
    do_save(ch);
    merc_down = true;
    // Unlike do_shutdown(), do_reboot() does not explicitly call close_socket()
    // for every connected player. That's because close_socket() actually
    // sends a PACKET_DISCONNECT to doorman, causing the client to get booted and
    // that's not the desired behaviour. Instead, setting the merc_down flag will
    // cause the main process to terminate while doorman holds onto the client's
    // connection and then reconnect them to the mud once it's back up.
}

void do_shutdow(Char *ch) { ch->send_line("If you want to SHUTDOWN, spell it out."); }

void do_shutdown(Char *ch) {
    extern bool merc_down;

    auto buf = fmt::format("Shutdown by {}.", ch->name.c_str());
    do_echo(ch, buf + "\n\r");
    do_force(ch, "all save");
    do_save(ch);
    merc_down = true;
    for (auto &d : descriptors().all())
        d.close();
}

void do_snoop(Char *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    Char *victim;

    one_argument(argument, arg);

    if (ch->desc == nullptr) {
        // MRG old code was split-brained about checking this. Seems like nothing would have worked if ch->desc was
        // actually null, so bugging out here.
        bug("null ch->desc in do_snoop");
        return;
    }

    if (arg[0] == '\0') {
        ch->send_line("Snoop whom?");
        return;
    }

    if ((victim = get_char_world(ch, arg)) == nullptr) {
        ch->send_line("They aren't here.");
        return;
    }

    if (!victim->desc) {
        ch->send_line("No descriptor to snoop.");
        return;
    }

    if (victim == ch) {
        ch->send_line("Cancelling all snoops.");
        ch->desc->stop_snooping();
        return;
    }

    if (victim->get_trust() >= ch->get_trust()) {
        ch->send_line("You failed.");
        return;
    }

    if (!ch->desc->try_start_snooping(*victim->desc)) {
        ch->send_line("No snoop loops.");
        return;
    }

    ch->send_line("Ok.");
}

void do_switch(Char *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    Char *victim;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        ch->send_line("Switch into whom?");
        return;
    }

    if (ch->desc == nullptr)
        return;

    if (ch->desc->is_switched()) {
        ch->send_line("You are already switched.");
        return;
    }

    if ((victim = get_char_world(ch, arg)) == nullptr) {
        ch->send_line("They aren't here.");
        return;
    }

    if (victim == ch) {
        ch->send_line("Ok.");
        return;
    }

    if (victim->is_pc()) {
        ch->send_line("You can only switch into mobiles.");
        return;
    }

    if (victim->desc != nullptr) {
        ch->send_line("Character in use.");
        return;
    }

    ch->desc->do_switch(victim);
    /* change communications to match */
    victim->comm = ch->comm;
    victim->lines = ch->lines;
    victim->send_line("Ok.");
}

void do_return(Char *ch) {
    if (ch->desc == nullptr)
        return;

    if (!ch->desc->is_switched()) {
        ch->send_line("You aren't switched.");
        return;
    }

    ch->send_line("You return to your original body.");
    ch->desc->do_return();
}

/* trust levels for load and clone */
/* cut out by Faramir but func retained in case of any
   calls I don't know about. */
bool obj_check(Char *ch, Object *obj) { return ch->get_trust() >= obj->level; }

/* for clone, to insure that cloning goes many levels deep */
void recursive_clone(Char *ch, Object *obj, Object *clone) {
    for (Object *c_obj : obj->contains) {
        if (obj_check(ch, c_obj)) {
            Object *t_obj = create_object(c_obj->objIndex);
            clone_object(c_obj, t_obj);
            obj_to_obj(t_obj, clone);
            recursive_clone(ch, c_obj, t_obj);
        }
    }
}

/* command that is similar to load */
void do_clone(Char *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    Char *mob = nullptr;
    Object *obj = nullptr;
    const char *rest = one_argument(argument, arg);

    if (arg[0] == '\0') {
        ch->send_line("Clone what?");
        return;
    }

    if (!str_prefix(arg, "object")) {
        obj = get_obj_here(ch, rest);
    } else if (!str_prefix(arg, "mobile") || !str_prefix(arg, "character")) {
        mob = get_char_room(ch, rest);
    } else { /* find both */
        mob = get_char_room(ch, argument);
        obj = get_obj_here(ch, argument);
    }

    /* clone an object */
    if (obj != nullptr) {
        Object *clone;

        if (!obj_check(ch, obj)) {
            ch->send_line("Your powers are not great enough for such a task.");
            return;
        }

        clone = create_object(obj->objIndex);
        clone_object(obj, clone);
        if (obj->carried_by != nullptr)
            obj_to_char(clone, ch);
        else
            obj_to_room(clone, ch->in_room);
        recursive_clone(ch, obj, clone);

        act("$n has created $p.", ch, clone, nullptr, To::Room);
        act("You clone $p.", ch, clone, nullptr, To::Char);
    } else if (mob != nullptr) {
        Char *clone;
        Object *new_obj;

        if (mob->is_pc()) {
            ch->send_line("You can only clone mobiles.");
            return;
        }

        if ((mob->level > 20 && ch->get_trust() < GOD) || (mob->level > 10 && ch->get_trust() < IMMORTAL)
            || (mob->level > 5 && ch->get_trust() < DEMI) || (mob->level > 0 && ch->get_trust() < ANGEL)
            || ch->get_trust() < AVATAR) {
            ch->send_line("Your powers are not great enough for such a task.");
            return;
        }

        clone = create_mobile(mob->mobIndex);
        clone_mobile(mob, clone);

        for (auto *carried : mob->carrying) {
            if (obj_check(ch, carried)) {
                new_obj = create_object(carried->objIndex);
                clone_object(carried, new_obj);
                recursive_clone(ch, carried, new_obj);
                obj_to_char(new_obj, clone);
                new_obj->wear_loc = carried->wear_loc;
            }
        }
        char_to_room(clone, ch->in_room);
        act("$n has created $N.", ch, nullptr, clone, To::Room);
        act("You clone $N.", ch, nullptr, clone, To::Char);
    } else {
        ch->send_line("You don't see that here.");
    }
}

/* RT to replace the two load commands */

void do_mload(Char *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    MobIndexData *mobIndex;

    one_argument(argument, arg);

    if (arg[0] == '\0' || !is_number(arg)) {
        ch->send_line("Syntax: load mob <vnum>.");
        return;
    }

    if ((mobIndex = get_mob_index(atoi(arg))) == nullptr) {
        ch->send_line("No mob has that vnum.");
        return;
    }

    Char *victim = create_mobile(mobIndex);
    char_to_room(victim, ch->in_room);
    act("$n has created $N!", ch, nullptr, victim, To::Room);
    ch->send_line("Ok.");
}

void do_oload(Char *ch, const char *argument) {
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    ObjectIndex *objIndex;
    Object *obj;

    argument = one_argument(argument, arg1);
    one_argument(argument, arg2);

    if (arg1[0] == '\0' || !is_number(arg1)) {
        ch->send_line("Syntax: load obj <vnum>.");
        return;
    }

    if ((objIndex = get_obj_index(atoi(arg1))) == nullptr) {
        ch->send_line("No object has that vnum.");
        return;
    }

    obj = create_object(objIndex);
    if (obj->is_takeable())
        obj_to_char(obj, ch);
    else
        obj_to_room(obj, ch->in_room);
    act("$n has created $p!", ch, obj, nullptr, To::Room);
    ch->send_line("Ok.");
}

void do_load(Char *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];

    argument = one_argument(argument, arg);

    if (arg[0] == '\0') {
        ch->send_line("Syntax:");
        ch->send_line("  load mob <vnum>");
        ch->send_line("  load obj <vnum> <level>");
        return;
    }

    if (!str_cmp(arg, "mob") || !str_cmp(arg, "char")) {
        do_mload(ch, argument);
        return;
    }

    if (!str_cmp(arg, "obj")) {
        do_oload(ch, argument);
        return;
    }
    /* echo syntax */
    do_load(ch, "");
}

void do_purge(Char *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    Descriptor *d;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        /* 'purge' */

        for (auto *victim : ch->in_room->people) {
            if (victim->is_npc() && !check_enum_bit(victim->act, CharActFlag::NoPurge)
                && victim != ch /* safety precaution */)
                extract_char(victim, true);
        }

        for (auto *obj : ch->in_room->contents) {
            if (!obj->is_no_purge())
                extract_obj(obj);
        }

        act("$n purges the room!", ch);
        ch->send_line("Ok.");
        return;
    }

    auto *victim = get_char_world(ch, arg);
    if (!victim) {
        ch->send_line("They aren't here.");
        return;
    }

    if (victim->is_pc()) {

        if (ch == victim) {
            ch->send_line("Ho ho ho.");
            return;
        }

        if (ch->get_trust() <= victim->get_trust()) {
            ch->send_line("Maybe that wasn't a good idea...");
            victim->send_line("{} tried to purge you!", ch->name);
            return;
        }

        act("$n disintegrates $N.", ch, nullptr, victim, To::NotVict);

        if (victim->level > 1)
            save_char_obj(victim);
        d = victim->desc;
        extract_char(victim, true);
        if (d != nullptr)
            d->close();

        return;
    }

    act("$n purges $N.", ch, nullptr, victim, To::NotVict);
    extract_char(victim, true);
}

void do_advance(Char *ch, const char *argument) {
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    Char *victim;
    int level;
    int iLevel;

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if (arg1[0] == '\0' || arg2[0] == '\0' || !is_number(arg2)) {
        ch->send_line("Syntax: advance <char> <level>.");
        return;
    }

    if ((victim = get_char_room(ch, arg1)) == nullptr) {
        ch->send_line("That player is not here.");
        return;
    }

    if (victim->is_npc()) {
        ch->send_line("Not on NPC's.");
        return;
    }

    if ((level = atoi(arg2)) < 1 || level > MAX_LEVEL) {
        ch->send_line("Level must be 1 to {}.", MAX_LEVEL);
        return;
    }

    if (level > ch->get_trust()) {
        ch->send_line("Limited to your trust level.");
        return;
    }

    /*
     * Lower level:
     *   Reset to level 1.
     *   Then raise again.
     *   Currently, an imp can lower another imp.
     *   -- Swiftest
     */
    if (level <= victim->level && (ch->level > victim->level)) {
        int temp_prac;

        ch->send_line("Lowering a player's level!");
        victim->send_line("**** OOOOHHHHHHHHHH  NNNNOOOO ****");
        temp_prac = victim->practice;
        victim->level = 1;
        victim->exp = exp_per_level(victim, victim->pcdata->points);
        victim->max_hit = 10;
        victim->max_mana = 100;
        victim->max_move = 100;
        victim->practice = 0;
        victim->hit = victim->max_hit;
        victim->mana = victim->max_mana;
        victim->move = victim->max_move;
        advance_level(victim);
        victim->practice = temp_prac;
    } else {
        ch->send_line("Raising a player's level!");
        victim->send_line("|R**** OOOOHHHHHHHHHH  YYYYEEEESSS ****|w");
    }

    if (ch->level > victim->level) {
        for (iLevel = victim->level; iLevel < level; iLevel++) {
            victim->send_line("You raise a level!!  ");
            victim->level += 1;
            advance_level(victim);
        }
        victim->exp = (exp_per_level(victim, victim->pcdata->points) * std::max(1_s, victim->level));
        victim->trust = 0;
        save_char_obj(victim);
    }
}

void do_trust(Char *ch, const char *argument) {
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    Char *victim;
    int level;

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if ((arg1[0] == '\0' && arg2[0] == '\0') || (arg2[0] != '\0' && !is_number(arg2))) {
        ch->send_line("Syntax: trust <char> <level>.");
        return;
    }

    if ((victim = get_char_room(ch, arg1)) == nullptr) {
        ch->send_line("That player is not here.");
        return;
    }

    if (arg2[0] == '\0') {
        ch->send_line("{} has a trust of {}.", victim->name, victim->trust);
        return;
    }

    if (victim->is_npc()) {
        ch->send_line("Not on NPCs.");
        return;
    }

    if ((level = atoi(arg2)) < 0 || level > 100) {
        ch->send_line("Level must be 0 (reset) or 1 to 100.");
        return;
    }

    if (level > ch->get_trust()) {
        ch->send_line("Limited to your trust.");
        return;
    }

    victim->trust = level;
}

void do_restore(Char *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    Char *victim;

    one_argument(argument, arg);
    if (arg[0] == '\0' || !str_cmp(arg, "room")) {
        /* cure room */

        for (auto *vch : ch->in_room->people) {
            affect_strip(vch, gsn_plague);
            affect_strip(vch, gsn_poison);
            affect_strip(vch, gsn_blindness);
            affect_strip(vch, gsn_sleep);
            affect_strip(vch, gsn_curse);

            vch->hit = vch->max_hit;
            vch->mana = vch->max_mana;
            vch->move = vch->max_move;
            update_pos(vch);
            act("$n has restored you.", ch, nullptr, vch, To::Vict);
        }

        ch->send_line("Room restored.");
        return;
    }

    if (ch->get_trust() >= MAX_LEVEL && !str_cmp(arg, "all")) {
        /* cure all */

        for (auto &d : descriptors().playing()) {
            victim = d.character();

            if (victim->is_npc())
                continue;

            affect_strip(victim, gsn_plague);
            affect_strip(victim, gsn_poison);
            affect_strip(victim, gsn_blindness);
            affect_strip(victim, gsn_sleep);
            affect_strip(victim, gsn_curse);

            victim->hit = victim->max_hit;
            victim->mana = victim->max_mana;
            victim->move = victim->max_move;
            update_pos(victim);
            if (victim->in_room != nullptr)
                act("$n has restored you.", ch, nullptr, victim, To::Vict);
        }
        ch->send_line("All active players restored.");
        return;
    }

    if ((victim = get_char_world(ch, arg)) == nullptr) {
        ch->send_line("They aren't here.");
        return;
    }

    affect_strip(victim, gsn_plague);
    affect_strip(victim, gsn_poison);
    affect_strip(victim, gsn_blindness);
    affect_strip(victim, gsn_sleep);
    affect_strip(victim, gsn_curse);
    victim->hit = victim->max_hit;
    victim->mana = victim->max_mana;
    victim->move = victim->max_move;
    update_pos(victim);
    act("$n has restored you.", ch, nullptr, victim, To::Vict);
    ch->send_line("Ok.");
}

void do_freeze(Char *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    Char *victim;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        ch->send_line("Freeze whom?");
        return;
    }

    if ((victim = get_char_world(ch, arg)) == nullptr) {
        ch->send_line("They aren't here.");
        return;
    }

    if (victim->is_npc()) {
        ch->send_line("Not on NPC's.");
        return;
    }

    if (victim->get_trust() >= ch->get_trust()) {
        ch->send_line("You failed.");
        return;
    }

    if (check_enum_bit(victim->act, PlayerActFlag::PlrFreeze)) {
        clear_enum_bit(victim->act, PlayerActFlag::PlrFreeze);
        victim->send_line("You can play again.");
        ch->send_line("FREEZE removed.");
    } else {
        set_enum_bit(victim->act, PlayerActFlag::PlrFreeze);
        victim->send_line("You can't do ANYthing!");
        ch->send_line("FREEZE set.");
    }

    save_char_obj(victim);
}

void do_log(Char *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    Char *victim;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        ch->send_line("Log whom?");
        return;
    }

    if (!str_cmp(arg, "all")) {
        if (fLogAll) {
            fLogAll = false;
            ch->send_line("Log ALL off.");
        } else {
            fLogAll = true;
            ch->send_line("Log ALL on.");
        }
        return;
    }

    if ((victim = get_char_world(ch, arg)) == nullptr) {
        ch->send_line("They aren't here.");
        return;
    }

    if (victim->is_npc()) {
        ch->send_line("Not on NPC's.");
        return;
    }

    /*
     * No level check, gods can log anyone.
     */
    if (check_enum_bit(victim->act, PlayerActFlag::PlrLog)) {
        clear_enum_bit(victim->act, PlayerActFlag::PlrLog);
        ch->send_line("LOG removed.");
    } else {
        set_enum_bit(victim->act, PlayerActFlag::PlrLog);
        ch->send_line("LOG set.");
    }
}

void do_noemote(Char *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    Char *victim;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        ch->send_line("Noemote whom?");
        return;
    }

    if ((victim = get_char_world(ch, arg)) == nullptr) {
        ch->send_line("They aren't here.");
        return;
    }

    if (victim->get_trust() >= ch->get_trust()) {
        ch->send_line("You failed.");
        return;
    }

    if (check_enum_bit(victim->comm, CommFlag::NoEmote)) {
        clear_enum_bit(victim->comm, CommFlag::NoEmote);
        victim->send_line("You can emote again.");
        ch->send_line("NOEMOTE removed.");
    } else {
        set_enum_bit(victim->comm, CommFlag::NoEmote);
        victim->send_line("You can't emote!");
        ch->send_line("NOEMOTE set.");
    }
}

void do_noshout(Char *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    Char *victim;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        ch->send_line("Noshout whom?");
        return;
    }

    if ((victim = get_char_world(ch, arg)) == nullptr) {
        ch->send_line("They aren't here.");
        return;
    }

    if (victim->is_npc()) {
        ch->send_line("Not on NPC's.");
        return;
    }

    if (victim->get_trust() >= ch->get_trust()) {
        ch->send_line("You failed.");
        return;
    }

    if (check_enum_bit(victim->comm, CommFlag::NoShout)) {
        clear_enum_bit(victim->comm, CommFlag::NoShout);
        victim->send_line("You can shout again.");
        ch->send_line("NOSHOUT removed.");
    } else {
        set_enum_bit(victim->comm, CommFlag::NoShout);
        victim->send_line("You can't shout!");
        ch->send_line("NOSHOUT set.");
    }
}

void do_notell(Char *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    Char *victim;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        ch->send_line("Notell whom?");
        return;
    }

    if ((victim = get_char_world(ch, arg)) == nullptr) {
        ch->send_line("They aren't here.");
        return;
    }

    if (victim->get_trust() >= ch->get_trust()) {
        ch->send_line("You failed.");
        return;
    }

    if (check_enum_bit(victim->comm, CommFlag::NoTell)) {
        clear_enum_bit(victim->comm, CommFlag::NoTell);
        victim->send_line("You can tell again.");
        ch->send_line("NOTELL removed.");
    } else {
        set_enum_bit(victim->comm, CommFlag::NoTell);
        victim->send_line("You can't tell!");
        ch->send_line("NOTELL set.");
    }
}

void do_peace(Char *ch) {
    for (auto *rch : ch->in_room->people) {
        if (rch->fighting)
            stop_fighting(rch, true);
        if (rch->is_npc() && check_enum_bit(rch->act, CharActFlag::Aggressive))
            clear_enum_bit(rch->act, CharActFlag::Aggressive);
        if (rch->is_npc())
            rch->sentient_victim.clear();
    }

    ch->send_line("Ok.");
}

void do_awaken(Char *ch, const char *argument) {
    Char *victim;
    char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        ch->send_line("Awaken whom?");
        return;
    }

    if ((victim = get_char_room(ch, arg)) == nullptr) {
        ch->send_line("They aren't here.");
        return;
    }
    if (!victim->is_pos_sleeping()) {
        ch->send_line("Duh!  They're not even asleep!");
        return;
    }

    if (ch == victim) {
        ch->send_line("Duh!  If you wanna wake up, get COFFEE!");
        return;
    }

    clear_enum_bit(victim->affected_by, AffectFlag::Sleep);
    victim->position = Position::Type::Standing;

    act("$n gives $t a kick, and wakes them up.", ch, victim->short_descr, nullptr, To::Room, Position::Type::Resting);
}

void do_owhere(Char *ch, const char *argument) {
    char target_name[MAX_INPUT_LENGTH];
    Object *in_obj;
    bool found;
    int number = 0;

    found = false;
    number = 0;

    if (argument[0] == '\0') {
        ch->send_line("Owhere which object?");
        return;
    }
    if (strlen(argument) < 2) {
        ch->send_line("Please be more specific.");
        return;
    }
    one_argument(argument, target_name);

    std::string buffer;
    for (auto *obj : object_list) {
        if (!is_name(target_name, obj->name))
            continue;

        found = true;
        number++;

        for (in_obj = obj; in_obj->in_obj != nullptr; in_obj = in_obj->in_obj)
            ;

        if (in_obj->carried_by != nullptr) {
            buffer += fmt::format("{:3} {:<25} carried by {:<20} in room {}\n\r", number, obj->short_descr,
                                  ch->describe(*in_obj->carried_by), in_obj->carried_by->in_room->vnum);
        } else if (in_obj->in_room != nullptr) {
            buffer += fmt::format("{:3} {:<25} in {:<30} [{}]\n\r", number, obj->short_descr, in_obj->in_room->name,
                                  in_obj->in_room->vnum);
        }
    }

    ch->page_to(buffer);
    if (!found)
        ch->send_line("Nothing like that in heaven or earth.");
}

/* Death's command */
void do_coma(Char *ch, const char *argument) {
    Char *victim;
    char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        ch->send_line("Comatoze whom?");
        return;
    }

    if ((victim = get_char_room(ch, arg)) == nullptr) {
        ch->send_line("They aren't here.");
        return;
    }

    if (victim->is_aff_sleep())
        return;

    if (ch == victim) {
        ch->send_line("Duh!  Don't you dare fall asleep on the job!");
        return;
    }
    if ((ch->get_trust() <= victim->get_trust()) || !((ch->is_immortal()) && victim->is_npc())) {
        ch->send_line("You failed.");
        return;
    }

    AFFECT_DATA af;
    af.type = 38; /* SLEEP */ // TODO 38? really?
    af.level = ch->trust;
    af.duration = 4 + ch->trust;
    af.bitvector = to_int(AffectFlag::Sleep);
    affect_join(victim, af);

    if (victim->is_pos_awake()) {
        victim->send_line("You feel very sleepy ..... zzzzzz.");
        act("$n goes to sleep.", victim);
        victim->position = Position::Type::Sleeping;
    }
}

bool osearch_is_valid_level_range(int min_level, int max_level) {
    if (min_level > MAX_LEVEL || max_level > MAX_LEVEL || min_level > max_level || max_level - min_level > 10) {
        return false;
    } else {
        return true;
    }
}

void osearch_display_syntax(Char *ch) {
    ch->send_line("Syntax: osearch [min level] [max level] [item type] optional item name...");
    ch->send_line("        Level range no greater than 10. Item types:\n\r        ");
    Columner col4(*ch, 4, 20);
    for (const auto type_name : ObjectTypes::sorted_type_names()) {
        col4.add(type_name);
    }
}

/**
 * Searches the item database for items of the specified type, level range and
 * optional name. Adds their item vnum, name, level and area name to a new buffer.
 */
std::string osearch_find_items(const int min_level, const int max_level, const ObjectType item_type,
                               std::string_view item_name) {
    const auto in_level_range = [&min_level, &max_level](const auto &obj_index) {
        return obj_index.level >= min_level && obj_index.level <= max_level;
    };
    const auto is_item_type = [&item_type](const auto &obj_index) { return obj_index.type == item_type; };
    const auto maybe_matches_name = [&item_name](const auto &obj_index) {
        return item_name.empty() || is_name(item_name, obj_index.name);
    };
    const auto to_result = [](const auto &obj_index) {
        return fmt::format("{:5} {:<27}|w ({:3}) {}\n\r", obj_index.vnum, obj_index.short_descr, obj_index.level,
                           obj_index.area->filename());
    };
    auto buffer = ranges::accumulate(
        all_object_indexes() | ranges::views::filter(in_level_range) | ranges::views::filter(is_item_type)
            | ranges::views::filter(maybe_matches_name) | ranges::views::transform(to_result),
        std::string());
    return buffer;
}

/**
 * osearch: an item search tool for immortals.
 * It searches the item database not the object instances in world!
 * You must provide min/max level range and an item type name.
 * You can optionally filter on the leading part of the object name.
 * As some items are level 0, specifing min level 0 is allowed.
 */
void do_osearch(Char *ch, ArgParser args) {
    if (args.empty()) {
        osearch_display_syntax(ch);
        return;
    }
    const auto opt_min_level = args.try_shift_number();
    const auto opt_max_level = args.try_shift_number();
    const auto item_type = args.shift();
    const auto item_name = args.shift();

    if (!opt_min_level || !opt_max_level || item_type.empty()) {
        osearch_display_syntax(ch);
        return;
    }
    if (!osearch_is_valid_level_range(*opt_min_level, *opt_max_level)) {
        osearch_display_syntax(ch);
        return;
    }
    const auto opt_item_type = ObjectTypes::try_lookup(item_type);
    if (!opt_item_type) {
        osearch_display_syntax(ch);
        return;
    }
    ch->page_to(osearch_find_items(*opt_min_level, *opt_max_level, *opt_item_type, item_name));
}

void do_slookup(Char *ch, std::string_view argument) {
    if (argument.empty()) {
        ch->send_line("Lookup which skill or spell?");
        return;
    }
    if (matches(argument, "all")) {
        for (auto sn = 0; sn < MAX_SKILL; sn++) {
            if (skill_table[sn].name == nullptr)
                break;
            ch->send_line("Sn: {}  Slot: {}  Skill/spell: '{}'", sn, skill_table[sn].slot, skill_table[sn].name);
        }
    } else {
        if (auto sn = skill_lookup(argument); sn >= 0) {
            ch->send_line("Sn: {}  Slot: {}  Skill/spell: '{}'", sn, skill_table[sn].slot, skill_table[sn].name);
        } else {
            ch->send_line("No such skill or spell.");
        }
    }
}

/* RT set replaces sset, mset, oset, and rset */

void do_set(Char *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];

    argument = one_argument(argument, arg);

    if (arg[0] == '\0') {
        ch->send_line("Syntax:");
        ch->send_line("  set mob   <name> <field> <value>");
        ch->send_line("  set obj   <name> <field> <value>");
        ch->send_line("  set room  <room> <field> <value>");
        ch->send_line("  set skill <name> <spell or skill> <value>");
        return;
    }

    if (!str_prefix(arg, "mobile") || !str_prefix(arg, "character")) {
        do_mset(ch, argument);
        return;
    }

    if (!str_prefix(arg, "skill") || !str_prefix(arg, "spell")) {
        do_sset(ch, argument);
        return;
    }

    if (!str_prefix(arg, "object")) {
        do_oset(ch, argument);
        return;
    }

    if (!str_prefix(arg, "room")) {
        do_rset(ch, argument);
        return;
    }
    /* echo syntax */
    do_set(ch, "");
}

void do_sset(Char *ch, const char *argument) {
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];
    Char *victim;
    int value;
    int sn;
    bool fAll;

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);
    argument = one_argument(argument, arg3);

    if (arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0') {
        ch->send_line("Syntax:");
        ch->send_line("  set skill <name> <spell or skill> <value>");
        ch->send_line("  set skill <name> all <value>");
        ch->send_line("   (use the name of the skill, not the number)");
        return;
    }

    if ((victim = get_char_world(ch, arg1)) == nullptr) {
        ch->send_line("They aren't here.");
        return;
    }

    if (victim->is_npc()) {
        ch->send_line("Not on NPC's.");
        return;
    }

    fAll = !str_cmp(arg2, "all");
    sn = 0;
    if (!fAll && (sn = skill_lookup(arg2)) < 0) {
        ch->send_line("No such skill or spell.");
        return;
    }

    /*
     * Snarf the value.
     */
    if (!is_number(arg3)) {
        ch->send_line("Value must be numeric.");
        return;
    }

    value = atoi(arg3);
    if (value < 0 || value > 100) {
        ch->send_line("Value range is 0 to 100.");
        return;
    }

    if (fAll) {
        for (sn = 0; sn < MAX_SKILL; sn++) {
            if (skill_table[sn].name != nullptr)
                victim->pcdata->learned[sn] = value;
        }
    } else {
        victim->pcdata->learned[sn] = value;
    }
}

void do_mset(Char *ch, const char *argument) {
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];
    char buf[100];
    Char *victim;
    int value;

    char smash_tilded[MAX_INPUT_LENGTH];
    strncpy(smash_tilded, smash_tilde(argument).c_str(),
            MAX_INPUT_LENGTH - 1); // TODO to minimize changes during refactor
    auto *args = smash_tilded;
    args = one_argument(args, arg1);
    args = one_argument(args, arg2);
    strcpy(arg3, args);

    if (arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0') {
        ch->send_line("Syntax:");
        ch->send_line("  set char <name> <field> <value>");
        ch->send_line("  Field being one of:");
        ch->send_line("    str int wis dex con sex class level");
        ch->send_line("    race gold hp mana move practice align");
        ch->send_line("    dam hit train thirst drunk full hours");
        return;
    }

    if ((victim = get_char_world(ch, arg1)) == nullptr) {
        ch->send_line("They aren't here.");
        return;
    }

    /*
     * Snarf the value (which need not be numeric).
     */
    value = is_number(arg3) ? atoi(arg3) : -1;

    /*
     * Set something.
     */
    if (!str_cmp(arg2, "str")) {
        if (value < 3 || value > get_max_train(victim, Stat::Str)) {
            ch->send_line("Strength range is 3 to {}.", get_max_train(victim, Stat::Str));
            return;
        }

        victim->perm_stat[Stat::Str] = value;
        return;
    }

    if (!str_cmp(arg2, "int")) {
        if (value < 3 || value > get_max_train(victim, Stat::Int)) {
            ch->send_line("Intelligence range is 3 to {}.", get_max_train(victim, Stat::Int));
            return;
        }

        victim->perm_stat[Stat::Int] = value;
        return;
    }

    if (!str_cmp(arg2, "wis")) {
        if (value < 3 || value > get_max_train(victim, Stat::Wis)) {
            ch->send_line("Wisdom range is 3 to {}.", get_max_train(victim, Stat::Wis));
            return;
        }

        victim->perm_stat[Stat::Wis] = value;
        return;
    }

    if (!str_cmp(arg2, "dex")) {
        if (value < 3 || value > get_max_train(victim, Stat::Dex)) {
            ch->send_line("Dexterity ranges is 3 to {}.", get_max_train(victim, Stat::Dex));
            return;
        }

        victim->perm_stat[Stat::Dex] = value;
        return;
    }

    if (!str_cmp(arg2, "con")) {
        if (value < 3 || value > get_max_train(victim, Stat::Con)) {
            ch->send_line("Constitution range is 3 to {}.", get_max_train(victim, Stat::Con));
            return;
        }

        victim->perm_stat[Stat::Con] = value;
        return;
    }

    if (!str_prefix(arg2, "sex")) {
        if (auto sex = Sex::try_from_name(arg3)) {
            victim->sex = *sex;
            if (victim->is_pc())
                victim->pcdata->true_sex = *sex;
        } else {
            ch->send_line("Invalid sex. Options: " + Sex::names_csv());
        }
        return;
    }

    if (!str_prefix(arg2, "class")) {
        int class_num;

        if (victim->is_npc()) {
            ch->send_line("Mobiles have no class.");
            return;
        }

        class_num = class_lookup(arg3);
        if (class_num == -1) {
            strcpy(buf, "Possible classes are: ");
            for (class_num = 0; class_num < MAX_CLASS; class_num++) {
                if (class_num > 0)
                    strcat(buf, " ");
                strcat(buf, class_table[class_num].name);
            }
            strcat(buf, ".\n\r");

            ch->send_to(buf);
            return;
        }

        victim->class_num = class_num;
        return;
    }

    if (!str_prefix(arg2, "level")) {
        if (victim->is_pc()) {
            ch->send_line("Not on PC's.");
            return;
        }

        if (value < 0 || value > 200) {
            ch->send_line("Level range is 0 to 200.");
            return;
        }
        victim->level = value;
        return;
    }

    if (!str_prefix(arg2, "gold")) {
        victim->gold = value;
        return;
    }

    if (!str_prefix(arg2, "hp")) {
        if (value < 1 || value > 30000) {
            ch->send_line("Hp range is 1 to 30,000 hit points.");
            return;
        }
        victim->max_hit = value;
        if (victim->is_pc())
            victim->pcdata->perm_hit = value;
        return;
    }

    if (!str_prefix(arg2, "mana")) {
        if (value < 0 || value > 30000) {
            ch->send_line("Mana range is 0 to 30,000 mana points.");
            return;
        }
        victim->max_mana = value;
        if (victim->is_pc())
            victim->pcdata->perm_mana = value;
        return;
    }

    if (!str_prefix(arg2, "move")) {
        if (value < 0 || value > 30000) {
            ch->send_line("Move range is 0 to 30,000 move points.");
            return;
        }
        victim->max_move = value;
        if (victim->is_pc())
            victim->pcdata->perm_move = value;
        return;
    }

    if (!str_prefix(arg2, "practice")) {
        if (value < 0 || value > 250) {
            ch->send_line("Practice range is 0 to 250 sessions.");
            return;
        }
        victim->practice = value;
        return;
    }

    if (!str_prefix(arg2, "train")) {
        if (value < 0 || value > 50) {
            ch->send_line("Training session range is 0 to 50 sessions.");
            return;
        }
        victim->train = value;
        return;
    }

    if (!str_prefix(arg2, "align")) {
        if (value < -1000 || value > 1000) {
            ch->send_line("Alignment range is -1000 to 1000.");
            return;
        }
        victim->alignment = value;
        return;
    }

    if (!str_cmp(arg2, "dam")) {
        if (value < 1 || value > 100) {
            ch->send_line("|RDamroll range is 1 to 100.|w");
            return;
        }

        victim->damroll = value;
        return;
    }

    if (!str_cmp(arg2, "hit")) {
        if (value < 1 || value > 100) {
            ch->send_line("|RHitroll range is 1 to 100.|w");
            return;
        }

        victim->hitroll = value;
        return;
    }

    if (!str_prefix(arg2, "thirst")) {
        if (victim->is_npc()) {
            ch->send_line("Not on NPC's.");
            return;
        }
        victim->pcdata->thirst.set(value);
        return;
    }

    if (!str_prefix(arg2, "drunk")) {
        if (victim->is_npc()) {
            ch->send_line("Not on NPC's.");
            return;
        }
        victim->pcdata->inebriation.set(value);
        return;
    }

    if (!str_prefix(arg2, "full")) {
        if (victim->is_npc()) {
            ch->send_line("Not on NPC's.");
            return;
        }
        victim->pcdata->hunger.set(value);
        return;
    }

    if (!str_prefix(arg2, "race")) {
        int race;

        race = race_lookup(arg3);

        if (race == 0) {
            ch->send_line("That is not a valid race.");
            return;
        }

        if (victim->is_pc() && !race_table[race].pc_race) {
            ch->send_line("That is not a valid player race.");
            return;
        }

        victim->race = race;
        return;
    }

    if (!str_cmp(arg2, "hours")) {
        if (value < 1 || value > 999) {
            ch->send_line("|RHours range is 1 to 999.|w");
            return;
        }

        victim->played = std::chrono::hours(value);
        return;
    }

    /*
     * Generate usage message.
     */
    do_mset(ch, "");
}

void do_string(Char *ch, const char *argument) {
    char type[MAX_INPUT_LENGTH];
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];
    Char *victim;
    Object *obj;

    char smash_tilded[MAX_INPUT_LENGTH];
    strncpy(smash_tilded, smash_tilde(argument).c_str(),
            MAX_INPUT_LENGTH - 1); // TODO to minimize changes during refactor
    auto *args = smash_tilded;
    args = one_argument(args, type);
    args = one_argument(args, arg1);
    args = one_argument(args, arg2);
    strcpy(arg3, args);

    if (type[0] == '\0' || arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0') {
        ch->send_line("Syntax:");
        ch->send_line("  string char <name> <field> <string>");
        ch->send_line("    fields: name short long desc title spec");
        ch->send_line("  string obj  <name> <field> <string>");
        ch->send_line("    fields: name short long extended wear");
        return;
    }

    if (!str_prefix(type, "character") || !str_prefix(type, "mobile")) {
        if ((victim = get_char_world(ch, arg1)) == nullptr) {
            ch->send_line("They aren't here.");
            return;
        }

        /* string something */

        if (!str_prefix(arg2, "name")) {
            if (victim->is_pc()) {
                ch->send_line("Not on PC's.");
                return;
            }

            victim->name = arg3;
            return;
        }

        if (!str_prefix(arg2, "description")) {
            victim->description = arg3;
            return;
        }

        if (!str_prefix(arg2, "short")) {
            victim->short_descr = arg3;
            return;
        }

        if (!str_prefix(arg2, "long")) {
            victim->long_descr = arg3;
            return;
        }

        if (!str_prefix(arg2, "title")) {
            if (victim->is_npc()) {
                ch->send_line("Not on NPC's.");
                return;
            }

            victim->set_title(arg3);
            return;
        }

        if (!str_prefix(arg2, "spec")) {
            if (victim->is_pc()) {
                ch->send_line("Not on PC's.");
                return;
            }

            if ((victim->spec_fun = spec_lookup(arg3)) == 0) {
                ch->send_line("No such spec fun.");
                return;
            }

            return;
        }
    }

    if (!str_prefix(type, "object")) {
        /* string an obj */

        if ((obj = get_obj_world(ch, arg1)) == nullptr) {
            ch->send_line("Nothing like that in heaven or earth.");
            return;
        }

        if (!str_prefix(arg2, "name")) {
            obj->name = arg3;
            return;
        }

        if (!str_prefix(arg2, "short")) {
            obj->short_descr = arg3;
            return;
        }

        if (!str_prefix(arg2, "long")) {
            obj->description = arg3;
            return;
        }

        if (!str_prefix(arg2, "wear")) {
            if (strlen(arg3) > 17) {
                ch->send_line("Wear_Strings may not be longer than 17 chars.");
            } else {
                obj->wear_string = arg3;
            }
            return;
        }

        if (!str_prefix(arg2, "ed") || !str_prefix(arg2, "extended")) {
            args = one_argument(args, arg3);
            if (args == nullptr) {
                ch->send_line("Syntax: oset <object> ed <keyword> <string>");
                return;
            }

            strcat(args, "\n\r");

            obj->extra_descr.emplace_back(ExtraDescription{arg3, args});
            return;
        }
    }

    /* echo bad use message */
    do_string(ch, "");
}

void do_oset(Char *ch, const char *argument) {
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];
    Object *obj;
    int value;

    char smash_tilded[MAX_INPUT_LENGTH];
    strncpy(smash_tilded, smash_tilde(argument).c_str(),
            MAX_INPUT_LENGTH - 1); // TODO to minimize changes during refactor
    auto *args = smash_tilded;
    args = one_argument(args, arg1);
    args = one_argument(args, arg2);
    strcpy(arg3, args);

    if (arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0') {
        ch->send_line("Syntax:");
        ch->send_line("  set obj <object> <field> <value>");
        ch->send_line("  Field being one of:");
        ch->send_line("    value0 value1 value2 value3 value4 (v1-v4)");
        ch->send_line("    extra wear level weight cost timer nolocate");
        ch->send_line("    (for extra and wear, use set obj <o> <extra/wear> ? to list");
        return;
    }

    if ((obj = get_obj_world(ch, arg1)) == nullptr) {
        ch->send_line("Nothing like that in heaven or earth.");
        return;
    }

    /*
     * Snarf the value (which need not be numeric).
     */
    value = atoi(arg3);

    /*
     * Set something.
     */
    if (!str_cmp(arg2, "value0") || !str_cmp(arg2, "v0")) {
        obj->value[0] = std::min(75, value);
        return;
    }

    if (!str_cmp(arg2, "value1") || !str_cmp(arg2, "v1")) {
        obj->value[1] = value;
        return;
    }

    if (!str_cmp(arg2, "value2") || !str_cmp(arg2, "v2")) {
        obj->value[2] = value;
        return;
    }

    if (!str_cmp(arg2, "value3") || !str_cmp(arg2, "v3")) {
        obj->value[3] = value;
        return;
    }

    if (!str_cmp(arg2, "value4") || !str_cmp(arg2, "v4")) {
        obj->value[4] = value;
        return;
    }

    if (!str_prefix(arg2, "extra")) {

        ch->send_line("Current extra flags are: ");
        auto flag_args = ArgParser(arg3);
        obj->extra_flags = static_cast<unsigned int>(flag_set(Object::AllExtraFlags, flag_args, obj->extra_flags, ch));
        return;
    }

    if (!str_prefix(arg2, "wear")) {
        ch->send_line("Current wear flags are: ");
        auto flag_args = ArgParser(arg3);
        obj->wear_flags = static_cast<unsigned int>(flag_set(Object::AllWearFlags, flag_args, obj->wear_flags, ch));
        return;
    }

    if (!str_prefix(arg2, "level")) {
        obj->level = value;
        return;
    }

    if (!str_prefix(arg2, "weight")) {
        obj->weight = value;
        return;
    }

    if (!str_prefix(arg2, "cost")) {
        obj->cost = value;
        return;
    }

    if (!str_prefix(arg2, "timer")) {
        obj->timer = value;
        return;
    }

    /* NoLocate flag.  0 turns it off, 1 turns it on *WHAHEY!* */
    if (!str_prefix(arg2, "nolocate")) {
        if (value == 0)
            clear_enum_bit(obj->extra_flags, ObjectExtraFlag::NoLocate);
        else
            set_enum_bit(obj->extra_flags, ObjectExtraFlag::NoLocate);
        return;
    }

    /*
     * Generate usage message.
     */
    do_oset(ch, "");
}

void do_rset(Char *ch, const char *argument) {
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];
    Room *location;
    int value;

    char smash_tilded[MAX_INPUT_LENGTH];
    strncpy(smash_tilded, smash_tilde(argument).c_str(),
            MAX_INPUT_LENGTH - 1); // TODO to minimize changes during refactor
    auto *args = smash_tilded;
    args = one_argument(args, arg1);
    args = one_argument(args, arg2);
    strcpy(arg3, args);

    if (arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0') {
        ch->send_line("Syntax:");
        ch->send_line("  set room <location> <field> <value>");
        ch->send_line("  Field being one of:");
        ch->send_line("    flags sector");
        ch->send_line("  (use set room <location> flags ? to list flags");
        return;
    }

    if ((location = find_location(ch, arg1)) == nullptr) {
        ch->send_line("No such location.");
        return;
    }

    if (!str_prefix(arg2, "flags")) {
        ch->send_line("The current room flags are:");
        auto flag_args = ArgParser(arg3);
        location->room_flags = static_cast<unsigned int>(flag_set(Room::AllFlags, flag_args, location->room_flags, ch));
        return;
    }

    /*
     * Snarf the value.
     */
    if (!is_number(arg3)) {
        ch->send_line("Value must be numeric.");
        return;
    }
    value = atoi(arg3);

    /*
     * Set something.
     */
    if (!str_prefix(arg2, "sector")) {
        if (auto sector_type = try_get_sector_type(value))
            location->sector_type = *sector_type;
        else
            ch->send_line("Invalid sector type number.");
        return;
    }

    /*
     * Generate usage message.
     */
    do_rset(ch, "");
}

void do_sockets(Char *ch, const char *argument) {
    std::string buf;
    char arg[MAX_INPUT_LENGTH];
    int count = 0;

    one_argument(argument, arg);
    const auto view_all = arg[0] == '\0';
    for (auto &d : descriptors().all()) {
        std::string_view name;
        if (auto *victim = d.character()) {
            if (!can_see(ch, victim))
                continue;
            if (view_all || is_name(arg, d.character()->name) || is_name(arg, d.person()->name))
                name = d.person()->name;
        } else if (ch->get_trust() == MAX_LEVEL) {
            // log even connections that haven't logged in yet
            // Level 100s only, mind
            name = "(unknown)";
        }
        if (!name.empty()) {
            count++;
            // Hostname is displayed to high level immortals when using the 'socket' command so that it
            // is possible for them to apply site bans. However, the mud doesn't write the full hostname
            // to the log files when connections are established or closed.
            buf += fmt::format("[{:3} {:>5}] {}@{}\n\r", d.channel(), short_name_of(d.state()), name,
                               d.raw_full_hostname());
        }
    }
    if (count == 0) {
        ch->send_line("No one by that name is connected.");
        return;
    }

    buf += fmt::format("{} user{}\n\r", count, count == 1 ? "" : "s");
    ch->desc->page_to(buf);
}

/*
 * Thanks to Grodyn for pointing out bugs in this function.
 */
void do_force(Char *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];

    argument = one_argument(argument, arg);

    if (arg[0] == '\0' || argument[0] == '\0') {
        ch->send_line("Force whom to do what?");
        return;
    }

    one_argument(argument, arg2);

    if (!str_cmp(arg2, "delete")) {
        ch->send_line("That will NOT be done.");
        return;
    }

    if (!str_cmp(arg2, "private")) {
        ch->send_line("That will NOT be done.");
        return;
    }

    const auto buf = fmt::format("$n forces you to '{}'.", argument);

    if (!str_cmp(arg, "all")) {
        if (ch->get_trust() < DEITY) {
            ch->send_line("Not at your level!");
            return;
        }

        for (auto *vch : char_list) {
            if (vch->is_pc() && vch->get_trust() < ch->get_trust()) {
                MOBtrigger = false;
                act(buf, ch, nullptr, vch, To::Vict);
                interpret(vch, argument);
            }
        }
    } else if (!str_cmp(arg, "players")) {
        if (ch->get_trust() < SUPREME) {
            ch->send_line("Not at your level!");
            return;
        }

        for (auto *vch : char_list) {
            if (vch->is_pc() && vch->get_trust() < ch->get_trust() && vch->level < LEVEL_HERO) {
                MOBtrigger = false;
                act(buf, ch, nullptr, vch, To::Vict);
                interpret(vch, argument);
            }
        }
    } else if (!str_cmp(arg, "gods")) {
        if (ch->get_trust() < SUPREME) {
            ch->send_line("Not at your level!");
            return;
        }

        for (auto *vch : char_list) {
            if (vch->is_pc() && vch->get_trust() < ch->get_trust() && vch->level >= LEVEL_HERO) {
                MOBtrigger = false;
                act(buf, ch, nullptr, vch, To::Vict);
                interpret(vch, argument);
            }
        }
    } else {
        Char *victim;

        if ((victim = get_char_world(ch, arg)) == nullptr) {
            ch->send_line("They aren't here.");
            return;
        }

        if (victim == ch) {
            ch->send_line("Aye aye, right away!");
            return;
        }

        if (victim->get_trust() >= ch->get_trust()) {
            ch->send_line("Do it yourself!");
            return;
        }

        if (victim->is_pc() && ch->get_trust() < DEITY) {
            ch->send_line("Not at your level!");
            return;
        }
        MOBtrigger = false;
        act(buf, ch, nullptr, victim, To::Vict);
        interpret(victim, argument);
    }

    ch->send_line("Ok.");
}

/*
 * New routines by Dionysos.
 */
void do_invis(Char *ch, const char *argument) {
    int level;
    char arg[MAX_STRING_LENGTH];

    if (ch->is_npc())
        return;

    one_argument(argument, arg);

    if (arg[0] == '\0')
        /* take the default path */

        if (check_enum_bit(ch->act, PlayerActFlag::PlrWizInvis)) {
            clear_enum_bit(ch->act, PlayerActFlag::PlrWizInvis);
            ch->invis_level = 0;
            act("$n slowly fades into existence.", ch);
            ch->send_line("You slowly fade back into existence.");
        } else {
            set_enum_bit(ch->act, PlayerActFlag::PlrWizInvis);
            if (check_enum_bit(ch->act, PlayerActFlag::PlrProwl))
                clear_enum_bit(ch->act, PlayerActFlag::PlrProwl);
            ch->invis_level = ch->get_trust();
            act("$n slowly fades into thin air.", ch);
            ch->send_line("You slowly vanish into thin air.");
            if (ch->pet != nullptr) {
                set_enum_bit(ch->pet->act, PlayerActFlag::PlrWizInvis);
                if (check_enum_bit(ch->pet->act, PlayerActFlag::PlrProwl))
                    clear_enum_bit(ch->pet->act, PlayerActFlag::PlrProwl);
                ch->pet->invis_level = ch->get_trust();
            }
        }
    else
    /* do the level thing */
    {
        level = atoi(arg);
        if (level < 2 || level > ch->get_trust()) {
            ch->send_line("Invis level must be between 2 and your level.");
            return;
        } else {
            ch->reply = nullptr;
            set_enum_bit(ch->act, PlayerActFlag::PlrWizInvis);
            ch->invis_level = level;
            act("$n slowly fades into thin air.", ch);
            ch->send_line("You slowly vanish into thin air.");
        }
    }
}

void do_prowl(Char *ch, const char *argument) {
    char arg[MAX_STRING_LENGTH];
    int level = 0;

    if (ch->is_npc())
        return;

    if (ch->get_trust() < LEVEL_HERO) {
        ch->send_line("Huh?");
        return;
    }

    argument = one_argument(argument, arg);
    if (arg[0] == '\0') {
        if (check_enum_bit(ch->act, PlayerActFlag::PlrProwl)) {
            clear_enum_bit(ch->act, PlayerActFlag::PlrProwl);
            ch->invis_level = 0;
            if (ch->pet != nullptr) {
                clear_enum_bit(ch->pet->act, PlayerActFlag::PlrProwl);
                ch->pet->invis_level = 0;
            }
            act("$n slowly fades into existence.", ch);
            ch->send_line("You slowly fade back into existence.");
            return;
        } else {
            ch->invis_level = ch->get_trust();
            set_enum_bit(ch->act, PlayerActFlag::PlrProwl);
            if (ch->pet != nullptr) {
                ch->pet->invis_level = ch->get_trust();
                set_enum_bit(ch->pet->act, PlayerActFlag::PlrProwl);
            }
            act("$n slowly fades into thin air.", ch);
            ch->send_line("You slowly vanish into thin air.");
            clear_enum_bit(ch->act, PlayerActFlag::PlrWizInvis);
            if (ch->pet != nullptr)
                clear_enum_bit(ch->pet->act, PlayerActFlag::PlrWizInvis);
            return;
        }
    }

    level = atoi(arg);

    if ((level > ch->get_trust()) || (level < 2)) {
        ch->send_line("You must specify a level between 2 and your level.");
        return;
    }

    if (check_enum_bit(ch->act, PlayerActFlag::PlrProwl)) {
        clear_enum_bit(ch->act, PlayerActFlag::PlrProwl);
        ch->invis_level = 0;
        if (ch->pet != nullptr) {
            clear_enum_bit(ch->pet->act, PlayerActFlag::PlrProwl);
            ch->pet->invis_level = 0;
        }
        act("$n slowly fades into existence.", ch);
        ch->send_line("You slowly fade back into existence.");
        return;
    } else {
        ch->invis_level = level;
        set_enum_bit(ch->act, PlayerActFlag::PlrProwl);
        if (ch->pet != nullptr) {
            set_enum_bit(ch->pet->act, PlayerActFlag::PlrProwl);
            ch->pet->invis_level = level;
        }
        act("$n slowly fades into thin air.", ch);
        ch->send_line("You slowly vanish into thin air.");
        clear_enum_bit(ch->act, PlayerActFlag::PlrWizInvis);
        if (ch->pet != nullptr)
            clear_enum_bit(ch->pet->act, PlayerActFlag::PlrWizInvis);
        return;
    }
}

void do_holylight(Char *ch) {
    if (ch->is_npc())
        return;

    if (check_enum_bit(ch->act, PlayerActFlag::PlrHolyLight)) {
        clear_enum_bit(ch->act, PlayerActFlag::PlrHolyLight);
        ch->send_line("Holy light mode off.");
    } else {
        set_enum_bit(ch->act, PlayerActFlag::PlrHolyLight);
        ch->send_line("Holy light mode on.");
    }
}

void do_sacname(Char *ch, ArgParser args) {
    if (args.empty()) {
        ch->send_line("You must tell me who they're gonna sacrifice to!");
        ch->send_line("Currently sacrificing to: {}", deity_name);
        return;
    }
    deity_name = args.remaining();
    ch->send_line("Players now sacrifice to {}.", deity_name);
}

void do_smit(Char *ch) { ch->send_line("If you wish to smite someone, then SPELL it out!"); }

void do_smite(Char *ch, const char *argument) {
    /* Power of the Immortals! By Faramir
                                 Don't use this too much, it hurts :) */

    const char *smitestring;
    Char *victim;
    Object *obj;

    if (argument[0] == '\0') {
        ch->send_line("Upon whom do you wish to unleash your power?");
        return;
    }

    if ((victim = get_char_room(ch, argument)) == nullptr) {
        ch->send_line("They aren't here.");
        return;
    } /* Not (visibly) present in room! */

    /* In case a mort thinks he can get away
                             with it, shah right! MMFOOMB
                             Should be dealt with in interp.c already*/

    if (ch->is_npc()) {
        ch->send_line("You must take your true form before unleashing your power.");
        return;
    } /* done whilst switched? No way Jose */

    if (victim->get_trust() > ch->get_trust()) {

        ch->send_line("You failed.\n\rUmmm...beware of retaliation!");
        act("$n attempted to smite $N!", ch, nullptr, victim, To::NotVict);
        act("$n attempted to smite you!", ch, nullptr, victim, To::Vict);
        return;
    }
    /* Immortals cannot smite those with a
                             greater trust than themselves */

    smitestring = "__NeutralSmite";

    if (ch->alignment >= 300) {
        smitestring = "__GoodSmite";
    } /* Good Gods */
    if (ch->alignment <= -300) {
        smitestring = "__BadSmite";
    } /* Establish what message will actually
           be sent when the smite takes place.
           Evil Gods have their own evil method of
           incurring their wrath, and so neutral and
           good Gods =)  smitemessage default of 0
           for neutral. */

    /* ok, now the appropriate smite
                                           string has been determined we must
                                           send it to the victim, and tell
                                           the smiter and others in the room. */

    do_help(victim, smitestring);
    act("|WThe wrath of the Gods has fallen upon you!\n\rYou are blown helplessly from your feet and are stunned!|w",
        ch, nullptr, victim, To::Vict);

    if ((obj = get_eq_char(victim, Wear::Wield)) == nullptr) {
        act("|R$n has been cast down by the power of $N!|w", victim, nullptr, ch, To::NotVict);
    } else {
        act("|R$n has been cast down by the power of $N!\n\rTheir weapon is sent flying!|w", victim, nullptr, ch,
            To::NotVict);
    }

    ch->send_line("You |W>>> |YSMITE|W <<<|w {} with all of your Godly powers!",
                  (victim == ch) ? "yourself" : victim->name);

    victim->hit /= 2; /* easiest way of halving hp? */
    if (victim->hit < 1)
        victim->hit = 1; /* Cap the damage */

    victim->position = Position::Type::Resting;
    /* Knock them into resting and disarm them regardless of whether they have talon or a noremove weapon */

    if ((obj = get_eq_char(victim, Wear::Wield)) == nullptr) {
        return;
    } /* can't be disarmed if no weapon */
    else {
        obj_from_char(obj);
        obj_to_room(obj, victim->in_room);
        if (victim->is_npc() && victim->wait == 0 && can_see_obj(victim, obj))
            get_obj(victim, obj, nullptr);
    } /* disarms them, and NPCs will collect
              their weapon if they can see it.
              Ta-daa, smite compleeeeeet. Ouch. */
}
