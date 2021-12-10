/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                    */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  act_info.c: standard information functions                           */
/*                                                                       */
/*************************************************************************/

#include "AFFECT_DATA.hpp"
#include "AffectFlag.hpp"
#include "Area.hpp"
#include "AreaList.hpp"
#include "ArmourClass.hpp"
#include "Char.hpp"
#include "CharActFlag.hpp"
#include "Classes.hpp"
#include "Columner.hpp"
#include "CommFlag.hpp"
#include "ContainerFlag.hpp"
#include "Descriptor.hpp"
#include "DescriptorList.hpp"
#include "Exit.hpp"
#include "ExitFlag.hpp"
#include "Help.hpp"
#include "Logging.hpp"
#include "Materials.hpp"
#include "Object.hpp"
#include "ObjectExtraFlag.hpp"
#include "ObjectIndex.hpp"
#include "ObjectType.hpp"
#include "ObjectWearFlag.hpp"
#include "PlayerActFlag.hpp"
#include "PracticeTabulator.hpp"
#include "Races.hpp"
#include "RoomFlag.hpp"
#include "SkillNumbers.hpp"
#include "SkillTables.hpp"
#include "TimeInfoData.hpp"
#include "ToleranceFlag.hpp"
#include "Wear.hpp"
#include "WeatherData.hpp"
#include "act_comm.hpp"
#include "comm.hpp"
#include "common/BitOps.hpp"
#include "db.h"
#include "fight.hpp"
#include "handler.hpp"
#include "interp.h"
#include "lookup.h"
#include "save.hpp"
#include "skills.hpp"
#include "string_utils.hpp"

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <magic_enum.hpp>
#include <range/v3/algorithm/find_if.hpp>
#include <range/v3/iterator/operations.hpp>

using namespace std::literals;

namespace {

std::string wear_string_for(const Object *obj, const Wear wear_location) {
    constexpr std::array<std::string_view, WearFilter::wearable_count()> where_name = {
        "used as light",     "worn on finger",   "worn on finger",
        "worn around neck",  "worn around neck", "worn on body",
        "worn on head",      "worn on legs",     "worn on feet",
        "worn on hands",     "worn on arms",     "worn as shield",
        "worn about body",   "worn about waist", "worn around wrist",
        "worn around wrist", "wielded",          "held",
        "worn on ears"};

    return fmt::format("<{}>", obj->wear_string.empty() ? where_name[to_int(wear_location)] : obj->wear_string);
}

}
/* for do_count */
size_t max_on = 0;

/*
 * Local functions.
 */
std::string format_obj_to_char(const Object *obj, const Char *ch, bool fShort);
void show_char_to_char_0(const Char *victim, const Char *ch);
void show_char_to_char_1(Char *victim, Char *ch);
void show_char_to_char(const GenericList<Char *> &list, const Char *ch);
bool check_blind(const Char *ch);
void set_prompt(Char *ch, std::string_view prompt);

std::string format_obj_to_char(const Object *obj, const Char *ch, bool fShort) {
    std::string buf;
    std::string desc = fShort ? obj->short_descr : obj->description;
    if (desc.empty()) {
        desc = "This object has no description. Please inform the IMP.";
        bug("Object {} has no description", obj->objIndex->vnum);
    }
    if (obj->is_unique())
        buf += "(U) ";
    if (obj->is_invisible())
        buf += "(|cInvis|w) ";
    if (ch->is_aff_detect_evil() && obj->is_evil())
        buf += "(|rRed Aura|w) ";
    if (ch->is_aff_detect_magic() && obj->is_magic())
        buf += "(|gMagical|w) ";
    if (obj->is_glowing())
        buf += "(|WGlowing|w) ";
    if (obj->is_humming())
        buf += "(|yHumming|w) ";
    buf += desc;
    return buf;
}

/*
 * Show a list to a character.
 * Can coalesce duplicated items.
 */
void show_list_to_char(const GenericList<Object *> &list, const Char *ch, bool fShort, bool fShowNothing) {
    if (!ch->desc)
        return;

    struct DescAndCount {
        std::string desc;
        int count{1};
    };
    std::vector<DescAndCount> to_show;

    const bool show_counts = ch->is_npc() || check_enum_bit(ch->comm, CommFlag::Combine);

    // Format the list of objects.
    for (auto *obj : list) {
        if (obj->wear_loc == Wear::None && can_see_obj(ch, obj)) {
            auto desc = format_obj_to_char(obj, ch, fShort);
            auto combined_same = false;

            if (show_counts) {
                // Look for duplicates, case sensitive.
                if (auto existing = ranges::find_if(to_show, [&](const auto &x) { return x.desc == desc; });
                    existing != to_show.end()) {
                    existing->count++;
                    combined_same = true;
                }
            }

            // Couldn't combine, or didn't want to.
            if (!combined_same)
                to_show.emplace_back(DescAndCount{std::move(desc)});
        }
    }

    // Output the formatted list.
    std::string buffer;
    auto indent = "     "sv;
    for (const auto &[name, count] : to_show) {
        if (show_counts)
            buffer += count > 1 ? fmt::format("({:2}) ", count) : indent;
        buffer += name + "\n\r";
    }

    if (fShowNothing && to_show.empty()) {
        if (show_counts)
            buffer += indent;
        buffer += "Nothing.\n\r";
    }

    ch->page_to(buffer);
}

void show_char_to_char_0(const Char *victim, const Char *ch) {
    std::string buf;

    if (victim->is_aff_invisible())
        buf += "(|WInvis|w) ";
    if (victim->is_pc() && check_enum_bit(victim->act, PlayerActFlag::PlrWizInvis))
        buf += "(|RWizi|w) ";
    if (victim->is_pc() && check_enum_bit(victim->act, PlayerActFlag::PlrProwl))
        buf += "(|RProwl|w) ";
    if (victim->is_aff_hide())
        buf += "(|WHide|w) ";
    if (victim->is_aff_charm())
        buf += "(|yCharmed|w) ";
    if (victim->is_aff_pass_door())
        buf += "(|bTranslucent|w) ";
    if (victim->is_aff_faerie_fire())
        buf += "(|PPink Aura|w) ";
    if (victim->is_aff_octarine_fire())
        buf += "(|GOctarine Aura|w) ";
    if (victim->is_evil() && ch->is_aff_detect_evil())
        buf += "(|rRed Aura|w) ";
    if (victim->is_aff_sanctuary())
        buf += "(|WWhite Aura|w) ";
    if (victim->is_pc() && check_enum_bit(victim->act, PlayerActFlag::PlrKiller))
        buf += "(|RKILLER|w) ";
    if (victim->is_pc() && check_enum_bit(victim->act, PlayerActFlag::PlrThief))
        buf += "(|RTHIEF|w) ";

    if (ch->is_affected_by(gsn_bless)) {
        if (check_enum_bit(victim->act, CharActFlag::Undead)) {
            buf += "(|bUndead|w) ";
        }
    }

    if (victim->position == victim->start_pos && !victim->long_descr.empty()) {
        buf += victim->long_descr;
        ch->send_to(buf);
        return;
    }

    buf += ch->describe(*victim);
    if (victim->is_pc() && !check_enum_bit(ch->comm, CommFlag::Brief))
        buf += victim->pcdata->title;

    buf += " is ";
    buf += victim->position.present_progressive_verb();
    switch (victim->position) {
    case Position::Type::Standing:
        if (victim->riding != nullptr) {
            buf += fmt::format(", riding {}.", victim->riding->name);
        } else {
            buf += ".";
        }
        break;
    case Position::Type::Fighting:
        if (victim->fighting == nullptr)
            buf += " thin air??";
        else if (victim->fighting == ch)
            buf += " |RYOU!|w";
        else if (victim->in_room == victim->fighting->in_room) {
            buf += fmt::format(" {}.", ch->describe(*victim->fighting));
        } else
            buf += " someone who left??";
        break;
    default:;
    }

    buf += "\n\r";
    buf[0] = toupper(buf[0]);
    ch->send_to(buf);
}

