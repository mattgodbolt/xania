/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                    */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  challeng.c: supervised player combat. Originally by Rohan and        */
/*              Wandera. Revised by Oshea 26/8/96                        */
/*************************************************************************/

#include "challeng.h"
#include "Descriptor.hpp"
#include "comm.hpp"
#include "handler.hpp"
#include "interp.h"
#include "merc.h"

#include <fmt/format.h>

#include <cstdio>
#include <cstring>
#include <ctime>
#include <sys/types.h>

using namespace fmt::literals;

/* Some local DEFINES to keep things general. */
#define NAME_SIZE 30

/* Private functions. */
static int check_duel_status(int);

/* Challenge variables.  Lets make them local to this file. */
namespace {
CHAR_DATA *challenger = nullptr;
CHAR_DATA *challengee = nullptr;
CHAR_DATA *imm = nullptr;
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

void do_challenge(CHAR_DATA *ch, const char *argument) {
    CHAR_DATA *victim;
    char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);

    if (ch->level < 20) {
        send_to_char("|cYou are too inexperienced to duel to the death.|w\n\r", ch);
        return;
    }

    if (ch->level > 91) {
        send_to_char("|cGods should not fight amongst themselves.|w\n\r", ch);
        return;
    }

    if (arg[0] == '\0') {
        send_to_char("|cChallenge whom?|w\n\r", ch);
        return;
    }

    if ((victim = get_char_world(ch, arg)) == nullptr) {
        send_to_char("|cThat player does not exist!|w\n\r", ch);
        return;
    }

    if (victim->is_npc()) {
        send_to_char("|cYou cannot challenge non-player characters.|w\n\r", ch);
        return;
    }

    if (victim->desc == nullptr && victim->is_pc()) {
        act("|W$N|c seems to have misplaced $S link...try again later.|w", ch, nullptr, victim, To::Char);
        return;
    }

    if (ch->leader != nullptr) { /* This might need to be checked.  It
                                 is set when you are grouped as well! */
        send_to_char("|cYou cannot challenge with pets or charmed mobs.|w\n\r", ch->leader);
        return;
    }

    if (ch->desc->is_switched() || victim->desc->is_switched()) {
        send_to_char("|cYou cannot challenge with switched mobs.|w\n\r", ch);
        return;
    }

    if (victim->level < 20) {
        send_to_char("|cPick on somebody your own size.|w\n\r", ch);
        return;
    }

    if (victim->level > 91) {
        send_to_char("|cYou should not fight with your Gods.|w\n\r", ch);
        return;
    }

    if (ch == victim) {
        send_to_char("|cIf you fight yourself you shall lose.|w\n\r", ch);
        return;
    }

    /* this, along with a few other tweaks - paranoia code! Challenge has
       been prone to crashes for a variety of reasons...this will help
       reduce such problems --Fara */
    if (victim->position == POS_FIGHTING) {
        send_to_char("|cYou pray for the right to duel and the Gods inform you that your opponent\n\ris already "
                     "engaged in combat.|w\n\r",
                     ch);
        return;
    }

    if (challenger != nullptr) {
        send_to_char("|cSorry, a challenge is about to, or is taking place. Please try again later.|w\n\r", ch);
        return;
    }

    challengee = victim;
    challenger = ch;
    /* Another bit of paranoia  -  Oshea */
    imm = nullptr;
    challenge_active = false;
    challenge_fighting = false;

    challenger->send_to("|cYou pray for the right to duel with {}.\n\r"_format(challengee->name));
    do_immtalk(ch, "|Ghas challenged {}, any imm takers? (type accept)|w"_format(victim->name));
    challenger_name = challenger->name;
    challengee_name = challengee->name;
    challenge_ticker = 4;
}

void do_accept(CHAR_DATA *ch, const char *argument) {
    (void)argument;

    if ((challenger == nullptr) || (((challengee != ch) || (imm == nullptr)) && (ch->level <= 91))) {
        ch->send_to("|cSorry, there is no challenge for you to accept.|w\n\r");
        return;
    }

    if (ch->level < 95 && imm == nullptr) {
        ch->send_to("|cYou have to be level 95 or higher to control a challenge.|w\n\r");
        return;
    }

    if (imm != nullptr && imm != ch) {
        if (ch->level > 94) {
            ch->send_to("|cSorry, an imm has already accepted to control the challenge.|w\n\r");
            return;
        }

        imm->send_to("|cType |gready|c when you are ready to be transfered.|w\n\r");
        challenger->send_to(
            "|cThe challenge has been accepted, please wait for the immortals to prepare\n|cthe challenge room.|w\n\r");
        challengee->send_to(
            "|cThe challenge has been accepted, please wait for the immortals to prepare\n|cthe challenge room.|w\n\r");
        challenge_ticker = 4;
        return;
    }

    ch->send_to("|cYou have accepted to control the challenge.\n\rNow asking {} if they wish to duel.|w\n\r"_format(
        challengee->name));

    if (imm == nullptr) {
        challenger->send_to(
            "|g{}|c has accepted to control the challenge. Now waiting to see\n\r"
            "if |g{}|c will accept your challenge.|w\n\r"_format(pers(ch, challenger), challengee->name));
    }

    challengee->send_to("|CYou have been challenged to a duel to the death by |G{}.\n\r"_format(challenger->name));

    challengee->send_to("|c{}'s stats are: level:{} class:{} race:{} alignment:{}.\n\r"_format(
        challenger->name, challenger->level, class_table[challenger->class_num].name, race_table[challenger->race].name,
        challenger->alignment));
    challengee->send_to("|cType |paccept |cor |grefuse.\n\r|w");
    imm = ch;
    imm_name = imm->name;
}

