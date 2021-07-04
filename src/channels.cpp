/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                 */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  channels.c: channel communications facilities within the MUD         */
/*                                                                       */
/*************************************************************************/

#include "Char.hpp"
#include "Descriptor.hpp"
#include "DescriptorList.hpp"
#include "comm.hpp"
#include "interp.h"
#include "merc.h"

#include <fmt/format.h>

#include <cstdio>

extern void print_status(const Char *ch, const char *name, const char *master_name, int state, int master_state);

static void print_channel_status(const Char *ch, const char *chan, unsigned long reference, unsigned long flag) {
    print_status(ch, chan, "OFF due to quiet mode", !check_bit(reference, flag), !check_bit(ch->comm, COMM_QUIET));
}

void do_channels(const Char *ch) {
    /* lists all channels and their status */
    ch->send_line("|Wchannel         status|w");
    ch->send_line("----------------------------");

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

    // Determine if the player is in a clan, and find which one.
    if (const auto *pc_clan = ch->player() ? ch->player()->pc_clan() : nullptr) {
        print_channel_status(ch, "clan channel", pc_clan->channelflags ^ CLANCHANNEL_ON, CLANCHANNEL_ON);
    }

    if (ch->is_immortal())
        print_channel_status(ch, "god channel", ch->comm, COMM_NOWIZ);

    print_status(ch, "quiet mode", "", check_bit(ch->comm, COMM_QUIET), 1);

    if (ch->lines != PAGELEN) {
        if (ch->lines) {
            ch->send_line("You display {} lines of scroll.", ch->lines + 2);
        } else
            ch->send_line("Scroll buffering is off.");
    }

    if (check_bit(ch->comm, COMM_NOTELL))
        ch->send_line("You cannot use tell.");

    if (check_bit(ch->comm, COMM_NOCHANNELS))
        ch->send_line("You cannot use channels.");

    if (check_bit(ch->comm, COMM_NOEMOTE))
        ch->send_line("You cannot show emotions.");
}

static void toggle_channel(Char *ch, unsigned long chan_flag, const char *chan_name) {
    char buf[MAX_STRING_LENGTH];

    if (check_bit(ch->comm, chan_flag)) {
        snprintf(buf, sizeof(buf), "|c%s channel is now %s|c.|w\n\r", chan_name,
                 check_bit(ch->comm, COMM_QUIET) ? "|rON (OFF due to quiet mode)" : "|gON");
        clear_bit(ch->comm, chan_flag);
    } else {
        snprintf(buf, sizeof(buf), "|c%s channel is now |rOFF|c.|w\n\r", chan_name);
        set_bit(ch->comm, chan_flag);
    }
    ch->send_to(buf);
}

void do_quiet(Char *ch) {
    if (check_bit(ch->comm, COMM_QUIET)) {
        ch->send_line("Quiet mode removed.");
        clear_bit(ch->comm, COMM_QUIET);
    } else {
        ch->send_line("From now on, you will only hear says and emotes.");
        set_bit(ch->comm, COMM_QUIET);
    }
}

void channel_command(Char *ch, const char *argument, unsigned long chan_flag, const char *chan_name,
                     const char *desc_self, const char *desc_other) {
    char buf[MAX_STRING_LENGTH];

    if (argument[0] == '\0') {
        toggle_channel(ch, chan_flag, chan_name);
    } else {
        if (check_bit(ch->comm, COMM_QUIET)) {
            ch->send_line("You must turn off quiet mode first.");
            return;
        }
        if (check_bit(ch->comm, COMM_NOCHANNELS)) {
            ch->send_line("The gods have revoked your channel privileges.");
            return;
        }
        ch->set_not_afk();
        clear_bit(ch->comm, chan_flag);

        snprintf(buf, sizeof(buf), desc_self, argument);
        ch->send_to(buf);
        for (auto &d : descriptors().all_but(*ch)) {
            auto *victim = d.person();
            if (!check_bit(victim->comm, chan_flag) && !check_bit(victim->comm, COMM_QUIET)) {
                act(desc_other, ch, argument, d.character(), To::Vict, Position::Type::Dead);
            }
        }
    }
}

void do_announce(Char *ch) { toggle_channel(ch, COMM_NOANNOUNCE, "Announce"); }

void do_immtalk(Char *ch, std::string_view argument) {
    if (argument.empty()) {
        toggle_channel(ch, COMM_NOWIZ, "Immortal");
        return;
    }

    clear_bit(ch->comm, COMM_NOWIZ);

    const char *format = check_bit(ch->act, PLR_AFK) ? "|w(|cAFK|w)|W $n: |c$t|w" : "|W$n: |c$t|w";
    if (ch->get_trust() >= LEVEL_HERO)
        act(format, ch, argument, nullptr, To::Char, Position::Type::Dead);
    for (auto &d : descriptors().playing()) {
        if (d.character()->is_immortal() && !check_bit(d.character()->comm, COMM_NOWIZ))
            act(format, ch, argument, d.character(), To::Vict, Position::Type::Dead);
    }
}

void do_gossip(Char *ch, const char *argument) {
    channel_command(ch, argument, COMM_NOGOSSIP, "Gossip", "|gYou gossip '%s|g'|w\n\r", "|g$n gossips '$t|g'|w");
}

void do_auction(Char *ch, const char *argument) {
    channel_command(ch, argument, COMM_NOAUCTION, "Auction", "You auction '%s|w'\n\r", "$n auctions '$t|w'");
}

void do_music(Char *ch, const char *argument) {
    channel_command(ch, argument, COMM_NOMUSIC, "Music", "|PYou MUSIC: '%s|P'|w\n\r", "|P$n MUSIC: '$t|P'|w");
}

void do_question(Char *ch, const char *argument) {
    channel_command(ch, argument, COMM_NOQUESTION, "Q/A", "|GYou question '%s|G'|w\n\r", "|G$n questions '$t|G'|w");
}

void do_answer(Char *ch, const char *argument) {
    channel_command(ch, argument, COMM_NOQUESTION, "Q/A", "|GYou answer '%s|G'|w\n\r", "|G$n answers '$t|G'|w");
}

void do_gratz(Char *ch, const char *argument) {
    channel_command(ch, argument, COMM_NOGRATZ, "Gratz", "|yYou gratz '%s|y'|w\n\r", "|y$n gratzes '$t|y'|w");
}

void do_allege(Char *ch, const char *argument) {
    channel_command(ch, argument, COMM_NOALLEGE, "Allege", "|pYou allege '%s|p'|w\n\r", "|p$n alleges '$t|p'|w");
}

void do_philosophise(Char *ch, const char *argument) {
    channel_command(ch, argument, COMM_NOPHILOSOPHISE, "Philosophise", "|WYou philosophise '%s|W'|w\n\r",
                    "|W$n philosophises '$t|W'|w");
}

void do_qwest(Char *ch, const char *argument) {
    channel_command(ch, argument, COMM_NOQWEST, "Qwest", "|YYou qwest '%s|Y'|w\n\r", "|Y$n qwests '$t|Y'|w");
}

void do_shout(Char *ch, const char *argument) {
    channel_command(ch, argument, COMM_NOSHOUT, "Shout", "|WYou shout '%s|W'|w\n\r", "|W$n shouts '$t|W'|w");
}
