/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                    */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  clan.c: organised player clans                                       */
/*                                                                       */
/*************************************************************************/

#include "Descriptor.hpp"
#include "DescriptorList.hpp"
#include "comm.hpp"
#include "handler.hpp"
#include "interp.h"
#include "merc.h"
#include "string_utils.hpp"

#include <range/v3/algorithm/find_if.hpp>

#include <algorithm>
#include <cstdio>

/* User servicable bits... you will also need to change the NUM_CLANS in clan.h */

const std::array<CLAN, NUM_CLANS> clantable = {{
    {
        "Keepers of the Lore",
        "|p(Lore)|w ",
        'C', /*clan 67 */
        {"a novice", "an initiate", "a Loreprotector", "a Lorebinder", "the Loremaster"},
        30151,
        30151 /* keep courtyard */
    }, /* Keepers of the Lore */

    {
        "Disciples of the Wyrm",
        "|p(Wyrm)|w ",
        'B', /*clan 66 */
        {"a neophyte", "an acolyte", "a priest", "a high priest", "the Preceptor"},
        30102,
        30102 /* courtyard */
    }, /* Disciples of the Wyrm */

    {"Implementor",
     "",
     'D', /* clan 68 */
     {"", "", "", "Implementor", ""},
     30002,
     0}, /* End IMP */

    {"One-eyed Snakes",
     "|c(O-II-S)|w ",
     'O', /* clan 79 */
     {"a member", "a mini-snake", "a major-snake", "a python", "the One-Eyed Trouser Snake"},
     30004,
     0} /* OES */

}}; /* end clantable */

/* End user servicable bits */

void do_clantalk(CHAR_DATA *ch, const char *argument) {
    char buf[MAX_STRING_LENGTH];

    auto *orig_clan = ch->desc->person() ? ch->desc->person()->pc_clan() : nullptr;
    if (orig_clan == nullptr) {
        send_to_char("You are not a member of a clan.\n\r", ch);
        return; /* Disallow mortal PC's with no clan */
    }

    if (IS_SET(ch->comm, COMM_QUIET)) {
        send_to_char("You must remove quiet mode first.\n\r", ch);
        return;
    }

    if (argument[0] == '\0' || !orig_clan->channelflags) {
        /* They want to turn it on/off */
        orig_clan->channelflags ^= CLANCHANNEL_ON;
        snprintf(buf, sizeof(buf), "Clan channel now %s\n\r",
                 (orig_clan->channelflags & CLANCHANNEL_ON) ? "on" : "off");
        send_to_char(buf, ch);
        return;
    }

    /* Next check to see if a CLAN_HERO or CLAN_LEADER is on first */

    auto playing = descriptors().playing();
    if (ranges::find_if(playing,
                        [&](const Descriptor &d) {
                            const auto *victim = d.person();
                            const auto *pcclan = victim->pc_clan();

                            return pcclan && pcclan->clan->clanchar == orig_clan->clan->clanchar
                                   && pcclan->clanlevel >= CLAN_HERO && !IS_SET(victim->comm, COMM_QUIET);
                        })
        == playing.end()) {
        send_to_char("Your clan lacks the necessary broadcast nexus, causing your vain telepathy to\n\r"
                     "be lost upon the winds.\n\r",
                     ch);
        return;
    }

    if (orig_clan->channelflags & CLANCHANNEL_NOCHANNED) {
        send_to_char("Your clan channel privileges have been revoked!\n\r", ch);
        return;
    }

    if (IS_SET(ch->act, PLR_AFK))
        do_afk(ch, nullptr);

    if (!orig_clan->channelflags) {
        orig_clan->channelflags ^= CLANCHANNEL_ON;
        snprintf(buf, sizeof(buf), "Clan channel now %s\n\r",
                 (orig_clan->channelflags & CLANCHANNEL_ON) ? "on" : "off");
        send_to_char(buf, ch);
    }

    /* Right here we go - tell all members of the clan the message */
    for (auto &d : descriptors().playing()) {
        auto *victim = d.person();
        const auto *pcclan = victim->pc_clan();
        if (pcclan && pcclan->clan->clanchar == orig_clan->clan->clanchar && pcclan->channelflags & CLANCHANNEL_ON
            && !IS_SET(victim->comm, COMM_QUIET)
            /* || they're an IMM snooping the channels */) {
            snprintf(buf, sizeof(buf), "|G<%s> %s|w\n\r", can_see(d.character(), ch) ? ch->name : "Someone", argument);
            send_to_char(buf, d.character());
        } /* If they can see the message */
    } /* for all descriptors */

} /* do_clanchannel */

