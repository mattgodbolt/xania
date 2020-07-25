/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                 */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  interp.c:  the command interpreter                                   */
/*                                                                       */
/*************************************************************************/

#include "interp.h"
#include "merc.h"
#include "note.h"
#include "trie.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

bool check_social(CHAR_DATA *ch, char *command, char *argument);

/* Merc-2.2 MOBProgs - Faramir 31/8/1998 */
bool MP_Commands(CHAR_DATA *ch);

/*
 * Command logging types.
 */
#define LOG_NORMAL 0
#define LOG_ALWAYS 1
#define LOG_NEVER 2

/*
 * Log-all switch.
 */
bool fLogAll = FALSE;

static const char *bad_position_string[] = {"Lie still; you are DEAD.\n\r",
                                            "You are hurt far too bad for that.\n\r",
                                            "You are hurt far too bad for that.\n\r",
                                            "You are too stunned to do that.\n\r",
                                            "In your dreams, or what?\n\r",
                                            "You're resting. Try standing up first!\n\r",
                                            "Better stand up first.\n\r",
                                            "No way!  You are still fighting!\n\r",
                                            "You're standing.\n\r"};

typedef struct cmd_type {
    const char *name;
    CommandFunc do_fun;
    sh_int position;
    sh_int level;
    sh_int log;
    bool show;
} cmd_type_t;

/* Trie-based version of command table -- Forrey, Sun Mar 19 21:43:10 CET 2000 */

static void *cmd_trie;

static void add_command(const char *name, CommandFunc do_fun, sh_int position, sh_int level, sh_int log, bool show) {
    cmd_type_t *cmd = malloc(sizeof(cmd_type_t));
    if (!cmd) {
        bug("Couldn't claim memory for new command \"%s\".", name);
        exit(1);
    }
    cmd->name = name;
    cmd->do_fun = do_fun;
    cmd->position = position;
    cmd->level = level;
    cmd->log = log;
    cmd->show = show;

    trie_add(cmd_trie, name, cmd, level);
}

