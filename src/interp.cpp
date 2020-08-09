/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                 */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  interp.c:  the command interpreter                                   */
/*                                                                       */
/*************************************************************************/

#include "interp.h"
#include "CommandSet.hpp"
#include "Descriptor.hpp"
#include "merc.h"
#include "note.h"

#include <fmt/format.h>

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <utility>

using namespace fmt::literals;

/* Merc-2.2 MOBProgs - Faramir 31/8/1998 */
bool MP_Commands(CHAR_DATA *ch);

enum class CommandLogLevel { Normal, Always, Never };

/*
 * Log-all switch.
 */
bool fLogAll = false;

static const char *bad_position_string[] = {"Lie still; you are DEAD.\n\r",
                                            "You are hurt far too bad for that.\n\r",
                                            "You are hurt far too bad for that.\n\r",
                                            "You are too stunned to do that.\n\r",
                                            "In your dreams, or what?\n\r",
                                            "You're resting. Try standing up first!\n\r",
                                            "Better stand up first.\n\r",
                                            "No way!  You are still fighting!\n\r",
                                            "You're standing.\n\r"};

// Function object for commands run by the interpreter, the do_ functions.
// argument is modified (currently) by some of the text processing routines. TODO make into a const.
using CommandFunc = std::function<void(CHAR_DATA *ch, char *argument)>;

struct CommandInfo {
    const char *name;
    CommandFunc do_fun;
    sh_int position;
    sh_int level;
    CommandLogLevel log;
    bool show;
    CommandInfo(const char *name, CommandFunc do_fun, sh_int position, sh_int level, CommandLogLevel log, bool show)
        : name(name), do_fun(std::move(do_fun)), position(position), level(level), log(log), show(show) {}
};

static CommandSet<CommandInfo> commands;

static void add_command(const char *name, CommandFunc do_fun, sh_int position = POS_DEAD, sh_int level = 0,
                        CommandLogLevel log = CommandLogLevel::Normal, bool show = true) {
    commands.add(name, CommandInfo(name, std::move(do_fun), position, level, log, show), level);
}

