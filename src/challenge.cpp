/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                 */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  challeng.c: supervised player combat. Originally by Rohan and        */
/*              Wandera. Revised by Oshea 26/8/96                        */
/*************************************************************************/

#include "challenge.hpp"
#include "Char.hpp"
#include "Classes.hpp"
#include "Descriptor.hpp"
#include "InjuredPart.hpp"
#include "Logging.hpp"
#include "Races.hpp"
#include "Room.hpp"
#include "VnumRooms.hpp"
#include "act_comm.hpp"
#include "comm.hpp"
#include "db.h"
#include "fight.hpp"
#include "handler.hpp"
#include "interp.h"

#include <fmt/format.h>

#include <cstdio>
#include <cstring>
#include <ctime>
#include <sys/types.h>

/* Some local DEFINES to keep things general. */
#define NAME_SIZE 30

/* Private functions. */
static int check_duel_status(int);

/* Challenge variables.  Lets make them local to this file. */
namespace {
Char *challenger = nullptr;
Char *challengee = nullptr;
Char *imm = nullptr;
std::string imm_name;
std::string challenger_name;
std::string challengee_name;
int imm_ready = 0;
int challenger_ready = 0;
int challengee_ready = 0;
int challenge_ticker;
bool challenge_active = false;
bool challenge_fighting = false;
}

/* And now on with the code. */

void do_challenge(Char *ch, const char *argument) {
    Char *victim;
    char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);

    if (ch->level < 20) {
        ch->send_line("|cYou are too inexperienced to duel to the death.|w");
        return;
    }

    if (ch->level > 91) {
        ch->send_line("|cGods should not fight amongst themselves.|w");
        return;
    }

    if (arg[0] == '\0') {
        ch->send_line("|cChallenge whom?|w");
        return;
    }

    if ((victim = get_char_world(ch, arg)) == nullptr) {
        ch->send_line("|cThat player does not exist!|w");
        return;
    }

    if (victim->is_npc()) {
        ch->send_line("|cYou cannot challenge non-player characters.|w");
        return;
    }

    if (victim->desc == nullptr && victim->is_pc()) {
        act("|W$N|c seems to have misplaced $S link...try again later.|w", ch, nullptr, victim, To::Char);
        return;
    }

    if (ch->leader != nullptr) { /* This might need to be checked.  It
                                 is set when you are grouped as well! */
        ch->leader->send_line("|cYou cannot challenge with pets or charmed mobs.|w");
        return;
    }

    if (ch->desc->is_switched() || victim->desc->is_switched()) {
        ch->send_line("|cYou cannot challenge with switched mobs.|w");
        return;
    }

    if (victim->level < 20) {
        ch->send_line("|cPick on somebody your own size.|w");
        return;
    }

    if (victim->level > 91) {
        ch->send_line("|cYou should not fight with your Gods.|w");
        return;
    }

    if (ch == victim) {
        ch->send_line("|cIf you fight yourself you shall lose.|w");
        return;
    }

    /* this, along with a few other tweaks - paranoia code! Challenge has
       been prone to crashes for a variety of reasons...this will help
       reduce such problems --Fara */
    if (victim->is_pos_fighting()) {
        ch->send_to("|cYou pray for the right to duel and the Gods inform you that your opponent\n\ris already "
                    "engaged in combat.|w\n\r");
        return;
    }

    if (challenger != nullptr) {
        ch->send_line("|cSorry, a challenge is about to, or is taking place. Please try again later.|w");
        return;
    }

    challengee = victim;
    challenger = ch;
    /* Another bit of paranoia  -  Oshea */
    imm = nullptr;
    challenge_active = false;
    challenge_fighting = false;

    challenger->send_line("|cYou pray for the right to duel with {}.", challengee->name);
    do_immtalk(ch, fmt::format("|Ghas challenged {}, any imm takers? (type accept)|w", victim->name));
    challenger_name = challenger->name;
    challengee_name = challengee->name;
    challenge_ticker = 4;
}

