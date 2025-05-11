/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                 */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  channels.c: channel communications facilities within the MUD         */
/*                                                                       */
/*************************************************************************/

#include "Act.hpp"
#include "Char.hpp"
#include "CommFlag.hpp"
#include "Descriptor.hpp"
#include "DescriptorList.hpp"
#include "Interpreter.hpp"
#include "PlayerActFlag.hpp"
#include "common/BitOps.hpp"

#include <fmt/format.h>

extern void print_status(const Char *ch, std::string_view name, const bool master_state, std::string_view master_name,
                         const bool state);

static void print_channel_status(const Char *ch, std::string_view chan, const unsigned long reference,
                                 const CommFlag flag) {
    print_status(ch, chan, !check_enum_bit(ch->comm, CommFlag::Quiet), "OFF due to quiet mode",
                 !check_enum_bit(reference, flag));
}

static void print_channel_status(const Char *ch, std::string_view chan, const unsigned long reference,
                                 const ClanCommFlag flag) {
    print_status(ch, chan, !check_enum_bit(ch->comm, CommFlag::Quiet), "OFF due to quiet mode",
                 !check_enum_bit(reference, flag));
}

void do_channels(const Char *ch) {
    /* lists all channels and their status */
    ch->send_line("|Wchannel         status|w");
    ch->send_line("----------------------------");

    print_channel_status(ch, "gossip", ch->comm, CommFlag::NoGossip);
    print_channel_status(ch, "auction", ch->comm, CommFlag::NoAuction);
    print_channel_status(ch, "music", ch->comm, CommFlag::NoMusic);
    print_channel_status(ch, "Q/A", ch->comm, CommFlag::NoQuestion);
    print_channel_status(ch, "gratz", ch->comm, CommFlag::NoGratz);
    print_channel_status(ch, "announce", ch->comm, CommFlag::NoAnnounce);
    print_channel_status(ch, "allege", ch->comm, CommFlag::NoAllege);
    print_channel_status(ch, "philosophise", ch->comm, CommFlag::NoPhilosophise);
    print_channel_status(ch, "qwest", ch->comm, CommFlag::NoQwest);
    print_channel_status(ch, "shout", ch->comm, CommFlag::NoYell);

    // Determine if the player is in a clan, and find which one.
    if (const auto *pc_clan = ch->player() ? ch->player()->pc_clan() : nullptr) {
        print_channel_status(ch, "clan channel", pc_clan->channelflags ^ to_int(ClanCommFlag::ChannelOn),
                             ClanCommFlag::ChannelOn);
    }

    if (ch->is_immortal())
        print_channel_status(ch, "god channel", ch->comm, CommFlag::NoWiz);

    print_status(ch, "quiet mode", true, "", check_enum_bit(ch->comm, CommFlag::Quiet));

    if (ch->lines != PAGELEN) {
        if (ch->lines) {
            ch->send_line("You display {} lines of scroll.", ch->lines + 2);
        } else
            ch->send_line("Scroll buffering is off.");
    }

    if (check_enum_bit(ch->comm, CommFlag::NoTell))
        ch->send_line("You cannot use tell.");

    if (check_enum_bit(ch->comm, CommFlag::NoChannels))
        ch->send_line("You cannot use channels.");

    if (check_enum_bit(ch->comm, CommFlag::NoEmote))
        ch->send_line("You cannot show emotions.");
}

static void toggle_channel(Char *ch, const CommFlag chan_flag, const char *chan_name) {
    if (check_enum_bit(ch->comm, chan_flag)) {
        ch->send_line("|c{} channel is now {}|c.|w", chan_name,
                      check_enum_bit(ch->comm, CommFlag::Quiet) ? "|rON (OFF due to quiet mode)" : "|gON");
        clear_enum_bit(ch->comm, chan_flag);
    } else {
        ch->send_line("|c{} channel is now |rOFF|c.|w", chan_name);
        set_enum_bit(ch->comm, chan_flag);
    }
}

void do_quiet(Char *ch) {
    if (check_enum_bit(ch->comm, CommFlag::Quiet)) {
        ch->send_line("Quiet mode removed.");
        clear_enum_bit(ch->comm, CommFlag::Quiet);
    } else {
        ch->send_line("From now on, you will only hear says and emotes.");
        set_enum_bit(ch->comm, CommFlag::Quiet);
    }
}

