/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                 */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  interp.c:  the command interpreter                                   */
/*                                                                       */
/*************************************************************************/

#include "interp.h"
#include "AffectFlag.hpp"
#include "Char.hpp"
#include "CommFlag.hpp"
#include "CommandSet.hpp"
#include "Logging.hpp"
#include "Note.hpp"
#include "PlayerActFlag.hpp"
#include "Socials.hpp"
#include "act_comm.hpp"
#include "comm.hpp"
#include "common/BitOps.hpp"
#include "db.h"
#include "handler.hpp"

#include <fmt/format.h>
#include <magic_enum.hpp>

#include <functional>
#include <utility>

namespace {
inline constexpr auto ML = MAX_LEVEL; /* implementor */
inline constexpr auto L1 = MAX_LEVEL - 1; /* creator */
inline constexpr auto L2 = MAX_LEVEL - 2; /* supreme being */
inline constexpr auto L3 = MAX_LEVEL - 3; /* deity */
inline constexpr auto L4 = MAX_LEVEL - 4; /* god */
inline constexpr auto L5 = MAX_LEVEL - 5; /* immortal */
inline constexpr auto L6 = MAX_LEVEL - 6; /* demigod */
inline constexpr auto L7 = MAX_LEVEL - 7; /* angel */
inline constexpr auto L8 = MAX_LEVEL - 8; /* avatar */
inline constexpr auto IM = LEVEL_IMMORTAL; /* angel */
inline constexpr auto HE = LEVEL_HERO; /* hero */
}

enum class CommandLogLevel { Normal, Always, Never };

extern bool fLogAll;

// Function object for commands run by the interpreter, the do_ functions.
using CommandFunc = std::function<void(Char *ch, const char *argument)>;
using CommandFuncNoArgs = std::function<void(Char *ch)>;
using CommandFuncArgParser = std::function<void(Char *ch, ArgParser)>;

struct CommandInfo {
    const char *name;
    CommandFunc do_fun;
    Position::Type position;
    sh_int level;
    CommandLogLevel log;
    bool show;
    CommandInfo(const char *name, CommandFunc do_fun, const Position::Type position, sh_int level, CommandLogLevel log,
                bool show)
        : name(name), do_fun(std::move(do_fun)), position(position), level(level), log(log), show(show) {}
};

static CommandSet<CommandInfo> commands;

static void add_command(const char *name, CommandFunc do_fun, const Position::Type position = Position::Type::Dead,
                        sh_int level = 0, CommandLogLevel log = CommandLogLevel::Normal, bool show = true) {
    commands.add(name, CommandInfo(name, std::move(do_fun), position, level, log, show), level);
}

// Add command with no args.
static void add_command(const char *name, CommandFuncNoArgs do_fun,
                        const Position::Type position = Position::Type::Dead, sh_int level = 0,
                        CommandLogLevel log = CommandLogLevel::Normal, bool show = true) {
    commands.add(name,
                 CommandInfo(
                     name, [f = std::move(do_fun)](Char *ch, const char *) { f(ch); }, position, level, log, show),
                 level);
}

// Add command with no args.
static void add_command(const char *name, CommandFuncArgParser do_fun,
                        const Position::Type position = Position::Type::Dead, sh_int level = 0,
                        CommandLogLevel log = CommandLogLevel::Normal, bool show = true) {
    commands.add(name,
                 CommandInfo(
                     name, [f = std::move(do_fun)](Char *ch, const char *args) { f(ch, ArgParser(args)); }, position,
                     level, log, show),
                 level);
}