void show_char_to_char_1(Char *victim, Char *ch) {
    if (can_see(victim, ch)) {
        if (ch == victim)
            act("$n looks at $r.", ch);
        else {
            act("$n looks at you.", ch, nullptr, victim, To::Vict);
            act("$n looks at $N.", ch, nullptr, victim, To::NotVict);
        }
    }

    if (victim->description[0] != '\0') {
        ch->send_to(victim->description);
    } else {
        act("You see nothing special about $M.", ch, nullptr, victim, To::Char);
    }

    ch->send_to(describe_fight_condition(*victim));

    bool found = false;
    for (const auto &wear : WearFilter::wearable()) {
        if (const auto *obj = get_eq_char(victim, wear); obj && can_see_obj(ch, obj)) {
            if (!found) {
                ch->send_line("");
                act("$N is using:", ch, nullptr, victim, To::Char);
                found = true;
            }
            ch->send_line("{:<20}{}", wear_string_for(obj, wear), format_obj_to_char(obj, ch, true));
        }
    }

    if (victim != ch && ch->is_pc() && number_percent() < ch->get_skill(gsn_peek)
        && check_enum_bit(ch->act, PlayerActFlag::PlrAutoPeek)) {
        ch->send_line("\n\rYou peek at the inventory:");
        check_improve(ch, gsn_peek, true, 4);
        show_list_to_char(victim->carrying, ch, true, true);
    }
}

void do_peek(Char *ch, ArgParser args) {
    if (!ch->desc)
        return;
    if (ch->is_pos_stunned_or_dying()) {
        ch->send_line("You can't see anything but stars!");
        return;
    }
    if (ch->is_pos_sleeping()) {
        ch->send_line("You can't see anything, you're sleeping!");
        return;
    }
    if (!check_blind(ch))
        return;
    if (ch->is_pc() && !check_enum_bit(ch->act, PlayerActFlag::PlrHolyLight) && room_is_dark(ch->in_room)) {
        ch->send_line("It is pitch black ... ");
        show_char_to_char(ch->in_room->people, ch);
        return;
    }
    if (auto *victim = get_char_room(ch, args.shift())) {
        if (victim != ch && ch->is_pc() && number_percent() < ch->get_skill(gsn_peek)) {
            ch->send_line("\n\rYou peek at their inventory:");
            check_improve(ch, gsn_peek, true, 4);
            show_list_to_char(victim->carrying, ch, true, true);
        }
    } else
        ch->send_line("They aren't here.");
}

void show_char_to_char(const GenericList<Char *> &list, const Char *ch) {
    for (auto *rch : list) {
        if (rch == ch)
            continue;

        if (rch->is_pc() && check_enum_bit(rch->act, PlayerActFlag::PlrWizInvis) && ch->get_trust() < rch->invis_level)
            continue;

        if (can_see(ch, rch)) {
            show_char_to_char_0(rch, ch);
        } else if (room_is_dark(ch->in_room) && rch->is_aff_infrared()) {
            ch->send_line("You see |Rglowing red|w eyes watching |RYOU!|w");
        }
    }
}

bool check_blind(const Char *ch) {
    if (!ch->is_aff_blind() || ch->has_holylight())
        return true;

    ch->send_line("You can't see a thing!");
    return false;
}

/* changes your scroll */
void do_scroll(Char *ch, ArgParser args) {
    // Pager limited to 52 due to memory complaints relating to
    // buffer code ...short term fix :) --Faramir
    static constexpr int MaxScrollLength = 52;
    static constexpr int MinScrollLength = 10;

    if (args.empty()) {
        if (ch->lines == 0) {
            ch->send_line("|cPaging is set to maximum.|w");
            ch->lines = MaxScrollLength;
        } else {
            ch->send_line("|cYou currently display {} lines per page.|w", ch->lines + 2);
        }
        return;
    }

    auto maybe_num = args.try_shift_number();
    if (!maybe_num) {
        ch->send_line("|cYou must provide a number.|w");
        return;
    }
    auto lines = *maybe_num;

    if (lines == 0) {
        ch->send_line("|cPaging at maximum.|w");
        ch->lines = MaxScrollLength;
        return;
    }

    if (lines < MinScrollLength || lines > MaxScrollLength) {
        ch->send_line("|cYou must provide a reasonable number (between {} and {}).|w", MinScrollLength,
                      MaxScrollLength);
        return;
    }

    ch->send_line("\"|cScroll set to {} lines.|w", lines);
    ch->lines = lines - 2;
}

/* RT Commands to replace news, motd, imotd, etc from ROM */

void do_motd(Char *ch) { do_help(ch, "motd"); }

void do_imotd(Char *ch) { do_help(ch, "imotd"); }

void do_rules(Char *ch) { do_help(ch, "rules"); }

void do_story(Char *ch) { do_help(ch, "story"); }

void do_changes(Char *ch) { do_help(ch, "changes"); }

void do_wizlist(Char *ch) { do_help(ch, "wizlist"); }

/* RT this following section holds all the auto commands from ROM, as well as
   replacements for config */

namespace {
struct OnOff {
    bool b;
};
}
template <>
struct fmt::formatter<OnOff> {
    constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const OnOff &onoff, FormatContext &ctx) {
        return fmt::format_to(ctx.out(), fmt::runtime(onoff.b ? "|RON|w" : "|ROFF|w"));
    }
};

void do_autolist(Char *ch) {
    /* lists most player flags */
    if (ch->is_npc())
        return;

    ch->send_line("   action     status");
    ch->send_line("---------------------");

    ch->send_line("ANSI colour    {}", OnOff{ch->pcdata->colour});
    ch->send_line("autoaffect     {}", OnOff{check_enum_bit(ch->comm, CommFlag::Affect)});
    ch->send_line("autoassist     {}", OnOff{check_enum_bit(ch->act, PlayerActFlag::PlrAutoAssist)});
    ch->send_line("autoexit       {}", OnOff{check_enum_bit(ch->act, PlayerActFlag::PlrAutoExit)});
    ch->send_line("autogold       {}", OnOff{check_enum_bit(ch->act, PlayerActFlag::PlrAutoGold)});
    ch->send_line("autoloot       {}", OnOff{check_enum_bit(ch->act, PlayerActFlag::PlrAutoLoot)});
    ch->send_line("autopeek       {}", OnOff{check_enum_bit(ch->act, PlayerActFlag::PlrAutoPeek)});
    ch->send_line("autosac        {}", OnOff{check_enum_bit(ch->act, PlayerActFlag::PlrAutoSac)});
    ch->send_line("autosplit      {}", OnOff{check_enum_bit(ch->act, PlayerActFlag::PlrAutoSplit)});
    ch->send_line("prompt         {}", OnOff{check_enum_bit(ch->comm, CommFlag::Prompt)});
    ch->send_line("combine items  {}", OnOff{check_enum_bit(ch->comm, CommFlag::Combine)});

    if (!check_enum_bit(ch->act, PlayerActFlag::PlrCanLoot))
        ch->send_line("Your corpse is safe from thieves.");
    else
        ch->send_line("Your corpse may be looted.");

    if (check_enum_bit(ch->act, PlayerActFlag::PlrNoSummon))
        ch->send_line("You cannot be summoned.");
    else
        ch->send_line("You can be summoned.");

    if (check_enum_bit(ch->act, PlayerActFlag::PlrNoFollow))
        ch->send_line("You do not welcome followers.");
    else
        ch->send_line("You accept followers.");

    if (check_enum_bit(ch->comm, CommFlag::Brief))
        ch->send_line("Only brief descriptions are being shown.");
    else
        ch->send_line("Full descriptions are being shown.");

    if (check_enum_bit(ch->comm, CommFlag::Compact))
        ch->send_line("Compact mode is set.");
    else
        ch->send_line("Compact mode is not set.");

    if (check_enum_bit(ch->comm, CommFlag::ShowAfk))
        ch->send_line("Messages sent to you will be shown when afk.");
    else
        ch->send_line("Messages sent to you will not be shown when afk.");

    if (check_enum_bit(ch->comm, CommFlag::ShowDefence))
        ch->send_line("Shield blocks, parries, and dodges are being shown.");
    else
        ch->send_line("Shield blocks, parries, and dodges are not being shown.");
}