void interp_initialise(void) {
    cmd_trie = trie_create(0);
    if (!cmd_trie) {
        bug("note_initialise: couldn't create trie.");
        exit(1);
    }

    /* Common movement commands. */
    add_command("north", do_north, POS_STANDING, 0, LOG_NEVER, 0);
    add_command("east", do_east, POS_STANDING, 0, LOG_NEVER, 0);
    add_command("south", do_south, POS_STANDING, 0, LOG_NEVER, 0);
    add_command("west", do_west, POS_STANDING, 0, LOG_NEVER, 0);
    add_command("up", do_up, POS_STANDING, 0, LOG_NEVER, 0);
    add_command("down", do_down, POS_STANDING, 0, LOG_NEVER, 0);

    /* Common other commands.
     * Placed here so one and two letter abbreviations work.
     */
    add_command("at", do_at, POS_DEAD, L6, LOG_NORMAL, 1);
    add_command("auction", do_auction, POS_SLEEPING, 0, LOG_NORMAL, 1);
    add_command("buy", do_buy, POS_RESTING, 0, LOG_NORMAL, 1);
    add_command("cast", do_cast, POS_FIGHTING, 0, LOG_NORMAL, 1);
    add_command("channels", do_channels, POS_DEAD, 0, LOG_NORMAL, 1);
    add_command("exits", do_exits, POS_RESTING, 0, LOG_NORMAL, 1);
    add_command("get", do_get, POS_RESTING, 0, LOG_NORMAL, 1);
    add_command("goto", do_goto, POS_DEAD, L8, LOG_NORMAL, 1);
    add_command("group", do_group, POS_SLEEPING, 0, LOG_NORMAL, 1);
    add_command("hit", do_kill, POS_FIGHTING, 0, LOG_NORMAL, 0);
    add_command("inventory", do_inventory, POS_DEAD, 0, LOG_NORMAL, 1);
    add_command("kill", do_kill, POS_FIGHTING, 0, LOG_NORMAL, 1);
    add_command("look", do_look, POS_RESTING, 0, LOG_NORMAL, 1);
    add_command("music", do_music, POS_SLEEPING, 0, LOG_NORMAL, 1);
    add_command("order", do_order, POS_RESTING, 0, LOG_ALWAYS, 1);
    add_command("peek", do_peek, POS_RESTING, 0, LOG_NORMAL, 1);
    add_command("practice", do_practice, POS_SLEEPING, 0, LOG_NORMAL, 1);
    add_command("rest", do_rest, POS_SLEEPING, 0, LOG_NORMAL, 1);
    add_command("score", do_score, POS_DEAD, 0, LOG_NORMAL, 1);
    add_command("scan", do_scan, POS_FIGHTING, 0, LOG_NORMAL, 1);
    add_command("sit", do_sit, POS_SLEEPING, 0, LOG_NORMAL, 1);
    add_command("sockets", do_sockets, POS_DEAD, L4, LOG_NORMAL, 1);
    add_command("stand", do_stand, POS_SLEEPING, 0, LOG_NORMAL, 1);
    add_command("tell", do_tell, POS_DEAD, 0, LOG_NORMAL, 1);
    add_command("wield", do_wear, POS_RESTING, 0, LOG_NORMAL, 1);
    add_command("wizhelp", do_wizhelp, POS_DEAD, HE, LOG_NORMAL, 1);
    add_command("wiznet", do_wiznet, POS_DEAD, L8, LOG_NORMAL, 1);

    /* Informational commands. */
    add_command("affect", do_affected, POS_DEAD, 0, LOG_NORMAL, 1);
    add_command("areas", do_areas, POS_DEAD, 0, LOG_NORMAL, 1);
    add_command("alist", do_alist, POS_DEAD, 1, LOG_NORMAL, 1);
    add_command("bug", do_bug, POS_DEAD, 0, LOG_NORMAL, 1);
    add_command("changes", do_changes, POS_DEAD, 0, LOG_NORMAL, 1);
    add_command("commands", do_commands, POS_DEAD, 0, LOG_NORMAL, 1);
    add_command("compare", do_compare, POS_RESTING, 0, LOG_NORMAL, 1);
    add_command("consider", do_consider, POS_RESTING, 0, LOG_NORMAL, 1);
    add_command("count", do_count, POS_SLEEPING, 0, LOG_NORMAL, 1);
    add_command("credits", do_credits, POS_DEAD, 0, LOG_NORMAL, 1);
    add_command("equipment", do_equipment, POS_DEAD, 0, LOG_NORMAL, 1);
    add_command("examine", do_examine, POS_RESTING, 0, LOG_NORMAL, 1);
    add_command("finger", do_finger, POS_DEAD, 0, LOG_NORMAL, 1);
    add_command("help", do_help, POS_DEAD, 0, LOG_NORMAL, 1);
    add_command("idea", do_idea, POS_DEAD, 0, LOG_NORMAL, 1);
    add_command("info", do_groups, POS_SLEEPING, 0, LOG_NORMAL, 1);
    add_command("motd", do_motd, POS_DEAD, 0, LOG_NORMAL, 1);
    add_command("read", do_read, POS_RESTING, 0, LOG_NORMAL, 1);
    add_command("report", do_report, POS_RESTING, 0, LOG_NORMAL, 1);
    add_command("rules", do_rules, POS_DEAD, 0, LOG_NORMAL, 1);
    add_command("skills", do_skills, POS_DEAD, 0, LOG_NORMAL, 1);
    add_command("socials", do_socials, POS_DEAD, 0, LOG_NORMAL, 1);
    add_command("spells", do_spells, POS_DEAD, 0, LOG_NORMAL, 1);
    add_command("story", do_story, POS_DEAD, 0, LOG_NORMAL, 1);
    add_command("time", do_time, POS_DEAD, 0, LOG_NORMAL, 1);
    add_command("timezone", do_timezone, POS_DEAD, 0, LOG_NORMAL, 1);
    add_command("tipwizard", do_tipwizard, POS_DEAD, 0, LOG_NORMAL, 1);
    add_command("tips", do_tipwizard, POS_DEAD, 0, LOG_NORMAL, 1);
    add_command("typo", do_typo, POS_DEAD, 0, LOG_NORMAL, 1);
    add_command("weather", do_weather, POS_RESTING, 0, LOG_NORMAL, 1);
    add_command("who", do_who, POS_DEAD, 0, LOG_NORMAL, 1);
    add_command("whois", do_whois, POS_DEAD, 0, LOG_NORMAL, 1);
    add_command("wizlist", do_wizlist, POS_DEAD, 0, LOG_NORMAL, 1);
    add_command("worth", do_worth, POS_SLEEPING, 0, LOG_NORMAL, 1);

    /* Configuration commands. */
    add_command("afk", do_afk, POS_DEAD, 0, LOG_NORMAL, 1);
    add_command("autolist", do_autolist, POS_DEAD, 0, LOG_NORMAL, 1);
    add_command("autoaffect", do_autoaffect, POS_DEAD, 0, LOG_NORMAL, 1);
    add_command("autoassist", do_autoassist, POS_DEAD, 0, LOG_NORMAL, 1);
    add_command("autoexit", do_autoexit, POS_DEAD, 0, LOG_NORMAL, 1);
    add_command("autogold", do_autogold, POS_DEAD, 0, LOG_NORMAL, 1);
    add_command("autoloot", do_autoloot, POS_DEAD, 0, LOG_NORMAL, 1);
    add_command("autopeek", do_autopeek, POS_DEAD, 0, LOG_NORMAL, 1);
    add_command("autosac", do_autosac, POS_DEAD, 0, LOG_NORMAL, 1);
    add_command("autosplit", do_autosplit, POS_DEAD, 0, LOG_NORMAL, 1);
    add_command("brief", do_brief, POS_DEAD, 0, LOG_NORMAL, 1);
    add_command("color", do_colour, POS_DEAD, 0, LOG_NORMAL, 1);
    add_command("colour", do_colour, POS_DEAD, 0, LOG_NORMAL, 1);
    add_command("combine", do_combine, POS_DEAD, 0, LOG_NORMAL, 1);
    add_command("compact", do_compact, POS_DEAD, 0, LOG_NORMAL, 1);
    add_command("description", do_description, POS_DEAD, 0, LOG_NORMAL, 1);
    add_command("delet", do_delet, POS_DEAD, 0, LOG_ALWAYS, 0);
    add_command("delete", do_delete, POS_DEAD, 0, LOG_ALWAYS, 1);
    add_command("nofollow", do_nofollow, POS_DEAD, 0, LOG_NORMAL, 1);
    add_command("noloot", do_noloot, POS_DEAD, 0, LOG_NORMAL, 1);
    add_command("nosummon", do_nosummon, POS_DEAD, 0, LOG_NORMAL, 1);
    add_command("outfit", do_outfit, POS_RESTING, 0, LOG_ALWAYS, 1);
    add_command("password", do_password, POS_DEAD, 0, LOG_NEVER, 1);
    add_command("prefix", do_prefix, POS_DEAD, 0, LOG_NORMAL, 1);
    add_command("prompt", do_prompt, POS_DEAD, 0, LOG_NORMAL, 1);
    add_command("scroll", do_scroll, POS_DEAD, 0, LOG_NORMAL, 1);
    add_command("showdefence", do_showdefence, POS_DEAD, 0, LOG_NORMAL, 1);
    add_command("showafk", do_showafk, POS_DEAD, 0, LOG_NORMAL, 1);
    add_command("title", do_title, POS_DEAD, 0, LOG_NORMAL, 1);
    add_command("wimpy", do_wimpy, POS_DEAD, 0, LOG_NORMAL, 1);

    /* Communication commands. */
    add_command("allege", do_allege, POS_SLEEPING, 0, LOG_NORMAL, 1);
    add_command("announce", do_announce, POS_SLEEPING, 0, LOG_NORMAL, 1);
    add_command("answer", do_answer, POS_SLEEPING, 0, LOG_NORMAL, 1);
    add_command("emote", do_emote, POS_RESTING, 0, LOG_NORMAL, 1);
    add_command(".", do_gossip, POS_SLEEPING, 0, LOG_NORMAL, 0);
    add_command("gossip", do_gossip, POS_SLEEPING, 0, LOG_NORMAL, 1);
    add_command(",", do_emote, POS_RESTING, 0, LOG_NORMAL, 0);
    add_command("gratz", do_gratz, POS_SLEEPING, 0, LOG_NORMAL, 1);
    add_command("gtell", do_gtell, POS_DEAD, 0, LOG_NORMAL, 1);
    add_command(";", do_gtell, POS_DEAD, 0, LOG_NORMAL, 0);
    add_command("note", do_note, POS_SLEEPING, 0, LOG_NORMAL, 1);
    add_command("news", do_news, POS_SLEEPING, 0, LOG_NORMAL, 1);
    add_command("mail", do_mail, POS_SLEEPING, 0, LOG_NORMAL, 1);
    add_command("philosophise", do_philosophise, POS_SLEEPING, 0, LOG_NORMAL, 1);
    add_command("pose", do_pose, POS_RESTING, 0, LOG_NORMAL, 1);
    add_command("question", do_question, POS_SLEEPING, 0, LOG_NORMAL, 1);
    add_command("quiet", do_quiet, POS_SLEEPING, 0, LOG_NORMAL, 1);
    add_command("qwest", do_qwest, POS_SLEEPING, 0, LOG_NORMAL, 1);
    add_command("reply", do_reply, POS_DEAD, 0, LOG_NORMAL, 1);
    add_command("say", do_say, POS_RESTING, 0, LOG_NORMAL, 1);
    add_command("'", do_say, POS_RESTING, 0, LOG_NORMAL, 0);
    add_command("shout", do_shout, POS_RESTING, 3, LOG_NORMAL, 1);
    add_command("yell", do_yell, POS_RESTING, 0, LOG_NORMAL, 1);

    /* Object manipulation commands. */
    add_command("brandish", do_brandish, POS_RESTING, 0, LOG_NORMAL, 1);
    add_command("close", do_close, POS_RESTING, 0, LOG_NORMAL, 1);
    add_command("donate", do_donate, POS_RESTING, 0, LOG_NORMAL, 1);
    add_command("drink", do_drink, POS_RESTING, 0, LOG_NORMAL, 1);
    add_command("drop", do_drop, POS_RESTING, 0, LOG_NORMAL, 1);
    add_command("eat", do_eat, POS_RESTING, 0, LOG_NORMAL, 1);
    add_command("fill", do_fill, POS_RESTING, 0, LOG_NORMAL, 1);
    add_command("pour", do_pour, POS_RESTING, 0, LOG_NORMAL, 1);
    add_command("give", do_give, POS_RESTING, 0, LOG_NORMAL, 1);
    /* hailcorpse and shortcut */
    add_command("hailcorpse", do_hailcorpse, POS_STANDING, 0, LOG_NORMAL, 1);
    add_command("hc", do_hailcorpse, POS_STANDING, 0, LOG_NORMAL, 1);
    add_command("heal", do_heal, POS_RESTING, 0, LOG_NORMAL, 1);
    add_command("hold", do_wear, POS_RESTING, 0, LOG_NORMAL, 1);
    add_command("list", do_list, POS_RESTING, 0, LOG_NORMAL, 1);
    add_command("lock", do_lock, POS_RESTING, 0, LOG_NORMAL, 1);
    add_command("open", do_open, POS_RESTING, 0, LOG_NORMAL, 1);
    add_command("pick", do_pick, POS_RESTING, 0, LOG_NORMAL, 1);
    add_command("put", do_put, POS_RESTING, 0, LOG_NORMAL, 1);
    add_command("quaff", do_quaff, POS_RESTING, 0, LOG_NORMAL, 1);
    add_command("recite", do_recite, POS_RESTING, 0, LOG_NORMAL, 1);
    add_command("remove", do_remove, POS_RESTING, 0, LOG_NORMAL, 1);
    add_command("sell", do_sell, POS_RESTING, 0, LOG_NORMAL, 1);
    add_command("take", do_get, POS_RESTING, 0, LOG_NORMAL, 1);
    add_command("throw", do_throw, POS_STANDING, 0, LOG_NORMAL, 1);
    add_command("sacrifice", do_sacrifice, POS_RESTING, 0, LOG_NORMAL, 1);
    add_command("junk", do_sacrifice, POS_RESTING, 0, LOG_NORMAL, 0);
    add_command("tap", do_sacrifice, POS_RESTING, 0, LOG_NORMAL, 0);
    add_command("unlock", do_unlock, POS_RESTING, 0, LOG_NORMAL, 1);
    add_command("value", do_value, POS_RESTING, 0, LOG_NORMAL, 1);
    add_command("wear", do_wear, POS_RESTING, 0, LOG_NORMAL, 1);
    add_command("zap", do_zap, POS_RESTING, 0, LOG_NORMAL, 1);
    /* Wandera's little baby. */
    add_command("sharpen", do_sharpen, POS_STANDING, 0, LOG_NORMAL, 1);
    /* Combat commands. */
    add_command("backstab", do_backstab, POS_STANDING, 0, LOG_NORMAL, 1);
    add_command("bash", do_bash, POS_FIGHTING, 0, LOG_NORMAL, 1);
    add_command("bs", do_backstab, POS_STANDING, 0, LOG_NORMAL, 0);
    add_command("berserk", do_berserk, POS_FIGHTING, 0, LOG_NORMAL, 1);
    add_command("dirt", do_dirt, POS_FIGHTING, 0, LOG_NORMAL, 1);
    add_command("disarm", do_disarm, POS_FIGHTING, 0, LOG_NORMAL, 1);
    add_command("flee", do_flee, POS_FIGHTING, 0, LOG_NORMAL, 1);
    add_command("kick", do_kick, POS_FIGHTING, 0, LOG_NORMAL, 1);
    /* TheMoog's command. */
    add_command("headbutt", do_headbutt, POS_FIGHTING, 0, LOG_NORMAL, 1);
    /* $-) */
    add_command("murde", do_murde, POS_FIGHTING, IM, LOG_NORMAL, 0);
    add_command("murder", do_murder, POS_FIGHTING, IM, LOG_ALWAYS, 1);
    add_command("rescue", do_rescue, POS_FIGHTING, 0, LOG_NORMAL, 0);
    add_command("trip", do_trip, POS_FIGHTING, 0, LOG_NORMAL, 1);

    /* Miscellaneous commands. */
    /* Rohan's bit of wizadry! */
    add_command("accept", do_accept, POS_STANDING, 0, LOG_NORMAL, 1);
    add_command("cancel", do_cancel_chal, POS_STANDING, 0, LOG_NORMAL, 1);
    add_command("challenge", do_challenge, POS_STANDING, 0, LOG_NORMAL, 1);
    add_command("duel", do_duel, POS_STANDING, 0, LOG_NORMAL, 1);
    add_command("follow", do_follow, POS_RESTING, 0, LOG_NORMAL, 1);
    add_command("gain", do_gain, POS_STANDING, 0, LOG_NORMAL, 1);
    add_command("groups", do_groups, POS_SLEEPING, 0, LOG_NORMAL, 1);
    add_command("hide", do_hide, POS_RESTING, 0, LOG_NORMAL, 1);
    add_command("enter", do_enter, POS_STANDING, 0, LOG_NEVER, 0);
    add_command("qui", do_qui, POS_DEAD, 0, LOG_NORMAL, 0);
    add_command("quit", do_quit, POS_DEAD, 0, LOG_NORMAL, 1);
    add_command("ready", do_ready, POS_STANDING, 0, LOG_NORMAL, 1);
    add_command("recall", do_recall, POS_FIGHTING, 0, LOG_NORMAL, 1);
    add_command("refuse", do_refuse, POS_STANDING, 0, LOG_NORMAL, 1);
    add_command("/", do_recall, POS_FIGHTING, 0, LOG_NORMAL, 0);
    add_command("save", do_save, POS_DEAD, 0, LOG_NORMAL, 1);
    add_command("sleep", do_sleep, POS_SLEEPING, 0, LOG_NORMAL, 1);
    add_command("sneak", do_sneak, POS_STANDING, 0, LOG_NORMAL, 1);
    add_command("split", do_split, POS_RESTING, 0, LOG_NORMAL, 1);
    add_command("steal", do_steal, POS_STANDING, 0, LOG_NORMAL, 1);
    add_command("train", do_train, POS_RESTING, 0, LOG_NORMAL, 1);
    add_command("visible", do_visible, POS_SLEEPING, 0, LOG_NORMAL, 1);
    add_command("wake", do_wake, POS_SLEEPING, 0, LOG_NORMAL, 1);
    add_command("where", do_where, POS_RESTING, 0, LOG_NORMAL, 1);

    /* Riding skill */
    add_command("ride", do_ride, POS_FIGHTING, 0, LOG_NORMAL, 1);
    add_command("mount", do_ride, POS_FIGHTING, 0, LOG_NORMAL, 1);
    add_command("dismount", do_dismount, POS_FIGHTING, 0, LOG_NORMAL, 1);

    /* Clan commands */
    add_command("member", do_member, POS_STANDING, 0, LOG_ALWAYS, 1);
    add_command("promote", do_promote, POS_STANDING, 0, LOG_ALWAYS, 1);
    add_command("demote", do_demote, POS_STANDING, 0, LOG_ALWAYS, 1);
    add_command("noclanchan", do_noclanchan, POS_SLEEPING, 0, LOG_ALWAYS, 1);
    add_command("clanwho", do_clanwho, POS_SLEEPING, 0, LOG_NORMAL, 1);
    add_command("clantalk", do_clantalk, POS_SLEEPING, 0, LOG_NORMAL, 1);
    add_command(">", do_clantalk, POS_SLEEPING, 0, LOG_NORMAL, 1);

    /* clan power for immortals .... */
    add_command("clanset", do_clanset, POS_SLEEPING, L4, LOG_ALWAYS, 1);

    /* Immortal commands. */
    add_command("advance", do_advance, POS_DEAD, ML, LOG_ALWAYS, 1);
    add_command("dump", do_dump, POS_DEAD, ML, LOG_ALWAYS, 0);
    add_command("trust", do_trust, POS_DEAD, ML, LOG_ALWAYS, 1);
    add_command("sacname", do_sacname, POS_DEAD, ML, LOG_ALWAYS, 1);
    /*"clipboard", do_clipboard, POS_DEAD, ML, LOG_NORMAL, 1)*/
    /*"edit", do_edit, POS_DEAD, ML, LOG_ALWAYS, 1)*/
    /*"security", do_security, POS_DEAD, ML, LOG_ALWAYS, 1)*/
    /*"mobile", do_mobile, POS_DEAD, ML, LOG_ALWAYS, 1)*/
    /*"object", do_object, POS_DEAD, L1, LOG_ALWAYS, 1)*/

    add_command("allow", do_allow, POS_DEAD, L2, LOG_ALWAYS, 1);
    add_command("ban", do_ban, POS_DEAD, L2, LOG_ALWAYS, 1);
    add_command("permban", do_permban, POS_DEAD, L2, LOG_ALWAYS, 1);
    add_command("permit", do_permit, POS_DEAD, L2, LOG_ALWAYS, 1);
    add_command("deny", do_deny, POS_DEAD, L1, LOG_ALWAYS, 1);
    add_command("disconnect", do_disconnect, POS_DEAD, L3, LOG_ALWAYS, 1);
    add_command("freeze", do_freeze, POS_DEAD, L3, LOG_ALWAYS, 1);
    add_command("reboo", do_reboo, POS_DEAD, L1, LOG_NORMAL, 0);
    add_command("reboot", do_reboot, POS_DEAD, L1, LOG_ALWAYS, 1);
    add_command("set", do_set, POS_DEAD, L2, LOG_ALWAYS, 1);
    add_command("setinfo", do_setinfo, POS_DEAD, 0, LOG_NORMAL, 1);
    add_command("shutdow", do_shutdow, POS_DEAD, L1, LOG_NORMAL, 0);
    add_command("shutdown", do_shutdown, POS_DEAD, L1, LOG_ALWAYS, 1);
    add_command("wizlock", do_wizlock, POS_DEAD, L2, LOG_ALWAYS, 1);
    add_command("force", do_force, POS_DEAD, L7, LOG_ALWAYS, 1);
    add_command("load", do_load, POS_DEAD, L4, LOG_ALWAYS, 1);
    add_command("newlock", do_newlock, POS_DEAD, L4, LOG_ALWAYS, 1);
    add_command("nochannels", do_nochannels, POS_DEAD, L6, LOG_ALWAYS, 1);
    add_command("noemote", do_noemote, POS_DEAD, L6, LOG_ALWAYS, 1);
    add_command("notell", do_notell, POS_DEAD, L6, LOG_ALWAYS, 1);
    add_command("pecho", do_pecho, POS_DEAD, L4, LOG_ALWAYS, 1);
    add_command("pardon", do_pardon, POS_DEAD, L3, LOG_ALWAYS, 1);
    add_command("purge", do_purge, POS_DEAD, L4, LOG_ALWAYS, 1);
    add_command("restore", do_restore, POS_DEAD, L4, LOG_ALWAYS, 1);
    add_command("sla", do_sla, POS_DEAD, L3, LOG_NORMAL, 0);
    add_command("slay", do_slay, POS_DEAD, L3, LOG_ALWAYS, 1);
    add_command("teleport", do_transfer, POS_DEAD, L5, LOG_ALWAYS, 1);
    add_command("transfer", do_transfer, POS_DEAD, L5, LOG_ALWAYS, 1);
    add_command("poofin", do_bamfin, POS_DEAD, L8, LOG_NORMAL, 1);
    add_command("poofout", do_bamfout, POS_DEAD, L8, LOG_NORMAL, 1);
    add_command("gecho", do_echo, POS_DEAD, L4, LOG_ALWAYS, 1);
    add_command("holylight", do_holylight, POS_DEAD, IM, LOG_NORMAL, 1);
    add_command("invis", do_invis, POS_DEAD, IM, LOG_NORMAL, 0);
    add_command("log", do_log, POS_DEAD, L1, LOG_ALWAYS, 1);
    add_command("memory", do_memory, POS_DEAD, IM, LOG_NORMAL, 1);
    add_command("mwhere", do_mwhere, POS_DEAD, IM, LOG_NORMAL, 1);
    add_command("peace", do_peace, POS_DEAD, L5, LOG_NORMAL, 1);
    /* Death's commands */
    add_command("coma", do_coma, POS_DEAD, IM, LOG_NORMAL, 1);
    add_command("owhere", do_owhere, POS_DEAD, IM, LOG_NORMAL, 1);
    /* =)) */
    add_command("awaken", do_awaken, POS_DEAD, IM, LOG_NORMAL, 1);
    add_command("echo", do_recho, POS_DEAD, L6, LOG_ALWAYS, 1);
    add_command("return", do_return, POS_DEAD, L6, LOG_NORMAL, 1);
    add_command("smit", do_smit, POS_STANDING, L7, LOG_NORMAL, 1);
    add_command("smite", do_smite, POS_STANDING, L7, LOG_NORMAL, 1);
    add_command("snoop", do_snoop, POS_DEAD, L5, LOG_ALWAYS, 1);
    add_command("stat", do_stat, POS_DEAD, IM, LOG_NORMAL, 1);
    add_command("string", do_string, POS_DEAD, L5, LOG_ALWAYS, 1);
    add_command("switch", do_switch, POS_DEAD, L6, LOG_ALWAYS, 1);
    add_command("wizinvis", do_invis, POS_DEAD, IM, LOG_NORMAL, 1);
    add_command("zecho", do_zecho, POS_DEAD, L6, LOG_ALWAYS, 1);
    add_command("prowl", do_prowl, POS_DEAD, IM, LOG_NORMAL, 1);
    add_command("vnum", do_vnum, POS_DEAD, L4, LOG_NORMAL, 1);
    add_command("clone", do_clone, POS_DEAD, L5, LOG_ALWAYS, 1);
    add_command("immworth", do_immworth, POS_DEAD, L3, LOG_NORMAL, 1);
    add_command("immtalk", do_immtalk, POS_DEAD, IM, LOG_NORMAL, 1);
    add_command("imotd", do_imotd, POS_DEAD, IM, LOG_NORMAL, 1);
    add_command(":", do_immtalk, POS_DEAD, IM, LOG_NORMAL, 0);

    /* Merc-2.2 MOBProgs - Faramir 31/8/1998 */
    /* MOBprogram commands. */
    add_command("mpasound", do_mpasound, POS_DEAD, MAX_LEVEL_MPROG, LOG_NORMAL, 0);
    add_command("mpjunk", do_mpjunk, POS_DEAD, MAX_LEVEL_MPROG, LOG_NORMAL, 0);
    add_command("mpecho", do_mpecho, POS_DEAD, MAX_LEVEL_MPROG, LOG_NORMAL, 0);
    add_command("mpechoat", do_mpechoat, POS_DEAD, MAX_LEVEL_MPROG, LOG_NORMAL, 0);
    add_command("mpechoaround", do_mpechoaround, POS_DEAD, MAX_LEVEL_MPROG, LOG_NORMAL, 0);
    add_command("mpkill", do_mpkill, POS_DEAD, MAX_LEVEL_MPROG, LOG_NORMAL, 0);
    add_command("mpmload", do_mpmload, POS_DEAD, MAX_LEVEL_MPROG, LOG_NORMAL, 0);
    add_command("mpoload", do_mpoload, POS_DEAD, MAX_LEVEL_MPROG, LOG_NORMAL, 0);
    add_command("mppurge", do_mppurge, POS_DEAD, MAX_LEVEL_MPROG, LOG_NORMAL, 0);
    add_command("mpgoto", do_mpgoto, POS_DEAD, MAX_LEVEL_MPROG, LOG_NORMAL, 0);
    add_command("mpat", do_mpat, POS_DEAD, MAX_LEVEL_MPROG, LOG_NORMAL, 0);
    add_command("mptransfer", do_mptransfer, POS_DEAD, MAX_LEVEL_MPROG, LOG_NORMAL, 0);
    add_command("mpforce", do_mpforce, POS_DEAD, MAX_LEVEL_MPROG, LOG_NORMAL, 0);
    // Moog's CPP wrapper test command
    add_command("cpptest", do_cpptest, POS_DEAD, ML, LOG_NORMAL, 1);
}