void interp_initialise() {
    /* Common movement commands. */
    add_command("north", do_north, Position::Type::Standing, 0, CommandLogLevel::Never, false);
    add_command("east", do_east, Position::Type::Standing, 0, CommandLogLevel::Never, false);
    add_command("south", do_south, Position::Type::Standing, 0, CommandLogLevel::Never, false);
    add_command("west", do_west, Position::Type::Standing, 0, CommandLogLevel::Never, false);
    add_command("up", do_up, Position::Type::Standing, 0, CommandLogLevel::Never, false);
    add_command("down", do_down, Position::Type::Standing, 0, CommandLogLevel::Never, false);

    /* Common other commands.
     * Placed here so one and two letter abbreviations work.
     */
    add_command("at", do_at, Position::Type::Dead, L6);
    add_command("auction", do_auction, Position::Type::Sleeping);
    add_command("buy", do_buy, Position::Type::Resting);
    add_command("cast", do_cast, Position::Type::Fighting);
    add_command("channels", do_channels);
    add_command("exits", do_exits, Position::Type::Resting);
    add_command("get", do_get, Position::Type::Resting);
    add_command("goto", do_goto, Position::Type::Dead, L8);
    add_command("group", do_group, Position::Type::Sleeping);
    add_command("hit", do_kill, Position::Type::Fighting, 0, CommandLogLevel::Normal, false);
    add_command("inventory", do_inventory);
    add_command("kill", do_kill, Position::Type::Fighting);
    add_command("look", do_look, Position::Type::Resting);
    add_command("music", do_music, Position::Type::Sleeping);
    add_command("order", do_order, Position::Type::Resting, 0, CommandLogLevel::Always);
    add_command("peek", do_peek, Position::Type::Resting);
    add_command("practice", do_practice, Position::Type::Sleeping);
    add_command("rest", do_rest, Position::Type::Sleeping);
    add_command("score", do_score);
    add_command("scan", do_scan, Position::Type::Fighting);
    add_command("sit", do_sit, Position::Type::Sleeping);
    add_command("sockets", do_sockets, Position::Type::Dead, L4);
    add_command("stand", do_stand, Position::Type::Sleeping);
    add_command("tell", do_tell);
    add_command("wield", do_wear, Position::Type::Resting);
    add_command("wizhelp", do_wizhelp, Position::Type::Dead, HE);
    add_command("wiznet", do_wiznet, Position::Type::Dead, L8);

    /* Informational commands. */
    add_command("affect", do_affected);
    add_command("areas", do_areas);
    add_command("alist", do_alist, Position::Type::Dead, L8);
    add_command("bug", do_bug);
    add_command("changes", do_changes);
    add_command("commands", do_commands);
    add_command("compare", do_compare, Position::Type::Resting);
    add_command("consider", do_consider, Position::Type::Resting);
    add_command("count", do_count, Position::Type::Sleeping);
    add_command("credits", do_credits);
    add_command("equipment", do_equipment);
    add_command("examine", do_examine, Position::Type::Resting);
    add_command("finger", do_finger);
    add_command("help", do_help);
    add_command("idea", do_idea);
    add_command("info", do_groups, Position::Type::Sleeping);
    add_command("motd", do_motd);
    add_command("read", do_look, Position::Type::Resting);
    add_command("report", do_report, Position::Type::Resting);
    add_command("rules", do_rules);
    add_command("skills", do_skills);
    add_command("socials", do_socials);
    add_command("spells", do_spells);
    add_command("story", do_story);
    add_command("time", do_time);
    add_command("tipwizard", do_tipwizard);
    add_command("tips", do_tipwizard);
    add_command("typo", do_typo);
    add_command("weather", do_weather, Position::Type::Resting);
    add_command("who", do_who);
    add_command("whois", do_whois);
    add_command("wizlist", do_wizlist);
    add_command("worth", do_worth, Position::Type::Sleeping);

    /* Configuration commands. */
    add_command("afk", do_afk);
    add_command("autolist", do_autolist);
    add_command("autoaffect", do_autoaffect);
    add_command("autoassist", do_autoassist);
    add_command("autoexit", do_autoexit);
    add_command("autogold", do_autogold);
    add_command("autoloot", do_autoloot);
    add_command("autopeek", do_autopeek);
    add_command("autosac", do_autosac);
    add_command("autosplit", do_autosplit);
    add_command("brief", do_brief);
    add_command("color", do_colour);
    add_command("colour", do_colour);
    add_command("combine", do_combine);
    add_command("compact", do_compact);
    add_command("description", do_description);
    add_command("delet", do_delet, Position::Type::Dead, 0, CommandLogLevel::Always, false);
    add_command("delete", do_delete, Position::Type::Dead, 0, CommandLogLevel::Always);
    add_command("nofollow", do_nofollow);
    add_command("noloot", do_noloot);
    add_command("nosummon", do_nosummon);
    add_command("outfit", do_outfit, Position::Type::Resting, 0, CommandLogLevel::Always);
    add_command("password", do_password, Position::Type::Dead, 0, CommandLogLevel::Never);
    add_command("prefix", do_prefix);
    add_command("prompt", do_prompt);
    add_command("pronouns", do_pronouns);
    add_command("scroll", do_scroll);
    add_command("showdefence", do_showdefence);
    add_command("showafk", do_showafk);
    add_command("title", do_title);
    add_command("wimpy", do_wimpy);

    /* Communication commands. */
    add_command("allege", do_allege, Position::Type::Sleeping);
    add_command("announce", do_announce, Position::Type::Sleeping);
    add_command("answer", do_answer, Position::Type::Sleeping);
    add_command("emote", do_emote, Position::Type::Resting);
    add_command(".", do_gossip, Position::Type::Sleeping, 0, CommandLogLevel::Normal, false);
    add_command("gossip", do_gossip, Position::Type::Sleeping);
    add_command(",", do_emote, Position::Type::Resting, 0, CommandLogLevel::Normal, false);
    add_command("gratz", do_gratz, Position::Type::Sleeping);
    add_command("gtell", do_gtell);
    add_command(";", do_gtell, Position::Type::Dead, 0, CommandLogLevel::Normal, false);
    add_command("note", do_note, Position::Type::Sleeping);
    add_command("philosophise", do_philosophise, Position::Type::Sleeping);
    add_command("pose", do_pose, Position::Type::Resting);
    add_command("question", do_question, Position::Type::Sleeping);
    add_command("quiet", do_quiet, Position::Type::Sleeping);
    add_command("qwest", do_qwest, Position::Type::Sleeping);
    add_command("reply", do_reply);
    add_command("say", do_say, Position::Type::Resting);
    add_command("'", do_say, Position::Type::Resting, 0, CommandLogLevel::Normal, false);
    add_command("shout", do_shout, Position::Type::Resting, 3);
    add_command("yell", do_yell, Position::Type::Resting);

    /* Object manipulation commands. */
    add_command("brandish", do_brandish, Position::Type::Resting);
    add_command("close", do_close, Position::Type::Resting);
    add_command("donate", do_donate, Position::Type::Resting);
    add_command("drink", do_drink, Position::Type::Resting);
    add_command("drop", do_drop, Position::Type::Resting);
    add_command("eat", do_eat, Position::Type::Resting);
    add_command("fill", do_fill, Position::Type::Resting);
    add_command("pour", do_pour, Position::Type::Resting);
    add_command("give", do_give, Position::Type::Resting);
    /* hailcorpse and shortcut */
    add_command("hailcorpse", do_hailcorpse, Position::Type::Standing);
    add_command("hc", do_hailcorpse, Position::Type::Standing);
    add_command("heal", do_heal, Position::Type::Resting);
    add_command("hold", do_wear, Position::Type::Resting);
    add_command("list", do_list, Position::Type::Resting);
    add_command("lock", do_lock, Position::Type::Resting);
    add_command("open", do_open, Position::Type::Resting);
    add_command("pick", do_pick, Position::Type::Resting);
    add_command("put", do_put, Position::Type::Resting);
    add_command("quaff", do_quaff, Position::Type::Resting);
    add_command("recite", do_recite, Position::Type::Resting);
    add_command("remove", do_remove, Position::Type::Resting);
    add_command("sell", do_sell, Position::Type::Resting);
    add_command("take", do_get, Position::Type::Resting);
    add_command("throw", do_throw, Position::Type::Standing);
    add_command("sacrifice", do_sacrifice, Position::Type::Resting);
    add_command("junk", do_sacrifice, Position::Type::Resting, 0, CommandLogLevel::Normal, false);
    add_command("tap", do_sacrifice, Position::Type::Resting, 0, CommandLogLevel::Normal, false);
    add_command("tras", do_tras, Position::Type::Resting);
    add_command("trash", do_trash, Position::Type::Resting);
    add_command("unlock", do_unlock, Position::Type::Resting);
    add_command("value", do_value, Position::Type::Resting);
    add_command("wear", do_wear, Position::Type::Resting);
    add_command("zap", do_zap, Position::Type::Resting);
    /* Wandera's little baby. */
    add_command("sharpen", do_sharpen, Position::Type::Standing);
    /* Combat commands. */
    add_command("backstab", do_backstab, Position::Type::Standing);
    add_command("bash", do_bash, Position::Type::Fighting);
    add_command("bs", do_backstab, Position::Type::Standing, 0, CommandLogLevel::Normal, false);
    add_command("berserk", do_berserk, Position::Type::Fighting);
    add_command("dirt", do_dirt, Position::Type::Fighting);
    add_command("disarm", do_disarm, Position::Type::Fighting);
    add_command("flee", do_flee, Position::Type::Fighting);
    add_command("kick", do_kick, Position::Type::Fighting);
    /* TheMoog's command. */
    add_command("headbutt", do_headbutt, Position::Type::Fighting);
    /* $-) */
    add_command("murde", do_murde, Position::Type::Fighting, IM, CommandLogLevel::Normal, false);
    add_command("murder", do_murder, Position::Type::Fighting, IM, CommandLogLevel::Always);
    add_command("rescue", do_rescue, Position::Type::Fighting, 0, CommandLogLevel::Normal, false);
    add_command("trip", do_trip, Position::Type::Fighting);

    /* Miscellaneous commands. */
    /* Rohan's bit of wizadry! */
    add_command("accept", do_accept, Position::Type::Standing);
    add_command("cancel", do_cancel_chal, Position::Type::Standing);
    add_command("challenge", do_challenge, Position::Type::Standing);
    add_command("duel", do_duel, Position::Type::Standing);
    add_command("follow", do_follow, Position::Type::Resting);
    add_command("gain", do_gain, Position::Type::Standing);
    add_command("groups", do_groups, Position::Type::Sleeping);
    add_command("hide", do_hide, Position::Type::Resting);
    add_command("enter", do_enter, Position::Type::Standing, 0, CommandLogLevel::Never, false);
    add_command("qui", do_qui, Position::Type::Dead, 0, CommandLogLevel::Normal, false);
    add_command("quit", do_quit);
    add_command("ready", do_ready, Position::Type::Standing);
    add_command("recall", do_recall, Position::Type::Fighting);
    add_command("refuse", do_refuse, Position::Type::Standing);
    add_command("/", do_recall, Position::Type::Fighting, 0, CommandLogLevel::Normal, false);
    add_command("save", do_save);
    add_command("sleep", do_sleep, Position::Type::Sleeping);
    add_command("sneak", do_sneak, Position::Type::Standing);
    add_command("split", do_split, Position::Type::Resting);
    add_command("steal", do_steal, Position::Type::Standing);
    add_command("train", do_train, Position::Type::Resting);
    add_command("visible", do_visible, Position::Type::Sleeping);
    add_command("wake", do_wake, Position::Type::Sleeping);
    add_command("where", do_where, Position::Type::Resting);

    /* Riding skill */
    add_command("ride", do_ride, Position::Type::Fighting);
    add_command("mount", do_ride, Position::Type::Fighting);
    add_command("dismount", do_dismount, Position::Type::Fighting);

    /* Clan commands */
    add_command("member", do_member, Position::Type::Standing, 0, CommandLogLevel::Always);
    add_command("promote", do_promote, Position::Type::Standing, 0, CommandLogLevel::Always);
    add_command("demote", do_demote, Position::Type::Standing, 0, CommandLogLevel::Always);
    add_command("noclanchan", do_noclanchan, Position::Type::Sleeping, 0, CommandLogLevel::Always);
    add_command("clanwho", do_clanwho, Position::Type::Sleeping);
    add_command("clantalk", do_clantalk, Position::Type::Sleeping);
    add_command(">", do_clantalk, Position::Type::Sleeping);

    /* clan power for immortals .... */
    add_command("clanset", do_clanset, Position::Type::Sleeping, L4, CommandLogLevel::Always);

    /* Immortal commands. */
    add_command("advance", do_advance, Position::Type::Dead, ML, CommandLogLevel::Always);
    add_command("dump", do_dump, Position::Type::Dead, ML, CommandLogLevel::Always, false);
    add_command("trust", do_trust, Position::Type::Dead, ML, CommandLogLevel::Always);
    add_command("sacname", do_sacname, Position::Type::Dead, ML, CommandLogLevel::Always);
    add_command("allow", do_allow, Position::Type::Dead, L2, CommandLogLevel::Always);
    add_command("ban", do_ban, Position::Type::Dead, L2, CommandLogLevel::Always);
    add_command("permban", do_permban, Position::Type::Dead, L2, CommandLogLevel::Always);
    add_command("permit", do_permit, Position::Type::Dead, L2, CommandLogLevel::Always);
    add_command("deny", do_deny, Position::Type::Dead, L1, CommandLogLevel::Always);
    add_command("disconnect", do_disconnect, Position::Type::Dead, L3, CommandLogLevel::Always);
    add_command("freeze", do_freeze, Position::Type::Dead, L3, CommandLogLevel::Always);
    add_command("reboo", do_reboo, Position::Type::Dead, L1, CommandLogLevel::Normal, false);
    add_command("reboot", do_reboot, Position::Type::Dead, L1, CommandLogLevel::Always);
    add_command("set", do_set, Position::Type::Dead, L2, CommandLogLevel::Always);
    add_command("setinfo", do_setinfo);
    add_command("shutdow", do_shutdow, Position::Type::Dead, L1, CommandLogLevel::Normal, false);
    add_command("shutdown", do_shutdown, Position::Type::Dead, L1, CommandLogLevel::Always);
    add_command("wizlock", do_wizlock, Position::Type::Dead, L2, CommandLogLevel::Always);
    add_command("force", do_force, Position::Type::Dead, L7, CommandLogLevel::Always);
    add_command("load", do_load, Position::Type::Dead, L4, CommandLogLevel::Always);
    add_command("newlock", do_newlock, Position::Type::Dead, L4, CommandLogLevel::Always);
    add_command("nochannels", do_nochannels, Position::Type::Dead, L6, CommandLogLevel::Always);
    add_command("noemote", do_noemote, Position::Type::Dead, L6, CommandLogLevel::Always);
    add_command("notell", do_notell, Position::Type::Dead, L6, CommandLogLevel::Always);
    add_command("pecho", do_pecho, Position::Type::Dead, L4, CommandLogLevel::Always);
    add_command("pardon", do_pardon, Position::Type::Dead, L3, CommandLogLevel::Always);
    add_command("purge", do_purge, Position::Type::Dead, L4, CommandLogLevel::Always);
    add_command("restore", do_restore, Position::Type::Dead, L4, CommandLogLevel::Always);
    add_command("sla", do_sla, Position::Type::Dead, L3, CommandLogLevel::Normal, false);
    add_command("slay", do_slay, Position::Type::Dead, L3, CommandLogLevel::Always);
    add_command("teleport", do_transfer, Position::Type::Dead, L5, CommandLogLevel::Always);
    add_command("transfer", do_transfer, Position::Type::Dead, L5, CommandLogLevel::Always);
    add_command("poofin", do_bamfin, Position::Type::Dead, L8);
    add_command("poofout", do_bamfout, Position::Type::Dead, L8);
    add_command("gecho", do_echo, Position::Type::Dead, L4, CommandLogLevel::Always);
    add_command("holylight", do_holylight, Position::Type::Dead, IM);
    add_command("invis", do_invis, Position::Type::Dead, IM, CommandLogLevel::Normal, false);
    add_command("log", do_log, Position::Type::Dead, L1, CommandLogLevel::Always);
    add_command("memory", do_memory, Position::Type::Dead, IM);
    add_command("mwhere", do_mwhere, Position::Type::Dead, IM);
    add_command("peace", do_peace, Position::Type::Dead, L5);
    /* Death's commands */
    add_command("coma", do_coma, Position::Type::Dead, IM);
    add_command("owhere", do_owhere, Position::Type::Dead, IM);
    /* =)) */
    add_command("osearch", do_osearch, Position::Type::Dead, IM);
    add_command("awaken", do_awaken, Position::Type::Dead, IM);
    add_command("echo", do_recho, Position::Type::Dead, L6, CommandLogLevel::Always);
    add_command("return", do_return, Position::Type::Dead, L6);
    add_command("smit", do_smit, Position::Type::Standing, L7);
    add_command("smite", do_smite, Position::Type::Standing, L7);
    add_command("snoop", do_snoop, Position::Type::Dead, L5, CommandLogLevel::Always);
    add_command("stat", do_stat, Position::Type::Dead, IM);
    add_command("string", do_string, Position::Type::Dead, L5, CommandLogLevel::Always);
    add_command("switch", do_switch, Position::Type::Dead, L6, CommandLogLevel::Always);
    add_command("wizinvis", do_invis, Position::Type::Dead, IM);
    add_command("zecho", do_zecho, Position::Type::Dead, L6, CommandLogLevel::Always);
    add_command("prowl", do_prowl, Position::Type::Dead, IM);
    add_command("vnum", do_vnum, Position::Type::Dead, L4);
    add_command("clone", do_clone, Position::Type::Dead, L5, CommandLogLevel::Always);
    add_command("immworth", do_immworth, Position::Type::Dead, L3);
    add_command("immtalk", do_immtalk, Position::Type::Dead, IM);
    add_command("imotd", do_imotd, Position::Type::Dead, IM);
    add_command(":", do_immtalk, Position::Type::Dead, IM, CommandLogLevel::Normal, false);

    /* MOBprogram commands. */
    add_command("mpasound", do_mpasound, Position::Type::Dead, MAX_LEVEL_MPROG, CommandLogLevel::Normal, false);
    add_command("mpjunk", do_mpjunk, Position::Type::Dead, MAX_LEVEL_MPROG, CommandLogLevel::Normal, false);
    add_command("mpecho", do_mpecho, Position::Type::Dead, MAX_LEVEL_MPROG, CommandLogLevel::Normal, false);
    add_command("mpechoat", do_mpechoat, Position::Type::Dead, MAX_LEVEL_MPROG, CommandLogLevel::Normal, false);
    add_command("mpechoaround", do_mpechoaround, Position::Type::Dead, MAX_LEVEL_MPROG, CommandLogLevel::Normal, false);
    add_command("mpkill", do_mpkill, Position::Type::Dead, MAX_LEVEL_MPROG, CommandLogLevel::Normal, false);
    add_command("mpmload", do_mpmload, Position::Type::Dead, MAX_LEVEL_MPROG, CommandLogLevel::Normal, false);
    add_command("mpoload", do_mpoload, Position::Type::Dead, MAX_LEVEL_MPROG, CommandLogLevel::Normal, false);
    add_command("mppurge", do_mppurge, Position::Type::Dead, MAX_LEVEL_MPROG, CommandLogLevel::Normal, false);
    add_command("mpgoto", do_mpgoto, Position::Type::Dead, MAX_LEVEL_MPROG, CommandLogLevel::Normal, false);
    add_command("mpat", do_mpat, Position::Type::Dead, MAX_LEVEL_MPROG, CommandLogLevel::Normal, false);
    add_command("mptransfer", do_mptransfer, Position::Type::Dead, MAX_LEVEL_MPROG, CommandLogLevel::Normal, false);
    add_command("mpforce", do_mpforce, Position::Type::Dead, MAX_LEVEL_MPROG, CommandLogLevel::Normal, false);
}