void do_autoaffect(Char *ch) {
    if (ch->is_npc())
        return;

    if (check_enum_bit(ch->comm, CommFlag::Affect)) {
        ch->send_line("Autoaffect removed.");
        clear_enum_bit(ch->comm, CommFlag::Affect);
    } else {
        ch->send_line("Affects will now be shown in score.");
        set_enum_bit(ch->comm, CommFlag::Affect);
    }
}
void do_autoassist(Char *ch) {
    if (ch->is_npc())
        return;

    if (check_enum_bit(ch->act, PlayerActFlag::PlrAutoAssist)) {
        ch->send_line("Autoassist removed.");
        clear_enum_bit(ch->act, PlayerActFlag::PlrAutoAssist);
    } else {
        ch->send_line("You will now assist when needed.");
        set_enum_bit(ch->act, PlayerActFlag::PlrAutoAssist);
    }
}

void do_autoexit(Char *ch) {
    if (ch->is_npc())
        return;

    if (check_enum_bit(ch->act, PlayerActFlag::PlrAutoExit)) {
        ch->send_line("Exits will no longer be displayed.");
        clear_enum_bit(ch->act, PlayerActFlag::PlrAutoExit);
    } else {
        ch->send_line("Exits will now be displayed.");
        set_enum_bit(ch->act, PlayerActFlag::PlrAutoExit);
    }
}

void do_autogold(Char *ch) {
    if (ch->is_npc())
        return;

    if (check_enum_bit(ch->act, PlayerActFlag::PlrAutoGold)) {
        ch->send_line("Autogold removed.");
        clear_enum_bit(ch->act, PlayerActFlag::PlrAutoGold);
    } else {
        ch->send_line("Automatic gold looting set.");
        set_enum_bit(ch->act, PlayerActFlag::PlrAutoGold);
    }
}

void do_autoloot(Char *ch) {
    if (ch->is_npc())
        return;

    if (check_enum_bit(ch->act, PlayerActFlag::PlrAutoLoot)) {
        ch->send_line("Autolooting removed.");
        clear_enum_bit(ch->act, PlayerActFlag::PlrAutoLoot);
    } else {
        ch->send_line("Automatic corpse looting set.");
        set_enum_bit(ch->act, PlayerActFlag::PlrAutoLoot);
    }
}

void do_autopeek(Char *ch) {
    if (ch->is_npc())
        return;

    if (check_enum_bit(ch->act, PlayerActFlag::PlrAutoPeek)) {
        ch->send_line("Autopeeking removed.");
        clear_enum_bit(ch->act, PlayerActFlag::PlrAutoPeek);
    } else {
        ch->send_line("Automatic peeking set.");
        set_enum_bit(ch->act, PlayerActFlag::PlrAutoPeek);
    }
}

void do_autosac(Char *ch) {
    if (ch->is_npc())
        return;

    if (check_enum_bit(ch->act, PlayerActFlag::PlrAutoSac)) {
        ch->send_line("Autosacrificing removed.");
        clear_enum_bit(ch->act, PlayerActFlag::PlrAutoSac);
    } else {
        ch->send_line("Automatic corpse sacrificing set.");
        set_enum_bit(ch->act, PlayerActFlag::PlrAutoSac);
    }
}

void do_autosplit(Char *ch) {
    if (ch->is_npc())
        return;

    if (check_enum_bit(ch->act, PlayerActFlag::PlrAutoSplit)) {
        ch->send_line("Autosplitting removed.");
        clear_enum_bit(ch->act, PlayerActFlag::PlrAutoSplit);
    } else {
        ch->send_line("Automatic gold splitting set.");
        set_enum_bit(ch->act, PlayerActFlag::PlrAutoSplit);
    }
}

void do_brief(Char *ch) {
    if (check_enum_bit(ch->comm, CommFlag::Brief)) {
        ch->send_line("Full descriptions activated.");
        clear_enum_bit(ch->comm, CommFlag::Brief);
    } else {
        ch->send_line("Short descriptions activated.");
        set_enum_bit(ch->comm, CommFlag::Brief);
    }
}

void do_colour(Char *ch) {
    if (ch->is_npc())
        return;

    if (ch->pcdata->colour) {
        ch->send_line("You feel less COLOURFUL.");

        ch->pcdata->colour = false;
    } else {
        ch->pcdata->colour = true;
        ch->send_line("You feel more |RC|GO|BL|rO|gU|bR|cF|YU|PL|W!.|w");
    }
}

void do_showafk(Char *ch) {
    if (check_enum_bit(ch->comm, CommFlag::ShowAfk)) {
        ch->send_line("Messages sent to you will now not be shown when afk.");
        clear_enum_bit(ch->comm, CommFlag::ShowAfk);
    } else {
        ch->send_line("Messages sent to you will now be shown when afk.");
        set_enum_bit(ch->comm, CommFlag::ShowAfk);
    }
}

void do_showdefence(Char *ch) {
    if (check_enum_bit(ch->comm, CommFlag::ShowDefence)) {
        ch->send_line("Shield blocks, parries and dodges will not be shown during combat.");
        clear_enum_bit(ch->comm, CommFlag::ShowDefence);
    } else {
        ch->send_line("Shield blocks, parries and dodges will be shown during combat.");
        set_enum_bit(ch->comm, CommFlag::ShowDefence);
    }
}

void do_compact(Char *ch) {
    if (check_enum_bit(ch->comm, CommFlag::Compact)) {
        ch->send_line("Compact mode removed.");
        clear_enum_bit(ch->comm, CommFlag::Compact);
    } else {
        ch->send_line("Compact mode set.");
        set_enum_bit(ch->comm, CommFlag::Compact);
    }
}

void do_prompt(Char *ch, std::string_view argument) {
    /* PCFN 24-05-97  Oh dear - it seems that you can't set prompt while switched
       into a MOB.  Let's change that.... */
    if (ch = ch->player(); !ch)
        return;

    if (matches(argument, "off")) {
        ch->send_line("You will no longer see prompts.");
        clear_enum_bit(ch->comm, CommFlag::Prompt);
        return;
    }
    if (matches(argument, "on")) {
        ch->send_line("You will now see prompts.");
        set_enum_bit(ch->comm, CommFlag::Prompt);
        return;
    }

    /* okay that was the old stuff */
    set_prompt(ch, smash_tilde(argument));
    ch->send_line("Ok - prompt set.");
    set_enum_bit(ch->comm, CommFlag::Prompt);
}