void channel_command(Char *ch, std::string_view argument, const CommFlag chan_flag, const char *chan_name,
                     const char *desc_self, const char *desc_other) {
    if (argument.empty()) {
        toggle_channel(ch, chan_flag, chan_name);
    } else {
        if (check_enum_bit(ch->comm, CommFlag::Quiet)) {
            ch->send_line("You must turn off quiet mode first.");
            return;
        }
        if (check_enum_bit(ch->comm, CommFlag::NoChannels)) {
            ch->send_line("The gods have revoked your channel privileges.");
            return;
        }
        ch->set_not_afk();
        clear_enum_bit(ch->comm, chan_flag);
        ch->send_line(desc_self, argument);
        for (auto &d : ch->mud_.descriptors().all_but(*ch)) {
            auto *victim = d.person();
            if (!check_enum_bit(victim->comm, chan_flag) && !check_enum_bit(victim->comm, CommFlag::Quiet)) {
                act(desc_other, ch, argument, d.character(), To::Vict, MobTrig::Yes, Position::Type::Dead);
            }
        }
    }
}

void do_announce(Char *ch) { toggle_channel(ch, CommFlag::NoAnnounce, "Announce"); }

void do_immtalk(Char *ch, std::string_view argument) {
    if (argument.empty()) {
        toggle_channel(ch, CommFlag::NoWiz, "Immortal");
        return;
    }

    clear_enum_bit(ch->comm, CommFlag::NoWiz);

    const char *format = check_enum_bit(ch->act, PlayerActFlag::PlrAfk) ? "|w(|cAFK|w)|W $n: |c$t|w" : "|W$n: |c$t|w";
    if (ch->get_trust() >= LEVEL_HERO)
        act(format, ch, argument, nullptr, To::Char, MobTrig::Yes, Position::Type::Dead);
    for (auto &d : ch->mud_.descriptors().playing()) {
        if (d.character()->is_immortal() && !check_enum_bit(d.character()->comm, CommFlag::NoWiz))
            act(format, ch, argument, d.character(), To::Vict, MobTrig::Yes, Position::Type::Dead);
    }
}

void do_gossip(Char *ch, std::string_view argument) {
    channel_command(ch, argument, CommFlag::NoGossip, "Gossip", "|gYou gossip '{}|g'|w", "|g$n gossips '$t|g'|w");
}

void do_auction(Char *ch, std::string_view argument) {
    channel_command(ch, argument, CommFlag::NoAuction, "Auction", "You auction '{}|w'", "$n auctions '$t|w'");
}

void do_music(Char *ch, std::string_view argument) {
    channel_command(ch, argument, CommFlag::NoMusic, "Music", "|PYou MUSIC: '{}|P'|w", "|P$n MUSIC: '$t|P'|w");
}

void do_question(Char *ch, std::string_view argument) {
    channel_command(ch, argument, CommFlag::NoQuestion, "Q/A", "|GYou question '{}|G'|w", "|G$n questions '$t|G'|w");
}

void do_answer(Char *ch, std::string_view argument) {
    channel_command(ch, argument, CommFlag::NoQuestion, "Q/A", "|GYou answer '{}|G'|w", "|G$n answers '$t|G'|w");
}

void do_gratz(Char *ch, std::string_view argument) {
    channel_command(ch, argument, CommFlag::NoGratz, "Gratz", "|yYou gratz '{}|y'|w", "|y$n gratzes '$t|y'|w");
}

void do_allege(Char *ch, std::string_view argument) {
    channel_command(ch, argument, CommFlag::NoAllege, "Allege", "|pYou allege '{}|p'|w", "|p$n alleges '$t|p'|w");
}

void do_philosophise(Char *ch, std::string_view argument) {
    channel_command(ch, argument, CommFlag::NoPhilosophise, "Philosophise", "|WYou philosophise '{}|W'|w",
                    "|W$n philosophises '$t|W'|w");
}

void do_qwest(Char *ch, std::string_view argument) {
    channel_command(ch, argument, CommFlag::NoQwest, "Qwest", "|YYou qwest '{}|Y'|w", "|Y$n qwests '$t|Y'|w");
}

void do_shout(Char *ch, std::string_view argument) {
    channel_command(ch, argument, CommFlag::NoYell, "Shout", "|WYou shout '{}|W'|w", "|W$n shouts '$t|W'|w");
}