std::string apply_prefix(Char *ch, const char *command) {
    // Unswitched MOBs don't have prefixes.  If we're switched, get the player's prefix.
    auto player = ch->player();
    if (!player)
        return command;

    if (0 == strcmp(command, "prefix")) {
        return command;
    } else {
        auto &pc_data = player->pcdata;
        if (command[0] == '\\') {
            if (command[1] == '\\') {
                ch->send_to(!pc_data->prefix.empty() ? "(prefix removed)\n\r" : "(no prefix to remove)\n\r");
                pc_data->prefix.clear();
                command++; /* skip the \ */
            }
            command++; /* skip the \ */
            return command;
        } else {
            return fmt::format("{}{}", pc_data->prefix.c_str(), command);
        }
    }
}

/*
 * The main entry point for executing commands.
 * Can be recursively called from 'at', 'order', 'force'.
 */
void interpret(Char *ch, const char *argument) {
    const auto command_line = ltrim_copy(apply_prefix(ch, argument));
    if (command_line.empty()) {
        return;
    }
    /* No hiding. */
    clear_enum_bit(ch->affected_by, AffectFlag::Hide);

    /* Implement freeze command. */
    if (ch->is_pc() && check_enum_bit(ch->act, PlayerActFlag::PlrFreeze)) {
        ch->send_line("You're totally frozen!");
        return;
    }

    auto arg_parser = ArgParser(command_line);
    // Grab the command word.
    // Special parsing so ' can be a command,
    const auto command = arg_parser.commandline_shift();
    const std::string remainder(arg_parser.remaining());
    auto cmd_info = commands.get(command, ch->get_trust());
    /* Look for command in socials table. */
    if (!cmd_info.has_value()) {
        if (!check_social(ch, command, remainder))
            ch->send_line("Huh?");
        // Return before logging. This is to prevent accidentally logging a typo'd "never log" command.
        return;
    }
    /* Log and snoop. */
    if (cmd_info->log != CommandLogLevel::Never) {
        if ((ch->is_pc() && check_enum_bit(ch->act, PlayerActFlag::PlrLog)) || fLogAll
            || cmd_info->log == CommandLogLevel::Always) {
            int level = (cmd_info->level >= LEVEL_IMMORTAL) ? (cmd_info->level) : 0;
            if (ch->is_pc()
                && (check_enum_bit(ch->act, PlayerActFlag::PlrWizInvis)
                    || check_enum_bit(ch->act, PlayerActFlag::PlrProwl)))
                level = std::max(level, ch->get_trust());
            auto log_level = (cmd_info->level >= LEVEL_IMMORTAL) ? CharExtraFlag::WiznetImm : CharExtraFlag::WiznetMort;
            if (ch->is_npc() && ch->desc && ch->desc->original()) {
                log_new(fmt::format("Log {} (as '{}'): {}", ch->desc->original()->name, ch->name, command_line),
                        log_level, level);
            } else {
                log_new(fmt::format("Log {}: {}", ch->name, command_line), log_level, level);
            }
        }
        if (ch->desc)
            ch->desc->note_input(ch->name, command_line);
    }

    /* Character not in position for command? */
    if (ch->position < cmd_info->position) {
        ch->send_line(ch->position.bad_position_msg());
        return;
    }

    /* Dispatch the command. */
    {
        // TODO #263: because do_fun expects a mutable char* (at least right now), make argument mutable in this
        // dreadful way. Eventually we'll pass a std::string or similar through and this will all go away.
        char mutable_argument[MAX_INPUT_LENGTH];
        strcpy(mutable_argument, remainder.c_str());
        cmd_info->do_fun(ch, mutable_argument);
    }
}