void interp_initialise() {
    /* Common movement commands. */
    add_command("north", do_north, POS_STANDING, 0, CommandLogLevel::Never, false);
    add_command("east", do_east, POS_STANDING, 0, CommandLogLevel::Never, false);
    add_command("south", do_south, POS_STANDING, 0, CommandLogLevel::Never, false);
    add_command("west", do_west, POS_STANDING, 0, CommandLogLevel::Never, false);
    add_command("up", do_up, POS_STANDING, 0, CommandLogLevel::Never, false);
    add_command("down", do_down, POS_STANDING, 0, CommandLogLevel::Never, false);

    /* Common other commands.
     * Placed here so one and two letter abbreviations work.
     */
    add_command("at", do_at, POS_DEAD, L6);
    add_command("auction", do_auction, POS_SLEEPING);
    add_command("buy", do_buy, POS_RESTING);
    add_command("cast", do_cast, POS_FIGHTING);
    add_command("channels", do_channels);
    add_command("exits", do_exits, POS_RESTING);
    add_command("get", do_get, POS_RESTING);
    add_command("goto", do_goto, POS_DEAD, L8);
    add_command("group", do_group, POS_SLEEPING);
    add_command("hit", do_kill, POS_FIGHTING, 0, CommandLogLevel::Normal, false);
    add_command("inventory", do_inventory);
    add_command("kill", do_kill, POS_FIGHTING);
    add_command("look", do_look, POS_RESTING);
    add_command("music", do_music, POS_SLEEPING);
    add_command("order", do_order, POS_RESTING, 0, CommandLogLevel::Always);
    add_command("peek", do_peek, POS_RESTING);
    add_command("practice", do_practice, POS_SLEEPING);
    add_command("rest", do_rest, POS_SLEEPING);
    add_command("score", do_score);
    add_command("scan", do_scan, POS_FIGHTING);
    add_command("sit", do_sit, POS_SLEEPING);
    add_command("sockets", do_sockets, POS_DEAD, L4);
    add_command("stand", do_stand, POS_SLEEPING);
    add_command("tell", do_tell);
    add_command("wield", do_wear, POS_RESTING);
    add_command("wizhelp", do_wizhelp, POS_DEAD, HE);
    add_command("wiznet", do_wiznet, POS_DEAD, L8);

    /* Informational commands. */
    add_command("affect", do_affected);
    add_command("areas", do_areas);
    add_command("alist", do_alist, POS_DEAD, L8);
    add_command("bug", do_bug);
    add_command("changes", do_changes);
    add_command("commands", do_commands);
    add_command("compare", do_compare, POS_RESTING);
    add_command("consider", do_consider, POS_RESTING);
    add_command("count", do_count, POS_SLEEPING);
    add_command("credits", do_credits);
    add_command("equipment", do_equipment);
    add_command("examine", do_examine, POS_RESTING);
    add_command("finger", do_finger);
    add_command("help", do_help);
    add_command("idea", do_idea);
    add_command("info", do_groups, POS_SLEEPING);
    add_command("motd", do_motd);
    add_command("read", do_read, POS_RESTING);
    add_command("report", do_report, POS_RESTING);
    add_command("rules", do_rules);
    add_command("skills", do_skills);
    add_command("socials", do_socials);
    add_command("spells", do_spells);
    add_command("story", do_story);
    add_command("time", do_time);
    add_command("timezone", do_timezone);
    add_command("tipwizard", do_tipwizard);
    add_command("tips", do_tipwizard);
    add_command("typo", do_typo);
    add_command("weather", do_weather, POS_RESTING);
    add_command("who", do_who);
    add_command("whois", do_whois);
    add_command("wizlist", do_wizlist);
    add_command("worth", do_worth, POS_SLEEPING);

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
    add_command("delet", do_delet, POS_DEAD, 0, CommandLogLevel::Always, false);
    add_command("delete", do_delete, POS_DEAD, 0, CommandLogLevel::Always);
    add_command("nofollow", do_nofollow);
    add_command("noloot", do_noloot);
    add_command("nosummon", do_nosummon);
    add_command("outfit", do_outfit, POS_RESTING, 0, CommandLogLevel::Always);
    add_command("password", do_password, POS_DEAD, 0, CommandLogLevel::Never);
    add_command("prefix", do_prefix);
    add_command("prompt", do_prompt);
    add_command("scroll", do_scroll);
    add_command("showdefence", do_showdefence);
    add_command("showafk", do_showafk);
    add_command("title", do_title);
    add_command("wimpy", do_wimpy);

    /* Communication commands. */
    add_command("allege", do_allege, POS_SLEEPING);
    add_command("announce", do_announce, POS_SLEEPING);
    add_command("answer", do_answer, POS_SLEEPING);
    add_command("emote", do_emote, POS_RESTING);
    add_command(".", do_gossip, POS_SLEEPING, 0, CommandLogLevel::Normal, false);
    add_command("gossip", do_gossip, POS_SLEEPING);
    add_command(",", do_emote, POS_RESTING, 0, CommandLogLevel::Normal, false);
    add_command("gratz", do_gratz, POS_SLEEPING);
    add_command("gtell", do_gtell);
    add_command(";", do_gtell, POS_DEAD, 0, CommandLogLevel::Normal, false);
    add_command("note", do_note, POS_SLEEPING);
    add_command("news", do_news, POS_SLEEPING);
    add_command("mail", do_mail, POS_SLEEPING);
    add_command("philosophise", do_philosophise, POS_SLEEPING);
    add_command("pose", do_pose, POS_RESTING);
    add_command("question", do_question, POS_SLEEPING);
    add_command("quiet", do_quiet, POS_SLEEPING);
    add_command("qwest", do_qwest, POS_SLEEPING);
    add_command("reply", do_reply);
    add_command("say", do_say, POS_RESTING);
    add_command("'", do_say, POS_RESTING, 0, CommandLogLevel::Normal, false);
    add_command("shout", do_shout, POS_RESTING, 3);
    add_command("yell", do_yell, POS_RESTING);

    /* Object manipulation commands. */
    add_command("brandish", do_brandish, POS_RESTING);
    add_command("close", do_close, POS_RESTING);
    add_command("donate", do_donate, POS_RESTING);
    add_command("drink", do_drink, POS_RESTING);
    add_command("drop", do_drop, POS_RESTING);
    add_command("eat", do_eat, POS_RESTING);
    add_command("fill", do_fill, POS_RESTING);
    add_command("pour", do_pour, POS_RESTING);
    add_command("give", do_give, POS_RESTING);
    /* hailcorpse and shortcut */
    add_command("hailcorpse", do_hailcorpse, POS_STANDING);
    add_command("hc", do_hailcorpse, POS_STANDING);
    add_command("heal", do_heal, POS_RESTING);
    add_command("hold", do_wear, POS_RESTING);
    add_command("list", do_list, POS_RESTING);
    add_command("lock", do_lock, POS_RESTING);
    add_command("open", do_open, POS_RESTING);
    add_command("pick", do_pick, POS_RESTING);
    add_command("put", do_put, POS_RESTING);
    add_command("quaff", do_quaff, POS_RESTING);
    add_command("recite", do_recite, POS_RESTING);
    add_command("remove", do_remove, POS_RESTING);
    add_command("sell", do_sell, POS_RESTING);
    add_command("take", do_get, POS_RESTING);
    add_command("throw", do_throw, POS_STANDING);
    add_command("sacrifice", do_sacrifice, POS_RESTING);
    add_command("junk", do_sacrifice, POS_RESTING, 0, CommandLogLevel::Normal, false);
    add_command("tap", do_sacrifice, POS_RESTING, 0, CommandLogLevel::Normal, false);
    add_command("unlock", do_unlock, POS_RESTING);
    add_command("value", do_value, POS_RESTING);
    add_command("wear", do_wear, POS_RESTING);
    add_command("zap", do_zap, POS_RESTING);
    /* Wandera's little baby. */
    add_command("sharpen", do_sharpen, POS_STANDING);
    /* Combat commands. */
    add_command("backstab", do_backstab, POS_STANDING);
    add_command("bash", do_bash, POS_FIGHTING);
    add_command("bs", do_backstab, POS_STANDING, 0, CommandLogLevel::Normal, false);
    add_command("berserk", do_berserk, POS_FIGHTING);
    add_command("dirt", do_dirt, POS_FIGHTING);
    add_command("disarm", do_disarm, POS_FIGHTING);
    add_command("flee", do_flee, POS_FIGHTING);
    add_command("kick", do_kick, POS_FIGHTING);
    /* TheMoog's command. */
    add_command("headbutt", do_headbutt, POS_FIGHTING);
    /* $-) */
    add_command("murde", do_murde, POS_FIGHTING, IM, CommandLogLevel::Normal, false);
    add_command("murder", do_murder, POS_FIGHTING, IM, CommandLogLevel::Always);
    add_command("rescue", do_rescue, POS_FIGHTING, 0, CommandLogLevel::Normal, false);
    add_command("trip", do_trip, POS_FIGHTING);

    /* Miscellaneous commands. */
    /* Rohan's bit of wizadry! */
    add_command("accept", do_accept, POS_STANDING);
    add_command("cancel", do_cancel_chal, POS_STANDING);
    add_command("challenge", do_challenge, POS_STANDING);
    add_command("duel", do_duel, POS_STANDING);
    add_command("follow", do_follow, POS_RESTING);
    add_command("gain", do_gain, POS_STANDING);
    add_command("groups", do_groups, POS_SLEEPING);
    add_command("hide", do_hide, POS_RESTING);
    add_command("enter", do_enter, POS_STANDING, 0, CommandLogLevel::Never, false);
    add_command("qui", do_qui, POS_DEAD, 0, CommandLogLevel::Normal, false);
    add_command("quit", do_quit);
    add_command("ready", do_ready, POS_STANDING);
    add_command("recall", do_recall, POS_FIGHTING);
    add_command("refuse", do_refuse, POS_STANDING);
    add_command("/", do_recall, POS_FIGHTING, 0, CommandLogLevel::Normal, false);
    add_command("save", do_save);
    add_command("sleep", do_sleep, POS_SLEEPING);
    add_command("sneak", do_sneak, POS_STANDING);
    add_command("split", do_split, POS_RESTING);
    add_command("steal", do_steal, POS_STANDING);
    add_command("train", do_train, POS_RESTING);
    add_command("visible", do_visible, POS_SLEEPING);
    add_command("wake", do_wake, POS_SLEEPING);
    add_command("where", do_where, POS_RESTING);

    /* Riding skill */
    add_command("ride", do_ride, POS_FIGHTING);
    add_command("mount", do_ride, POS_FIGHTING);
    add_command("dismount", do_dismount, POS_FIGHTING);

    /* Clan commands */
    add_command("member", do_member, POS_STANDING, 0, CommandLogLevel::Always);
    add_command("promote", do_promote, POS_STANDING, 0, CommandLogLevel::Always);
    add_command("demote", do_demote, POS_STANDING, 0, CommandLogLevel::Always);
    add_command("noclanchan", do_noclanchan, POS_SLEEPING, 0, CommandLogLevel::Always);
    add_command("clanwho", do_clanwho, POS_SLEEPING);
    add_command("clantalk", do_clantalk, POS_SLEEPING);
    add_command(">", do_clantalk, POS_SLEEPING);

    /* clan power for immortals .... */
    add_command("clanset", do_clanset, POS_SLEEPING, L4, CommandLogLevel::Always);

    /* Immortal commands. */
    add_command("advance", do_advance, POS_DEAD, ML, CommandLogLevel::Always);
    add_command("dump", do_dump, POS_DEAD, ML, CommandLogLevel::Always, false);
    add_command("trust", do_trust, POS_DEAD, ML, CommandLogLevel::Always);
    add_command("sacname", do_sacname, POS_DEAD, ML, CommandLogLevel::Always);
    add_command("allow", do_allow, POS_DEAD, L2, CommandLogLevel::Always);
    add_command("ban", do_ban, POS_DEAD, L2, CommandLogLevel::Always);
    add_command("permban", do_permban, POS_DEAD, L2, CommandLogLevel::Always);
    add_command("permit", do_permit, POS_DEAD, L2, CommandLogLevel::Always);
    add_command("deny", do_deny, POS_DEAD, L1, CommandLogLevel::Always);
    add_command("disconnect", do_disconnect, POS_DEAD, L3, CommandLogLevel::Always);
    add_command("freeze", do_freeze, POS_DEAD, L3, CommandLogLevel::Always);
    add_command("reboo", do_reboo, POS_DEAD, L1, CommandLogLevel::Normal, false);
    add_command("reboot", do_reboot, POS_DEAD, L1, CommandLogLevel::Always);
    add_command("set", do_set, POS_DEAD, L2, CommandLogLevel::Always);
    add_command("setinfo", do_setinfo);
    add_command("shutdow", do_shutdow, POS_DEAD, L1, CommandLogLevel::Normal, false);
    add_command("shutdown", do_shutdown, POS_DEAD, L1, CommandLogLevel::Always);
    add_command("wizlock", do_wizlock, POS_DEAD, L2, CommandLogLevel::Always);
    add_command("force", do_force, POS_DEAD, L7, CommandLogLevel::Always);
    add_command("load", do_load, POS_DEAD, L4, CommandLogLevel::Always);
    add_command("newlock", do_newlock, POS_DEAD, L4, CommandLogLevel::Always);
    add_command("nochannels", do_nochannels, POS_DEAD, L6, CommandLogLevel::Always);
    add_command("noemote", do_noemote, POS_DEAD, L6, CommandLogLevel::Always);
    add_command("notell", do_notell, POS_DEAD, L6, CommandLogLevel::Always);
    add_command("pecho", do_pecho, POS_DEAD, L4, CommandLogLevel::Always);
    add_command("pardon", do_pardon, POS_DEAD, L3, CommandLogLevel::Always);
    add_command("purge", do_purge, POS_DEAD, L4, CommandLogLevel::Always);
    add_command("restore", do_restore, POS_DEAD, L4, CommandLogLevel::Always);
    add_command("sla", do_sla, POS_DEAD, L3, CommandLogLevel::Normal, false);
    add_command("slay", do_slay, POS_DEAD, L3, CommandLogLevel::Always);
    add_command("teleport", do_transfer, POS_DEAD, L5, CommandLogLevel::Always);
    add_command("transfer", do_transfer, POS_DEAD, L5, CommandLogLevel::Always);
    add_command("poofin", do_bamfin, POS_DEAD, L8);
    add_command("poofout", do_bamfout, POS_DEAD, L8);
    add_command("gecho", do_echo, POS_DEAD, L4, CommandLogLevel::Always);
    add_command("holylight", do_holylight, POS_DEAD, IM);
    add_command("invis", do_invis, POS_DEAD, IM, CommandLogLevel::Normal, false);
    add_command("log", do_log, POS_DEAD, L1, CommandLogLevel::Always);
    add_command("memory", do_memory, POS_DEAD, IM);
    add_command("mwhere", do_mwhere, POS_DEAD, IM);
    add_command("peace", do_peace, POS_DEAD, L5);
    /* Death's commands */
    add_command("coma", do_coma, POS_DEAD, IM);
    add_command("owhere", do_owhere, POS_DEAD, IM);
    /* =)) */
    add_command("osearch", do_osearch, POS_DEAD, IM);
    add_command("awaken", do_awaken, POS_DEAD, IM);
    add_command("echo", do_recho, POS_DEAD, L6, CommandLogLevel::Always);
    add_command("return", do_return, POS_DEAD, L6);
    add_command("smit", do_smit, POS_STANDING, L7);
    add_command("smite", do_smite, POS_STANDING, L7);
    add_command("snoop", do_snoop, POS_DEAD, L5, CommandLogLevel::Always);
    add_command("stat", do_stat, POS_DEAD, IM);
    add_command("string", do_string, POS_DEAD, L5, CommandLogLevel::Always);
    add_command("switch", do_switch, POS_DEAD, L6, CommandLogLevel::Always);
    add_command("wizinvis", do_invis, POS_DEAD, IM);
    add_command("zecho", do_zecho, POS_DEAD, L6, CommandLogLevel::Always);
    add_command("prowl", do_prowl, POS_DEAD, IM);
    add_command("vnum", do_vnum, POS_DEAD, L4);
    add_command("clone", do_clone, POS_DEAD, L5, CommandLogLevel::Always);
    add_command("immworth", do_immworth, POS_DEAD, L3);
    add_command("immtalk", do_immtalk, POS_DEAD, IM);
    add_command("imotd", do_imotd, POS_DEAD, IM);
    add_command(":", do_immtalk, POS_DEAD, IM, CommandLogLevel::Normal, false);

    /* Merc-2.2 MOBProgs - Faramir 31/8/1998 */
    /* MOBprogram commands. */
    add_command("mpasound", do_mpasound, POS_DEAD, MAX_LEVEL_MPROG, CommandLogLevel::Normal, false);
    add_command("mpjunk", do_mpjunk, POS_DEAD, MAX_LEVEL_MPROG, CommandLogLevel::Normal, false);
    add_command("mpecho", do_mpecho, POS_DEAD, MAX_LEVEL_MPROG, CommandLogLevel::Normal, false);
    add_command("mpechoat", do_mpechoat, POS_DEAD, MAX_LEVEL_MPROG, CommandLogLevel::Normal, false);
    add_command("mpechoaround", do_mpechoaround, POS_DEAD, MAX_LEVEL_MPROG, CommandLogLevel::Normal, false);
    add_command("mpkill", do_mpkill, POS_DEAD, MAX_LEVEL_MPROG, CommandLogLevel::Normal, false);
    add_command("mpmload", do_mpmload, POS_DEAD, MAX_LEVEL_MPROG, CommandLogLevel::Normal, false);
    add_command("mpoload", do_mpoload, POS_DEAD, MAX_LEVEL_MPROG, CommandLogLevel::Normal, false);
    add_command("mppurge", do_mppurge, POS_DEAD, MAX_LEVEL_MPROG, CommandLogLevel::Normal, false);
    add_command("mpgoto", do_mpgoto, POS_DEAD, MAX_LEVEL_MPROG, CommandLogLevel::Normal, false);
    add_command("mpat", do_mpat, POS_DEAD, MAX_LEVEL_MPROG, CommandLogLevel::Normal, false);
    add_command("mptransfer", do_mptransfer, POS_DEAD, MAX_LEVEL_MPROG, CommandLogLevel::Normal, false);
    add_command("mpforce", do_mpforce, POS_DEAD, MAX_LEVEL_MPROG, CommandLogLevel::Normal, false);
}