static const struct cmd_type *find_command(CHAR_DATA *ch, char *command, int trust) {
    (void)ch;
    return trie_get(cmd_trie, command, trust);
}

static char *apply_prefix(char *buf, CHAR_DATA *ch, char *command) {
    char *pc_prefix = NULL;

    /* Unswitched MOBs don't have prefixes.  If we're switched, get the player's prefix. */
    if (IS_NPC(ch)) {
        if (ch->desc && ch->desc->original)
            pc_prefix = ch->desc->original->pcdata->prefix;
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
        int prefix_len = strlen(pc_prefix);
        memcpy(buf, pc_prefix, (prefix_len < MAX_INPUT_LENGTH) ? prefix_len : MAX_INPUT_LENGTH - 1);
        memcpy(buf + prefix_len, command, MAX_INPUT_LENGTH - prefix_len);
        buf[MAX_INPUT_LENGTH - 1] = '\0';
        return buf;
    }
}

/*
 * The main entry point for executing commands.
 * Can be recursively called from 'at', 'order', 'force'.
 */
void interpret(CHAR_DATA *ch, char *argument) {
    char cmd_buf[MAX_INPUT_LENGTH];
    char command[MAX_INPUT_LENGTH];
    char logline[MAX_INPUT_LENGTH];
    const struct cmd_type *cmd;

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
    cmd = find_command(ch, command, get_trust(ch));

    /* Look for command in socials table. */
    if (!cmd) {
        if (!check_social(ch, command, argument))
            send_to_char("Huh?\n\r", ch);
        return;
    }

    /* Log and snoop. */
    if (cmd->log == LOG_NEVER)
        strcpy(logline, "");

    if ((!IS_NPC(ch) && IS_SET(ch->act, PLR_LOG)) || fLogAll || cmd->log == LOG_ALWAYS) {
        int level = (cmd->level >= 91) ? (cmd->level) : 0;
        if (!IS_NPC(ch) && (IS_SET(ch->act, PLR_WIZINVIS) || IS_SET(ch->act, PLR_PROWL)))
            level = UMAX(level, get_trust(ch));
        if (IS_NPC(ch) && ch->desc && ch->desc->original) {
            snprintf(log_buf, LOG_BUF_SIZE, "Log %s (as '%s'): %s", ch->desc->original->name, ch->name, logline);
        } else {
            snprintf(log_buf, LOG_BUF_SIZE, "Log %s: %s", ch->name, logline);
        }
        log_new(log_buf, (cmd->level >= 91) ? EXTRA_WIZNET_IMM : EXTRA_WIZNET_MORT, level);
    }

    if (ch->desc != NULL && ch->desc->snoop_by != NULL) {
        write_to_buffer(ch->desc->snoop_by, "% ", 2);
        write_to_buffer(ch->desc->snoop_by, logline, 0);
        write_to_buffer(ch->desc->snoop_by, "\n\r", 2);
    }

    /* Character not in position for command? */
    if (ch->position < cmd->position) {
        send_to_char(bad_position_string[ch->position], ch);
        return;
    }

    /* Dispatch the command. */
    (*cmd->do_fun)(ch, argument);

    tail_chain();
}

