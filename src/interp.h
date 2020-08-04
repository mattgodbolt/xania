/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                    */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  interp.h:  a list of commands and related data                       */
/*                                                                       */
/*************************************************************************/

#pragma once

#include "merc.h"

/* for command types */
#define ML MAX_LEVEL /* implementor */
#define L1 MAX_LEVEL - 1 /* creator */
#define L2 MAX_LEVEL - 2 /* supreme being */
#define L3 MAX_LEVEL - 3 /* deity */
#define L4 MAX_LEVEL - 4 /* god */
#define L5 MAX_LEVEL - 5 /* immortal */
#define L6 MAX_LEVEL - 6 /* demigod */
#define L7 MAX_LEVEL - 7 /* angel */
#define L8 MAX_LEVEL - 8 /* avatar */
#define IM LEVEL_IMMORTAL /* angel */
#define HE LEVEL_HERO /* hero */

extern void interp_initialise(void);

/*
 * Command functions.
 * Defined in act_*.c (mostly).
 */
extern void do_permban(CHAR_DATA *ch, char *arg);
extern void do_permit(CHAR_DATA *ch, char *arg);
extern void do_prowl(CHAR_DATA *ch, char *arg);
extern void do_sharpen(CHAR_DATA *ch, char *arg);
extern void do_ride(CHAR_DATA *ch, char *arg);
extern void do_dismount(CHAR_DATA *ch, char *arg);
extern void do_accept(CHAR_DATA *ch, char *arg);
extern void do_advance(CHAR_DATA *ch, char *arg);
extern void do_affected(CHAR_DATA *ch, char *arg);
extern void do_afk(CHAR_DATA *ch, char *arg);
extern void do_allege(CHAR_DATA *ch, char *arg);
extern void do_allow(CHAR_DATA *ch, char *arg);
extern void do_announce(CHAR_DATA *ch, char *arg);
extern void do_answer(CHAR_DATA *ch, char *arg);
extern void do_areas(CHAR_DATA *ch, char *arg);
extern void do_at(CHAR_DATA *ch, char *arg);
extern void do_auction(CHAR_DATA *ch, char *arg);
extern void do_autoaffect(CHAR_DATA *ch, char *arg);
extern void do_autoassist(CHAR_DATA *ch, char *arg);
extern void do_autoexit(CHAR_DATA *ch, char *arg);
extern void do_autogold(CHAR_DATA *ch, char *arg);
extern void do_autolist(CHAR_DATA *ch, char *arg);
extern void do_autoloot(CHAR_DATA *ch, char *arg);
extern void do_autopeek(CHAR_DATA *ch, char *arg);
extern void do_autosac(CHAR_DATA *ch, char *arg);
extern void do_autosplit(CHAR_DATA *ch, char *arg);
extern void do_awaken(CHAR_DATA *ch, char *arg);
extern void do_backstab(CHAR_DATA *ch, char *arg);
extern void do_bamfin(CHAR_DATA *ch, char *arg);
extern void do_bamfout(CHAR_DATA *ch, char *arg);
extern void do_ban(CHAR_DATA *ch, char *arg);
extern void do_bash(CHAR_DATA *ch, char *arg);
extern void do_berserk(CHAR_DATA *ch, char *arg);
extern void do_brandish(CHAR_DATA *ch, char *arg);
extern void do_brief(CHAR_DATA *ch, char *arg);
extern void do_bug(CHAR_DATA *ch, char *arg);
extern void do_buy(CHAR_DATA *ch, char *arg);
extern void do_cast(CHAR_DATA *ch, char *arg);
extern void do_challenge(CHAR_DATA *ch, char *arg);
extern void do_cancel_chal(CHAR_DATA *ch, char *arg);
extern void do_changes(CHAR_DATA *ch, char *arg);
extern void do_channels(CHAR_DATA *ch, char *arg);
/*void  do_clipboard(CHAR_DATA* ch, char* arg); */
extern void do_clone(CHAR_DATA *ch, char *arg);
extern void do_close(CHAR_DATA *ch, char *arg);
extern void do_colour(CHAR_DATA *ch, char *arg);
extern void do_coma(CHAR_DATA *ch, char *arg);
extern void do_commands(CHAR_DATA *ch, char *arg);
extern void do_combine(CHAR_DATA *ch, char *arg);
extern void do_compact(CHAR_DATA *ch, char *arg);
extern void do_compare(CHAR_DATA *ch, char *arg);
extern void do_consider(CHAR_DATA *ch, char *arg);
extern void do_count(CHAR_DATA *ch, char *arg);
extern void do_credits(CHAR_DATA *ch, char *arg);
extern void do_deaf(CHAR_DATA *ch, char *arg);
extern void do_delet(CHAR_DATA *ch, char *arg);
extern void do_delete(CHAR_DATA *ch, char *arg);
extern void do_deny(CHAR_DATA *ch, char *arg);
extern void do_description(CHAR_DATA *ch, char *arg);
extern void do_dirt(CHAR_DATA *ch, char *arg);
extern void do_disarm(CHAR_DATA *ch, char *arg);
extern void do_disconnect(CHAR_DATA *ch, char *arg);
extern void do_down(CHAR_DATA *ch, char *arg);
extern void do_drink(CHAR_DATA *ch, char *arg);
extern void do_drop(CHAR_DATA *ch, char *arg);
extern void do_duel(CHAR_DATA *ch, char *arg);
extern void do_dump(CHAR_DATA *ch, char *arg);
extern void do_east(CHAR_DATA *ch, char *arg);
extern void do_eat(CHAR_DATA *ch, char *arg);
extern void do_echo(CHAR_DATA *ch, char *arg);
extern void do_emote(CHAR_DATA *ch, char *arg);
extern void do_equipment(CHAR_DATA *ch, char *arg);
extern void do_examine(CHAR_DATA *ch, char *arg);
extern void do_exits(CHAR_DATA *ch, char *arg);
extern void do_fill(CHAR_DATA *ch, char *arg);
extern void do_pour(CHAR_DATA *ch, char *arg);
extern void do_finger(CHAR_DATA *ch, char *arg);
extern void do_flee(CHAR_DATA *ch, char *arg);
extern void do_follow(CHAR_DATA *ch, char *arg);
extern void do_force(CHAR_DATA *ch, char *arg);
extern void do_freeze(CHAR_DATA *ch, char *arg);
extern void do_gain(CHAR_DATA *ch, char *arg);
extern void do_get(CHAR_DATA *ch, char *arg);
extern void do_give(CHAR_DATA *ch, char *arg);
extern void do_gossip(CHAR_DATA *ch, char *arg);
extern void do_goto(CHAR_DATA *ch, char *arg);
extern void do_gratz(CHAR_DATA *ch, char *arg);
extern void do_group(CHAR_DATA *ch, char *arg);
extern void do_groups(CHAR_DATA *ch, char *arg);
extern void do_gtell(CHAR_DATA *ch, char *arg);
extern void do_hailcorpse(CHAR_DATA *ch, char *arg);
extern void do_headbutt(CHAR_DATA *ch, char *arg);
extern void do_heal(CHAR_DATA *ch, char *arg);
extern void do_help(CHAR_DATA *ch, char *arg);
extern void do_hide(CHAR_DATA *ch, char *arg);
extern void do_holylight(CHAR_DATA *ch, char *arg);
extern void do_idea(CHAR_DATA *ch, char *arg);
extern void do_immtalk(CHAR_DATA *ch, char *arg);
extern void do_immworth(CHAR_DATA *ch, char *arg);
extern void do_imotd(CHAR_DATA *ch, char *arg);
extern void do_inventory(CHAR_DATA *ch, char *arg);
extern void do_invis(CHAR_DATA *ch, char *arg);
extern void do_kick(CHAR_DATA *ch, char *arg);
extern void do_kill(CHAR_DATA *ch, char *arg);
extern void do_list(CHAR_DATA *ch, char *arg);
extern void do_load(CHAR_DATA *ch, char *arg);
extern void do_lock(CHAR_DATA *ch, char *arg);
extern void do_log(CHAR_DATA *ch, char *arg);
extern void do_look(CHAR_DATA *ch, char *arg);
extern void do_memory(CHAR_DATA *ch, char *arg);
extern void do_mfind(CHAR_DATA *ch, char *arg);
extern void do_mload(CHAR_DATA *ch, char *arg);