static const char *apply_prefix(char *buf, CHAR_DATA *ch, const char *command) {
    char *pc_prefix = nullptr;

    /* Unswitched MOBs don't have prefixes.  If we're switched, get the player's prefix. */
    if (IS_NPC(ch)) {
        if (ch->desc && ch->desc->original())
            pc_prefix = ch->desc->original()->pcdata->prefix;
        else
            return command;
    } else
        pc_prefix = ch->pcdata->prefix;

    if (0 == strcmp(command, "prefix")) {
        return command;
    } else if (command[0] == '\\') {
        if (command[1] == '\\') {
            send_to_char(pc_prefix[0] ? "(prefix removed)\n\r" : "(no prefix to remove)\n\r", ch);
            pc_prefix[0] = '\0';
            command++; /* skip the \ */
        }
        command++; /* skip the \ */
        return command;
    } else {
        snprintf(buf, MAX_INPUT_LENGTH, "%s%s", pc_prefix, command);
        return buf;
    }
}

/*
 * The main entry point for executing commands.
 * Can be recursively called from 'at', 'order', 'force'.
 */
void interpret(CHAR_DATA *ch, const char *argument) {
    char cmd_buf[MAX_INPUT_LENGTH];
    char command[MAX_INPUT_LENGTH];
    char logline[MAX_INPUT_LENGTH];
    argument = apply_prefix(cmd_buf, ch, argument);

    /* Strip leading spaces. */
    while (isspace(*argument))
        argument++;
    if (argument[0] == '\0')
        return;

    /* No hiding. */
    REMOVE_BIT(ch->affected_by, AFF_HIDE);

    /* Implement freeze command. */
    if (!IS_NPC(ch) && IS_SET(ch->act, PLR_FREEZE)) {
        send_to_char("You're totally frozen!\n\r", ch);
        return;
    }

    /* Grab the command word.
     * Special parsing so ' can be a command,
     *   also no spaces needed after punctuation.
     */
    strcpy(logline, argument);
    if (!isalpha(argument[0]) && !isdigit(argument[0])) {
        command[0] = argument[0];
        command[1] = '\0';
        argument++;
        while (isspace(*argument))
            argument++;
    } else {
        argument = one_argument(argument, command);
    }

    /* Look for command in command table. */
    auto cmd = commands.get(command, get_trust(ch));

    /* Look for command in socials table. */
    if (!cmd.has_value()) {
        if (!check_social(ch, command, argument))
            send_to_char("Huh?\n\r", ch);
        // Return before logging. This is to prevent accidentally logging a typo'd "never log" command.
        return;
    }

    /* Log and snoop. */
    if (cmd->log == CommandLogLevel::Never)
        strcpy(logline, "");

    if ((!IS_NPC(ch) && IS_SET(ch->act, PLR_LOG)) || fLogAll || cmd->log == CommandLogLevel::Always) {
        int level = (cmd->level >= 91) ? (cmd->level) : 0;
        if (!IS_NPC(ch) && (IS_SET(ch->act, PLR_WIZINVIS) || IS_SET(ch->act, PLR_PROWL)))
            level = UMAX(level, get_trust(ch));
        if (IS_NPC(ch) && ch->desc && ch->desc->original()) {
            snprintf(log_buf, LOG_BUF_SIZE, "Log %s (as '%s'): %s", ch->desc->original()->name, ch->name, logline);
        } else {
            snprintf(log_buf, LOG_BUF_SIZE, "Log %s: %s", ch->name, logline);
        }
        log_new(log_buf, (cmd->level >= 91) ? EXTRA_WIZNET_IMM : EXTRA_WIZNET_MORT, level);
    }

    if (ch->desc)
        ch->desc->note_input(ch->name, logline);

    /* Character not in position for command? */
    if (ch->position < cmd->position) {
        send_to_char(bad_position_string[ch->position], ch);
        return;
    }

    /* Dispatch the command. */
    {
        // TODO: because do_fun expects a mutable char* (at least right now), make argument mutable in this dreadful
        // way. Eventually we'll pass a std::string or similar through and this will all go away.
        char mutable_argument[MAX_INPUT_LENGTH];
        strcpy(mutable_argument, argument);
        cmd->do_fun(ch, mutable_argument);
    }

    tail_chain();
}

