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
#include "act_info.hpp"
#include "act_move.hpp"
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
SpecialFunc spec_lookup(std::string_view name);

/*
 * Local functions.
 */
Room *find_location(Char *ch, std::string_view arg);

/* Permits or denies a player from playing the Mud from a PERMIT banned site */
void do_permit(Char *ch, ArgParser args) {
    if (ch->is_npc())
        return;
    if (args.empty()) {
        ch->send_line("Permit whom?");
        return;
    }
    auto set_permit = true;
    auto player = args.shift();
    if (player[0] == '-') {
        player.remove_prefix(1);
        set_permit = false;
    } else if (player[0] == '+')
        player.remove_prefix(1);
    auto *victim = get_char_room(ch, player);
    if (victim == nullptr || victim->is_npc()) {
        ch->send_line("To grant or remove the permit flag, the intended player must be in the same room as you.");
        return;
    }
    if (set_permit) {
        victim->set_extra(CharExtraFlag::Permit);
    } else {
        victim->remove_extra(CharExtraFlag::Permit);
    }
    ch->send_line("PERMIT flag {} for {}.", set_permit ? "set" : "removed", victim->name);
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

void do_bamfin(Char *ch, std::string_view argument) {
    if (ch = ch->player(); !ch)
        return;
    auto bamfin = smash_tilde(argument);

    if (bamfin.empty()) {
        ch->send_line("Your poofin is {}", ch->pcdata->bamfin);
        return;
    }
    if (matches(bamfin, "reset")) {
        ch->pcdata->bamfin = "";
        ch->send_line("Your poofin message has been reset.");
        return;
    }
    if (!matches_inside(ch->name, bamfin)) {
        ch->send_line("You must include your name.");
        return;
    }
    ch->pcdata->bamfin = bamfin;
    ch->send_line("Your poofin is now {}", ch->pcdata->bamfin);
}

void do_bamfout(Char *ch, std::string_view argument) {
    if (ch = ch->player(); !ch)
        return;
    auto bamfout = smash_tilde(argument);
    if (matches(bamfout, "reset")) {
        ch->pcdata->bamfout = "";
        ch->send_line("Your poofout message has been reset.");
        return;
    }
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

void do_deny(Char *ch, ArgParser args) {
    if (args.empty()) {
        ch->send_line("Deny whom?");
        return;
    }
    auto *victim = get_char_world(ch, args.shift());
    if (!victim) {
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

void do_disconnect(Char *ch, ArgParser args) {
    if (args.empty()) {
        ch->send_line("Usage:");
        ch->send_line("   disconnect <player name>");
        ch->send_line("   disconnect <socket number>");
        return;
    }

    if (auto opt_channel = args.try_shift_number()) {
        const uint32_t channel_num = *opt_channel;
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
            if (d.character() && matches(d.character()->name, args.shift())) {
                d.close();
                ch->send_line("Ok.");
                return;
            }
        }
        ch->send_line("Player not found!");
        return;
    }
}

void do_pardon(Char *ch, ArgParser args) {
    auto whom = args.shift();
    auto crime = args.shift();
    if (whom.empty() || crime.empty()) {
        ch->send_line("Syntax: pardon <character> <killer|thief>.");
        return;
    }
    auto *victim = get_char_world(ch, whom);
    if (!victim) {
        ch->send_line("They aren't here.");
        return;
    }
    if (victim->is_npc()) {
        ch->send_line("Not on NPC's.");
        return;
    }
    if (matches(crime, "killer")) {
        if (check_enum_bit(victim->act, PlayerActFlag::PlrKiller)) {
            clear_enum_bit(victim->act, PlayerActFlag::PlrKiller);
            ch->send_line("Killer flag removed.");
            victim->send_line("You are no longer a KILLER.");
        }
        return;
    }
    if (matches(crime, "thief")) {
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

void do_recho(Char *ch, std::string_view argument) {
    if (argument.empty()) {
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

void do_zecho(Char *ch, std::string_view argument) {
    if (argument.empty()) {
        ch->send_line("Zone echo what?");
        return;
    }

    for (auto &victim : descriptors().playing() | DescriptorFilter::same_area(*ch) | DescriptorFilter::to_character()) {
        victim.send_line("{}{}", victim.get_trust() >= ch->get_trust() ? "zone> " : "", argument);
    }
}

void do_pecho(Char *ch, ArgParser args) {
    auto whom = args.shift();
    auto message = args.remaining();
    if (whom.empty() || message.empty()) {
        ch->send_line("Personal echo what?");
        return;
    }
    auto *victim = get_char_world(ch, whom);
    if (!victim) {
        ch->send_line("Target not found.");
        return;
    }
    if (victim->get_trust() >= ch->get_trust() && ch->get_trust() != MAX_LEVEL)
        victim->send_to("personal> ");

    victim->send_line("{}", message);
    ch->send_line("personal> {}", message);
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

void do_at(Char *ch, ArgParser args) {
    if (args.empty()) {
        ch->send_line("At where what?");
        return;
    }
    auto *location = find_location(ch, args.shift());
    if (!location) {
        ch->send_line("No such location.");
        return;
    }
    if (location == ch->in_room) {
        ch->send_line("But that's in here.......");
        return;
    }
    if (room_is_private(location) && (ch->get_trust() < IMPLEMENTOR)) {
        ch->send_line("That room is private right now.");
        return;
    }
    auto *original = ch->in_room;
    char_from_room(ch);
    char_to_room(ch, location);
    interpret(ch, args.remaining());

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

void do_goto(Char *ch, ArgParser args) {
    if (args.empty()) {
        ch->send_line("Goto where?");
        return;
    }

    auto *location = find_location(ch, args.shift());
    if (!location) {
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

void do_mwhere(Char *ch, ArgParser args) {
    bool find_pc = false;
    auto target = args.shift();
    if (target.empty()) {
        find_pc = true;
    } else if (target.length() < 2) {
        ch->send_line("Please be more specific.");
        return;
    }

    bool found = false;
    int number = 0;
    std::string buffer;
    for (auto *victim : char_list) {
        if ((victim->is_npc() && victim->in_room != nullptr && is_name(target, victim->name) && !find_pc)
            || (victim->is_pc() && find_pc && can_see(ch, victim))) {
            found = true;
            number++;
            buffer += fmt::format("{:3} [{:5}] {:<28} [{:5}] {:>20}\n\r", number, find_pc ? 0 : victim->mobIndex->vnum,
                                  victim->short_name(), victim->in_room->vnum, victim->in_room->name);
        }
    }
    ch->page_to(buffer);

    if (!found)
        act("You didn't find any $T.", ch, nullptr, target, To::Char); // Poor Arthur.
}

void do_reboo(Char *ch) { ch->send_line("If you want to REBOOT, spell it out."); }

void do_reboot(Char *ch) {
    extern bool merc_down;

    if (!check_enum_bit(ch->act, PlayerActFlag::PlrWizInvis)) {
        do_echo(ch, fmt::format("Reboot by {}.", ch->name));
    }
    do_force(ch, ArgParser("all save"));
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
    do_force(ch, ArgParser("all save"));
    do_save(ch);
    merc_down = true;
    for (auto &d : descriptors().all())
        d.close();
}

void do_snoop(Char *ch, ArgParser args) {
    if (!ch->desc) {
        // MRG old code was split-brained about checking this. Seems like nothing would have worked if ch->desc was
        // actually null, so bugging out here.
        bug("null ch->desc in do_snoop");
        return;
    }
    if (args.empty()) {
        ch->send_line("Snoop whom?");
        return;
    }
    auto *victim = get_char_world(ch, args.shift());
    if (!victim) {
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

void do_switch(Char *ch, ArgParser args) {
    if (args.empty()) {
        ch->send_line("Switch into whom?");
        return;
    }
    if (!ch->desc)
        return;
    if (ch->desc->is_switched()) {
        ch->send_line("You are already switched.");
        return;
    }
    auto *victim = get_char_world(ch, args.shift());
    if (!victim) {
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
    if (victim->desc) {
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
void do_clone(Char *ch, ArgParser args) {
    if (args.empty()) {
        ch->send_line("Clone what?");
        return;
    }
    auto arg = args.shift();
    Char *mob = nullptr;
    Object *obj = nullptr;
    if (matches_start(arg, "object")) {
        obj = get_obj_here(ch, args.remaining());
    } else if (matches_start(arg, "mobile") || matches_start(arg, "character")) {
        mob = get_char_room(ch, args.remaining());
    } else { /* find both */
        mob = get_char_room(ch, arg);
        obj = get_obj_here(ch, arg);
    }

    /* clone an object */
    if (obj != nullptr) {
        if (!obj_check(ch, obj)) {
            ch->send_line("Your powers are not great enough for such a task.");
            return;
        }

        auto *clone = create_object(obj->objIndex);
        clone_object(obj, clone);
        if (obj->carried_by != nullptr)
            obj_to_char(clone, ch);
        else
            obj_to_room(clone, ch->in_room);
        recursive_clone(ch, obj, clone);

        act("$n has created $p.", ch, clone, nullptr, To::Room);
        act("You clone $p.", ch, clone, nullptr, To::Char);
    } else if (mob != nullptr) {

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

        auto *cloned_mob = create_mobile(mob->mobIndex);
        clone_mobile(mob, cloned_mob);

        for (auto *carried : mob->carrying) {
            if (obj_check(ch, carried)) {
                auto *cloned_obj = create_object(carried->objIndex);
                clone_object(carried, cloned_obj);
                recursive_clone(ch, carried, cloned_obj);
                obj_to_char(cloned_obj, cloned_mob);
                cloned_obj->wear_loc = carried->wear_loc;
            }
        }
        char_to_room(cloned_mob, ch->in_room);
        act("$n has created $N.", ch, nullptr, cloned_mob, To::Room);
        act("You clone $N.", ch, nullptr, cloned_mob, To::Char);
    } else {
        ch->send_line("You don't see that here.");
    }
}

/* RT to replace the two load commands */

void do_mload(Char *ch, ArgParser args) {
    const auto opt_vnum = args.try_shift_number();
    if (!opt_vnum) {
        ch->send_line("Syntax: load mob <vnum>.");
        return;
    }
    auto *mob_index = get_mob_index(*opt_vnum);
    if (!mob_index) {
        ch->send_line("No mob has that vnum.");
        return;
    }

    auto *victim = create_mobile(mob_index);
    char_to_room(victim, ch->in_room);
    act("$n has created $N!", ch, nullptr, victim, To::Room);
    ch->send_line("Ok.");
}

void do_oload(Char *ch, ArgParser args) {
    const auto opt_vnum = args.try_shift_number();
    if (!opt_vnum) {
        ch->send_line("Syntax: load obj <vnum>.");
        return;
    }
    auto *obj_index = get_obj_index(*opt_vnum);
    if (!obj_index) {
        ch->send_line("No object has that vnum.");
        return;
    }
    auto *obj = create_object(obj_index);
    if (obj->is_takeable())
        obj_to_char(obj, ch);
    else
        obj_to_room(obj, ch->in_room);
    act("$n has created $p!", ch, obj, nullptr, To::Room);
    ch->send_line("Ok.");
}

void do_load(Char *ch, ArgParser args) {
    auto type = args.shift();
    if (matches(type, "mob") || matches(type, "char")) {
        do_mload(ch, args);
    } else if (matches(type, "obj")) {
        do_oload(ch, args);
    } else {
        ch->send_line("Syntax:");
        ch->send_line("  load mob <vnum>");
        ch->send_line("  load obj <vnum>");
    }
}

void do_purge(Char *ch, ArgParser args) {
    if (args.empty()) {
        auto obj_count = 0;
        auto char_count = 0;
        for (auto *victim : ch->in_room->people) {
            if (victim->is_npc() && !check_enum_bit(victim->act, CharActFlag::NoPurge)
                && victim != ch /* safety precaution */) {
                extract_char(victim, true);
                char_count++;
            }
        }
        for (auto *obj : ch->in_room->contents) {
            if (!obj->is_no_purge()) {
                extract_obj(obj);
                obj_count++;
            }
        }
        act("$n purges the room!", ch);
        ch->send_line("Purged characters: {}, objects: {}", char_count, obj_count);
        return;
    }
    auto *victim = get_char_world(ch, args.shift());
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
        auto *desc = victim->desc;
        extract_char(victim, true);
        if (desc != nullptr)
            desc->close();
        return;
    }
    act("$n purges $N.", ch, nullptr, victim, To::NotVict);
    extract_char(victim, true);
}

void do_advance(Char *ch, ArgParser args) {
    const auto name = args.shift();
    const auto opt_level = args.try_shift_number();
    if (name.empty() || !opt_level) {
        ch->send_line("Syntax: advance <char> <level>.");
        return;
    }

    auto *victim = get_char_room(ch, name);
    if (!victim) {
        ch->send_line("That player is not here.");
        return;
    }

    if (victim->is_npc()) {
        ch->send_line("Not on NPC's.");
        return;
    }
    const auto level = std::clamp(*opt_level, 1, MAX_LEVEL);
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
        for (auto vict_level = victim->level; vict_level < level; vict_level++) {
            victim->send_line("You raise a level!!  ");
            victim->level += 1;
            advance_level(victim);
        }
        victim->exp = (exp_per_level(victim, victim->pcdata->points) * std::max(1_s, victim->level));
        victim->trust = 0;
        save_char_obj(victim);
    }
}

void do_trust(Char *ch, ArgParser args) {
    auto whom = args.shift();
    const auto opt_level = args.try_shift_number();
    if (whom.empty()) {
        ch->send_line("Syntax: trust <char> <level>.");
        return;
    }
    auto *victim = get_char_room(ch, whom);
    if (!victim) {
        ch->send_line("That player is not here.");
        return;
    }
    if (!opt_level) {
        ch->send_line("{} has a trust of {}.", victim->name, victim->trust);
        return;
    }
    if (victim->is_npc()) {
        ch->send_line("Not on NPCs.");
        return;
    }
    if (*opt_level < 0 || *opt_level > 100) {
        ch->send_line("Level must be 0 (reset) or 1 to 100.");
        return;
    }
    if (*opt_level > ch->get_trust()) {
        ch->send_line("Limited to your trust.");
        return;
    }
    victim->trust = *opt_level;
}

void do_restore(Char *ch, ArgParser args) {
    auto target = args.shift();
    if (target.empty() || matches(target, "room")) {
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
    if (ch->get_trust() >= MAX_LEVEL && matches(target, "all")) {
        /* cure all */

        for (auto &d : descriptors().playing()) {
            auto *victim = d.character();
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
    auto *victim = get_char_world(ch, target);
    if (!victim) {
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

void do_freeze(Char *ch, ArgParser args) {
    if (args.empty()) {
        ch->send_line("Freeze whom?");
        return;
    }

    auto *victim = get_char_world(ch, args.shift());
    if (!victim) {
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

void do_log(Char *ch, ArgParser args) {
    if (args.empty()) {
        ch->send_line("Log whom?");
        return;
    }
    auto target = args.shift();
    if (matches(target, "all")) {
        if (fLogAll) {
            fLogAll = false;
            ch->send_line("Log ALL off.");
        } else {
            fLogAll = true;
            ch->send_line("Log ALL on.");
        }
        return;
    }
    auto *victim = get_char_world(ch, target);
    if (!victim) {
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

namespace {

// Enables imms to toggle the comm channel flags on a victim (NoChannel, NoTell, NoEmote, NoYell).
// Useful if a player is spamming or acting inappropriately.
void toggle_player_comm_flag(Char *ch, ArgParser args, const CommFlag comm_flag, std::string_view verb) {
    if (args.empty()) {
        ch->send_line("Mute which player?");
        return;
    }
    auto *victim = get_char_world(ch, args.shift());
    if (!victim || victim->is_npc()) {
        ch->send_line("No player with that name found.");
        return;
    }

    if (victim->get_trust() >= ch->get_trust()) {
        ch->send_line("You failed.");
        return;
    }

    if (check_enum_bit(victim->comm, comm_flag)) {
        clear_enum_bit(victim->comm, comm_flag);
        victim->send_line("You can {} again.", verb);
        ch->send_line("{} can {} again.", victim->name, verb);
    } else {
        set_enum_bit(victim->comm, comm_flag);
        victim->send_line("You can no longer {}!", verb);
        ch->send_line("{} can no longer {}.", victim->name, verb);
    }
}

}

void do_noemote(Char *ch, ArgParser args) { toggle_player_comm_flag(ch, args, CommFlag::NoEmote, "emote"); }

void do_noyell(Char *ch, ArgParser args) { toggle_player_comm_flag(ch, args, CommFlag::NoYell, "yell"); }

void do_notell(Char *ch, ArgParser args) { toggle_player_comm_flag(ch, args, CommFlag::NoTell, "tell"); }

void do_nochannels(Char *ch, ArgParser args) { toggle_player_comm_flag(ch, args, CommFlag::NoChannels, "talk"); }

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

void do_awaken(Char *ch, ArgParser args) {
    if (args.empty()) {
        ch->send_line("Awaken whom?");
        return;
    }
    auto *victim = get_char_room(ch, args.shift());
    if (!victim) {
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
    act("You wake $N up.", ch, nullptr, victim, To::Char);
    act("You wake up.", victim, nullptr, nullptr, To::Char);
    act("$n gives $N a kick and wakes them up.", ch, nullptr, victim, To::NotVict, MobTrig::Yes,
        Position::Type::Resting);
}

void do_owhere(Char *ch, ArgParser args) {
    Object *in_obj;
    bool found = false;
    int number = 0;
    if (args.empty()) {
        ch->send_line("Owhere which object?");
        return;
    }
    auto obj_name = args.shift();
    if (obj_name.length() < 2) {
        ch->send_line("Please be more specific.");
        return;
    }
    std::string buffer;
    for (auto *obj : object_list) {
        if (!is_name(obj_name, obj->name))
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
void do_coma(Char *ch, ArgParser args) {
    if (args.empty()) {
        ch->send_line("Comatose whom?");
        return;
    }

    auto *victim = get_char_room(ch, args.shift());
    if (!victim) {
        ch->send_line("They aren't here.");
        return;
    }

    if (victim->is_aff_sleep())
        return;

    if (ch == victim) {
        ch->send_line("Duh!  Don't you dare fall asleep on the job!");
        return;
    }
    if (ch->get_trust() <= victim->get_trust()) {
        ch->send_line("You failed.");
        return;
    }

    AFFECT_DATA af;
    af.type = skill_lookup("sleep");
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

void do_set(Char *ch, ArgParser args) {
    const auto show_usage = [&ch]() {
        ch->send_line("Syntax:");
        ch->send_line("  set mob   <name> <field> <value>");
        ch->send_line("  set obj   <name> <field> <value>");
        ch->send_line("  set room  <room> <field> <value>");
        ch->send_line("  set skill <name> <spell or skill> <value>");
    };
    if (args.empty()) {
        show_usage();
        return;
    }
    auto type = args.shift();
    if (matches_start(type, "mobile") || matches_start(type, "character")) {
        do_mset(ch, args);
        return;
    }

    if (matches_start(type, "skill") || matches_start(type, "spell")) {
        do_sset(ch, args);
        return;
    }

    if (matches_start(type, "object")) {
        do_oset(ch, args);
        return;
    }

    if (matches_start(type, "room")) {
        do_rset(ch, args);
        return;
    }
    show_usage();
}

void do_sset(Char *ch, ArgParser args) {
    auto target = args.shift();
    auto skill_name = args.shift();
    const auto opt_value_num = args.try_shift_number();
    if (target.empty() || skill_name.empty()) {
        ch->send_line("Syntax:");
        ch->send_line("  set skill <name> <spell or skill> <value>");
        ch->send_line("  set skill <name> all <value>");
        ch->send_line("   (use the name of the skill, not the number)");
        return;
    }
    auto *victim = get_char_world(ch, target);
    if (!victim) {
        ch->send_line("They aren't here.");
        return;
    }
    if (victim->is_npc()) {
        ch->send_line("Not on NPC's.");
        return;
    }
    if (!opt_value_num) {
        ch->send_line("Value must be numeric.");
        return;
    }
    const auto value = *opt_value_num;
    if (value < 0 || value > 100) {
        ch->send_line("Value range is 0 to 100.");
        return;
    }
    if (matches(skill_name, "all")) {
        for (auto sn = 0; sn < MAX_SKILL; sn++) {
            if (skill_table[sn].name != nullptr)
                victim->pcdata->learned[sn] = value;
        }
    } else if (const auto skill_num = skill_lookup(skill_name); skill_num > 0) {
        victim->pcdata->learned[skill_num] = value;
    } else {
        ch->send_line("No such skill or spell: {}.", skill_name);
    }
}

// TODO: this could be rewritten to be less procedural.
void do_mset(Char *ch, ArgParser args) {
    const auto show_usage = [&ch]() {
        ch->send_line("Syntax:");
        ch->send_line("  set char <name> <field> <value>");
        ch->send_line("  Field being one of:");
        ch->send_line("    str int wis dex con sex class level");
        ch->send_line("    race gold hp mana move practice align");
        ch->send_line("    dam hit train thirst drunk full hours");
    };
    auto target = args.shift();
    auto field = args.shift();
    const auto opt_value_num = args.try_shift_number();
    auto value_str = opt_value_num.has_value() ? "" : smash_tilde(args.remaining());
    if (target.empty() || field.empty()) {
        show_usage();
        return;
    }
    auto *victim = get_char_world(ch, target);
    if (!victim) {
        ch->send_line("They aren't here.");
        return;
    }
    if (matches(field, "str")) {
        if (!opt_value_num || *opt_value_num < 3 || *opt_value_num > get_max_train(victim, Stat::Str)) {
            ch->send_line("Strength range is 3 to {}.", get_max_train(victim, Stat::Str));
            return;
        }
        victim->perm_stat[Stat::Str] = *opt_value_num;
        return;
    }
    if (matches(field, "int")) {
        if (!opt_value_num || *opt_value_num < 3 || *opt_value_num > get_max_train(victim, Stat::Int)) {
            ch->send_line("Intelligence range is 3 to {}.", get_max_train(victim, Stat::Int));
            return;
        }
        victim->perm_stat[Stat::Int] = *opt_value_num;
        return;
    }
    if (matches(field, "wis")) {
        if (!opt_value_num || *opt_value_num < 3 || *opt_value_num > get_max_train(victim, Stat::Wis)) {
            ch->send_line("Wisdom range is 3 to {}.", get_max_train(victim, Stat::Wis));
            return;
        }
        victim->perm_stat[Stat::Wis] = *opt_value_num;
        return;
    }
    if (matches(field, "dex")) {
        if (!opt_value_num || *opt_value_num < 3 || *opt_value_num > get_max_train(victim, Stat::Dex)) {
            ch->send_line("Dexterity ranges is 3 to {}.", get_max_train(victim, Stat::Dex));
            return;
        }
        victim->perm_stat[Stat::Dex] = *opt_value_num;
        return;
    }
    if (matches(field, "con")) {
        if (!opt_value_num || *opt_value_num < 3 || *opt_value_num > get_max_train(victim, Stat::Con)) {
            ch->send_line("Constitution range is 3 to {}.", get_max_train(victim, Stat::Con));
            return;
        }
        victim->perm_stat[Stat::Con] = *opt_value_num;
        return;
    }
    if (matches_start(field, "sex")) {
        if (auto sex = Sex::try_from_name(value_str)) {
            victim->sex = *sex;
            if (victim->is_pc())
                victim->pcdata->true_sex = *sex;
        } else {
            ch->send_line("Invalid sex. Options: " + Sex::names_csv());
        }
        return;
    }
    if (matches_start(field, "class")) {
        if (victim->is_npc()) {
            ch->send_line("Mobiles have no class.");
            return;
        }
        const auto class_num = class_lookup(value_str);
        if (class_num == -1) {
            std::string buf{"Possible classes are: "};
            // TODO: use ranges & fmt, and maybe put it in a Classes class.
            for (size_t c = 0; c < MAX_CLASS; c++) {
                if (c > 0)
                    buf += " ";
                buf += class_table[c].name;
            }
            ch->send_line(buf);
            return;
        }
        victim->class_num = class_num;
        return;
    }
    if (matches_start(field, "level")) {
        if (victim->is_pc()) {
            ch->send_line("Not on PC's.");
            return;
        }
        if (!opt_value_num || *opt_value_num < 0 || *opt_value_num > 200) {
            ch->send_line("Level range is 0 to 200.");
            return;
        }
        victim->level = *opt_value_num;
        return;
    }
    if (matches_start(field, "gold")) {
        if (!opt_value_num || *opt_value_num < 0 || *opt_value_num > 100000) {
            ch->send_line("Specify a gold value of 0 to 100,000.");
            return;
        }
        victim->gold = *opt_value_num;
        return;
    }
    if (matches_start(field, "hp")) {
        if (!opt_value_num || *opt_value_num < 1 || *opt_value_num > 30000) {
            ch->send_line("Hp range is 1 to 30,000 hit points.");
            return;
        }
        victim->max_hit = *opt_value_num;
        if (victim->is_pc())
            victim->pcdata->perm_hit = *opt_value_num;
        return;
    }
    if (matches_start(field, "mana")) {
        if (!opt_value_num || *opt_value_num < 0 || *opt_value_num > 30000) {
            ch->send_line("Mana range is 0 to 30,000 mana points.");
            return;
        }
        victim->max_mana = *opt_value_num;
        if (victim->is_pc())
            victim->pcdata->perm_mana = *opt_value_num;
        return;
    }
    if (matches_start(field, "move")) {
        if (!opt_value_num || *opt_value_num < 0 || *opt_value_num > 30000) {
            ch->send_line("Move range is 0 to 30,000 move points.");
            return;
        }
        victim->max_move = *opt_value_num;
        if (victim->is_pc())
            victim->pcdata->perm_move = *opt_value_num;
        return;
    }
    if (matches_start(field, "practice")) {
        if (!opt_value_num || *opt_value_num < 0 || *opt_value_num > 250) {
            ch->send_line("Practice range is 0 to 250 sessions.");
            return;
        }
        victim->practice = *opt_value_num;
        return;
    }
    if (matches_start(field, "train")) {
        if (!opt_value_num || *opt_value_num < 0 || *opt_value_num > 50) {
            ch->send_line("Training session range is 0 to 50 sessions.");
            return;
        }
        victim->train = *opt_value_num;
        return;
    }
    if (matches_start(field, "align")) {
        if (!opt_value_num || *opt_value_num < -1000 || *opt_value_num > 1000) {
            ch->send_line("Alignment range is -1000 to 1000.");
            return;
        }
        victim->alignment = *opt_value_num;
        return;
    }
    if (matches_start(field, "dam")) {
        if (!opt_value_num || *opt_value_num < 1 || *opt_value_num > 100) {
            ch->send_line("|RDamroll range is 1 to 100.|w");
            return;
        }
        victim->damroll = *opt_value_num;
        return;
    }
    if (matches_start(field, "hit")) {
        if (!opt_value_num || *opt_value_num < 1 || *opt_value_num > 100) {
            ch->send_line("|RHitroll range is 1 to 100.|w");
            return;
        }
        victim->hitroll = *opt_value_num;
        return;
    }
    if (matches_start(field, "thirst")) {
        if (victim->is_npc()) {
            ch->send_line("Not on NPC's.");
            return;
        }
        if (!opt_value_num) {
            ch->send_line("Specify a thirst number.");
            return;
        }
        victim->pcdata->thirst.set(*opt_value_num);
        return;
    }
    if (matches_start(field, "drunk")) {
        if (victim->is_npc()) {
            ch->send_line("Not on NPC's.");
            return;
        }
        if (!opt_value_num) {
            ch->send_line("Specify a drunk number.");
            return;
        }
        victim->pcdata->inebriation.set(*opt_value_num);
        return;
    }
    if (matches_start(field, "full")) {
        if (victim->is_npc()) {
            ch->send_line("Not on NPC's.");
            return;
        }
        if (!opt_value_num) {
            ch->send_line("Specify a full number.");
            return;
        }
        victim->pcdata->hunger.set(*opt_value_num);
        return;
    }
    if (matches_start(field, "race")) {
        const int race = race_lookup(value_str);
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
    if (matches_start(field, "hours")) {
        if (!opt_value_num || *opt_value_num < 1 || *opt_value_num > 999) {
            ch->send_line("|RHours range is 1 to 999.|w");
            return;
        }
        victim->played = std::chrono::hours(*opt_value_num);
        return;
    }
    show_usage();
}

void do_string(Char *ch, ArgParser args) {
    const auto show_usage = [&ch]() {
        ch->send_line("Syntax:");
        ch->send_line("  string char <name> <field> <string>");
        ch->send_line("    fields: name short long desc title spec");
        ch->send_line("  string obj  <name> <field> <string>");
        ch->send_line("    fields: name short long extended wear");
    };
    auto type = args.shift();
    auto target = args.shift();
    auto field = args.shift();
    auto value = smash_tilde(args.remaining());
    if (type.empty() || target.empty() || field.empty() || value.empty()) {
        show_usage();
        return;
    }

    if (matches_start(type, "character") || matches_start(type, "mobile")) {
        auto *victim = get_char_world(ch, target);
        if (!victim) {
            ch->send_line("They aren't here.");
            return;
        }
        if (matches_start(field, "name")) {
            if (victim->is_pc()) {
                ch->send_line("Not on PC's.");
                return;
            }
            victim->name = value;
            return;
        }
        if (matches_start(field, "description")) {
            victim->description = value + "\n\r";
            return;
        }
        if (matches_start(field, "short")) {
            victim->short_descr = value + "\n\r";
            return;
        }
        if (matches_start(field, "long")) {
            victim->long_descr = value + "\n\r";
            return;
        }
        if (matches_start(field, "title")) {
            if (victim->is_npc()) {
                ch->send_line("Not on NPC's.");
                return;
            }
            victim->set_title(value);
            return;
        }
        if (matches_start(field, "spec")) {
            if (victim->is_pc()) {
                ch->send_line("Not on PC's.");
                return;
            }
            if ((victim->spec_fun = spec_lookup(value)) == 0) {
                ch->send_line("No such spec fun.");
                return;
            }
            return;
        }
    }
    if (matches_start(type, "object")) {
        auto *obj = get_obj_world(ch, target);
        if (!obj) {
            ch->send_line("Nothing like that in heaven or earth.");
            return;
        }
        if (matches_start(field, "name")) {
            obj->name = value;
            return;
        }
        if (matches_start(field, "short")) {
            obj->short_descr = value;
            return;
        }
        if (matches_start(field, "long")) {
            obj->description = value;
            return;
        }
        if (matches_start(field, "wear")) {
            if (value.length() > 17) {
                ch->send_line("Wear_Strings may not be longer than 17 chars.");
            } else {
                obj->wear_string = value;
            }
            return;
        }

        if (matches_start(field, "ed") || matches_start(field, "extended")) {
            auto extended_parser = ArgParser(value);
            auto ext_keyword = std::string(extended_parser.shift());
            auto ext_desc = std::string(extended_parser.remaining());
            if (ext_keyword.empty() || ext_desc.empty()) {
                ch->send_line("Syntax: string obj <object> ed <keyword> <string>");
                return;
            }
            ext_desc += "\n\r";
            obj->extra_descr.emplace_back(ExtraDescription{ext_keyword, ext_desc});
            return;
        }
    }
    show_usage();
}

namespace {

bool oset_value_field(Char *ch, Object *obj, std::string_view specified_field, const size_t index,
                      const std::optional<int> opt_num) {
    if (matches(specified_field, fmt::format("value{}", index))) {
        if (opt_num) {
            obj->value[index] = *opt_num;
        } else {
            ch->send_line("Value field requires a number.");
        }
        return true;
    }
    return false;
}

}

void do_oset(Char *ch, ArgParser args) {
    const auto show_usage = [&ch]() {
        ch->send_line("Syntax:");
        ch->send_line("  set obj <object> <field> <value>");
        ch->send_line("  Field being one of:");
        ch->send_line("    value0 value1 value2 value3 value4 (v1-v4)");
        ch->send_line("    extra wear level weight cost timer nolocate");
        ch->send_line("    (for extra and wear, use set obj <o> <extra/wear> ? to list");
    };
    auto target = args.shift();
    auto field = args.shift();
    const auto opt_value_num = args.try_shift_number();
    auto value_str = opt_value_num.has_value() ? "" : smash_tilde(args.remaining());
    if (target.empty() || field.empty()) {
        show_usage();
        return;
    }
    auto *obj = get_obj_world(ch, target);
    if (!obj) {
        ch->send_line("Nothing like that in heaven or earth.");
        return;
    }
    for (size_t index = 0; index < 5; index++) {
        if (oset_value_field(ch, obj, field, index, opt_value_num)) {
            return;
        }
    }
    if (matches_start(field, "extra")) {
        ch->send_line("Current extra flags are: ");
        auto flag_args = ArgParser(value_str);
        obj->extra_flags = static_cast<unsigned int>(flag_set(Object::AllExtraFlags, flag_args, obj->extra_flags, ch));
        return;
    }
    if (matches_start(field, "wear")) {
        ch->send_line("Current wear flags are: ");
        auto flag_args = ArgParser(value_str);
        obj->wear_flags = static_cast<unsigned int>(flag_set(Object::AllWearFlags, flag_args, obj->wear_flags, ch));
        return;
    }
    if (matches_start(field, "level")) {
        if (!opt_value_num || *opt_value_num < 0 || *opt_value_num > 200) {
            ch->send_line("Level range is 0 to 200.");
            return;
        }
        obj->level = *opt_value_num;
        return;
    }
    if (matches_start(field, "weight")) {
        if (!opt_value_num || *opt_value_num < 0 || *opt_value_num > 100000) {
            ch->send_line("Weight range is 0 to 100000.");
            return;
        }
        obj->weight = *opt_value_num;
        return;
    }
    if (matches_start(field, "cost")) {
        if (!opt_value_num || *opt_value_num < 0 || *opt_value_num > 100000) {
            ch->send_line("Cost range is 0 to 100000.");
            return;
        }
        obj->cost = *opt_value_num;
        return;
    }
    if (matches_start(field, "timer")) {
        if (!opt_value_num || *opt_value_num < 0 || *opt_value_num > 250) {
            ch->send_line("Timer range is 0 to 250.");
            return;
        }
        obj->timer = *opt_value_num;
        return;
    }

    /* NoLocate flag.  0 turns it off, everything else turns it on *WHAHEY!* */
    if (matches(field, "nolocate")) {
        if (opt_value_num && *opt_value_num == 0)
            clear_enum_bit(obj->extra_flags, ObjectExtraFlag::NoLocate);
        else
            set_enum_bit(obj->extra_flags, ObjectExtraFlag::NoLocate);
        return;
    }
    show_usage();
}

void do_rset(Char *ch, ArgParser args) {
    const auto show_usage = [&ch]() {
        ch->send_line("Syntax:");
        ch->send_line("  set room <location> <field> <value>");
        ch->send_line("  Field being one of:");
        ch->send_line("    flags sector");
        ch->send_line("  (use set room <location> flags ? to list flags");
    };
    auto target = args.shift();
    auto field = args.shift();
    const auto opt_value_num = args.try_shift_number();
    auto value_str = opt_value_num.has_value() ? "" : smash_tilde(args.remaining());
    if (target.empty() || field.empty()) {
        show_usage();
        return;
    }
    auto *location = find_location(ch, target);
    if (!location) {
        ch->send_line("No such location.");
        return;
    }
    if (matches_start(field, "flags")) {
        ch->send_line("The current room flags are:");
        auto flag_args = ArgParser(value_str);
        location->room_flags = static_cast<unsigned int>(flag_set(Room::AllFlags, flag_args, location->room_flags, ch));
        return;
    }
    if (matches_start(field, "sector")) {
        if (!opt_value_num) {
            ch->send_line("Please specify a sector number.");
        }
        if (auto sector_type = try_get_sector_type(*opt_value_num))
            location->sector_type = *sector_type;
        else
            ch->send_line("Invalid sector type number.");
        return;
    }
    show_usage();
}

void do_sockets(Char *ch, ArgParser args) {
    std::string buf;
    int count = 0;
    const auto view_all = args.empty();
    auto target = args.shift();
    for (auto &d : descriptors().all()) {
        std::string_view name;
        if (auto *victim = d.character()) {
            if (!can_see(ch, victim))
                continue;
            if (view_all || is_name(target, d.character()->name) || is_name(target, d.person()->name))
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
void do_force(Char *ch, ArgParser args) {
    auto target = args.shift();
    auto command = args.remaining();
    if (target.empty() || command.empty()) {
        ch->send_line("Force whom to do what?");
        return;
    }
    if (matches(command, "delete") || matches(command, "private")) {
        ch->send_line("That will NOT be done.");
        return;
    }
    const auto buf = fmt::format("$n forces you to '{}'.", command);
    if (matches(target, "all")) {
        if (ch->get_trust() < DEITY) {
            ch->send_line("Not at your level!");
            return;
        }

        for (auto *vch : char_list) {
            if (vch->is_pc() && vch->get_trust() < ch->get_trust()) {
                act(buf, ch, nullptr, vch, To::Vict, MobTrig::No);
                interpret(vch, command);
            }
        }
    } else if (matches(target, "players")) {
        if (ch->get_trust() < SUPREME) {
            ch->send_line("Not at your level!");
            return;
        }

        for (auto *vch : char_list) {
            if (vch->is_pc() && vch->get_trust() < ch->get_trust() && vch->level < LEVEL_HERO) {
                act(buf, ch, nullptr, vch, To::Vict, MobTrig::No);
                interpret(vch, command);
            }
        }
    } else if (matches(target, "gods")) {
        if (ch->get_trust() < SUPREME) {
            ch->send_line("Not at your level!");
            return;
        }

        for (auto *vch : char_list) {
            if (vch->is_pc() && vch->get_trust() < ch->get_trust() && vch->level >= LEVEL_HERO) {
                act(buf, ch, nullptr, vch, To::Vict, MobTrig::No);
                interpret(vch, command);
            }
        }
    } else {
        auto *victim = get_char_world(ch, target);
        if (!victim) {
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
        act(buf, ch, nullptr, victim, To::Vict, MobTrig::No);
        interpret(victim, command);
    }

    ch->send_line("Ok.");
}

namespace {
void toggle_wiz_visibility(Char *ch, ArgParser args, const PlayerActFlag invis_flag,
                           const PlayerActFlag complementary_flag) {
    if (ch->is_npc())
        return;
    const auto opt_level = args.try_shift_number();
    const auto invis_level = opt_level ? std::clamp(*opt_level, 2, ch->get_trust()) : ch->get_trust();
    const auto is_changing_level = opt_level.has_value();
    if (!is_changing_level && check_enum_bit(ch->act, invis_flag)) {
        clear_enum_bit(ch->act, invis_flag);
        ch->invis_level = 0;
        if (ch->pet != nullptr) {
            clear_enum_bit(ch->pet->act, invis_flag);
            ch->pet->invis_level = 0;
        }
        act("$n slowly fades into existence.", ch);
        ch->send_line("You slowly fade back into existence.");
    } else {
        ch->invis_level = invis_level;
        set_enum_bit(ch->act, invis_flag);
        if (ch->pet != nullptr) {
            ch->pet->invis_level = invis_level;
            set_enum_bit(ch->pet->act, invis_flag);
        }
        act("$n slowly fades into thin air.", ch);
        ch->send_line("You slowly vanish into thin air.");
        clear_enum_bit(ch->act, complementary_flag);
        if (ch->pet != nullptr)
            clear_enum_bit(ch->pet->act, complementary_flag);
    }
}
}

void do_invis(Char *ch, ArgParser args) {
    toggle_wiz_visibility(ch, args, PlayerActFlag::PlrWizInvis, PlayerActFlag::PlrProwl);
}

void do_prowl(Char *ch, ArgParser args) {
    toggle_wiz_visibility(ch, args, PlayerActFlag::PlrProwl, PlayerActFlag::PlrWizInvis);
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

void do_smite(Char *ch, ArgParser args) {
    if (args.empty()) {
        ch->send_line("Upon whom do you wish to unleash your power?");
        return;
    }
    auto *victim = get_char_room(ch, args.shift());
    if (!victim) {
        ch->send_line("They aren't here.");
        return;
    }
    if (ch->is_npc()) {
        ch->send_line("You must take your true form before unleashing your power.");
        return;
    }
    if (victim->get_trust() > ch->get_trust()) {
        ch->send_line("You failed.\n\rUmmm...beware of retaliation!");
        act("$n attempted to smite $N!", ch, nullptr, victim, To::NotVict);
        act("$n attempted to smite you!", ch, nullptr, victim, To::Vict);
        return;
    }
    /* Immortals cannot smite those with a
                             greater trust than themselves */

    auto *smitestring = "__NeutralSmite";
    if (ch->alignment >= 300) {
        smitestring = "__GoodSmite";
    } else if (ch->alignment <= -300) {
        smitestring = "__BadSmite";
    }
    do_help(victim, smitestring);
    act("|WThe wrath of the Gods has fallen upon you!\n\rYou are blown helplessly from your feet and are stunned!|w",
        ch, nullptr, victim, To::Vict);
    auto *wielded_obj = get_eq_char(victim, Wear::Wield);
    if (wielded_obj) {
        act("|R$n has been cast down by the power of $N!\n\rTheir weapon is sent flying!|w", victim, nullptr, ch,
            To::NotVict);
    } else {
        act("|R$n has been cast down by the power of $N!|w", victim, nullptr, ch, To::NotVict);
    }
    ch->send_line("You |W>>> |YSMITE|W <<<|w {} with all of your Godly powers!",
                  (victim == ch) ? "yourself" : victim->name);

    victim->hit /= 2; /* easiest way of halving hp? */
    if (victim->hit < 1)
        victim->hit = 1; /* Cap the damage */

    /* Knock them into resting and disarm them regardless of whether they have talon or a noremove weapon */
    victim->position = Position::Type::Resting;
    if (wielded_obj) {
        obj_from_char(wielded_obj);
        obj_to_room(wielded_obj, victim->in_room);
        if (victim->is_npc() && victim->wait == 0 && can_see_obj(victim, wielded_obj))
            get_obj(victim, wielded_obj, nullptr);
    }
}