void do_combine(Char *ch) {
    if (check_enum_bit(ch->comm, CommFlag::Combine)) {
        ch->send_line("Long inventory selected.");
        clear_enum_bit(ch->comm, CommFlag::Combine);
    } else {
        ch->send_line("Combined inventory selected.");
        set_enum_bit(ch->comm, CommFlag::Combine);
    }
}

void do_noloot(Char *ch) {
    if (ch->is_npc())
        return;

    if (check_enum_bit(ch->act, PlayerActFlag::PlrCanLoot)) {
        ch->send_line("Your corpse is now safe from thieves.");
        clear_enum_bit(ch->act, PlayerActFlag::PlrCanLoot);
    } else {
        ch->send_line("Your corpse may now be looted.");
        set_enum_bit(ch->act, PlayerActFlag::PlrCanLoot);
    }
}

void do_nofollow(Char *ch) {
    if (ch->is_npc())
        return;

    if (check_enum_bit(ch->act, PlayerActFlag::PlrNoFollow)) {
        ch->send_line("You now accept followers.");
        clear_enum_bit(ch->act, PlayerActFlag::PlrNoFollow);
    } else {
        ch->send_line("You no longer accept followers.");
        set_enum_bit(ch->act, PlayerActFlag::PlrNoFollow);
        die_follower(ch);
    }
}

void do_nosummon(Char *ch) {
    if (ch->is_npc()) {
        if (check_enum_bit(ch->imm_flags, ToleranceFlag::Summon)) {
            ch->send_line("You are no longer immune to summon.");
            clear_enum_bit(ch->imm_flags, ToleranceFlag::Summon);
        } else {
            ch->send_line("You are now immune to summoning.");
            set_enum_bit(ch->imm_flags, ToleranceFlag::Summon);
        }
    } else {
        if (check_enum_bit(ch->act, PlayerActFlag::PlrNoSummon)) {
            ch->send_line("You are no longer immune to summon.");
            clear_enum_bit(ch->act, PlayerActFlag::PlrNoSummon);
        } else {
            ch->send_line("You are now immune to summoning.");
            set_enum_bit(ch->act, PlayerActFlag::PlrNoSummon);
        }
    }
}

void do_lore(Char *ch, Object *obj, std::string_view description) {
    if (ch->is_pc() && number_percent() > ch->get_skill(skill_lookup("lore"))) {
        ch->send_line(description);
        check_improve(ch, gsn_lore, false, 1);
    } else {
        const auto identify = skill_lookup("identify");
        if (ch->is_mortal())
            ch->wait_state(skill_table[identify].beats);
        ch->send_line(description);
        check_improve(ch, gsn_lore, true, 1);
        auto spell_target = SpellTarget(obj);
        (*skill_table[identify].spell_fun)(identify, ch->level, ch, spell_target);
    }
}

namespace {

void room_look(const Char &ch, bool force_full) {
    ch.send_line("|R{}|w\n\r", ch.in_room->name);
    if (force_full || !ch.is_comm_brief()) {
        ch.send_to("  {}", ch.in_room->description);
    }

    if (ch.should_autoexit()) {
        ch.send_line("");
        do_exits(&ch, "auto");
    }

    show_list_to_char(ch.in_room->contents, &ch, false, false);
    show_char_to_char(ch.in_room->people, &ch);
}

void look_in_object(const Char &ch, const Object &obj) {
    switch (obj.type) {
    default: ch.send_line("That is not a container."); break;

    case ObjectType::Drink: {
        if (obj.value[1] <= 0) {
            ch.send_line("It is empty.");
            break;
        }
        const auto *liquid = Liquid::get_by_index(obj.value[2]);
        if (!liquid) {
            bug("{} attempted to look in a drink containing an unknown liquid: {} {} -> {}", ch.name,
                obj.objIndex->vnum, obj.short_descr, obj.value[2]);
            return;
        }
        const auto &liq_color = liquid->color;
        ch.send_line("It's {} full of a{} {} liquid.",
                     obj.value[1] < obj.value[0] / 4 ? "less than"
                                                     : obj.value[1] < 3 * obj.value[0] / 4 ? "about" : "more than",
                     is_vowel(liq_color[0]) ? "n" : "", liq_color);
        break;
    }
    case ObjectType::Container:
    case ObjectType::Npccorpse:
    case ObjectType::Pccorpse:
        if (check_enum_bit(obj.value[1], ContainerFlag::Closed)) {
            ch.send_line("It is closed.");
            break;
        }

        act("$p contains:", &ch, &obj, nullptr, To::Char);
        show_list_to_char(obj.contains, &ch, true, true);
        break;
    }
}

const char *try_get_descr(const Object &obj, std::string_view name) {
    if (auto *pdesc = get_extra_descr(name, obj.extra_descr))
        return pdesc;
    if (auto *pdesc = get_extra_descr(name, obj.objIndex->extra_descr))
        return pdesc;
    return nullptr;
}

bool handled_as_look_at_object(Char &ch, std::string_view first_arg) {
    auto &&[number, obj_desc] = number_argument(first_arg);
    int count = 0;
    for (auto *obj : ch.carrying) {
        if (!ch.can_see(*obj))
            continue;
        if (auto *pdesc = try_get_descr(*obj, obj_desc)) {
            if (++count == number) {
                do_lore(&ch, obj, pdesc);
                return true;
            } else
                continue;
        } else if (is_name(obj_desc, obj->name)) {
            if (++count == number) {
                do_lore(&ch, obj, obj->description);
                return true;
            }
        }
    }

    for (auto *obj : ch.in_room->contents) {
        if (!ch.can_see(*obj))
            continue;
        if (auto *pdesc = try_get_descr(*obj, obj_desc)) {
            if (++count == number) {
                ch.send_to(pdesc);
                return true;
            }
        }
        if (is_name(obj_desc, obj->name))
            if (++count == number) {
                ch.send_line("{}", obj->description);
                return true;
            }
    }

    if (count > 0 && count != number) {
        if (count == 1)
            ch.send_line("You only see one {} here.", obj_desc);
        else
            ch.send_line("You only see {} {}s here.", count, obj_desc);
        return true;
    }
    return false;
}

void look_direction(const Char &ch, Direction direction) {
    const auto &exit = ch.in_room->exits[direction];
    if (!exit) {
        ch.send_line("Nothing special there.");
        return;
    }

    if (!exit->description.empty())
        ch.send_to(exit->description);
    else
        ch.send_line("Nothing special there.");

    if (!exit->keyword.empty()) {
        if (check_enum_bit(exit->exit_info, ExitFlag::Closed)) {
            act("The $d is closed.", &ch, nullptr, exit->keyword, To::Char);
        } else if (check_enum_bit(exit->exit_info, ExitFlag::IsDoor)) {
            act("The $d is open.", &ch, nullptr, exit->keyword, To::Char);
        }
    }
}

bool check_look(Char *ch) {
    if (ch->desc == nullptr)
        return false;

    if (ch->is_pos_stunned_or_dying()) {
        ch->send_line("You can't see anything but stars!");
        return false;
    }

    if (ch->is_pos_sleeping()) {
        ch->send_line("You can't see anything, you're sleeping!");
        return false;
    }

    if (!check_blind(ch))
        return false;

    if (!ch->has_holylight() && room_is_dark(ch->in_room)) {
        ch->send_line("It is pitch black ... ");
        show_char_to_char(ch->in_room->people, ch);
        return false;
    }
    return true;
}

}

void look_auto(Char *ch) {
    if (!check_look(ch))
        return;
    room_look(*ch, false);
}