static const struct social_type *find_social(char *name) {
    int cmd;

    for (cmd = 0; social_table[cmd].name[0] != '\0'; cmd++) {
        if (name[0] == social_table[cmd].name[0] && !str_prefix(name, social_table[cmd].name)) {
            return social_table + cmd;
        }
    }
    return NULL;
}

bool check_social(CHAR_DATA *ch, char *command, char *argument) {
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    const struct social_type *social;

    if (!(social = find_social(command)))
        return FALSE;

    if (!IS_NPC(ch) && IS_SET(ch->comm, COMM_NOEMOTE)) {
        send_to_char("You are anti-social!\n\r", ch);
        return TRUE;
    }

    if ((ch->position < POS_SLEEPING) || (ch->position == POS_SLEEPING && str_cmp(social->name, "snore"))) {
        send_to_char(bad_position_string[ch->position], ch);
        return TRUE;
    }

    one_argument(argument, arg);
    victim = NULL;
    if (arg[0] == '\0') {
        act(social->others_no_arg, ch, NULL, victim, TO_ROOM);
        act(social->char_no_arg, ch, NULL, victim, TO_CHAR);
    } else if ((victim = get_char_room(ch, arg)) == NULL) {
        send_to_char("They aren't here.\n\r", ch);
    } else if (victim == ch) {
        act(social->others_auto, ch, NULL, victim, TO_ROOM);
        act(social->char_auto, ch, NULL, victim, TO_CHAR);
    } else {
        act(social->others_found, ch, NULL, victim, TO_NOTVICT);
        act(social->char_found, ch, NULL, victim, TO_CHAR);
        act(social->vict_found, ch, NULL, victim, TO_VICT);

        if (!IS_NPC(ch) && IS_NPC(victim) && !IS_AFFECTED(victim, AFF_CHARM) && IS_AWAKE(victim)
            && victim->desc == NULL) {
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
                act(social->others_found, victim, NULL, ch, TO_NOTVICT);
                act(social->char_found, victim, NULL, ch, TO_CHAR);
                act(social->vict_found, victim, NULL, ch, TO_VICT);
                break;

            case 9:
            case 10:
            case 11:
            case 12:
                act("$n slaps $N.", victim, NULL, ch, TO_NOTVICT);
                act("You slap $N.", victim, NULL, ch, TO_CHAR);
                act("$n slaps you.", victim, NULL, ch, TO_VICT);
                break;
            }
        }
    }

    return TRUE;
}

