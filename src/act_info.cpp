/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                    */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  act_info.c: standard information functions                           */
/*                                                                       */
/*************************************************************************/

#include "AFFECT_DATA.hpp"
#include "AREA_DATA.hpp"
#include "ArmourClass.hpp"
#include "BitsAffect.hpp"
#include "BitsCharAct.hpp"
#include "BitsContainerState.hpp"
#include "BitsDamageTolerance.hpp"
#include "BitsExitState.hpp"
#include "BitsObjectExtra.hpp"
#include "BitsObjectWear.hpp"
#include "BitsPlayerAct.hpp"
#include "BitsRoomState.hpp"
#include "Char.hpp"
#include "Classes.hpp"
#include "Columner.hpp"
#include "Descriptor.hpp"
#include "DescriptorList.hpp"
#include "Exit.hpp"
#include "Help.hpp"
#include "Materials.hpp"
#include "Object.hpp"
#include "ObjectIndex.hpp"
#include "ObjectType.hpp"
#include "Races.hpp"
#include "SkillNumbers.hpp"
#include "SkillTables.hpp"
#include "Socials.hpp"
#include "TimeInfoData.hpp"
#include "WearLocation.hpp"
#include "WeatherData.hpp"
#include "act_comm.hpp"
#include "comm.hpp"
#include "db.h"
#include "fight.hpp"
#include "handler.hpp"
#include "interp.h"
#include "lookup.h"
#include "merc.h"
#include "save.hpp"
#include "skills.hpp"
#include "string_utils.hpp"

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <magic_enum.hpp>
#include <range/v3/algorithm/find_if.hpp>
#include <range/v3/iterator/operations.hpp>

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace std::literals;