void do_look(Char *ch, ArgParser args) {
    if (!check_look(ch))
        return;

    auto first_arg = args.shift();

    // A normal look, or a look auto to describe the room?
    if (first_arg.empty() || matches(first_arg, "auto")) {
        room_look(*ch, first_arg.empty());
        return;
    }

    // Look in something?
    if (matches_start(first_arg, "in")) {
        if (args.empty()) {
            ch->send_line("Look in what?");
            return;
        }
        if (auto *obj = get_obj_here(ch, args.shift()))
            look_in_object(*ch, *obj);
        else
            ch->send_line("You do not see that here.");
        return;
    }

    // Look at a person?
    if (auto *victim = get_char_room(ch, first_arg)) {
        show_char_to_char_1(victim, ch);
        return;
    }

    // Look at an object?
    if (handled_as_look_at_object(*ch, first_arg))
        return;

    // Look at something in the extra description of the room?
    if (auto *pdesc = get_extra_descr(first_arg, ch->in_room->extra_descr)) {
        ch->send_to(pdesc);
        return;
    }

    // Look in a direction?
    if (auto opt_direction = try_parse_direction(first_arg)) {
        look_direction(*ch, *opt_direction);
        return;
    }

    ch->send_line("You do not see that here.");
}

void do_examine(Char *ch, ArgParser args) {
    if (args.empty()) {
        ch->send_line("Examine what?");
        return;
    }

    auto arg = args.shift();
    do_look(ch, ArgParser(arg));

    if (auto *obj = get_obj_here(ch, arg)) {
        switch (obj->type) {
        default: break;

        case ObjectType::Drink:
        case ObjectType::Container:
        case ObjectType::Npccorpse:
        case ObjectType::Pccorpse:
            ch->send_line("When you look inside, you see:");
            do_look(ch, ArgParser(fmt::format("in {}", arg)));
            break;
        }
    }
}

/*
 * Thanks to Zrin for auto-exit part.
 */
void do_exits(const Char *ch, std::string_view arguments) {

    const auto is_compact = matches(arguments, "auto");
    if (!check_blind(ch))
        return;

    std::string buf = is_compact ? "|W[Exits:" : "Obvious exits:\n\r";

    auto found = false;
    for (auto direction : all_directions) {
        if (const auto &exit = ch->in_room->exits[direction]; exit && exit->u1.to_room
                                                              && can_see_room(ch, exit->u1.to_room)
                                                              && !check_enum_bit(exit->exit_info, ExitFlag::Closed)) {
            found = true;
            if (is_compact) {
                buf += fmt::format(" {}", to_string(direction));
            } else {
                buf += fmt::format("{:<5} - {}\n\r", initial_caps_only(to_string(direction)),
                                   !ch->has_holylight() && room_is_dark(exit->u1.to_room) ? "Too dark to tell"
                                                                                          : exit->u1.to_room->name);
            }
        }
    }

    if (!found)
        buf += is_compact ? " none" : "None.\n\r";

    if (is_compact)
        buf += "]\n\r|w";

    ch->send_to(buf);
}

void do_worth(Char *ch) {
    if (ch->is_npc()) {
        ch->send_line("You have {} gold.", (int)ch->gold);
        return;
    }

    ch->send_line("You have {} gold, and {} experience ({} exp to level).", ch->gold, ch->exp,
                  ((ch->level + 1) * exp_per_level(ch, ch->pcdata->points) - ch->exp));
}

namespace {

void describe_armour(Char *ch, const ArmourClass ac_slot, const char *name) {
    static const std::array armour_desc = {
        "hopelessly vulnerable to",  "defenseless against",        "barely protected from",
        "slightly armoured against", "somewhat armoured against",  "armoured against",
        "well-armoured against",     "very well-armoured against", "heavily armoured against",
        "superbly armoured against", "almost invulnerable to",     "divinely armoured against"};
    // Armour ratings around -400 and beyond is labelled divine.
    static constexpr int ArmourBucketSize = 400 / armour_desc.size();
    const auto armour_class = -(ch->get_armour_class(ac_slot));
    int armour_bucket = armour_class / ArmourBucketSize;
    auto armour_index = std::clamp(armour_bucket, 0, static_cast<int>(armour_desc.size()) - 1);
    if (ch->level < 25)
        ch->send_line("|CYou are|w: |y{} |W{}|w.", armour_desc[armour_index], name);
    else
        ch->send_line("|CYou are|w: |y{} |W{}|w. (|W{}|w)", armour_desc[armour_index], name, armour_class);
}

const char *get_align_description(int align) {
    static const std::array align_descriptions = {"|Rsatanic", "|Rdemonic", "|Yevil",    "|Ymean",   "|Mneutral",
                                                  "|Gkind",    "|Ggood",    "|Wsaintly", "|Wangelic"};
    return align_descriptions[std::clamp((align + 1000) * 8 / 2000, 0,
                                         static_cast<int>(align_descriptions.size()) - 1)];
}

}

void do_score(Char *ch) {
    using namespace std::chrono;
    ch->send_line("|CYou are|w: |W{}{}|w.", ch->name, ch->is_npc() ? "" : ch->pcdata->title);

    Columner col2(*ch, 2);
    Columner col3(*ch, 3);

    if (ch->level == ch->get_trust())
        col2.stat("Level", ch->level);
    else
        col2.kv("Level", "|W{}|w (trust |W{}|w)", ch->level, ch->get_trust());
    col2.kv("Age", "|W{}|w years (|W{}|w hours)", get_age(ch), duration_cast<hours>(ch->total_played()).count());

    col2.stat("Race", race_table[ch->race].name)
        .stat("Class", ch->is_npc() ? "mobile" : class_table[ch->class_num].name)
        .stat("Sex", ch->sex.name())
        .stat("Position", ch->position.short_description())
        .stat_of("Items", ch->carry_number, can_carry_n(ch))
        .stat_of("Weight", ch->carry_weight, can_carry_w(ch))
        .stat("Gold", ch->gold)
        .flush();

    col2.stat("Wimpy", ch->wimpy);
    if (ch->is_pc() && ch->level < LEVEL_HERO)
        col2.kv("Score", "|W{}|w (|W{}|w to next level)", ch->exp,
                ((ch->level + 1) * exp_per_level(ch, ch->pcdata->points) - ch->exp));
    else
        col2.stat("Score", ch->exp);

    ch->send_line("");

    col3.stat_of("Hit", ch->hit, ch->max_hit)
        .stat_of("Mana", ch->mana, ch->max_mana)
        .stat_of("Move", ch->move, ch->max_move);

    describe_armour(ch, ArmourClass::Pierce, "piercing");
    describe_armour(ch, ArmourClass::Bash, "bashing");
    describe_armour(ch, ArmourClass::Slash, "slashing");
    describe_armour(ch, ArmourClass::Exotic, "magic");

    if (ch->level >= 15) {
        col2.stat("Hit roll", ch->get_hitroll());
        col2.stat("Damage roll", ch->get_damroll());
    }
    ch->send_line("");

    col3.stat_eff("Strength", ch->perm_stat[Stat::Str], get_curr_stat(ch, Stat::Str))
        .stat_eff("Intelligence", ch->perm_stat[Stat::Int], get_curr_stat(ch, Stat::Int))
        .stat_eff("Wisdom", ch->perm_stat[Stat::Wis], get_curr_stat(ch, Stat::Wis));

    col2.stat_eff("Dexterity", ch->perm_stat[Stat::Dex], get_curr_stat(ch, Stat::Dex))
        .stat_eff("Constitution", ch->perm_stat[Stat::Con], get_curr_stat(ch, Stat::Con))
        .stat("Practices", ch->practice)
        .stat("Training sessions", ch->train);

    if (ch->level >= 10)
        col2.kv("Alignment", "{}|w (|W{}|w).", get_align_description(ch->alignment), ch->alignment);
    else
        col2.stat("Alignment", get_align_description(ch->alignment));
    col2.flush();

    if (ch->is_immortal()) {
        ch->send_line("");
        col3.stat("Holy light", check_enum_bit(ch->act, PlayerActFlag::PlrHolyLight) ? "on" : "off");
        if (check_enum_bit(ch->act, PlayerActFlag::PlrWizInvis))
            col3.stat("Invisible", ch->invis_level);
        if (check_enum_bit(ch->act, PlayerActFlag::PlrProwl))
            col3.stat("Prowl", ch->invis_level);
        col3.flush();
    }

    if (const auto opt_nutrition_msg = ch->describe_nutrition()) {
        ch->send_line(*opt_nutrition_msg);
    }

    if (check_enum_bit(ch->comm, CommFlag::Affect)) {
        ch->send_line("");
        do_affected(ch);
    }
}

