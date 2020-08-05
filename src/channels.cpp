/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                 */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  channels.c: channel communications facilities within the MUD         */
/*                                                                       */
/*************************************************************************/

#include "interp.h"
#include "merc.h"

#include <cstdio>

extern void print_status(CHAR_DATA *ch, const char *name, const char *master_name, int state, int master_state);

static void print_channel_status(CHAR_DATA *ch, const char *chan, unsigned long reference, unsigned long flag) {
    print_status(ch, chan, "OFF due to quiet mode", !IS_SET(reference, flag), !IS_SET(ch->comm, COMM_QUIET));
}

void do_channels(CHAR_DATA *ch, const char *argument) {
    (void)argument;
    PCCLAN *OrigClan;

    /* lists all channels and their status */
    send_to_char("|Wchannel         status|w\n\r", ch);
    send_to_char("----------------------------\n\r", ch);

    print_channel_status(ch, "gossip", ch->comm, COMM_NOGOSSIP);
    print_channel_status(ch, "auction", ch->comm, COMM_NOAUCTION);
    print_channel_status(ch, "music", ch->comm, COMM_NOMUSIC);
    print_channel_status(ch, "Q/A", ch->comm, COMM_NOQUESTION);
    print_channel_status(ch, "gratz", ch->comm, COMM_NOGRATZ);
    print_channel_status(ch, "announce", ch->comm, COMM_NOANNOUNCE);
    print_channel_status(ch, "allege", ch->comm, COMM_NOALLEGE);
    print_channel_status(ch, "philosophise", ch->comm, COMM_NOPHILOSOPHISE);
    print_channel_status(ch, "qwest", ch->comm, COMM_NOQWEST);
    print_channel_status(ch, "shout", ch->comm, COMM_NOSHOUT);

    /* Determine if the player is in a clan, and find which one */
    if (IS_NPC(ch)) {
        if (ch->desc->original != nullptr)
            OrigClan = ch->desc->original->pcdata->pcclan;
        else
            OrigClan = nullptr;
    } else
        OrigClan = ch->pcdata->pcclan;

    if (OrigClan) {
        print_channel_status(ch, "clan channel", (OrigClan->channelflags) ^ CLANCHANNEL_ON, CLANCHANNEL_ON);
    }

    if (IS_IMMORTAL(ch))
        print_channel_status(ch, "god channel", ch->comm, COMM_NOWIZ);

    print_status(ch, "quiet mode", "", IS_SET(ch->comm, COMM_QUIET), 1);

    if (ch->lines != PAGELEN) {
        char buf[100];
        if (ch->lines) {
            snprintf(buf, sizeof(buf), "You display %d lines of scroll.\n\r", ch->lines + 2);
            send_to_char(buf, ch);
        } else
            send_to_char("Scroll buffering is off.\n\r", ch);
    }

    if (IS_SET(ch->comm, COMM_NOTELL))
        send_to_char("You cannot use tell.\n\r", ch);

    if (IS_SET(ch->comm, COMM_NOCHANNELS))
        send_to_char("You cannot use channels.\n\r", ch);

    if (IS_SET(ch->comm, COMM_NOEMOTE))
        send_to_char("You cannot show emotions.\n\r", ch);
}

static void toggle_channel(CHAR_DATA *ch, int chan_flag, const char *chan_name) {
    char buf[MAX_STRING_LENGTH];

    if (IS_SET(ch->comm, chan_flag)) {
        snprintf(buf, sizeof(buf), "|c%s channel is now %s|c.|w\n\r", chan_name,
                 IS_SET(ch->comm, COMM_QUIET) ? "|rON (OFF due to quiet mode)" : "|gON");
        REMOVE_BIT(ch->comm, chan_flag);
    } else {
        snprintf(buf, sizeof(buf), "|c%s channel is now |rOFF|c.|w\n\r", chan_name);
        SET_BIT(ch->comm, chan_flag);
    }
    send_to_char(buf, ch);
}

void do_quiet(CHAR_DATA *ch, const char *argument) {
    (void)argument;
    if (IS_SET(ch->comm, COMM_QUIET)) {
        send_to_char("Quiet mode removed.\n\r", ch);
        REMOVE_BIT(ch->comm, COMM_QUIET);
    } else {
        send_to_char("From now on, you will only hear says and emotes.\n\r", ch);
        SET_BIT(ch->comm, COMM_QUIET);
    }
}

