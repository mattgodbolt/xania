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

#include <cstdio>
#include <cstring>
#include <ctime>
#include <sys/types.h>

/* Some local DEFINES to keep things general. */
#define NAME_SIZE 30

/* Private functions. */
static int check_duel_status(int);

/* Challenge variables.  Lets make them local to this file. */
static CHAR_DATA *challenger = nullptr;
static CHAR_DATA *challengee = nullptr;
static CHAR_DATA *imm = nullptr;
static char imm_name[NAME_SIZE + 1];
static char challenger_name[NAME_SIZE + 1];
static char challengee_name[NAME_SIZE + 1];
static int imm_ready = 0;
static int challenger_ready = 0;
static int challengee_ready = 0;
static int challenge_ticker;
static bool challenge_active = false;
static bool challenge_fighting = false;

/* And now on with the code. */

void do_challenge(CHAR_DATA *ch, const char *argument) {
    char buf[MAX_STRING_LENGTH];
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

    if (IS_NPC(victim)) {
        send_to_char("|cYou cannot challenge non-player characters.|w\n\r", ch);
        return;
    }

    if (victim->desc == nullptr && !IS_NPC(victim)) {
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

    snprintf(buf, sizeof(buf), "|cYou pray for the right to duel with %s.\n\r", challengee->name);
    send_to_char(buf, challenger);
    snprintf(buf, sizeof(buf), "|Ghas challenged %s, any imm takers? (type accept)|w", victim->name);
    do_immtalk(ch, buf);
    strncpy(challenger_name, challenger->name, NAME_SIZE);
    strncpy(challengee_name, challengee->name, NAME_SIZE);
    challenge_ticker = 4;
}

void do_accept(CHAR_DATA *ch, const char *argument) {
    (void)argument;
    char buf[MAX_STRING_LENGTH];

    if ((challenger == nullptr) || (((challengee != ch) || (imm == nullptr)) && (ch->level <= 91))) {
        send_to_char("|cSorry, there is no challenge for you to accept.|w\n\r", ch);
        return;
    }

    if (ch->level < 95 && imm == nullptr) {
        send_to_char("|cYou have to be level 95 or higher to control a challenge.|w\n\r", ch);
        return;
    }

    if (imm != nullptr && imm != ch) {
        if (ch->level > 94) {
            send_to_char("|cSorry, an imm has already accepted to control the challenge.|w\n\r", ch);
            return;
        }

        send_to_char("|cType |gready|c when you are ready to be transfered.|w\n\r", imm);
        send_to_char(
            "|cThe challenge has been accepted, please wait for the immortals to prepare\n|cthe challenge room.|w\n\r",
            challenger);
        send_to_char(
            "|cThe challenge has been accepted, please wait for the immortals to prepare\n|cthe challenge room.|w\n\r",
            challengee);
        challenge_ticker = 4;
        return;
    }

    snprintf(buf, sizeof(buf),
             "|cYou have accepted to control the challenge.\n\rNow asking %s if they wish to duel.|w\n\r",
             challengee->name);
    send_to_char(buf, ch);

    if (imm == nullptr) {
        snprintf(buf, sizeof(buf),
                 "|g%s|c has accepted to control the challenge. Now waiting to see\nif |g%s|c will accept your "
                 "challenge.|w\n\r",
                 pers(ch, challenger), challengee->name);
        send_to_char(buf, challenger);
    }

    snprintf(buf, sizeof(buf), "|CYou have been challenged to a duel to the death by |G%s.\n\r", challenger->name);
    send_to_char(buf, challengee);

    snprintf(buf, sizeof(buf), "|c%s's stats are: level:%d class:%s race:%s alignment:%d.\n\r", challenger->name,
             challenger->level, class_table[challenger->class_num].name, race_table[challenger->race].name,
             challenger->alignment);
    send_to_char(buf, challengee);
    send_to_char("|cType |paccept |cor |grefuse.\n\r|w", challengee);
    imm = ch;
    strncpy(imm_name, imm->name, NAME_SIZE);
}

void do_refuse(CHAR_DATA *ch, const char *argument) {
    (void)argument;
    char buf[MAX_STRING_LENGTH];

    if (ch != imm && ch != challenger && ch != challengee) {
        send_to_char("|cRefuse what?|w\n\r", ch);
        return;
    }

    if (ch == challengee && imm != nullptr) {
        snprintf(buf, sizeof(buf), "|cYou have refused to fight to the death with %s.|w\n\r", challenger->name);
        send_to_char(buf, ch);

        snprintf(buf, sizeof(buf), "|c%s has refused to fight to the death with %s.|w\n\r", challengee->name,
                 challenger->name);
        send_to_char(buf, imm);

        snprintf(buf, sizeof(buf), "|c%s has refused to fight to the death with you.|w\n\r", challengee->name);
        send_to_char(buf, challenger);

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
    char buf[MAX_STRING_LENGTH];

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

    if (ch == imm && imm_ready == 0) {
        snprintf(buf, sizeof(buf), "%s %d", ch->name, CHAL_PREP);
        do_transfer(imm, buf);
        imm_ready = 1;
        challenge_ticker = 4;
        snprintf(buf, sizeof(buf), "|cType |gready|c when you are ready to be transfered.|w\n\r");
        send_to_char(buf, challenger);
        send_to_char(buf, challengee);
    }

    if (ch == challenger && challenger_ready == 0) {
        snprintf(buf, sizeof(buf), "%s %d", ch->name, CHAL_PREP);
        do_transfer(imm, buf);
        challenger_ready = 1;
    }

    if (ch == challengee && challengee_ready == 0) {
        snprintf(buf, sizeof(buf), "%s %d", ch->name, CHAL_PREP);
        do_transfer(imm, buf);
        challengee_ready = 1;
    }

    if (challenge_ticker > 0) {
        if (imm_ready == 1 && challenger_ready == 1 && challengee_ready == 1) {
            snprintf(buf, sizeof(buf), "|W### Go to the |Pviewing gallery|W to watch a duel between %s and %s.|w",
                     challenger->name, challengee->name);
            announce(buf, imm);
            send_to_char("|CRemember that you can |Gcancel|C the challenge at any time should it be\nnecessary to do "
                         "so. (use cancel + challenger name)\n\r|w",
                         imm);
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

    snprintf(buf, sizeof(buf), "|c%s has either quit or lost their link.|w\n\r", ch->name);
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

int do_check_chal(CHAR_DATA *ch) {
    int who;
    char buf[MAX_STRING_LENGTH];

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
    snprintf(buf, sizeof(buf), "%s 3054", ch->name);
    if (imm == nullptr) {
        bug("do_check_chal: crash potential here guys, challengee/r is being killed...");
        return 0;
    }
    do_transfer(imm, buf);

    snprintf(buf, sizeof(buf), "|W### |P%s |Wwas defeated in a duel to the death with |P%s|W.|w", ch->name,
             who == 0 ? challengee->name : challenger->name);
    announce(buf, imm);

    if (who == 0) {
        snprintf(buf, sizeof(buf), "|cYou have lost your fight to the death with %s.|w\n\r", challengee->name);
        send_to_char(buf, challenger);
        snprintf(buf, sizeof(buf), "|cCongratulations! You have won the fight to the death with %s.|w\n\r",
                 challenger->name);
        send_to_char(buf, challengee);
    }

    if (who == 1) {
        snprintf(buf, sizeof(buf), "|cYou have lost your fight to the death with %s.|w\n\r", challenger->name);
        send_to_char(buf, challengee);
        snprintf(buf, sizeof(buf), "|cCongratulations! You have won the fight to the death with %s.|w\n\r",
                 challengee->name);
        send_to_char(buf, challenger);
    }

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
    char buf[MAX_STRING_LENGTH];

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

    snprintf(buf, sizeof(buf), "|W### |P%s |Whas cowardly fled from |P%s|W.", ch->name,
             who == 0 ? challengee->name : challenger->name);
    announce(buf, imm);

    if (who == 0) {
        snprintf(buf, sizeof(buf), "|cYou have lost your fight to the death with %s.|w\n\r", challengee->name);
        send_to_char(buf, challenger);
        snprintf(buf, sizeof(buf), "|cCongratulations! You have won the fight to the death with %s.|w\n\r",
                 challenger->name);
        send_to_char(buf, challengee);
    }

    if (who == 1) {
        snprintf(buf, sizeof(buf), "|cYou have lost your fight to the death with %s.|w\n\r", challenger->name);
        send_to_char(buf, challengee);
        snprintf(buf, sizeof(buf), "|cCongratulations! You have won the fight to the death with %s.|w\n\r",
                 challengee->name);
        send_to_char(buf, challenger);
    }

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

int in_duel(CHAR_DATA *ch) {
    if (!challenge_fighting)
        return false;
    if (ch->in_room->vnum != CHAL_ROOM)
        return false;
    if ((ch != challenger) && (ch != challengee))
        return false;
    return true;
}

/* Checks to see that various things are correct.  Only input is
   toggle to say what to do on error.  0=nothing, 1=abort duel. */
static int check_duel_status(int f) {
    int flag = 0;

    if (challenger != nullptr) {
        if (strncmp(challenger->name, challenger_name, NAME_SIZE))
            flag = 1;
    }

    if (challengee != nullptr) {
        if (strncmp(challengee->name, challengee_name, NAME_SIZE))
            flag += 2;
    }

    if (imm != nullptr) {
        if (strncmp(imm->name, imm_name, NAME_SIZE))
            flag = +4;
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