void do_refuse(CHAR_DATA *ch, const char *argument) {
    (void)argument;

    if (ch != imm && ch != challenger && ch != challengee) {
        send_to_char("|cRefuse what?|w\n\r", ch);
        return;
    }

    if (ch == challengee && imm != nullptr) {
        ch->send_to("|cYou have refused to fight to the death with {}.|w\n\r"_format(challenger->name));
        imm->send_to(
            "|c{} has refused to fight to the death with {}.|w\n\r"_format(challengee->name, challenger->name));
        challenger->send_to("|c{} has refused to fight to the death with you.|w\n\r"_format(challengee->name));

        challenger = nullptr;
        challengee = nullptr;
        imm = nullptr;
        challenge_ticker = 0;
        return;
    }

    if (ch == challenger) {
        send_to_char("You can't withdraw your challenge like that!\n\r", ch);
    }

    /* Else it is imm who is trying to refuse! */
    send_to_char("You can't withdraw from control once you have accepted it.\n\r", ch);
}

void do_ready(CHAR_DATA *ch, const char *argument) {
    (void)argument;
    if (check_duel_status(1))
        return;

    if (ch != imm && ch != challenger && ch != challengee) {
        send_to_char("|cWhat are you ready for?|w\n\r", ch);
        return;
    }

    if ((ch == challenger || ch == challengee) && imm_ready != 1) {
        send_to_char("|cWhat are you ready for?|w\n\r", ch);
        return;
    }

    auto prep_room = get_room_index(CHAL_PREP);
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
            auto msg = "|W### Go to the |Pviewing gallery|W to watch a duel between {} and {}.|w"_format(
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
    char buf[MAX_STRING_LENGTH];

    check_duel_status(0);

    if (challenge_ticker == 0)
        return;
    else {
        challenge_ticker--;
        if (challenge_ticker == 0) {
            if (imm != nullptr && challenger != nullptr && challengee != nullptr) {
                /* Update this at some point to make it more friendly. End
                   duel silently less often. */
                snprintf(buf, sizeof(buf), "|cThe challenge has been cancelled.|w\n\r");

                send_to_char(buf, imm);
                send_to_char(buf, challenger);
                send_to_char(buf, challengee);
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
        snprintf(buf, sizeof(buf), "|CThere are %d ticks left before the challenge is cancelled.|w\n\r",
                 challenge_ticker);
        if (imm != nullptr)
            send_to_char(buf, imm);
        if (imm != nullptr && imm_ready != 0)
            send_to_char(buf, challengee);
        /* Don't want to only tell chellenger when control has been
           accepted.  Warn him if he is on. Need to change this to check
           that challenger is still here. */
        if (imm_ready != 0)
            send_to_char(buf, challenger);
    }
}

void do_chal_canc(CHAR_DATA *ch) {
    char buf[MAX_STRING_LENGTH];

    /* We don't check status here since we are going to quit the duel
       anyway! */

    /* Ugly.  May need to alter this later! */
    if (ch != imm && ch != challenger && ch != challengee)
        return;

    auto msg = "|c{} has either quit or lost their link.|w\n\r"_format(ch->name);
    if (imm != nullptr && get_char_world(ch, imm_name))
        send_to_char(buf, imm);
    if (get_char_world(ch, challenger_name))
        send_to_char(buf, challenger);
    if (get_char_world(ch, challengee_name))
        send_to_char(buf, challengee);
    challenge_ticker = 1;
    do_chal_tick();
}

void do_cancel_chal(CHAR_DATA *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    one_argument(argument, arg);
    victim = get_char_world(ch, arg);

    if (check_duel_status(1))
        return;

    if (ch != imm) {
        send_to_char("|cYou can't cancel anything.|w\n\r", ch);
        return;
    }

    if (imm_ready != 1 || challenger_ready != 1 || challengee_ready != 1) {
        send_to_char("|cEveryone must be ready to cancel the challenge.|w\n\r", imm);
        return;
    }

    if (victim == challenger) {
        send_to_char("|cYou have cancelled the challenge.|w\n\r", imm);
        challenge_ticker = 1;
        do_chal_tick();
    } else {
        send_to_char("|cTo cancel the challenge, you must specify the challenger.|w\n\r", imm);
        return;
    }
}

void do_duel(CHAR_DATA *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    one_argument(argument, arg);
    victim = get_char_world(ch, arg);

    if (check_duel_status(1))
        return;

    if (ch != challenger && ch != challengee) {
        send_to_char("|cYou are not in a challenge to duel.|w\n\r", ch);
        return;
    }

    if (arg[0] == '\0') {
        send_to_char("|cDuel who?|w\n\r", ch);
        return;
    }

    if (ch->in_room->vnum != CHAL_ROOM) {
        send_to_char("|cYou must be in the challenge room to duel.|w\n\r", ch);
        return;
    }

    if (imm_ready != 1 || challenger_ready != 1 || challengee_ready != 1) {
        send_to_char("|cNot everyone is ready yet.|w\n\r", ch);
        return;
    }

    if ((ch == challenger && victim == challengee) || (ch == challengee && victim == challenger)) {
        if (ch->fighting != nullptr || victim->fighting != nullptr)
            return;
        challenge_fighting = true;
        multi_hit(ch, victim, 100);
        return;
    }

    send_to_char("|cYou did not challenge that person.|w\n\r", ch);
}

namespace {
void tell_results_to(int who) {
    if (who == 0) {
        challenger->send_to("|cYou have lost your fight to the death with {}.|w\n\r"_format(challengee->name));
        challengee->send_to(
            "|cCongratulations! You have won the fight to the death with {}.|w\n\r"_format(challenger->name));
    }

    if (who == 1) {
        challengee->send_to("|cYou have lost your fight to the death with {}.|w\n\r"_format(challenger->name));
        challenger->send_to(
            "|cCongratulations! You have won the fight to the death with {}.|w\n\r"_format(challengee->name));
    }
}
}

int do_check_chal(CHAR_DATA *ch) {
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

    raw_kill(ch);
    if (imm == nullptr) {
        bug("do_check_chal: crash potential here guys, challengee/r is being killed...");
        return 0;
    }
    auto altar = get_room_index(ROOM_VNUM_ALTAR);
    if (!altar) {
        bug("do_check_chal: couldn't find altar!");
        return 0;
    }
    transfer(imm, ch, altar);

    announce("|W### |P{}|W was defeated in a duel to the death with |P{}|W.|w"_format(
                 ch->name, who == 0 ? challengee->name : challenger->name),
             imm);

    tell_results_to(who);

    send_to_char("|cChallenge over. Challenge variables reset. New challenge can now be initiated.|w\n\r", imm);

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

void do_room_check(CHAR_DATA *ch) {
    if (check_duel_status(1))
        return;

    if (ch != challenger && ch != challengee)
        return;

    challenge_ticker = 1;
    do_chal_tick();
}

void do_flee_check(CHAR_DATA *ch) {
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

    announce(
        "|W### |P{} |Whas cowardly fled from |P{}|W."_format(ch->name, who == 0 ? challengee->name : challenger->name),
        imm);

    tell_results_to(who);

    send_to_char("|cChallenge over. Challenge variables reset. New challenge can now be initiated.|w\n\r", imm);

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

int fighting_duel(CHAR_DATA *ch, CHAR_DATA *victim) {
    if (!challenge_fighting)
        return false;
    if (ch->in_room->vnum != CHAL_ROOM)
        return false;
    if ((ch != challenger) && (ch != challengee))
        return false;
    if ((victim != challenger) && (victim != challengee))
        return false;
    return true;
}

int in_duel(const CHAR_DATA *ch) {
    if (!challenge_fighting)
        return false;
    if (ch->in_room->vnum != CHAL_ROOM)
        return false;
    return ch == challenger || ch == challengee;
}

/* Checks to see that various things are correct.  Only input is
   toggle to say what to do on error.  0=nothing, 1=abort duel. */
static int check_duel_status(int f) {
    unsigned int flag = 0;

    if (challenger != nullptr) {
        if (challenger->name == challenger_name)
            flag |= 1u;
    }

    if (challengee != nullptr) {
        if (challengee->name == challengee_name)
            flag |= 2u;
    }

    if (imm != nullptr) {
        if (imm->name == imm_name)
            flag |= 4u;
    }

    if (!flag)
        return 0;
    else {
        bug("check_duel_status: Name match failure. Code %d", flag);
    }
    if (f) {
        challenge_ticker = 1;
        do_chal_tick();
    }
    return 1;
}