void do_accept(Char *ch) {
    if ((challenger == nullptr) || (((challengee != ch) || (imm == nullptr)) && (ch->level <= 91))) {
        ch->send_line("|cSorry, there is no challenge for you to accept.|w");
        return;
    }

    if (ch->level < 95 && imm == nullptr) {
        ch->send_line("|cYou have to be level 95 or higher to control a challenge.|w");
        return;
    }

    if (imm != nullptr && imm != ch) {
        if (ch->level > 94) {
            ch->send_line("|cSorry, an imm has already accepted to control the challenge.|w");
            return;
        }

        imm->send_line("|cType |gready|c when you are ready to be transfered.|w");
        challenger->send_to(
            "|cThe challenge has been accepted, please wait for the immortals to prepare\n|cthe challenge room.|w\n\r");
        challengee->send_to(
            "|cThe challenge has been accepted, please wait for the immortals to prepare\n|cthe challenge room.|w\n\r");
        challenge_ticker = 4;
        return;
    }

    ch->send_to(
        fmt::format("|cYou have accepted to control the challenge.\n\rNow asking {} if they wish to duel.|w\n\r",
                    challengee->name));

    if (imm == nullptr) {
        challenger->send_to("|g{}|c has accepted to control the challenge. Now waiting to see\n\r"
                            "if |g{}|c will accept your challenge.|w\n\r",
                            pers(ch, challenger), challengee->name);
    }

    challengee->send_line("|CYou have been challenged to a duel to the death by |G{}.", challenger->name);

    challengee->send_to(fmt::format("|c{}'s stats are: level:{} class:{} race:{} alignment:{}.\n\r", challenger->name,
                                    challenger->level, class_table[challenger->class_num].name,
                                    race_table[challenger->race].name, challenger->alignment));
    challengee->send_to("|cType |paccept |cor |grefuse.\n\r|w");
    imm = ch;
    imm_name = imm->name;
}

void do_refuse(Char *ch) {
    if (ch != imm && ch != challenger && ch != challengee) {
        ch->send_line("|cRefuse what?|w");
        return;
    }

    if (ch == challengee && imm != nullptr) {
        ch->send_line("|cYou have refused to fight to the death with {}.|w", challenger->name);
        imm->send_to(
            fmt::format("|c{} has refused to fight to the death with {}.|w\n\r", challengee->name, challenger->name));
        challenger->send_line("|c{} has refused to fight to the death with you.|w", challengee->name);

        challenger = nullptr;
        challengee = nullptr;
        imm = nullptr;
        challenge_ticker = 0;
        return;
    }

    if (ch == challenger) {
        ch->send_line("You can't withdraw your challenge like that!");
    }

    /* Else it is imm who is trying to refuse! */
    ch->send_line("You can't withdraw from control once you have accepted it.");
}

void do_ready(Char *ch) {
    if (check_duel_status(1))
        return;

    if (ch != imm && ch != challenger && ch != challengee) {
        ch->send_line("|cWhat are you ready for?|w");
        return;
    }

    if ((ch == challenger || ch == challengee) && imm_ready != 1) {
        ch->send_line("|cWhat are you ready for?|w");
        return;
    }

    auto prep_room = get_room(rooms::ChallengePrep);
    if (!prep_room) {
        bug("Unable to find CHAL_PREP room!");
        return;
    }
    if (ch == imm && imm_ready == 0) {
        transfer(imm, ch, prep_room);
        imm_ready = 1;
        challenge_ticker = 4;
        auto msg = "|cType |gready|c when you are ready to be transfered.|w\n\r";
        challenger->send_to(msg);
        challengee->send_to(msg);
    }

    if (ch == challenger && challenger_ready == 0) {
        transfer(imm, ch, prep_room);
        challenger_ready = 1;
    }

    if (ch == challengee && challengee_ready == 0) {
        transfer(imm, ch, prep_room);
        challengee_ready = 1;
    }

    if (challenge_ticker > 0) {
        if (imm_ready == 1 && challenger_ready == 1 && challengee_ready == 1) {
            auto msg = fmt::format("|W### Go to the |Pviewing gallery|W to watch a duel between {} and {}.|w",
                                   challenger->name, challengee->name);
            announce(msg.c_str(), imm);
            imm->send_to("|CRemember that you can |Gcancel|C the challenge at any time should it be\nnecessary to do "
                         "so. (use cancel + challenger name)\n\r|w");
            challenge_ticker = 0;
            challenge_active = true;
            return;
        }
    }
}