void do_noclanchan(CHAR_DATA *ch, const char *argument) {
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;

    /* Check for ability to noclanchan */
    if (IS_NPC(ch))
        return;

    if ((ch->get_trust() > (MAX_LEVEL - 5)) || ((ch->pc_clan() != nullptr) && (ch->pc_clan()->clanlevel > CLAN_HERO))) {
    } else {
        send_to_char("Huh?\n\r", ch);
        return;
    }

    victim = get_char_world(ch, argument);
    if ((victim == nullptr) || IS_NPC(victim)) {
        send_to_char("They're not here.\n\r", ch);
        return;
    } /* If can't find victim */

    /* Check if victim is same clan, and not higher status */

    auto *victim_pcclan = victim->pc_clan();
    if ((victim_pcclan == nullptr) /* If the victim is not in any clan */
        || (victim_pcclan->clan->clanchar != ch->clan()->clanchar) /* or in a different clan */
        || (victim_pcclan->clanlevel > ch->pc_clan()->clanlevel)) /* or they're a higher rank */
    {
        snprintf(buf, sizeof(buf), "You can't noclanchan %s!\n\r", victim->name);
        send_to_char(buf, ch);
        return;
    }

    if (victim == ch) {
        send_to_char("You cannot noclanchannel yourself.\n\r", ch);
        return;
    }

    /* If we get here everything is ok */

    victim_pcclan->channelflags ^= CLANCHANNEL_NOCHANNED; /* Change the victim's flags */

    /* Tell the char how things went */
    snprintf(buf, sizeof(buf), "You have %sed %s's clan channel privileges.\n\r",
             victim_pcclan->channelflags & CLANCHANNEL_NOCHANNED ? "revok" : "reinstat", victim->name);
    send_to_char(buf, ch);

    /* Inform the hapless victim */
    snprintf(buf, sizeof(buf), "%s has %sed your clan channel privileges.\n\r", ch->name,
             victim_pcclan->channelflags & CLANCHANNEL_NOCHANNED ? "revok" : "reinstat");
    buf[0] = UPPER(buf[0]);
    send_to_char(buf, victim);
} /* do_noclanchan */

void do_member(CHAR_DATA *ch, const char *argument) {
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    CHAR_DATA *victim;

    /* Check for ability to member */

    if (IS_NPC(ch))
        return;

    if (!ch->pc_clan() || ch->pc_clan()->clanlevel < CLAN_LEADER) {
        send_to_char("Huh?\n\r", ch); /* Cheesy cheat */
        return;
    } /* If not privileged enough */

    argument = one_argument(argument, buf2); /* Get the command */
    if (buf2[0] != '+' && buf2[0] != '-') {
        send_to_char("Usage:\n\r       member + <character name>\n\r       member - <character name>\n\r", ch);
        return;
    }

    victim = get_char_room(ch, argument);
    if ((victim == nullptr) || IS_NPC(victim)) {
        send_to_char("You can't see them here.\n\r", ch);
        return;
    }

    if (victim == ch) {
        send_to_char("What kind of wally are you?  You can't do that!\n\r", ch);
        return;
    }
    if (victim->get_trust() > ch->get_trust()) {
        snprintf(buf, sizeof(buf), "You cannot do that to %s.\n\r", victim->name);
        return;
    }

    if (buf2[0] == '+') {
        /* Adding a new member of the clan */
        if (victim->clan()) { /* The person is *already* in a clan */
            if (victim->clan()->clanchar == ch->clan()->clanchar) {
                /* Leader is trying to 'member +' a person who is already a member of their
                           clan.  They're probably trying to promote the person in question */
                snprintf(buf, sizeof(buf),
                         "%s is already a member of the %s.\n\rUse 'promote' to promote characters.\n\r", victim->name,
                         ch->clan()->name);
                send_to_char(buf, ch);
                return;
            } else {
                /* In another clan ! */
                snprintf(buf, sizeof(buf), "%s is a member of the %s.\n\rThey must leave that clan first.\n\r",
                         victim->name, ch->clan()->name);
                send_to_char(buf, ch);
                return;
            } /* in your clan? */
        } /* if victim already in a clan */
        victim->pcdata->pcclan.emplace(PCCLAN{ch->clan()});
        snprintf(buf, sizeof(buf), "%s welcomes %s to the %s", ch->name, victim->name, ch->clan()->name);
        act(buf, ch, nullptr, victim, To::NotVict);
        snprintf(buf, sizeof(buf), "You have become %s of the %s.\n\r", ch->pc_clan()->level_name(), ch->clan()->name);
        send_to_char(buf, victim);
        snprintf(buf, sizeof(buf), "You welcome %s as %s of the %s.\n\r", victim->name, ch->pc_clan()->level_name(),
                 ch->clan()->name);
        send_to_char(buf, ch);
    } else { /* End adding member */
        /* Removing a person from a clan */
        if (victim->clan() == nullptr || victim->clan()->clanchar != ch->clan()->clanchar) {
            snprintf(buf, sizeof(buf), "%s is not a member of your clan.\n\r", victim->name);
            send_to_char(buf, ch);
            return;
        } /* If not in clan */
        victim->pcdata->pcclan.reset();
        snprintf(buf, sizeof(buf), "%s removes %s from the %s", ch->name, victim->name, ch->clan()->name);
        act(buf, ch, nullptr, victim, To::NotVict);
        snprintf(buf, sizeof(buf), "You have been discharged from the %s.\n\r", ch->clan()->name);
        send_to_char(buf, victim);
        snprintf(buf, sizeof(buf), "You remove %s from the %s.\n\r", victim->name, ch->clan()->name);
        send_to_char(buf, ch);
    } /* ..else */
} /* do_member */