/* for MOBProgs */

extern void do_mpasound(CHAR_DATA *ch, char *arg);
extern void do_mpat(CHAR_DATA *ch, char *arg);
extern void do_mpecho(CHAR_DATA *ch, char *arg);
extern void do_mpechoaround(CHAR_DATA *ch, char *arg);
extern void do_mpechoat(CHAR_DATA *ch, char *arg);
extern void do_mpforce(CHAR_DATA *ch, char *arg);
extern void do_mpgoto(CHAR_DATA *ch, char *arg);
extern void do_mpjunk(CHAR_DATA *ch, char *arg);
extern void do_mpkill(CHAR_DATA *ch, char *arg);
extern void do_mpmload(CHAR_DATA *ch, char *arg);
extern void do_mpoload(CHAR_DATA *ch, char *arg);
extern void do_mppurge(CHAR_DATA *ch, char *arg);
extern void do_mpstat(CHAR_DATA *ch, char *arg);
extern void do_mptransfer(CHAR_DATA *ch, char *arg);

extern void do_mset(CHAR_DATA *ch, char *arg);
extern void do_mstat(CHAR_DATA *ch, char *arg);
extern void do_mwhere(CHAR_DATA *ch, char *arg);
extern void do_motd(CHAR_DATA *ch, char *arg);
extern void do_murde(CHAR_DATA *ch, char *arg);
extern void do_murder(CHAR_DATA *ch, char *arg);
extern void do_music(CHAR_DATA *ch, char *arg);
extern void do_newlock(CHAR_DATA *ch, char *arg);
extern void do_news(CHAR_DATA *ch, char *arg);
extern void do_mail(CHAR_DATA *ch, char *arg);
extern void _do_news(CHAR_DATA *ch, char *arg);
extern void do_nochannels(CHAR_DATA *ch, char *arg);
extern void do_noemote(CHAR_DATA *ch, char *arg);
extern void do_nofollow(CHAR_DATA *ch, char *arg);
extern void do_noloot(CHAR_DATA *ch, char *arg);
extern void do_north(CHAR_DATA *ch, char *arg);
extern void do_noshout(CHAR_DATA *ch, char *arg);
extern void do_nosummon(CHAR_DATA *ch, char *arg);
extern void do_notell(CHAR_DATA *ch, char *arg);
extern void do_ofind(CHAR_DATA *ch, char *arg);
extern void do_oload(CHAR_DATA *ch, char *arg);
extern void do_open(CHAR_DATA *ch, char *arg);
extern void do_order(CHAR_DATA *ch, char *arg);
extern void do_oset(CHAR_DATA *ch, char *arg);
extern void do_ostat(CHAR_DATA *ch, char *arg);
extern void do_owhere(CHAR_DATA *ch, char *arg);
extern void do_osearch(CHAR_DATA *ch, char *arg);
extern void do_outfit(CHAR_DATA *ch, char *arg);
extern void do_pardon(CHAR_DATA *ch, char *arg);
extern void do_password(CHAR_DATA *ch, char *arg);
extern void do_peace(CHAR_DATA *ch, char *arg);
extern void do_pecho(CHAR_DATA *ch, char *arg);
extern void do_peek(CHAR_DATA *ch, char *arg);
extern void do_philosophise(CHAR_DATA *ch, char *arg);
extern void do_pick(CHAR_DATA *ch, char *arg);
extern void do_pose(CHAR_DATA *ch, char *arg);
extern void do_practice(CHAR_DATA *ch, char *arg);
extern void do_prompt(CHAR_DATA *ch, char *arg);
extern void do_purge(CHAR_DATA *ch, char *arg);
extern void do_put(CHAR_DATA *ch, char *arg);
extern void do_quaff(CHAR_DATA *ch, char *arg);
extern void do_question(CHAR_DATA *ch, char *arg);
extern void do_enter(CHAR_DATA *ch, char *arg);
extern void do_qui(CHAR_DATA *ch, char *arg);
extern void do_quiet(CHAR_DATA *ch, char *arg);
extern void do_quit(CHAR_DATA *ch, char *arg);
extern void do_qwest(CHAR_DATA *ch, char *arg);
extern void do_read(CHAR_DATA *ch, char *arg);
extern void do_ready(CHAR_DATA *ch, char *arg);
extern void do_reboo(CHAR_DATA *ch, char *arg);
extern void do_reboot(CHAR_DATA *ch, char *arg);
extern void do_recall(CHAR_DATA *ch, char *arg);
extern void do_recho(CHAR_DATA *ch, char *arg);
extern void do_recite(CHAR_DATA *ch, char *arg);
extern void do_refuse(CHAR_DATA *ch, char *arg);
extern void do_remove(CHAR_DATA *ch, char *arg);
extern void do_reply(CHAR_DATA *ch, char *arg);
extern void do_report(CHAR_DATA *ch, char *arg);
extern void do_rescue(CHAR_DATA *ch, char *arg);
extern void do_rest(CHAR_DATA *ch, char *arg);
extern void do_restore(CHAR_DATA *ch, char *arg);
extern void do_return(CHAR_DATA *ch, char *arg);
extern void do_rset(CHAR_DATA *ch, char *arg);
extern void do_rstat(CHAR_DATA *ch, char *arg);
extern void do_rules(CHAR_DATA *ch, char *arg);
extern void do_sacname(CHAR_DATA *ch, char *arg);
extern void do_sacrifice(CHAR_DATA *ch, char *arg);
extern void do_save(CHAR_DATA *ch, char *arg);
extern void do_say(CHAR_DATA *ch, char *arg);
extern void do_score(CHAR_DATA *ch, char *arg);
extern void do_scroll(CHAR_DATA *ch, char *arg);
extern void do_sell(CHAR_DATA *ch, char *arg);
extern void do_set(CHAR_DATA *ch, char *arg);
extern void do_setinfo(CHAR_DATA *ch, char *arg);
extern void do_shout(CHAR_DATA *ch, char *arg);
extern void do_showafk(CHAR_DATA *ch, char *arg);
extern void do_showdefence(CHAR_DATA *ch, char *arg);
extern void do_shutdow(CHAR_DATA *ch, char *arg);
extern void do_shutdown(CHAR_DATA *ch, char *arg);
extern void do_sit(CHAR_DATA *ch, char *arg);
extern void do_skills(CHAR_DATA *ch, char *arg);
extern void do_sla(CHAR_DATA *ch, char *arg);
extern void do_slay(CHAR_DATA *ch, char *arg);
extern void do_sleep(CHAR_DATA *ch, char *arg);
extern void do_slookup(CHAR_DATA *ch, char *arg);
extern void do_smit(CHAR_DATA *ch, char *arg);
extern void do_smite(CHAR_DATA *ch, char *arg);
extern void do_sneak(CHAR_DATA *ch, char *arg);
extern void do_snoop(CHAR_DATA *ch, char *arg);
extern void do_socials(CHAR_DATA *ch, char *arg);
extern void do_south(CHAR_DATA *ch, char *arg);
extern void do_sockets(CHAR_DATA *ch, char *arg);
extern void do_spells(CHAR_DATA *ch, char *arg);
extern void do_specialise(CHAR_DATA *ch, char *arg);
extern void do_split(CHAR_DATA *ch, char *arg);
extern void do_sset(CHAR_DATA *ch, char *arg);
extern void do_stand(CHAR_DATA *ch, char *arg);
extern void do_stat(CHAR_DATA *ch, char *arg);
extern void do_steal(CHAR_DATA *ch, char *arg);
extern void do_story(CHAR_DATA *ch, char *arg);
extern void do_string(CHAR_DATA *ch, char *arg);
extern void do_switch(CHAR_DATA *ch, char *arg);
extern void do_tell(CHAR_DATA *ch, char *arg);
extern void do_time(CHAR_DATA *ch, char *arg);
extern void do_tipwizard(CHAR_DATA *ch, char *arg);
extern void do_tips(CHAR_DATA *ch, char *arg);
extern void do_title(CHAR_DATA *ch, char *arg);
extern void do_train(CHAR_DATA *ch, char *arg);
extern void do_transfer(CHAR_DATA *ch, char *arg);
extern void do_trip(CHAR_DATA *ch, char *arg);
extern void do_trust(CHAR_DATA *ch, char *arg);
extern void do_typo(CHAR_DATA *ch, char *arg);
extern void do_unlock(CHAR_DATA *ch, char *arg);
extern void do_up(CHAR_DATA *ch, char *arg);
extern void do_value(CHAR_DATA *ch, char *arg);
extern void do_visible(CHAR_DATA *ch, char *arg);
extern void do_vnum(CHAR_DATA *ch, char *arg);
extern void do_wake(CHAR_DATA *ch, char *arg);
extern void do_wear(CHAR_DATA *ch, char *arg);
extern void do_weather(CHAR_DATA *ch, char *arg);
extern void do_west(CHAR_DATA *ch, char *arg);
extern void do_where(CHAR_DATA *ch, char *arg);
extern void do_scan(CHAR_DATA *ch, char *arg);
extern void do_who(CHAR_DATA *ch, char *arg);
extern void do_whois(CHAR_DATA *ch, char *arg);
extern void do_wimpy(CHAR_DATA *ch, char *arg);
extern void do_wizhelp(CHAR_DATA *ch, char *arg);
extern void do_wizlock(CHAR_DATA *ch, char *arg);
extern void do_wizlist(CHAR_DATA *ch, char *arg);
extern void do_wiznet(CHAR_DATA *ch, char *arg);
extern void do_worth(CHAR_DATA *ch, char *arg);
extern void do_yell(CHAR_DATA *ch, char *arg);
extern void do_zap(CHAR_DATA *ch, char *arg);
extern void do_zecho(CHAR_DATA *ch, char *arg);
extern void do_throw(CHAR_DATA *ch, char *arg);
extern void do_prefix(CHAR_DATA *ch, char *arg); /* 'prefix' added PCFN 19-05-97 */
extern void do_timezone(CHAR_DATA *ch, char *arg); /* 'timezone' added PCFN 24-05-97 */
extern void do_donate(CHAR_DATA *ch, char *arg); /* 'donate' added PCFN 01.06.97 */