void do_chal_tick() {
    check_duel_status(0);

    if (challenge_ticker == 0)
        return;
    else {
        challenge_ticker--;
        if (challenge_ticker == 0) {
            if (imm != nullptr && challenger != nullptr && challengee != nullptr) {
                /* Update this at some point to make it more friendly. End
                   duel silently less often. */
                const auto buf = "|cThe challenge has been cancelled.|w";
                imm->send_line(buf);
                challenger->send_line(buf);
                challengee->send_line(buf);
                stop_fighting(challenger, true);
                stop_fighting(challengee, true);
                challenger = nullptr;
                challengee = nullptr;
                imm = nullptr;
                imm_ready = 0;
                challenger_ready = 0;
                challengee_ready = 0;
                challenge_active = false;
                return;
            }
            bug("do_chal_tick: either the imm, or challengee/r are null");
            imm = nullptr;
            imm_ready = 0;
            challenger_ready = 0;
            challengee_ready = 0;
            challenger = nullptr;
            challengee = nullptr;
            challenge_active = false;
            challenge_fighting = false;
            return;
        }
        const auto buf =
            fmt::format("|CThere are {} ticks left before the challenge is cancelled.|w", challenge_ticker);
        if (imm != nullptr)
            imm->send_line(buf);
        if (imm != nullptr && imm_ready != 0)
            challengee->send_line(buf);
        /* Don't want to only tell chellenger when control has been
           accepted.  Warn him if he is on. Need to change this to check
           that challenger is still here. */
        if (imm_ready != 0)
            challenger->send_line(buf);
    }
}

void do_chal_canc(Char *ch) {
    /* We don't check status here since we are going to quit the duel
       anyway! */

    /* Ugly.  May need to alter this later! */
    if (ch != imm && ch != challenger && ch != challengee)
        return;

    auto msg = fmt::format("|c{} has either quit or lost their link.|w", ch->name);
    if (imm != nullptr && get_char_world(ch, imm_name))
        imm->send_line(msg);
    if (get_char_world(ch, challenger_name))
        challenger->send_to(msg);
    if (get_char_world(ch, challengee_name))
        challengee->send_to(msg);
    challenge_ticker = 1;
    do_chal_tick();
}

void do_cancel_chal(Char *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    Char *victim;
    one_argument(argument, arg);
    victim = get_char_world(ch, arg);

    if (check_duel_status(1))
        return;

    if (ch != imm) {
        ch->send_line("|cYou can't cancel anything.|w");
        return;
    }

    if (imm_ready != 1 || challenger_ready != 1 || challengee_ready != 1) {
        imm->send_line("|cEveryone must be ready to cancel the challenge.|w");
        return;
    }

    if (victim == challenger) {
        imm->send_line("|cYou have cancelled the challenge.|w");
        challenge_ticker = 1;
        do_chal_tick();
    } else {
        imm->send_line("|cTo cancel the challenge, you must specify the challenger.|w");
        return;
    }
}

void do_duel(Char *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    Char *victim;
    one_argument(argument, arg);
    victim = get_char_world(ch, arg);

    if (check_duel_status(1))
        return;

    if (ch != challenger && ch != challengee) {
        ch->send_line("|cYou are not in a challenge to duel.|w");
        return;
    }

    if (arg[0] == '\0') {
        ch->send_line("|cDuel who?|w");
        return;
    }

    if (ch->in_room->vnum != rooms::ChallengeArena) {
        ch->send_line("|cYou must be in the challenge arena to duel.|w");
        return;
    }

    if (imm_ready != 1 || challenger_ready != 1 || challengee_ready != 1) {
        ch->send_line("|cNot everyone is ready yet.|w");
        return;
    }

    if ((ch == challenger && victim == challengee) || (ch == challengee && victim == challenger)) {
        if (ch->fighting != nullptr || victim->fighting != nullptr)
            return;
        challenge_fighting = true;
        multi_hit(ch, victim);
        return;
    }

    ch->send_line("|cYou did not challenge that person.|w");
}

