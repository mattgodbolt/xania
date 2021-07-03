/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                    */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  interp.h:  a list of commands and related data                       */
/*                                                                       */
/*************************************************************************/

#pragma once

#include "ArgParser.hpp"

#include <string_view>

struct Char;
struct Room;

void interp_initialise();
bool check_social(Char *ch, std::string_view command, std::string_view argument);
void interpret(Char *ch, const char *argument);
const char *one_argument(const char *argument, char *arg_first);
char *one_argument(char *argument, char *arg_first); // TODO(MRG) get rid of this as soon as we can.

/*
 * Command functions.
 * Defined in act_*.cpp (mostly).
 */
extern void do_permban(Char *ch, const char *arg);
extern void do_permit(Char *ch, const char *arg);
extern void do_prowl(Char *ch, const char *arg);
extern void do_sharpen(Char *ch);
extern void do_ride(Char *ch, const char *arg);
extern void do_dismount(Char *ch);
extern void do_accept(Char *ch);
extern void do_advance(Char *ch, const char *argument);
extern void do_affected(Char *ch);
extern void do_afk(Char *ch, std::string_view argument);
extern void do_alist(Char *ch);
extern void do_allege(Char *ch, const char *arg);
extern void do_allow(Char *ch, const char *arg);
extern void do_announce(Char *ch);
extern void do_answer(Char *ch, const char *arg);
extern void do_areas(Char *ch, ArgParser args);
extern void do_at(Char *ch, const char *arg);
extern void do_auction(Char *ch, const char *arg);
extern void do_autoaffect(Char *ch);
extern void do_autoassist(Char *ch);
extern void do_autoexit(Char *ch);
extern void do_autogold(Char *ch);
extern void do_autolist(Char *ch);
extern void do_autoloot(Char *ch);
extern void do_autopeek(Char *ch);
extern void do_autosac(Char *ch);
extern void do_autosplit(Char *ch);
extern void do_awaken(Char *ch, const char *arg);
extern void do_backstab(Char *ch, const char *arg);
extern void do_bamfin(Char *ch, const char *argument);
extern void do_bamfout(Char *ch, const char *argument);
extern void do_ban(Char *ch, const char *arg);
extern void do_bash(Char *ch, const char *arg);
extern void do_berserk(Char *ch);
extern void do_brandish(Char *ch);
extern void do_brief(Char *ch);
extern void do_bug(Char *ch, const char *arg);
extern void do_buy(Char *ch, const char *arg);
extern void do_cast(Char *ch, const char *argument);
extern void do_challenge(Char *ch, const char *arg);
extern void do_cancel_chal(Char *ch, const char *arg);
extern void do_changes(Char *ch);
extern void do_channels(const Char *ch);
extern void do_clone(Char *ch, const char *arg);
extern void do_close(Char *ch, ArgParser args);
extern void do_colour(Char *ch);
extern void do_coma(Char *ch, const char *arg);
extern void do_commands(Char *ch);
extern void do_combine(Char *ch);
extern void do_compact(Char *ch);
extern void do_compare(Char *ch, ArgParser args);
extern void do_consider(Char *ch, const char *arg);
extern void do_count(Char *ch);
extern void do_credits(Char *ch);
extern void do_delet(Char *ch);
extern void do_delete(Char *ch, const char *argument);
extern void do_deny(Char *ch, const char *argument);
extern void do_description(Char *ch, const char *argument);
extern void do_dirt(Char *ch, const char *arg);
extern void do_disarm(Char *ch);
extern void do_disconnect(Char *ch, const char *arg);
extern void do_down(Char *ch);
extern void do_drink(Char *ch, const char *arg);
extern void do_drop(Char *ch, const char *arg);
extern void do_duel(Char *ch, const char *arg);
extern void do_dump(Char *ch);
extern void do_east(Char *ch);
extern void do_eat(Char *ch, const char *argument);
extern void do_echo(Char *ch, std::string_view argument);
extern void do_emote(Char *ch, const char *arg);
extern void do_equipment(Char *ch);
extern void do_examine(Char *ch, ArgParser args);
extern void do_exits(const Char *ch, const char *argument);
extern void do_fill(Char *ch, const char *arg);
extern void do_pour(Char *ch, const char *arg);
extern void do_finger(Char *ch, ArgParser args);
extern void do_flee(Char *ch);
extern void do_follow(Char *ch, ArgParser args);
extern void do_force(Char *ch, const char *argument);
extern void do_freeze(Char *ch, const char *arg);
extern void do_gain(Char *ch, const char *arg);
extern void do_get(Char *ch, const char *arg);
extern void do_give(Char *ch, const char *arg);
extern void do_gossip(Char *ch, const char *arg);
extern void do_goto(Char *ch, const char *arg);
extern void do_gratz(Char *ch, const char *arg);
extern void do_group(Char *ch, const char *arg);
extern void do_groups(Char *ch, const char *arg);
extern void do_gtell(Char *ch, std::string_view argument);
extern void do_hailcorpse(Char *ch);
extern void do_headbutt(Char *ch, const char *arg);
extern void do_heal(Char *ch, const char *arg);
extern void do_help(Char *ch, const char *arg);
extern void do_hide(Char *ch);
extern void do_holylight(Char *ch);
extern void do_idea(Char *ch, const char *arg);
extern void do_immtalk(Char *ch, std::string_view argument);
extern void do_immworth(Char *ch, const char *arg);
extern void do_imotd(Char *ch);
extern void do_inventory(Char *ch);
extern void do_invis(Char *ch, const char *arg);
extern void do_kick(Char *ch, const char *arg);
extern void do_kill(Char *ch, const char *arg);
extern void do_list(Char *ch, const char *arg);
extern void do_load(Char *ch, const char *argument);
extern void do_lock(Char *ch, ArgParser args);
extern void do_log(Char *ch, const char *arg);
extern void do_look(Char *ch, ArgParser args);
extern void look_auto(Char *ch);
extern void do_memory(Char *ch);