/*
 * Return true if an argument is completely numeric.
 */
bool is_number(char *arg) {
    if (*arg == '\0')
        return FALSE;

    if (*arg == '+' || *arg == '-')
        arg++;

    for (; *arg != '\0'; arg++) {
        if (!isdigit(*arg))
            return FALSE;
    }

    return TRUE;
}

/*
 * Given a string like 14.foo, return 14 and 'foo'
 */
int number_argument(char *argument, char *arg) {
    char *pdot;
    int number;

    for (pdot = argument; *pdot != '\0'; pdot++) {
        if (*pdot == '.') {
            *pdot = '\0';
            number = atoi(argument);
            *pdot = '.';
            strcpy(arg, pdot + 1);
            return number;
        }
    }

    strcpy(arg, argument);
    return 1;
}

/*
 * Pick off one argument from a string and return the rest.
 * Understands quotes.
 */
char *one_argument(char *argument, void *arg_f) {
    char cEnd;
    char *arg_first = (char *)arg_f;

    while (isspace(*argument))
        argument++;

    cEnd = ' ';
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

/* Columniser and new do_wizhelp, do_commands to cope with trie-based command
 * tables -- Forrey, Sun Mar 19 21:43:10 CET 2000
 */

typedef struct columniser {
    CHAR_DATA *ch;
    char buf[MAX_STRING_LENGTH];
    int col_width;
    int max_width;
} columniser_t;

static void command_enumerator(const char *name, int lev, void *value, void *metadata) {
    (void)lev;
    columniser_t *col = (columniser_t *)metadata;
    int buf_len = strlen(col->buf);
    int name_len = strlen(name);
    int start_pos;
    cmd_type_t *cmd = (cmd_type_t *)value;

    if (!cmd->show)
        return;

    if (col->buf[0] == '\0')
        start_pos = 0;
    else
        start_pos = ((buf_len + col->col_width) / col->col_width) * col->col_width;

    if (start_pos + name_len > col->max_width) {
        strcat(col->buf, "\n\r");
        send_to_char(col->buf, col->ch);
        col->buf[0] = '\0';
        start_pos = buf_len = 0;
    }

    if (start_pos > buf_len)
        memset(col->buf + buf_len, ' ', start_pos - buf_len);
    memcpy(col->buf + start_pos, name, name_len + 1);
}

void do_commands(CHAR_DATA *ch, char *argument) {
    (void)argument;
    columniser_t col;

    col.ch = ch;
    col.buf[0] = '\0';
    col.col_width = 12;
    col.max_width = 72;
    trie_enumerate(cmd_trie, 0, (get_trust(ch) < LEVEL_HERO) ? get_trust(ch) : (LEVEL_HERO - 1), command_enumerator,
                   &col);
    if (col.buf[0] != '\0') {
        strcat(col.buf, "\n\r");
        send_to_char(col.buf, ch);
    }
}

void do_wizhelp(CHAR_DATA *ch, char *argument) {
    (void)argument;
    columniser_t col;

    col.ch = ch;
    col.buf[0] = '\0';
    col.col_width = 12;
    col.max_width = 72;
    trie_enumerate(cmd_trie, LEVEL_HERO, get_trust(ch), command_enumerator, &col);
    if (col.buf[0] != '\0') {
        strcat(col.buf, "\n\r");
        send_to_char(col.buf, ch);
    }
}

bool MP_Commands(CHAR_DATA *ch) /* Can MOBProged mobs
                                   use mpcommands? TRUE if yes.
                                   - Kahn */
{
    if (is_switched(ch))
        return FALSE;

    if (IS_NPC(ch) && ch->pIndexData->progtypes && !IS_AFFECTED(ch, AFF_CHARM))
        return TRUE;

    return FALSE;
}