namespace {
void tell_results_to(int who) {
    if (who == 0) {
        challenger->send_line("|cYou have lost your fight to the death with {}.|w", challengee->name);
        challengee->send_to(
            fmt::format("|cCongratulations! You have won the fight to the death with {}.|w\n\r", challenger->name));
    }

    if (who == 1) {
        challengee->send_line("|cYou have lost your fight to the death with {}.|w", challenger->name);
        challenger->send_to(
            fmt::format("|cCongratulations! You have won the fight to the death with {}.|w\n\r", challengee->name));
    }
}
}

int do_check_chal(Char *ch) {
    int who;

    /* Just check the status here in case of error.  May want to do
       better error correction later if we discover a problem. */
    check_duel_status(0);

    if (ch == nullptr)
        return 0;

    if (ch != challenger && ch != challengee)
        return 0;

    if (ch == challenger)
        who = 0;
    else
        who = 1;

    raw_kill(ch, std::nullopt);
    if (imm == nullptr) {
        bug("do_check_chal: crash potential here guys, challengee/r is being killed...");
        return 0;
    }
    auto altar = get_room(rooms::MidgaardAltar);
    if (!altar) {
        bug("do_check_chal: couldn't find altar!");
        return 0;
    }
    transfer(imm, ch, altar);

    announce(fmt::format("|W### |P{}|W was defeated in a duel to the death with |P{}|W.|w", ch->name,
                         who == 0 ? challengee->name : challenger->name),
             imm);

    tell_results_to(who);

    imm->send_line("|cChallenge over. Challenge variables reset. New challenge can now be initiated.|w");

    challenger = nullptr;
    challengee = nullptr;
    imm = nullptr;
    imm_ready = 0;
    challenger_ready = 0;
    challengee_ready = 0;
    challenge_active = false;
    challenge_fighting = false;
    challenge_ticker = 0;
    return 1;
}

void do_room_check(Char *ch) {
    if (check_duel_status(1))
        return;

    if (ch != challenger && ch != challengee)
        return;

    challenge_ticker = 1;
    do_chal_tick();
}

void do_flee_check(Char *ch) {
    int who;

    /* Just check status.  Add more error correction in future
       possibly. */
    check_duel_status(0);

    if (challenger == nullptr || challengee == nullptr)
        return;

    if (ch != challenger && ch != challengee)
        return;

    if (!challenge_active)
        return;

    if (ch == challenger)
        who = 0;
    else
        who = 1;

    announce(fmt::format("|W### |P{} |Whas cowardly fled from |P{}|W.", ch->name,
                         who == 0 ? challengee->name : challenger->name),
             imm);

    tell_results_to(who);

    imm->send_line("|cChallenge over. Challenge variables reset. New challenge can now be initiated.|w");

    challenger = nullptr;
    challengee = nullptr;
    imm = nullptr;
    imm_ready = 0;
    challenger_ready = 0;
    challengee_ready = 0;
    challenge_fighting = false;
    challenge_active = false;
    challenge_ticker = 0;
}

int fighting_duel(Char *ch, Char *victim) {
    if (!challenge_fighting)
        return false;
    if (ch->in_room->vnum != rooms::ChallengeArena)
        return false;
    if ((ch != challenger) && (ch != challengee))
        return false;
    if ((victim != challenger) && (victim != challengee))
        return false;
    return true;
}

int in_duel(const Char *ch) {
    if (!challenge_fighting)
        return false;
    if (ch->in_room->vnum != rooms::ChallengeArena)
        return false;
    return ch == challenger || ch == challengee;
}

/* Checks to see that various things are correct.  Only input is
   toggle to say what to do on error.  0=nothing, 1=abort duel. */
static int check_duel_status(int f) {
    unsigned int flag = 0;

    if (challenger && challenger->name != challenger_name)
        flag |= 1u;

    if (challengee && challengee->name != challengee_name)
        flag |= 2u;

    if (imm && imm->name != imm_name)
        flag |= 4u;

    if (!flag)
        return 0;

    bug("check_duel_status: Name match failure. Code {}", flag);
    if (f) {
        challenge_ticker = 1;
        do_chal_tick();
    }
    return 1;
}