void do_affected(Char *ch) {
    ch->send_line("|CYou are affected by|w:");

    if (ch->affected.empty()) {
        ch->send_line("Nothing.");
        return;
    }
    for (auto &af : ch->affected)
        ch->send_line("|C{}|w: '{}'{}.", af.is_skill() ? "Skill" : "Spell", skill_table[af.type].name,
                      ch->level >= 20 ? af.describe_char_effect() : "");
}

void do_time(Char *ch) {
    ch->send_line(time_info.describe());
    ch->send_line("Xania started up at {}Z.", formatted_time(boot_time));
    ch->send_line("The system time is {}Z.", formatted_time(current_time));
}

void do_weather(Char *ch) {
    if (ch->is_inside()) {
        ch->send_line("You can't see the weather indoors.");
        return;
    }

    ch->send_to(weather_info.describe() + "\n\r");
}

void do_help(Char *ch, std::string_view argument) {
    const std::string topic{argument.empty() ? "summary" : argument};
    if (auto *help = HelpList::singleton().lookup(ch->get_trust(), topic)) {
        if (help->level() >= 0 && !matches(topic, "imotd"))
            ch->send_line("|W{}|w", help->keyword());
        ch->page_to(help->text());
    } else {
        ch->send_line("No help on that word.");
    }
}

namespace {

std::string_view who_class_name_of(const Char &wch) {
    switch (wch.level) {
    case MAX_LEVEL - 0: return "|WIMP|w"sv; break;
    case MAX_LEVEL - 1: return "|YCRE|w"sv; break;
    case MAX_LEVEL - 2: return "|YSUP|w"sv; break;
    case MAX_LEVEL - 3: return "|GDEI|w"sv; break;
    case MAX_LEVEL - 4: return "|GGOD|w"sv; break;
    case MAX_LEVEL - 5: return "|gIMM|w"sv; break;
    case MAX_LEVEL - 6: return "|gDEM|w"sv; break;
    case MAX_LEVEL - 7: return "ANG"sv; break;
    case MAX_LEVEL - 8: return "AVA"sv; break;
    }
    return class_table[wch.class_num].who_name;
}

std::string_view who_race_name_of(const Char &wch) {
    return wch.race < MAX_PC_RACE ? pc_race_table[wch.race].who_name : "     "sv;
}

std::string_view who_clan_name_of(const Char &wch) { return wch.clan() ? wch.clan()->whoname : ""sv; }

std::string who_line_for(const Char &to, const Char &wch) {
    return fmt::format(
        "[{:3} {} {}] {}{}{}{}{}{}|w{}{}\n\r", wch.level, who_race_name_of(wch), who_class_name_of(wch),
        who_clan_name_of(wch), check_enum_bit(wch.act, PlayerActFlag::PlrKiller) ? "(|RKILLER|w) " : "",
        check_enum_bit(wch.act, PlayerActFlag::PlrThief) ? "(|RTHIEF|w) " : "",
        check_enum_bit(wch.act, PlayerActFlag::PlrAfk) ? "(|cAFK|w) " : "", wch.name,
        wch.is_pc() ? wch.pcdata->title : "",
        wch.is_wizinvis() && to.is_immortal() ? fmt::format(" |g(Wizi at level {})|w", wch.invis_level) : "",
        wch.is_prowlinvis() && to.is_immortal() ? fmt::format(" |g(Prowl level {})|w", wch.invis_level) : "");
}

}

/* whois command */
void do_whois(Char *ch, ArgParser args) {
    if (args.empty()) {
        ch->send_line("You must provide a name.");
        return;
    }

    std::string output;
    auto filter = args.shift();
    for (auto &d : descriptors().all_visible_to(*ch)) {
        auto *wch = d.person();
        // TODO: can or should this be part of all_visible_to?
        if (!can_see(ch, wch))
            continue;

        if (matches_start(filter, wch->name))
            output += who_line_for(*ch, *wch);
    }

    if (output.empty()) {
        ch->send_line("No one of that name is playing.");
        return;
    }

    ch->page_to(output);
}

/*
 * New 'who' command originally by Alander of Rivers of Mud.
 */
void do_who(Char *ch, ArgParser args) {
    int iClass;
    int iRace;
    int iLevelLower = 0;
    int iLevelUpper = MAX_LEVEL;
    int nNumber;
    int nMatch;
    bool rgfClass[MAX_CLASS]{};
    bool rgfRace[MAX_PC_RACE]{};
    std::unordered_set<const Clan *> rgfClan;
    bool fClassRestrict = false;
    bool fRaceRestrict = false;
    bool fClanRestrict = false;
    bool fImmortalOnly = false;
    /*
     * Parse arguments.
     */
    nNumber = 0;
    for (;;) {
        auto arg = args.shift();
        if (arg.empty())
            break;

        if (is_number(arg)) {
            switch (++nNumber) {
            case 1: iLevelLower = parse_number(arg); break;
            case 2: iLevelUpper = parse_number(arg); break;
            default: ch->send_line("Only two level numbers allowed."); return;
            }
        } else {

            /*
             * Look for classes to turn on.
             */
            if (arg[0] == 'i') {
                fImmortalOnly = true;
            } else {
                iClass = class_lookup(arg);
                if (iClass == -1) {
                    iRace = race_lookup(arg);
                    if (iRace == 0 || iRace >= MAX_PC_RACE) {
                        /* Check if clan exists */
                        const Clan *clan_ptr = nullptr; // TODO this could be much better phrased
                        for (auto &clan : clantable) {
                            if (is_name(arg, clan.name))
                                clan_ptr = &clan;
                        }
                        /* Check for NO match on clans */
                        if (!clan_ptr) {
                            ch->send_line("That's not a valid race, class, or clan.");
                            return;
                        } else
                        /* It DID match! */
                        {
                            fClanRestrict = true;
                            rgfClan.emplace(clan_ptr);
                        }
                    } else {
                        fRaceRestrict = true;
                        rgfRace[iRace] = true;
                    }
                } else {
                    fClassRestrict = true;
                    rgfClass[iClass] = true;
                }
            }
        }
    }

    /*
     * Now show matching chars.
     */
    nMatch = 0;
    std::string output;
    for (auto &d : descriptors().all_visible_to(*ch)) {
        // Check for match against restrictions.
        // Don't use trust as that exposes trusted mortals.
        // added Faramir 13/8/96 because switched imms were visible to all
        if (!can_see(ch, d.person()))
            continue;

        auto *wch = d.person();
        if (wch->level < iLevelLower || wch->level > iLevelUpper || (fImmortalOnly && wch->level < LEVEL_HERO)
            || (fClassRestrict && !rgfClass[wch->class_num]) || (fRaceRestrict && !rgfRace[wch->race]))
            continue;
        if (fClanRestrict) {
            if (!wch->clan() || rgfClan.count(wch->clan()) == 0)
                continue;
        }

        nMatch++;

        output += who_line_for(*ch, *wch);
    }

    output += fmt::format("\n\rPlayers found: {}\n\r", nMatch);
    ch->page_to(output);
}