void mote(CHAR_DATA *ch, const char *argument, int add) {
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;

    /* Check for ability to *mote */

    if (IS_NPC(ch))
        return;

    if (!ch->pc_clan() || ch->pc_clan()->clanlevel < CLAN_LEADER) {
        send_to_char("Huh?\n\r", ch); /* Cheesy cheat */
        return;
    } /* If not privileged enough */

    victim = get_char_room(ch, argument);
    if (victim == nullptr) {
        send_to_char("You can't see them here.\n\r", ch);
        return;
    } /* can see? */

    if (victim == ch) {
        send_to_char("What kind of wally are you?  You can't do that!\n\r", ch);
        return;
    }

    if (!victim->pc_clan() || victim->clan()->clanchar != ch->clan()->clanchar) {
        send_to_char("You don't have authority to promote or demote them.\n\r", ch);
        return;
    }

    /* Idiot-proofing */
    if ((victim->pc_clan()->clanlevel + add) > CLAN_HERO) {
        snprintf(buf, sizeof(buf), "You cannot make %s into another leader.\n\r", victim->name);
        send_to_char(buf, ch);
        return;
    }
    if ((victim->pc_clan()->clanlevel + add) < CLAN_MEMBER) {
        send_to_char("You can't demote below member status.\n\r", ch);
        return;
    }

    victim->pcdata->pcclan->clanlevel += add;
    snprintf(buf, sizeof(buf), "$n is now %s of the %s.", victim->pc_clan()->level_name(), victim->clan()->name);
    act(buf, victim);
    snprintf(buf, sizeof(buf), "You are now %s of the %s.\n\r", victim->pc_clan()->level_name(), victim->clan()->name);
    send_to_char(buf, victim);
} /* c'est le end */

void do_promote(CHAR_DATA *ch, const char *argument) { mote(ch, argument, 1); }

void do_demote(CHAR_DATA *ch, const char *argument) { mote(ch, argument, -1); }

void do_clanwho(CHAR_DATA *ch, const char *argument) {
    (void)argument;
    char buf[MAX_STRING_LENGTH];

    if (IS_NPC(ch))
        return;

    if (!ch->clan()) {
        send_to_char("You have no clan.\n\r", ch);
        return;
    }

    send_to_char("|gCharacter name     |c|||g Clan level|w\n\r", ch);
    send_to_char("|c-------------------+-------------------------------|w\n\r", ch);
    for (auto &d : descriptors().all_visible_to(*ch)) {
        auto *wch = d.person();
        if (wch->clan() && wch->clan()->clanchar == ch->clan()->clanchar) {
            snprintf(buf, sizeof(buf), "%-19s|c|||w %s\n\r", wch->name, wch->pc_clan()->level_name());
            send_to_char(buf, ch);
        }
    }
}

/*
 *  Oh well, this one _had_ to be put in some time. Faramir.
 */