bool check_social(Char *ch, std::string_view command, std::string_view argument) {
    const auto social = Socials::singleton().find(command);
    if (!social)
        return false;

    if (ch->is_pc() && check_enum_bit(ch->comm, CommFlag::NoEmote)) {
        ch->send_line("You are anti-social!");
        return true;
    }

    if ((ch->is_pos_stunned_or_dying()) || (ch->is_pos_sleeping() && !matches(social->name_, "snore"))) {
        ch->send_line(ch->position.bad_position_msg());
        return true;
    }

    ArgParser args(argument);
    Char *victim = nullptr;
    if (args.empty()) {
        act(social->others_no_arg_, ch, nullptr, victim, To::Room);
        act(social->char_no_arg_, ch, nullptr, victim, To::Char);
    } else if ((victim = get_char_room(ch, args.shift())) == nullptr) {
        ch->send_line("They aren't here.");
    } else if (victim == ch) {
        act(social->others_auto_, ch, nullptr, victim, To::Room);
        act(social->char_auto_, ch, nullptr, victim, To::Char);
    } else {
        act(social->others_found_, ch, nullptr, victim, To::NotVict);
        act(social->char_found_, ch, nullptr, victim, To::Char);
        act(social->vict_found_, ch, nullptr, victim, To::Vict);

        if (ch->is_pc() && victim->is_npc() && !victim->is_aff_charm() && victim->is_pos_awake()
            && victim->desc == nullptr) {
            switch (number_bits(4)) {
            case 0:
            case 1:
            case 2:
            case 3:
            case 4:
            case 5:
            case 6:
            case 7:
            case 8:
                act(social->others_found_, victim, nullptr, ch, To::NotVict);
                act(social->char_found_, victim, nullptr, ch, To::Char);
                act(social->vict_found_, victim, nullptr, ch, To::Vict);
                break;

            case 9:
            case 10:
            case 11:
            case 12:
                act("$n slaps $N.", victim, nullptr, ch, To::NotVict);
                act("You slap $N.", victim, nullptr, ch, To::Char);
                act("$n slaps you.", victim, nullptr, ch, To::Vict);
                break;
            }
        }
    }

    return true;
}