void do_count(Char *ch) {
    auto count = static_cast<size_t>(ranges::distance(descriptors().all_visible_to(*ch)));
    max_on = std::max(count, max_on);

    if (max_on == count)
        ch->send_line("There are {} characters on, the most so far today.", count);
    else
        ch->send_line("There are {} characters on, the most son today was {}.", count, max_on);
}

void do_inventory(Char *ch) {
    ch->send_line("You are carrying:");
    show_list_to_char(ch->carrying, ch, true, true);
}

void do_equipment(Char *ch) {
    ch->send_line("You are using:");
    bool found = false;
    for (const auto &wear : WearFilter::wearable()) {
        if (const auto *obj = get_eq_char(ch, wear)) {
            ch->send_line("{:<20}{}", wear_string_for(obj, wear),
                          can_see_obj(ch, obj) ? format_obj_to_char(obj, ch, true) : "something.");
            found = true;
        }
    }

    if (!found)
        ch->send_line("Nothing.");
}

namespace {
Object *find_comparable(Char *ch, Object *obj_to_compare_to) {
    for (auto *obj : ch->carrying) {
        if (obj->wear_loc != Wear::None && can_see_obj(ch, obj) && obj_to_compare_to->type == obj->type
            && (obj_to_compare_to->wear_flags & obj->wear_flags & (~to_int(ObjectWearFlag::Take))) != 0) {
            return obj;
        }
    }
    return nullptr;
}
}

void do_compare(Char *ch, ArgParser args) {
    if (args.empty()) {
        ch->send_line("Compare what to what?");
        return;
    }

    auto *obj1 = ch->find_in_inventory(args.shift());
    if (!obj1) {
        ch->send_line("You do not have that item.");
        return;
    }

    Object *obj2{};
    if (args.empty()) {
        obj2 = find_comparable(ch, obj1);
        if (!obj2) {
            ch->send_line("You aren't wearing anything comparable.");
            return;
        }
    } else {
        obj2 = ch->find_in_inventory(args.shift());
        if (!obj2) {
            ch->send_line("You do not have that item.");
            return;
        }
    }

    std::string_view msg;
    int value1 = 0;
    int value2 = 0;

    if (obj1 == obj2) {
        msg = "You compare $p to itself.  It looks about the same.";
    } else if (obj1->type != obj2->type) {
        msg = "You can't compare $p and $P.";
    } else {
        switch (obj1->type) {
        default: msg = "You can't compare $p and $P."; break;

        case ObjectType::Armor:
            value1 = obj1->value[0] + obj1->value[1] + obj1->value[2];
            value2 = obj2->value[0] + obj2->value[1] + obj2->value[2];
            break;

        case ObjectType::Weapon:
            value1 = (1 + obj1->value[2]) * obj1->value[1];
            value2 = (1 + obj2->value[2]) * obj2->value[1];
            break;
        }
    }

    if (msg.empty()) {
        if (value1 == value2)
            msg = "$p and $P look about the same.";
        else if (value1 > value2)
            msg = "$p looks better than $P.";
        else
            msg = "$p looks worse than $P.";
    }

    act(msg, ch, obj1, obj2, To::Char);
}

void do_credits(Char *ch) { do_help(ch, "diku"); }

void do_where(Char *ch, ArgParser args) {
    if (args.empty()) {
        ch->send_line("|cYou are in {}\n\rPlayers near you:|w", ch->in_room->area->short_name());
        auto found = false;
        for (auto &victim : descriptors().all_visible_to(*ch) | DescriptorFilter::except(*ch)
                                | DescriptorFilter::same_area(*ch) | DescriptorFilter::to_character()) {
            if (victim.is_pc()) {
                found = true;
                ch->send_line("|W{:<28}|w {}", victim.name, victim.in_room->name);
            }
        }
        if (!found)
            ch->send_line("None");
        if (ch->pet && ch->pet->in_room->area == ch->in_room->area) {
            ch->send_line("You sense that your pet is near {}.", ch->pet->in_room->name);
        }
    } else {
        auto found = false;
        auto name = args.shift();
        for (auto *victim : char_list) {
            if (victim->in_room != nullptr && victim->in_room->area == ch->in_room->area && !victim->is_aff_hide()
                && !victim->is_aff_sneak() && can_see(ch, victim) && victim != ch && is_name(name, victim->name)) {
                found = true;
                ch->send_line("|W{:<28}|w {}", ch->describe(*victim), victim->in_room->name);
                break;
            }
        }
        if (!found)
            act("You didn't find any $T.", ch, nullptr, name, To::Char);
    }
}

void do_consider(Char *ch, ArgParser args) {
    Char *victim;
    const char *msg;
    int diff;

    const auto arg = args.shift();
    if (arg.empty()) {
        ch->send_line("Consider killing whom?");
        return;
    }

    if ((victim = get_char_room(ch, arg)) == nullptr) {
        ch->send_line("They're not here.");
        return;
    }

    if (is_safe(ch, victim)) {
        ch->send_line("Don't even think about it.");
        return;
    }

    diff = victim->level - ch->level;

    if (diff <= -10)
        msg = "You can kill $N naked and weaponless.";
    else if (diff <= -5)
        msg = "$N is no match for you.";
    else if (diff <= -2)
        msg = "$N looks like an easy kill.";
    else if (diff <= 1)
        msg = "The perfect match!";
    else if (diff <= 4)
        msg = "$N says 'Do you feel lucky, punk?'.";
    else if (diff <= 9)
        msg = "$N laughs at you mercilessly.";
    else
        msg = "|RDeath will thank you for your gift.|w";

    act(msg, ch, nullptr, victim, To::Char);
    if (ch->level >= LEVEL_CONSIDER) {
        do_mstat(ch, arg);
    }
}

void set_prompt(Char *ch, std::string_view prompt) {
    if (ch->is_npc()) {
        bug("Set_prompt: NPC.");
        return;
    }
    ch->pcdata->prompt = prompt;
}

void do_title(Char *ch, std::string_view argument) {
    if (ch->is_npc())
        return;
    if (argument.empty()) {
        ch->send_line("Change your title to what?");
        return;
    }
    auto new_title = smash_tilde(argument);
    if (new_title.length() > 45)
        new_title.resize(45);
    ch->set_title(new_title);
    ch->send_line("Ok.");
}

void do_description(Char *ch, std::string_view argument) {
    if (auto desc_line = smash_tilde(argument); !desc_line.empty()) {
        if (desc_line.front() == '+') {
            ch->description += fmt::format("{}\n\r", ltrim(desc_line.substr(1)));
        } else if (desc_line == "-") {
            if (ch->description.empty()) {
                ch->send_line("You have no description.");
                return;
            }
            ch->description = remove_last_line(ch->description);
        } else {
            ch->description = desc_line + "\n\r";
        }
        if (ch->description.size() >= MAX_STRING_LENGTH - 2) {
            ch->send_line("Description too long.");
            return;
        }
    }

    ch->send_to("Your description is:\n\r{}", ch->description.empty() ? "(None).\n\r" : ch->description);
}