void do_clanset(CHAR_DATA *ch, const char *argument) {
    char buf[MAX_STRING_LENGTH * 2];
    char arg1[MAX_INPUT_LENGTH];
    char marker;
    CHAR_DATA *victim;

    argument = one_argument(argument, arg1);

    if (arg1[0] == '\0') {

        send_to_char("Syntax: clanset {+/-}  <player> <clan>\n\r        clanset leader <player>\n\r", ch);
        return;
    }

    if ((arg1[0] == '+') || (arg1[0] == '-')) {
        marker = arg1[0];
        argument = one_argument(argument, arg1);

        if (arg1[0] == '\0') {
            send_to_char("You must provide a valid player name.\n\r", ch);
            return;
        }

        victim = get_char_world(ch, arg1);
        if ((victim == nullptr) || (IS_NPC(victim))) {
            send_to_char("They're not here.\n\r", ch);
            return;
        }

        if ((victim->get_trust()) > ch->get_trust()) {
            send_to_char("You do not have the powers to do that.\n\r", ch);
            return;
        }

        const CLAN *clan_to_add_to{};
        if (marker == '+') {

            argument = one_argument(argument, arg1);

            if (arg1[0] == '\0') {
                send_to_char("You must provide a valid clan name.\n\r", ch);
                return;
            }

            for (auto &clan : clantable)
                if (is_name(arg1, clan.name)) {
                    clan_to_add_to = &clan;
                    break;
                }
            if (!clan_to_add_to || (!str_prefix(arg1, "Implementor") && ch->get_trust() < IMPLEMENTOR)) {
                send_to_char("That is not a valid clan name.\n\r", ch);
                send_to_char("Do 'help clanlist' to see those available.\n\r", ch);
                return;
            }
        }
        switch (marker) {

        case '+': { /* make someone a member of a clan */

            if (victim->clan()) {
                snprintf(buf, sizeof(buf), "%s is already in a clan.\n\r", victim->name);
                send_to_char(buf, ch);
                return;
            }

            victim->pcdata->pcclan.emplace(PCCLAN{clan_to_add_to});
            snprintf(buf, sizeof(buf), "You set %s as %s of the %s.\n\r", victim->name, victim->pc_clan()->level_name(),
                     victim->clan()->name);
            send_to_char(buf, ch);
            return;
            break;
        }

            /* Removing a person from a clan */

        case '-': {
            if (!victim->clan()) {
                snprintf(buf, sizeof(buf), "%s is not a member of a clan.\n\r", victim->name);
                send_to_char(buf, ch);
                return;
            }
            auto clan_name = victim->clan()->name;
            victim->pcdata->pcclan.reset();
            snprintf(buf, sizeof(buf), "You remove %s from the %s.\n\r", victim->name, clan_name);
            send_to_char(buf, ch);
            return;
        }
        }
    }

    if (!str_prefix(arg1, "leader")) {

        argument = one_argument(argument, arg1);
        if (arg1[0] == '\0') {
            send_to_char("You must provide a valid player name.\n\r", ch);
            return;
        }
        victim = get_char_world(ch, arg1);
        if ((victim == nullptr) || (IS_NPC(victim))) {
            send_to_char("They're not here.\n\r", ch);
            return;
        }

        if ((victim->get_trust()) > ch->get_trust()) {
            send_to_char("You do not have the powers to do that.\n\r", ch);
            return;
        }

        if (!victim->clan()) {
            snprintf(buf, sizeof(buf), "%s is not in a clan.\n\r", victim->name);
            send_to_char(buf, ch);
            return;
        }

        victim->pcdata->pcclan->clanlevel = CLAN_LEADER;
        send_to_char("Ok.\n\r", ch);
        return;
    }

    send_to_char("Syntax: clanset {+/-}  <player> <clan>\n\r        clanset leader <player>\n\r", ch);
}

/*
umm..has to check descriptor list for clan rank of all present
         and in that particular clan. Has to do this every time
         someone uses channel?
         if so then channel  is_active
         else
         send_to_char("Your clan lacks the necessary broadcast nexus, causing your vain telepathy is lost upon the
winds.\n\r");


         And what if ldr/ldr -1 is invisible?
         a member would always be able to tell if a member of
         higher status was logged on. - tough! mrg

Cool     basic reasoning for channs: the Gods support a telepathic framework for the main,
Cool     public channels. All mortals have a certain ability in telepathy which enables them to
Cool     communicate with other mortals; via tell to individuals, or group energies can be used
Cool     for group tell. To transmit to a whole clan requires a focus of power not provided by
Cool     the Gods - this burden falls upon those who control clans - it is they
Cool     who have adapted the God's telepathic framework for secretive purposes and provided
Cool     to only those who serve the clan.

poss problems mit AFK and QUIET on the leader

*/