/*
 * Pick off one argument from a string and return the rest.
 * Understands quotes.
 */
const char *one_argument(const char *argument, char *arg_first) {
    while (isspace(*argument))
        argument++;

    char cEnd = ' ';
    if (*argument == '\'' || *argument == '"')
        cEnd = *argument++;

    while ((*argument != '\0') && (*argument != '\n')) {
        if (*argument == cEnd) {
            argument++;
            break;
        }
        *arg_first = tolower(*argument);
        arg_first++;
        argument++;
    }
    *arg_first = '\0';

    while (isspace(*argument))
        argument++;

    return argument;
}
// TODO(MRG) this duplication is beyond heinous. BUT I want to ensure no new functionality but try and move towards
// const-correctness and this seems the easiest path.
char *one_argument(char *argument, char *arg_first) {
    while (isspace(*argument))
        argument++;

    char cEnd = ' ';
    if (*argument == '\'' || *argument == '"')
        cEnd = *argument++;

    while ((*argument != '\0') && (*argument != '\n')) {
        if (*argument == cEnd) {
            argument++;
            break;
        }
        *arg_first = tolower(*argument);
        arg_first++;
        argument++;
    }
    *arg_first = '\0';

    while (isspace(*argument))
        argument++;

    return argument;
}

struct Columniser {
public:
    explicit Columniser(Char *ch) : ch(ch) { buf[0] = '\0'; }