void channel_command(CHAR_DATA *ch, const char *argument, int chan_flag, const char *chan_name, const char *desc_self,
                     const char *desc_other) {
    char buf[MAX_STRING_LENGTH];

    if (argument[0] == '\0') {
        toggle_channel(ch, chan_flag, chan_name);
    } else {
        DESCRIPTOR_DATA *d;

        if (IS_SET(ch->comm, COMM_QUIET)) {
            send_to_char("You must turn off quiet mode first.\n\r", ch);
            return;
        }
        if (IS_SET(ch->comm, COMM_NOCHANNELS)) {
            send_to_char("The gods have revoked your channel priviliges.\n\r", ch);
            return;
        }
        if (IS_SET(ch->act, PLR_AFK))
            do_afk(ch, nullptr);
        REMOVE_BIT(ch->comm, chan_flag);

        snprintf(buf, sizeof(buf), desc_self, argument);
        send_to_char(buf, ch);
        for (d = descriptor_list; d != nullptr; d = d->next) {
            CHAR_DATA *victim;

            victim = d->original ? d->original : d->character;

            if (d->connected == CON_PLAYING && d->character != ch && !IS_SET(victim->comm, chan_flag)
                && !IS_SET(victim->comm, COMM_QUIET)) {
                act_new(desc_other, ch, argument, d->character, TO_VICT, POS_DEAD);
            }
        }
    }
}

void do_announce(CHAR_DATA *ch, const char *argument) {
    (void)argument;
    toggle_channel(ch, COMM_NOANNOUNCE, "Announce");
}

void do_immtalk(CHAR_DATA *ch, const char *argument) {
    DESCRIPTOR_DATA *d;
    const char *format = "|W$n: |c$t|w";

    if (argument[0] == '\0') {
        toggle_channel(ch, COMM_NOWIZ, "Immortal");
        return;
    }

    REMOVE_BIT(ch->comm, COMM_NOWIZ);

    if (IS_SET(ch->act, PLR_AFK))
        format = "|w(|cAFK|w)|W $n: |c$t|w";

    if (get_trust(ch) >= 91)
        act_new(format, ch, argument, nullptr, TO_CHAR, POS_DEAD);
    for (d = descriptor_list; d != nullptr; d = d->next) {
        if (d->connected == CON_PLAYING && IS_IMMORTAL(d->character) && !IS_SET(d->character->comm, COMM_NOWIZ)) {
            act_new(format, ch, argument, d->character, TO_VICT, POS_DEAD);
        }
    }
}

void do_gossip(CHAR_DATA *ch, const char *argument) {
    channel_command(ch, argument, COMM_NOGOSSIP, "Gossip", "|gYou gossip '%s|g'|w\n\r", "|g$n gossips '$t|g'|w");
}

void do_auction(CHAR_DATA *ch, const char *argument) {
    channel_command(ch, argument, COMM_NOAUCTION, "Auction", "You auction '%s|w'\n\r", "$n auctions '$t|w'");
}

void do_music(CHAR_DATA *ch, const char *argument) {
    channel_command(ch, argument, COMM_NOMUSIC, "Music", "|PYou MUSIC: '%s|P'|w\n\r", "|P$n MUSIC: '$t|P'|w");
}

void do_question(CHAR_DATA *ch, const char *argument) {
    channel_command(ch, argument, COMM_NOQUESTION, "Q/A", "|GYou question '%s|G'|w\n\r", "|G$n questions '$t|G'|w");
}

void do_answer(CHAR_DATA *ch, const char *argument) {
    channel_command(ch, argument, COMM_NOQUESTION, "Q/A", "|GYou answer '%s|G'|w\n\r", "|G$n answers '$t|G'|w");
}

void do_gratz(CHAR_DATA *ch, const char *argument) {
    channel_command(ch, argument, COMM_NOGRATZ, "Gratz", "|yYou gratz '%s|y'|w\n\r", "|y$n gratzes '$t|y'|w");
}

void do_allege(CHAR_DATA *ch, const char *argument) {
    channel_command(ch, argument, COMM_NOALLEGE, "Allege", "|pYou allege '%s|p'|w\n\r", "|p$n alleges '$t|p'|w");
}

void do_philosophise(CHAR_DATA *ch, const char *argument) {
    channel_command(ch, argument, COMM_NOPHILOSOPHISE, "Philosophise", "|WYou philosophise '%s|W'|w\n\r",
                    "|W$n philosophises '$t|W'|w");
}

void do_qwest(CHAR_DATA *ch, const char *argument) {
    channel_command(ch, argument, COMM_NOQWEST, "Qwest", "|YYou qwest '%s|Y'|w\n\r", "|Y$n qwests '$t|Y'|w");
}

void do_shout(CHAR_DATA *ch, const char *argument) {
    channel_command(ch, argument, COMM_NOSHOUT, "Shout", "|WYou shout '%s|W'|w\n\r", "|W$n shouts '$t|W'|w");
}