/* for MOBProgs */

extern void do_mpasound(Char *ch, const char *arg);
extern void do_mpat(Char *ch, const char *arg);
extern void do_mpecho(Char *ch, const char *arg);
extern void do_mpechoaround(Char *ch, const char *arg);
extern void do_mpechoat(Char *ch, const char *arg);
extern void do_mpforce(Char *ch, const char *arg);
extern void do_mpgoto(Char *ch, const char *arg);
extern void do_mpjunk(Char *ch, const char *arg);
extern void do_mpkill(Char *ch, const char *arg);
extern void do_mpmload(Char *ch, const char *arg);
extern void do_mpoload(Char *ch, const char *arg);
extern void do_mppurge(Char *ch, const char *arg);
extern void do_mpstat(Char *ch, const char *arg);
extern void do_mptransfer(Char *ch, const char *arg);

extern void do_mset(Char *ch, const char *argument);
extern void do_mstat(Char *ch, const char *arg);
extern void do_mwhere(Char *ch, const char *arg);
extern void do_motd(Char *ch);
extern void do_murde(Char *ch);
extern void do_murder(Char *ch, const char *arg);
extern void do_music(Char *ch, const char *arg);
extern void do_newlock(Char *ch);
extern void do_nochannels(Char *ch, const char *arg);
extern void do_noemote(Char *ch, const char *arg);
extern void do_nofollow(Char *ch);
extern void do_noloot(Char *ch);
extern void do_north(Char *ch);
extern void do_noshout(Char *ch, const char *arg);
extern void do_nosummon(Char *ch);
extern void do_notell(Char *ch, const char *arg);
extern void do_open(Char *ch, ArgParser args);
extern void do_order(Char *ch, const char *argument);
extern void do_oset(Char *ch, const char *argument);
extern void do_ostat(Char *ch, const char *arg);
extern void do_owhere(Char *ch, const char *arg);
extern void do_osearch(Char *ch, const char *arg);
extern void do_outfit(Char *ch);
extern void do_pardon(Char *ch, const char *arg);
extern void do_password(Char *ch, const char *arg);
extern void do_peace(Char *ch);
extern void do_pecho(Char *ch, const char *arg);
extern void do_peek(Char *ch, const char *arg);
extern void do_philosophise(Char *ch, const char *arg);
extern void do_pick(Char *ch, ArgParser args);
extern void do_pose(Char *ch);
extern void do_practice(Char *ch, const char *arg);
extern void do_prompt(Char *ch, const char *arg);
extern void do_purge(Char *ch, const char *argument);
extern void do_put(Char *ch, const char *arg);
extern void do_quaff(Char *ch, const char *arg);
extern void do_question(Char *ch, const char *arg);
extern void do_enter(Char *ch, std::string_view argument);
extern void do_pronouns(Char *ch, ArgParser args);
extern void do_qui(Char *ch);
extern void do_quiet(Char *ch);
extern void do_quit(Char *ch);
extern void do_qwest(Char *ch, const char *arg);
extern void do_ready(Char *ch);
extern void do_reboo(Char *ch);
extern void do_reboot(Char *ch);
extern void do_recall(Char *ch, ArgParser args);
extern void do_recho(Char *ch, const char *arg);
extern void do_recite(Char *ch, const char *arg);
extern void do_refuse(Char *ch);
extern void do_remove(Char *ch, const char *arg);
extern void do_reply(Char *ch, const char *arg);
extern void do_report(Char *ch);
extern void do_rescue(Char *ch, const char *arg);
extern void do_rest(Char *ch);
extern void do_restore(Char *ch, const char *arg);
extern void do_return(Char *ch);
extern void do_rset(Char *ch, const char *argument);
extern void do_rstat(Char *ch, std::string_view argument);
extern void do_rules(Char *ch);
extern void do_sacname(Char *ch, ArgParser args);
extern void do_sacrifice(Char *ch, const char *arg);
extern void do_save(Char *ch);
extern void do_say(Char *ch, const char *argument);
extern void do_score(Char *ch);
extern void do_scroll(Char *ch, ArgParser args);
extern void do_sell(Char *ch, const char *arg);
extern void do_set(Char *ch, const char *arg);
extern void do_setinfo(Char *ch, const char *arg);
extern void do_shout(Char *ch, const char *arg);
extern void do_showafk(Char *ch);
extern void do_showdefence(Char *ch);
extern void do_shutdow(Char *ch);
extern void do_shutdown(Char *ch);
extern void do_sit(Char *ch);
extern void do_skills(Char *ch);
extern void do_sla(Char *ch);
extern void do_slay(Char *ch, const char *arg);
extern void do_sleep(Char *ch);
extern void do_slookup(Char *ch, const char *arg);
extern void do_smit(Char *ch);
extern void do_smite(Char *ch, const char *arg);
extern void do_sneak(Char *ch);
extern void do_snoop(Char *ch, const char *arg);
extern void do_socials(Char *ch);
extern void do_south(Char *ch);
extern void do_sockets(Char *ch, const char *arg);
extern void do_spells(Char *ch);
extern void do_split(Char *ch, ArgParser args);
extern void split_coins(Char *ch, int amount);
extern void do_sset(Char *ch, const char *argument);
extern void do_stand(Char *ch);
extern void do_stat(Char *ch, const char *arg);
extern void do_steal(Char *ch, const char *arg);
extern void do_story(Char *ch);
extern void do_string(Char *ch, const char *argument);
extern void do_switch(Char *ch, const char *arg);
extern void do_tell(Char *ch, const char *argument);
extern void do_time(Char *ch);
extern void do_tipwizard(Char *ch, ArgParser args);
extern void do_title(Char *ch, const char *arg);
extern void do_train(Char *ch, ArgParser args);
extern void do_transfer(Char *ch, ArgParser args);
extern void do_tras(Char *ch, ArgParser args);
extern void do_trash(Char *ch, ArgParser args);
extern void transfer(const Char *imm, Char *victim, Room *location);
extern void do_trip(Char *ch, const char *arg);
extern void do_trust(Char *ch, const char *arg);
extern void do_typo(Char *ch, const char *arg);
extern void do_unlock(Char *ch, ArgParser args);
extern void do_up(Char *ch);
extern void do_value(Char *ch, const char *arg);
extern void do_visible(Char *ch);
extern void do_vnum(Char *ch, const char *arg);
extern void do_wake(Char *ch, ArgParser args);
extern void do_wear(Char *ch, const char *arg);
extern void do_weather(Char *ch);
extern void do_west(Char *ch);
extern void do_west(Char *ch);
extern void do_where(Char *ch, const char *arg);
extern void do_scan(Char *ch);
extern void do_who(Char *ch, const char *arg);
extern void do_whois(Char *ch, ArgParser args);
extern void do_wimpy(Char *ch, ArgParser args);
extern void do_wizhelp(Char *ch);
extern void do_wizlock(Char *ch);
extern void do_wizlist(Char *ch);
extern void do_wiznet(Char *ch, const char *arg);
extern void do_worth(Char *ch);
extern void do_yell(Char *ch, std::string_view argument);
extern void do_zap(Char *ch, const char *arg);
extern void do_zecho(Char *ch, const char *arg);
extern void do_throw(Char *ch, const char *arg);
extern void do_prefix(Char *ch, const char *argument); /* 'prefix' added PCFN 19-05-97 */
extern void do_donate(Char *ch, const char *arg); /* 'donate' added PCFN 01.06.97 */

/* clan stuff */
extern void do_member(Char *ch, const char *arg);
extern void do_promote(Char *ch, const char *arg);
extern void do_demote(Char *ch, const char *arg);
extern void do_noclanchan(Char *ch, const char *arg);
extern void do_clantalk(Char *ch, const char *arg);
extern void do_clanwho(Char *ch);
extern void do_clanset(Char *ch, const char *arg);
/* end of clan stuff */