void do_report(Char *ch) {
    ch->send_line("You say 'I have {}/{} hp {}/{} mana {}/{} mv {} xp.'\n\r", ch->hit, ch->max_hit, ch->mana,
                  ch->max_mana, ch->move, ch->max_move, ch->exp);
    act(fmt::format("$n says 'I have {}/{} hp {}/{} mana {}/{} mv {} xp.'", ch->hit, ch->max_hit, ch->mana,
                    ch->max_mana, ch->move, ch->max_move, ch->exp),
        ch);
}

namespace {
Char *find_prac_mob(Room *room) {
    for (auto *mob : room->people) {
        if (mob->is_npc() && check_enum_bit(mob->act, CharActFlag::Practice))
            return mob;
    }
    return nullptr;
}
}

void do_practice(Char *ch, ArgParser args) {
    if (ch->is_npc())
        return;

    if (args.empty()) {
        PracticeTabulator::tabulate(ch);
    } else {
        if (!ch->is_pos_awake()) {
            ch->send_line("In your dreams, or what?");
            return;
        }

        Char *mob = find_prac_mob(ch->in_room);
        if (!mob) {
            ch->send_line("You can't do that here.");
            return;
        }

        if (ch->practice <= 0) {
            ch->send_line("You have no practice sessions left.");
            return;
        }
        auto sn = skill_lookup(args.shift());
        if (sn < 0) {
            ch->send_line("You can't practice that.");
            return;
        }

        auto &skill_level = ch->pcdata->learned[sn]; // NOT ch.get_skill()
        if (ch->level < get_skill_level(ch, sn) || skill_level < 1) {
            ch->send_line("You can't practice that.");
            return;
        }

        auto adept = ch->is_npc() ? 100 : class_table[ch->class_num].skill_adept;

        if (skill_level >= adept) {
            ch->send_line("You are already learned at {}.", skill_table[sn].name);
        } else {
            ch->practice--;
            if (get_skill_trains(ch, sn) < 0) {
                skill_level += int_app[get_curr_stat(ch, Stat::Int)].learn / 4;
            } else {
                skill_level += int_app[get_curr_stat(ch, Stat::Int)].learn / get_skill_difficulty(ch, sn);
            }
            if (skill_level < adept) {
                act("You practice $T.", ch, nullptr, skill_table[sn].name, To::Char);
                act("$n practices $T.", ch, nullptr, skill_table[sn].name, To::Room);
            } else {
                skill_level = adept;
                act("You are now learned at $T.", ch, nullptr, skill_table[sn].name, To::Char);
                act("$n is now learned at $T.", ch, nullptr, skill_table[sn].name, To::Room);
            }
        }
    }
}

/*
 * 'Wimpy' originally by Dionysos.
 */
void do_wimpy(Char *ch, ArgParser args) {
    auto wimpy = args.try_shift_number().value_or(ch->max_hit / 5);

    if (wimpy < 0) {
        ch->send_line("Your courage exceeds your wisdom.");
        return;
    }

    if (wimpy > ch->max_hit / 2) {
        ch->send_line("Such cowardice ill becomes you.");
        return;
    }

    ch->wimpy = wimpy;
    ch->send_line("Wimpy set to {} hit points.", wimpy);
}

void do_password(Char *ch, ArgParser args) {
    if (ch->is_npc())
        return;
    auto old_pass = std::string(args.shift());
    auto new_pass = std::string(args.shift());
    if (old_pass.empty() || new_pass.empty()) {
        ch->send_line("Syntax: password <old> <new>.");
        return;
    }

    if (!ch->pcdata->pwd.empty()) {
        if (auto old_crypt = crypt(old_pass.c_str(), ch->pcdata->pwd.c_str()); old_crypt != ch->pcdata->pwd) {
            ch->wait_state(40);
            ch->send_line("Wrong password.  Wait 10 seconds.");
            return;
        }
    }
    if (new_pass.length() < MinPasswordLen) {
        ch->send_line("New password must be at least five characters long.");
        return;
    }
    /*
     * No tilde allowed because of player file format.
     */
    auto new_crypt = crypt(new_pass.c_str(), ch->name.c_str());
    if (matches_inside("~", new_crypt)) {
        ch->send_line("New password not acceptable, try again.");
        return;
    }
    ch->pcdata->pwd = new_crypt;
    save_char_obj(ch);
    ch->send_line("Ok.");
}

/* RT configure command SMASHED */

/* MrG Scan command */

void do_scan(Char *ch) {
    Room *current_place;
    int count_num_rooms;
    int num_rooms_scan = std::max(1, ch->level / 10);
    bool found_anything = false;
    std::vector<sh_int> found_rooms{ch->in_room->vnum};

    ch->send_line("You can see around you :");
    /* Loop for each point of the compass */

    for (auto direction : all_directions) {
        /* No exits in that direction */
        current_place = ch->in_room;
        /* Loop for the distance see-able */
        for (count_num_rooms = 0; count_num_rooms < num_rooms_scan; count_num_rooms++) {
            const auto &exit = current_place->exits[direction];
            if (!exit)
                break;
            if (current_place = exit->u1.to_room; !current_place || !can_see_room(ch, exit->u1.to_room)
                                                  || check_enum_bit(exit->exit_info, ExitFlag::Closed))
                break;
            // Eliminate cycles in labyrinthine areas.
            if (std::find(found_rooms.begin(), found_rooms.end(), exit->u1.to_room->vnum) != found_rooms.end())
                break;
            found_rooms.push_back(exit->u1.to_room->vnum);

            /* This loop goes through each character in a room and says
                        whether or not they are visible */

            for (auto *current_person : current_place->people) {
                if (ch->can_see(*current_person)) {
                    ch->send_to(fmt::format("{} {:<5}: |W{}|w\n\r", count_num_rooms + 1,
                                            initial_caps_only(to_string(direction)), current_person->short_name()));
                    found_anything = true;
                }
            } /* Closes the for_each_char_loop */

        } /* Closes the for_each distance seeable loop */

    } /* closes main loop for each direction */
    if (!found_anything)
        ch->send_line("Nothing of great interest.");
}

/*
 * alist to list all areas
 */

void do_alist(Char *ch) {
    auto format_str = "{:3} {:29} {:<5}-{:>5} {:12}\n\r"sv;
    auto buffer = fmt::vformat(format_str, fmt::make_format_args("Num", "Area Name", "Lvnum", "Uvnum", "Filename"));
    for (auto &pArea : AreaList::singleton())
        buffer +=
            fmt::vformat(format_str, fmt::make_format_args(pArea->num(), pArea->short_name(), pArea->lowest_vnum(),
                              pArea->highest_vnum(), pArea->filename()));
    ch->page_to(buffer);
}

/* do_prefix added 19-05-97 PCFN */
void do_prefix(Char *ch, std::string_view argument) {
    if (ch = ch->player(); !ch)
        return;

    auto prefix = smash_tilde(argument);
    if (prefix.length() > (MAX_STRING_LENGTH - 1))
        prefix.resize(MAX_STRING_LENGTH - 1);

    if (prefix.empty()) {
        if (ch->pcdata->prefix.empty()) {
            ch->send_line("No prefix to remove.");
        } else {
            ch->send_line("Prefix removed.");
            ch->pcdata->prefix.clear();
        }
    } else {
        ch->pcdata->prefix = prefix;
        ch->send_line("Prefix set to \"{}\"", ch->pcdata->prefix);
    }
}