namespace {

std::string wear_string_for(const Object *obj, int wear_location) {
    constexpr std::array<std::string_view, MAX_WEAR> where_name = {
        "used as light",     "worn on finger",   "worn on finger",
        "worn around neck",  "worn around neck", "worn on body",
        "worn on head",      "worn on legs",     "worn on feet",
        "worn on hands",     "worn on arms",     "worn as shield",
        "worn about body",   "worn about waist", "worn around wrist",
        "worn around wrist", "wielded",          "held",
        "worn on ears"};

    return fmt::format("<{}>", obj->wear_string.empty() ? where_name[wear_location] : obj->wear_string);
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

/* Mg's funcy shun */
void set_prompt(Char *ch, const char *prompt);

std::string format_obj_to_char(const Object *obj, const Char *ch, bool fShort) {
    std::string buf;
    std::string desc = fShort ? obj->short_descr : obj->description;
    if (desc.empty()) {
        desc = "This object has no description. Please inform the IMP.";
        bug("Object {} has no description", obj->objIndex->vnum);
    }
    if (IS_OBJ_STAT(obj, ITEM_UNIQUE))
        buf += "(U) ";
    if (IS_OBJ_STAT(obj, ITEM_INVIS))
        buf += "(|cInvis|w) ";
    if (ch->has_detect_evil() && IS_OBJ_STAT(obj, ITEM_EVIL))
        buf += "(|rRed Aura|w) ";
    if (ch->has_detect_magic() && IS_OBJ_STAT(obj, ITEM_MAGIC))
        buf += "(|gMagical|w) ";
    if (IS_OBJ_STAT(obj, ITEM_GLOW))
        buf += "(|WGlowing|w) ";
    if (IS_OBJ_STAT(obj, ITEM_HUM))
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

    const bool show_counts = ch->is_npc() || check_bit(ch->comm, COMM_COMBINE);

    // Format the list of objects.
    for (auto *obj : list) {
        if (obj->wear_loc == WEAR_NONE && can_see_obj(ch, obj)) {
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

    if (IS_AFFECTED(victim, AFF_INVISIBLE))
        buf += "(|WInvis|w) ";
    if (victim->is_pc() && check_bit(victim->act, PLR_WIZINVIS))
        buf += "(|RWizi|w) ";
    if (victim->is_pc() && check_bit(victim->act, PLR_PROWL))
        buf += "(|RProwl|w) ";
    if (IS_AFFECTED(victim, AFF_HIDE))
        buf += "(|WHide|w) ";
    if (IS_AFFECTED(victim, AFF_CHARM))
        buf += "(|yCharmed|w) ";
    if (IS_AFFECTED(victim, AFF_PASS_DOOR))
        buf += "(|bTranslucent|w) ";
    if (IS_AFFECTED(victim, AFF_FAERIE_FIRE))
        buf += "(|PPink Aura|w) ";
    if (IS_AFFECTED(victim, AFF_OCTARINE_FIRE))
        buf += "(|GOctarine Aura|w) ";
    if (victim->is_evil() && IS_AFFECTED(ch, AFF_DETECT_EVIL))
        buf += "(|rRed Aura|w) ";
    if (IS_AFFECTED(victim, AFF_SANCTUARY))
        buf += "(|WWhite Aura|w) ";
    if (victim->is_pc() && check_bit(victim->act, PLR_KILLER))
        buf += "(|RKILLER|w) ";
    if (victim->is_pc() && check_bit(victim->act, PLR_THIEF))
        buf += "(|RTHIEF|w) ";

    if (is_affected(ch, gsn_bless)) {
        if (check_bit(victim->act, ACT_UNDEAD)) {
            buf += "(|bUndead|w) ";
        }
    }

    if (victim->position == victim->start_pos && !victim->long_descr.empty()) {
        buf += victim->long_descr;
        ch->send_to(buf);
        return;
    }

    buf += pers(victim, ch);
    if (victim->is_pc() && !check_bit(ch->comm, COMM_BRIEF))
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
            buf += fmt::format(" {}.", pers(victim->fighting, ch));
        } else
            buf += " somone who left??";
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
    for (int iWear = 0; iWear < MAX_WEAR; iWear++) {
        if (const auto *obj = get_eq_char(victim, iWear); obj && can_see_obj(ch, obj)) {
            if (!found) {
                ch->send_line("");
                act("$N is using:", ch, nullptr, victim, To::Char);
                found = true;
            }
            ch->send_line("{:<20}{}", wear_string_for(obj, iWear), format_obj_to_char(obj, ch, true));
        }
    }

    if (victim != ch && ch->is_pc() && number_percent() < ch->get_skill(gsn_peek) && check_bit(ch->act, PLR_AUTOPEEK)) {
        ch->send_line("\n\rYou peek at the inventory:");
        check_improve(ch, gsn_peek, true, 4);
        show_list_to_char(victim->carrying, ch, true, true);
    }
}

void do_peek(Char *ch, const char *argument) {
    Char *victim;
    char arg1[MAX_INPUT_LENGTH];

    if (ch->desc == nullptr)
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

    if (ch->is_pc() && !check_bit(ch->act, PLR_HOLYLIGHT) && room_is_dark(ch->in_room)) {
        ch->send_line("It is pitch black ... ");
        show_char_to_char(ch->in_room->people, ch);
        return;
    }

    argument = one_argument(argument, arg1);

    if ((victim = get_char_room(ch, arg1)) != nullptr) {
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

        if (rch->is_pc() && check_bit(rch->act, PLR_WIZINVIS) && ch->get_trust() < rch->invis_level)
            continue;

        if (can_see(ch, rch)) {
            show_char_to_char_0(rch, ch);
        } else if (room_is_dark(ch->in_room) && IS_AFFECTED(rch, AFF_INFRARED)) {
            ch->send_line("You see |Rglowing red|w eyes watching |RYOU!|w");
        }
    }
}

bool check_blind(const Char *ch) {
    if (!ch->is_blind() || ch->has_holylight())
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

/* RT does socials */
void do_socials(Char *ch) {
    Columner col(*ch, 6, 12);
    for (int iSocial = 0; social_table[iSocial].name[0] != '\0'; iSocial++)
        col.add(social_table[iSocial].name);
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
        return fmt::format_to(ctx.out(), onoff.b ? "|RON|w" : "|ROFF|w");
    }
};

void do_autolist(Char *ch) {
    /* lists most player flags */
    if (ch->is_npc())
        return;

    ch->send_line("   action     status");
    ch->send_line("---------------------");

    ch->send_line("ANSI colour    {}", OnOff{ch->pcdata->colour});
    ch->send_line("autoaffect     {}", OnOff{check_bit(ch->comm, COMM_AFFECT)});
    ch->send_line("autoassist     {}", OnOff{check_bit(ch->act, PLR_AUTOASSIST)});
    ch->send_line("autoexit       {}", OnOff{check_bit(ch->act, PLR_AUTOEXIT)});
    ch->send_line("autogold       {}", OnOff{check_bit(ch->act, PLR_AUTOGOLD)});
    ch->send_line("autoloot       {}", OnOff{check_bit(ch->act, PLR_AUTOLOOT)});
    ch->send_line("autopeek       {}", OnOff{check_bit(ch->act, PLR_AUTOPEEK)});
    ch->send_line("autosac        {}", OnOff{check_bit(ch->act, PLR_AUTOSAC)});
    ch->send_line("autosplit      {}", OnOff{check_bit(ch->act, PLR_AUTOSPLIT)});
    ch->send_line("prompt         {}", OnOff{check_bit(ch->comm, COMM_PROMPT)});
    ch->send_line("combine items  {}", OnOff{check_bit(ch->comm, COMM_COMBINE)});

    if (!check_bit(ch->act, PLR_CANLOOT))
        ch->send_line("Your corpse is safe from thieves.");
    else
        ch->send_line("Your corpse may be looted.");

    if (check_bit(ch->act, PLR_NOSUMMON))
        ch->send_line("You cannot be summoned.");
    else
        ch->send_line("You can be summoned.");

    if (check_bit(ch->act, PLR_NOFOLLOW))
        ch->send_line("You do not welcome followers.");
    else
        ch->send_line("You accept followers.");

    if (check_bit(ch->comm, COMM_BRIEF))
        ch->send_line("Only brief descriptions are being shown.");
    else
        ch->send_line("Full descriptions are being shown.");

    if (check_bit(ch->comm, COMM_COMPACT))
        ch->send_line("Compact mode is set.");
    else
        ch->send_line("Compact mode is not set.");

    if (check_bit(ch->comm, COMM_SHOWAFK))
        ch->send_line("Messages sent to you will be shown when afk.");
    else
        ch->send_line("Messages sent to you will not be shown when afk.");

    if (check_bit(ch->comm, COMM_SHOWDEFENCE))
        ch->send_line("Shield blocks, parries, and dodges are being shown.");
    else
        ch->send_line("Shield blocks, parries, and dodges are not being shown.");
}

void do_autoaffect(Char *ch) {
    if (ch->is_npc())
        return;

    if (check_bit(ch->comm, COMM_AFFECT)) {
        ch->send_line("Autoaffect removed.");
        clear_bit(ch->comm, COMM_AFFECT);
    } else {
        ch->send_line("Affects will now be shown in score.");
        set_bit(ch->comm, COMM_AFFECT);
    }
}
void do_autoassist(Char *ch) {
    if (ch->is_npc())
        return;

    if (check_bit(ch->act, PLR_AUTOASSIST)) {
        ch->send_line("Autoassist removed.");
        clear_bit(ch->act, PLR_AUTOASSIST);
    } else {
        ch->send_line("You will now assist when needed.");
        set_bit(ch->act, PLR_AUTOASSIST);
    }
}

void do_autoexit(Char *ch) {
    if (ch->is_npc())
        return;

    if (check_bit(ch->act, PLR_AUTOEXIT)) {
        ch->send_line("Exits will no longer be displayed.");
        clear_bit(ch->act, PLR_AUTOEXIT);
    } else {
        ch->send_line("Exits will now be displayed.");
        set_bit(ch->act, PLR_AUTOEXIT);
    }
}

void do_autogold(Char *ch) {
    if (ch->is_npc())
        return;

    if (check_bit(ch->act, PLR_AUTOGOLD)) {
        ch->send_line("Autogold removed.");
        clear_bit(ch->act, PLR_AUTOGOLD);
    } else {
        ch->send_line("Automatic gold looting set.");
        set_bit(ch->act, PLR_AUTOGOLD);
    }
}

void do_autoloot(Char *ch) {
    if (ch->is_npc())
        return;

    if (check_bit(ch->act, PLR_AUTOLOOT)) {
        ch->send_line("Autolooting removed.");
        clear_bit(ch->act, PLR_AUTOLOOT);
    } else {
        ch->send_line("Automatic corpse looting set.");
        set_bit(ch->act, PLR_AUTOLOOT);
    }
}

void do_autopeek(Char *ch) {
    if (ch->is_npc())
        return;

    if (check_bit(ch->act, PLR_AUTOPEEK)) {
        ch->send_line("Autopeeking removed.");
        clear_bit(ch->act, PLR_AUTOPEEK);
    } else {
        ch->send_line("Automatic peeking set.");
        set_bit(ch->act, PLR_AUTOPEEK);
    }
}

void do_autosac(Char *ch) {
    if (ch->is_npc())
        return;

    if (check_bit(ch->act, PLR_AUTOSAC)) {
        ch->send_line("Autosacrificing removed.");
        clear_bit(ch->act, PLR_AUTOSAC);
    } else {
        ch->send_line("Automatic corpse sacrificing set.");
        set_bit(ch->act, PLR_AUTOSAC);
    }
}

void do_autosplit(Char *ch) {
    if (ch->is_npc())
        return;

    if (check_bit(ch->act, PLR_AUTOSPLIT)) {
        ch->send_line("Autosplitting removed.");
        clear_bit(ch->act, PLR_AUTOSPLIT);
    } else {
        ch->send_line("Automatic gold splitting set.");
        set_bit(ch->act, PLR_AUTOSPLIT);
    }
}

void do_brief(Char *ch) {
    if (check_bit(ch->comm, COMM_BRIEF)) {
        ch->send_line("Full descriptions activated.");
        clear_bit(ch->comm, COMM_BRIEF);
    } else {
        ch->send_line("Short descriptions activated.");
        set_bit(ch->comm, COMM_BRIEF);
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
    if (check_bit(ch->comm, COMM_SHOWAFK)) {
        ch->send_line("Messages sent to you will now not be shown when afk.");
        clear_bit(ch->comm, COMM_SHOWAFK);
    } else {
        ch->send_line("Messages sent to you will now be shown when afk.");
        set_bit(ch->comm, COMM_SHOWAFK);
    }
}

void do_showdefence(Char *ch) {
    if (check_bit(ch->comm, COMM_SHOWDEFENCE)) {
        ch->send_line("Shield blocks, parries and dodges will not be shown during combat.");
        clear_bit(ch->comm, COMM_SHOWDEFENCE);
    } else {
        ch->send_line("Shield blocks, parries and dodges will be shown during combat.");
        set_bit(ch->comm, COMM_SHOWDEFENCE);
    }
}

void do_compact(Char *ch) {
    if (check_bit(ch->comm, COMM_COMPACT)) {
        ch->send_line("Compact mode removed.");
        clear_bit(ch->comm, COMM_COMPACT);
    } else {
        ch->send_line("Compact mode set.");
        set_bit(ch->comm, COMM_COMPACT);
    }
}

void do_prompt(Char *ch, const char *argument) {
    /* PCFN 24-05-97  Oh dear - it seems that you can't set prompt while switched
       into a MOB.  Let's change that.... */
    if (ch = ch->player(); !ch)
        return;

    if (str_cmp(argument, "off") == 0) {
        ch->send_line("You will no longer see prompts.");
        clear_bit(ch->comm, COMM_PROMPT);
        return;
    }
    if (str_cmp(argument, "on") == 0) {
        ch->send_line("You will now see prompts.");
        set_bit(ch->comm, COMM_PROMPT);
        return;
    }

    /* okay that was the old stuff */
    set_prompt(ch, smash_tilde(argument).c_str());
    ch->send_line("Ok - prompt set.");
    set_bit(ch->comm, COMM_PROMPT);
}

void do_combine(Char *ch) {
    if (check_bit(ch->comm, COMM_COMBINE)) {
        ch->send_line("Long inventory selected.");
        clear_bit(ch->comm, COMM_COMBINE);
    } else {
        ch->send_line("Combined inventory selected.");
        set_bit(ch->comm, COMM_COMBINE);
    }
}

void do_noloot(Char *ch) {
    if (ch->is_npc())
        return;

    if (check_bit(ch->act, PLR_CANLOOT)) {
        ch->send_line("Your corpse is now safe from thieves.");
        clear_bit(ch->act, PLR_CANLOOT);
    } else {
        ch->send_line("Your corpse may now be looted.");
        set_bit(ch->act, PLR_CANLOOT);
    }
}

void do_nofollow(Char *ch) {
    if (ch->is_npc())
        return;

    if (check_bit(ch->act, PLR_NOFOLLOW)) {
        ch->send_line("You now accept followers.");
        clear_bit(ch->act, PLR_NOFOLLOW);
    } else {
        ch->send_line("You no longer accept followers.");
        set_bit(ch->act, PLR_NOFOLLOW);
        die_follower(ch);
    }
}

void do_nosummon(Char *ch) {
    if (ch->is_npc()) {
        if (check_bit(ch->imm_flags, DMG_TOL_SUMMON)) {
            ch->send_line("You are no longer immune to summon.");
            clear_bit(ch->imm_flags, DMG_TOL_SUMMON);
        } else {
            ch->send_line("You are now immune to summoning.");
            set_bit(ch->imm_flags, DMG_TOL_SUMMON);
        }
    } else {
        if (check_bit(ch->act, PLR_NOSUMMON)) {
            ch->send_line("You are no longer immune to summon.");
            clear_bit(ch->act, PLR_NOSUMMON);
        } else {
            ch->send_line("You are now immune to summoning.");
            set_bit(ch->act, PLR_NOSUMMON);
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
        (*skill_table[identify].spell_fun)(identify, ch->level, ch, (void *)obj);
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

    case ObjectType::Drink:
        if (obj.value[1] <= 0) {
            ch.send_line("It is empty.");
            break;
        }

        ch.send_line("It's {} full of a {} liquid.",
                     obj.value[1] < obj.value[0] / 4 ? "less than"
                                                     : obj.value[1] < 3 * obj.value[0] / 4 ? "about" : "more than",
                     liq_table[obj.value[2]].liq_color);
        break;

    case ObjectType::Container:
    case ObjectType::Npccorpse:
    case ObjectType::Pccorpse:
        if (check_bit(obj.value[1], CONT_CLOSED)) {
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

void look_direction(const Char &ch, Direction door) {
    const auto *pexit = ch.in_room->exit[door];
    if (!pexit) {
        ch.send_line("Nothing special there.");
        return;
    }

    if (pexit->description && pexit->description[0] != '\0')
        ch.send_to(pexit->description);
    else
        ch.send_line("Nothing special there.");

    if (pexit->keyword && pexit->keyword[0] != '\0' && pexit->keyword[0] != ' ') {
        if (check_bit(pexit->exit_info, EX_CLOSED)) {
            act("The $d is closed.", &ch, nullptr, pexit->keyword, To::Char);
        } else if (check_bit(pexit->exit_info, EX_ISDOOR)) {
            act("The $d is open.", &ch, nullptr, pexit->keyword, To::Char);
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
    if (auto opt_door = try_parse_direction(first_arg)) {
        look_direction(*ch, *opt_door);
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
void do_exits(const Char *ch, const char *argument) {

    auto fAuto = matches(argument, "auto");

    if (!check_blind(ch))
        return;

    std::string buf = fAuto ? "|W[Exits:" : "Obvious exits:\n\r";

    auto found = false;
    for (auto door : all_directions) {
        if (auto *pexit = ch->in_room->exit[door]; pexit && pexit->u1.to_room && can_see_room(ch, pexit->u1.to_room)
                                                   && !check_bit(pexit->exit_info, EX_CLOSED)) {
            found = true;
            if (fAuto) {
                buf += fmt::format(" {}", to_string(door));
            } else {
                buf += fmt::format("{:<5} - {}\n\r", capitalize(to_string(door)),
                                   !ch->has_holylight() && room_is_dark(pexit->u1.to_room) ? "Too dark to tell"
                                                                                           : pexit->u1.to_room->name);
            }
        }
    }

    if (!found)
        buf += fAuto ? " none" : "None.\n\r";

    if (fAuto)
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
        col3.stat("Holy light", check_bit(ch->act, PLR_HOLYLIGHT) ? "on" : "off");
        if (check_bit(ch->act, PLR_WIZINVIS))
            col3.stat("Invisible", ch->invis_level);
        if (check_bit(ch->act, PLR_PROWL))
            col3.stat("Prowl", ch->invis_level);
        col3.flush();
    }

    if (const auto opt_nutrition_msg = ch->describe_nutrition()) {
        ch->send_line(*opt_nutrition_msg);
    }

    if (check_bit(ch->comm, COMM_AFFECT)) {
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
    if (!IS_OUTSIDE(ch)) {
        ch->send_line("You can't see the weather indoors.");
        return;
    }

    ch->send_to(weather_info.describe() + "\n\r");
}

void do_help(Char *ch, const char *argument) {
    char argall[MAX_INPUT_LENGTH], argone[MAX_INPUT_LENGTH];

    if (argument[0] == '\0')
        argument = "summary";

    /* this parts handles help a b so that it returns help 'a b' */
    argall[0] = '\0';
    while (argument[0] != '\0') {
        argument = one_argument(argument, argone);
        if (argall[0] != '\0')
            strcat(argall, " ");
        strcat(argall, argone);
    }

    if (auto *help = HelpList::singleton().lookup(ch->get_trust(), argall)) {
        if (help->level() >= 0 && !matches(argall, "imotd"))
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
        who_clan_name_of(wch), check_bit(wch.act, PLR_KILLER) ? "(|RKILLER|w) " : "",
        check_bit(wch.act, PLR_THIEF) ? "(|RTHIEF|w) " : "", check_bit(wch.act, PLR_AFK) ? "(|cAFK|w) " : "", wch.name,
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
void do_who(Char *ch, const char *argument) {
    int iClass;
    int iRace;
    int iLevelLower;
    int iLevelUpper;
    int nNumber;
    int nMatch;
    bool rgfClass[MAX_CLASS];
    bool rgfRace[MAX_PC_RACE];
    std::unordered_set<const Clan *> rgfClan;
    bool fClassRestrict;
    bool fRaceRestrict;
    bool fClanRestrict;
    bool fImmortalOnly;
    char arg[MAX_STRING_LENGTH];

    /*
     * Set default arguments.
     */
    iLevelLower = 0;
    iLevelUpper = MAX_LEVEL;
    fClassRestrict = false;
    fRaceRestrict = false;
    fClanRestrict = false;
    fImmortalOnly = false;
    for (iClass = 0; iClass < MAX_CLASS; iClass++)
        rgfClass[iClass] = false;
    for (iRace = 0; iRace < MAX_PC_RACE; iRace++)
        rgfRace[iRace] = false;

    /*
     * Parse arguments.
     */
    nNumber = 0;
    for (;;) {

        argument = one_argument(argument, arg);
        if (arg[0] == '\0')
            break;

        if (is_number(arg)) {
            switch (++nNumber) {
            case 1: iLevelLower = atoi(arg); break;
            case 2: iLevelUpper = atoi(arg); break;
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
    for (int iWear = 0; iWear < MAX_WEAR; iWear++) {
        if (const auto *obj = get_eq_char(ch, iWear)) {
            ch->send_line("{:<20}{}", wear_string_for(obj, iWear),
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
        if (obj->wear_loc != WEAR_NONE && can_see_obj(ch, obj) && obj_to_compare_to->type == obj->type
            && (obj_to_compare_to->wear_flags & obj->wear_flags & ~ITEM_TAKE) != 0) {
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

void do_where(Char *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        ch->send_line("|cYou are in {}\n\rPlayers near you:|w", ch->in_room->area->areaname);
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
        for (auto *victim : char_list) {
            if (victim->in_room != nullptr && victim->in_room->area == ch->in_room->area
                && !IS_AFFECTED(victim, AFF_HIDE) && !IS_AFFECTED(victim, AFF_SNEAK) && can_see(ch, victim)
                && victim != ch && is_name(arg, victim->name)) {
                found = true;
                ch->send_line("|W{:<28}|w {}", pers(victim, ch), victim->in_room->name);
                break;
            }
        }
        if (!found)
            act("You didn't find any $T.", ch, nullptr, arg, To::Char);
    }
}

void do_consider(Char *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    Char *victim;
    const char *msg;
    int diff;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
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
    if (ch->level >= LEVEL_CONSIDER)
        do_mstat(ch, argument);
}

void set_prompt(Char *ch, const char *prompt) {
    if (ch->is_npc()) {
        bug("Set_prompt: NPC.");
        return;
    }
    ch->pcdata->prompt = prompt;
}

void do_title(Char *ch, const char *argument) {
    if (ch->is_npc())
        return;

    if (argument[0] == '\0') {
        ch->send_line("Change your title to what?");
        return;
    }

    auto new_title = smash_tilde(argument);
    if (new_title.length() > 45)
        new_title.resize(45);

    ch->set_title(new_title);
    ch->send_line("Ok.");
}

void do_description(Char *ch, const char *argument) {
    if (auto desc_line = smash_tilde(argument); !desc_line.empty()) {
        if (desc_line.front() == '+') {
            ch->description += ltrim(desc_line.substr(1)) + "\n\r";
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
        if (mob->is_npc() && check_bit(mob->act, ACT_PRACTICE))
            return mob;
    }
    return nullptr;
}
}

void do_practice(Char *ch, const char *argument) {
    if (ch->is_npc())
        return;

    if (argument[0] == '\0') {
        Columner col(*ch, 3);
        for (auto sn = 0; sn < MAX_SKILL; sn++) {
            if (skill_table[sn].name == nullptr)
                break;
            auto skill_level = ch->pcdata->learned[sn]; // NOT ch.get_skill()
            if (ch->level >= get_skill_level(ch, sn) && skill_level > 0)
                col.add("{:<18} {:3}%", skill_table[sn].name, skill_level);
        }
        col.flush();

        ch->send_line("You have {} practice sessions left.", ch->practice);
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
        auto sn = skill_lookup(argument);
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

void do_password(Char *ch, const char *argument) {
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char *pArg;
    char *pwdnew;
    char *p;
    char cEnd;

    if (ch->is_npc())
        return;

    /*
     * Can't use one_argument here because it smashes case.
     * So we just steal all its code.  Bleagh.
     */
    pArg = arg1;
    while (isspace(*argument))
        argument++;

    cEnd = ' ';
    if (*argument == '\'' || *argument == '"')
        cEnd = *argument++;

    while (*argument != '\0') {
        if (*argument == cEnd) {
            argument++;
            break;
        }
        *pArg++ = *argument++;
    }
    *pArg = '\0';

    pArg = arg2;
    while (isspace(*argument))
        argument++;

    cEnd = ' ';
    if (*argument == '\'' || *argument == '"')
        cEnd = *argument++;

    while (*argument != '\0') {
        if (*argument == cEnd) {
            argument++;
            break;
        }
        *pArg++ = *argument++;
    }
    *pArg = '\0';

    if (arg1[0] == '\0' || arg2[0] == '\0') {
        ch->send_line("Syntax: password <old> <new>.");
        return;
    }

    if (!ch->pcdata->pwd.empty() && strcmp(crypt(arg1, ch->pcdata->pwd.c_str()), ch->pcdata->pwd.c_str())) {
        ch->wait_state(40);
        ch->send_line("Wrong password.  Wait 10 seconds.");
        return;
    }

    if ((int)strlen(arg2) < 5) {
        ch->send_line("New password must be at least five characters long.");
        return;
    }

    /*
     * No tilde allowed because of player file format.
     */
    pwdnew = crypt(arg2, ch->name.c_str());
    for (p = pwdnew; *p != '\0'; p++) {
        if (*p == '~') {
            ch->send_line("New password not acceptable, try again.");
            return;
        }
    }

    ch->pcdata->pwd = pwdnew;
    save_char_obj(ch);
    ch->send_line("Ok.");
}

/* RT configure command SMASHED */

/* MrG Scan command */

void do_scan(Char *ch) {
    Room *current_place;
    Exit *pexit;
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

            if ((pexit = current_place->exit[direction]) == nullptr || (current_place = pexit->u1.to_room) == nullptr
                || !can_see_room(ch, pexit->u1.to_room) || check_bit(pexit->exit_info, EX_CLOSED))
                break;
            // Eliminate cycles in labyrinthine areas.
            if (std::find(found_rooms.begin(), found_rooms.end(), pexit->u1.to_room->vnum) != found_rooms.end()) {
                break;
            }
            found_rooms.push_back(pexit->u1.to_room->vnum);

            /* This loop goes through each character in a room and says
                        whether or not they are visible */

            for (auto *current_person : current_place->people) {
                if (ch->can_see(*current_person)) {
                    ch->send_to(fmt::format("{} {:<5}: |W{}|w\n\r", count_num_rooms + 1,
                                            capitalize(to_string(direction)), current_person->short_name()));
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
    auto buffer = fmt::format(format_str, "Num", "Area Name", "Lvnum", "Uvnum", "Filename");
    for (auto &pArea : AreaList::singleton())
        buffer +=
            fmt::format(format_str, pArea->area_num, pArea->areaname, pArea->lvnum, pArea->uvnum, pArea->filename);
    ch->page_to(buffer);
}

/* do_prefix added 19-05-97 PCFN */
void do_prefix(Char *ch, const char *argument) {
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