static const struct social_type *find_social(const char *name) {
    for (int cmd = 0; social_table[cmd].name[0] != '\0'; cmd++) {
        if (name[0] == social_table[cmd].name[0] && !str_prefix(name, social_table[cmd].name)) {
            return social_table + cmd;
        }
    }
    return nullptr;
}

bool check_social(CHAR_DATA *ch, const char *command, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    const struct social_type *social;

    if (!(social = find_social(command)))
        return false;

    if (!IS_NPC(ch) && IS_SET(ch->comm, COMM_NOEMOTE)) {
        send_to_char("You are anti-social!\n\r", ch);
        return true;
    }

    if ((ch->position < POS_SLEEPING) || (ch->position == POS_SLEEPING && str_cmp(social->name, "snore"))) {
        send_to_char(bad_position_string[ch->position], ch);
        return true;
    }

    one_argument(argument, arg);
    CHAR_DATA *victim = nullptr;
    if (arg[0] == '\0') {
        act(social->others_no_arg, ch, nullptr, victim, TO_ROOM);
        act(social->char_no_arg, ch, nullptr, victim, TO_CHAR);
    } else if ((victim = get_char_room(ch, arg)) == nullptr) {
        send_to_char("They aren't here.\n\r", ch);
    } else if (victim == ch) {
        act(social->others_auto, ch, nullptr, victim, TO_ROOM);
        act(social->char_auto, ch, nullptr, victim, TO_CHAR);
    } else {
        act(social->others_found, ch, nullptr, victim, TO_NOTVICT);
        act(social->char_found, ch, nullptr, victim, TO_CHAR);
        act(social->vict_found, ch, nullptr, victim, TO_VICT);

        if (!IS_NPC(ch) && IS_NPC(victim) && !IS_AFFECTED(victim, AFF_CHARM) && IS_AWAKE(victim)
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
                act(social->others_found, victim, nullptr, ch, TO_NOTVICT);
                act(social->char_found, victim, nullptr, ch, TO_CHAR);
                act(social->vict_found, victim, nullptr, ch, TO_VICT);
                break;

            case 9:
            case 10:
            case 11:
            case 12:
                act("$n slaps $N.", victim, nullptr, ch, TO_NOTVICT);
                act("You slap $N.", victim, nullptr, ch, TO_CHAR);
                act("$n slaps you.", victim, nullptr, ch, TO_VICT);
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
        *arg_first = LOWER(*argument);
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
        *arg_first = LOWER(*argument);
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
    explicit Columniser(CHAR_DATA *ch) : ch(ch) { buf[0] = '\0'; }

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
            send_to_char(buf, ch);
            buf[0] = '\0';
            start_pos = buf_len = 0;
        }

        if (start_pos > buf_len)
            memset(buf + buf_len, ' ', start_pos - buf_len);
        memcpy(buf + start_pos, name.c_str(), name_len + 1);
    }

    CHAR_DATA *ch;
    char buf[MAX_STRING_LENGTH];
    int col_width = 12;
    int max_width = 72;
};

void do_commands(CHAR_DATA *ch, const char *argument) {
    (void)argument;
    Columniser col(ch);
    auto max_level = (get_trust(ch) < LEVEL_HERO) ? get_trust(ch) : (LEVEL_HERO - 1);
    commands.enumerate(commands.level_restrict(0, max_level, col.visitor()));
    if (col.buf[0] != '\0') {
        strcat(col.buf, "\n\r");
        send_to_char(col.buf, ch);
    }
}

void do_wizhelp(CHAR_DATA *ch, const char *argument) {
    (void)argument;
    Columniser col(ch);
    commands.enumerate(commands.level_restrict(LEVEL_HERO, get_trust(ch), col.visitor()));
    if (col.buf[0] != '\0') {
        strcat(col.buf, "\n\r");
        send_to_char(col.buf, ch);
    }
}

bool MP_Commands(CHAR_DATA *ch) /* Can MOBProged mobs
                                   use mpcommands? true if yes.
                                   - Kahn */
{
    if (is_switched(ch))
        return false;

    if (IS_NPC(ch) && ch->pIndexData->progtypes && !IS_AFFECTED(ch, AFF_CHARM))
        return true;

    return false;
}