/* clan stuff */
extern void do_member(CHAR_DATA *ch, char *arg);
extern void do_promote(CHAR_DATA *ch, char *arg);
extern void do_demote(CHAR_DATA *ch, char *arg);
extern void do_noclanchan(CHAR_DATA *ch, char *arg);
extern void do_clantalk(CHAR_DATA *ch, char *arg);
extern void do_clanwho(CHAR_DATA *ch, char *arg);
extern void do_clanset(CHAR_DATA *ch, char *arg);
/* end of clan stuff */

/* Misc stuff */
extern void announce(char *, CHAR_DATA *ch);
bool check_social(CHAR_DATA *ch, char *command, char *arg);

/* OLC */

extern void do_aedit(CHAR_DATA *ch, char *arg); /* OLC 1.1b */
extern void do_redit(CHAR_DATA *ch, char *arg); /* OLC 1.1b */
extern void do_oedit(CHAR_DATA *ch, char *arg); /* OLC 1.1b */
extern void do_medit(CHAR_DATA *ch, char *arg); /* OLC 1.1b */
extern void do_mpedit(CHAR_DATA *ch, char *arg);
extern void do_hedit(CHAR_DATA *ch, char *arg);
extern void do_asave(CHAR_DATA *ch, char *arg);
extern void do_alist(CHAR_DATA *ch, char *arg);
extern void do_resets(CHAR_DATA *ch, char *arg);

extern void do_cpptest(CHAR_DATA *ch, char *arg);