    std::function<void(const std::string &name, CommandInfo info, int level)> visitor() {
        return [this](const std::string &name, CommandInfo info, int level) {
            (void)level;
            visit_command(name, info);
        };
    }

    void visit_command(const std::string &name, CommandInfo info) {
        int buf_len = strlen(buf);
        int name_len = name.size();
        int start_pos;

        if (!info.show)
            return;

        if (buf[0] == '\0')
            start_pos = 0;
        else
            start_pos = ((buf_len + col_width) / col_width) * col_width;

        if (start_pos + name_len > max_width) {
            strcat(buf, "\n\r");
            ch->send_to(buf);
            buf[0] = '\0';
            start_pos = buf_len = 0;
        }

        if (start_pos > buf_len)
            memset(buf + buf_len, ' ', start_pos - buf_len);
        memcpy(buf + start_pos, name.c_str(), name_len + 1);
    }

    Char *ch;
    char buf[MAX_STRING_LENGTH];
    int col_width = 12;
    int max_width = 72;
};

void do_commands(Char *ch) {
    Columniser col(ch);
    auto max_level = (ch->get_trust() < LEVEL_HERO) ? ch->get_trust() : (LEVEL_HERO - 1);
    commands.enumerate(commands.level_restrict(0, max_level, col.visitor()));
    if (col.buf[0] != '\0') {
        strcat(col.buf, "\n\r");
        ch->send_to(col.buf);
    }
}

void do_wizhelp(Char *ch) {
    Columniser col(ch);
    commands.enumerate(commands.level_restrict(LEVEL_HERO, ch->get_trust(), col.visitor()));
    if (col.buf[0] != '\0') {
        strcat(col.buf, "\n\r");
        ch->send_to(col.buf);
    }
}
