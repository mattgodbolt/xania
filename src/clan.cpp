/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                    */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  clan.c: organised player clans                                       */
/*                                                                       */
/*************************************************************************/

#include "Act.hpp"
#include "Char.hpp"
#include "CommFlag.hpp"
#include "Descriptor.hpp"
#include "DescriptorList.hpp"
#include "Interpreter.hpp"
#include "common/BitOps.hpp"
#include "db.h"
#include "handler.hpp"
#include "string_utils.hpp"

#include <fmt/format.h>
#include <range/v3/algorithm/find_if.hpp>

#include <algorithm>

/* User serviceable bits... you will also need to change the NUM_CLANS in clan.h */

const std::array<Clan, NUM_CLANS> clantable = {{
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

}};

void do_clantalk(Char *ch, std::string_view argument) {
    auto *orig_clan = ch->desc->person() ? ch->desc->person()->pc_clan() : nullptr;
    if (orig_clan == nullptr) {
        ch->send_line("You are not a member of a clan.");
        return; /* Disallow mortal PC's with no clan */
    }

    if (check_enum_bit(ch->comm, CommFlag::Quiet)) {
        ch->send_line("You must remove quiet mode first.");
        return;
    }

    if (argument.empty() || !orig_clan->channelflags) {
        /* They want to turn it on/off */
        toggle_enum_bit(orig_clan->channelflags, ClanCommFlag::ChannelOn);
        ch->send_line("Clan channel now {}.",
                      check_enum_bit(orig_clan->channelflags, ClanCommFlag::ChannelOn) ? "on" : "off");
        return;
    }

    /* Next check to see if a CLAN_HERO or CLAN_LEADER is on first */

    auto playing = ch->mud_.descriptors().playing();
    if (ranges::find_if(playing,
                        [&](const Descriptor &d) {
                            const auto *victim = d.person();
                            const auto *pcclan = victim->pc_clan();

                            return pcclan && pcclan->clan.clanchar == orig_clan->clan.clanchar
                                   && pcclan->clanlevel >= CLAN_HERO && !check_enum_bit(victim->comm, CommFlag::Quiet);
                        })
        == playing.end()) {
        ch->send_to("Your clan lacks the necessary broadcast nexus, causing your vain telepathy to\n\r"
                    "be lost upon the winds.\n\r");
        return;
    }

    if (check_enum_bit(orig_clan->channelflags, ClanCommFlag::ChannelRevoked)) {
        ch->send_line("Your clan channel privileges have been revoked!");
        return;
    }

    ch->set_not_afk();

    if (!orig_clan->channelflags) {
        toggle_enum_bit(orig_clan->channelflags, ClanCommFlag::ChannelOn);
        ch->send_line("Clan channel now {}.",
                      check_enum_bit(orig_clan->channelflags, ClanCommFlag::ChannelOn) ? "on" : "off");
    }

    /* Right here we go - tell all members of the clan the message */
    for (auto &d : ch->mud_.descriptors().playing()) {
        auto *victim = d.person();
        const auto *pcclan = victim->pc_clan();
        if (pcclan && pcclan->clan.clanchar == orig_clan->clan.clanchar
            && check_enum_bit(pcclan->channelflags, ClanCommFlag::ChannelOn)
            && !check_enum_bit(victim->comm, CommFlag::Quiet)
            /* || they're an IMM snooping the channels */) {
            d.character()->send_line("|G<{}> {}|w", d.character()->can_see(*ch) ? ch->name : "Someone", argument);
        }
    }
}

void do_noclanchan(Char *ch, ArgParser args) {
    if (ch->is_npc())
        return;
    if ((ch->get_trust() > (MAX_LEVEL - 5)) || ((ch->pc_clan() != nullptr) && (ch->pc_clan()->clanlevel > CLAN_HERO))) {
    } else {
        ch->send_line("Huh?");
        return;
    }
    auto *victim = get_char_world(ch, args.shift());
    if (!victim || victim->is_npc()) {
        ch->send_line("They're not here.");
        return;
    }
    /* Check if victim is same clan, and not higher status */
    auto *victim_pcclan = victim->pc_clan();
    if (!victim_pcclan || (victim_pcclan->clan.clanchar != ch->clan()->clanchar)
        || (victim_pcclan->clanlevel > ch->pc_clan()->clanlevel)) {
        ch->send_line("You can't noclanchan {}!", victim->name);
        return;
    }
    if (victim == ch) {
        ch->send_line("You cannot noclanchannel yourself.");
        return;
    }
    toggle_enum_bit(victim_pcclan->channelflags, ClanCommFlag::ChannelRevoked);
    ch->send_line("You have {}ed {}'s clan channel privileges.",
                  check_enum_bit(victim_pcclan->channelflags, ClanCommFlag::ChannelRevoked) ? "revok" : "reinstat",
                  victim->name);
    victim->send_line("{} has {}ed your clan channel privileges.", upper_first_character(ch->name),
                      check_enum_bit(victim_pcclan->channelflags, ClanCommFlag::ChannelRevoked) ? "revok" : "reinstat");
}

void do_member(Char *ch, ArgParser args) {
    if (ch->is_npc())
        return;
    if (!ch->pc_clan() || ch->pc_clan()->clanlevel < CLAN_LEADER) {
        ch->send_line("Huh?"); /* Cheesy cheat */
        return;
    }
    auto action = args.shift();
    if (action != "+" && action != "-") {
        ch->send_line("Usage:\n\r       member + <character name>\n\r       member - <character name>");
        return;
    }
    auto whom = args.shift();
    auto *victim = get_char_room(ch, whom);
    if (!victim || victim->is_npc()) {
        ch->send_line("You can't see them here.");
        return;
    }
    if (victim == ch) {
        ch->send_line("What kind of wally are you?  You can't do that!");
        return;
    }
    if (victim->get_trust() > ch->get_trust()) {
        ch->send_line("You cannot do that to {}.", victim->name);
        return;
    }

    if (action == "+") {
        /* Adding a new member of the clan */
        if (victim->clan()) { /* The person is *already* in a clan */
            if (victim->clan()->clanchar == ch->clan()->clanchar) {
                ch->send_to("{} is already a member of the {}.\n\rUse 'promote' to promote characters.\n\r",
                            victim->name, ch->clan()->name);
                return;
            } else {
                /* In another clan ! */
                ch->send_to("{} is a member of the {}.\n\rThey must leave that clan first.\n\r", victim->name,
                            ch->clan()->name);
                return;
            }
        }
        victim->pcdata->pcclan.emplace(PcClan{*ch->clan()});
        act(fmt::format("{} welcomes {} to the {}", ch->name, victim->name, ch->clan()->name), ch, nullptr, victim,
            To::NotVict);
        victim->send_line("You have become {} of the {}.", ch->pc_clan()->level_name(), ch->clan()->name);
        ch->send_line("You welcome {} as {} of the {}.", victim->name, ch->pc_clan()->level_name(), ch->clan()->name);
    } else {
        /* Removing a person from a clan */
        if (victim->clan() == nullptr || victim->clan()->clanchar != ch->clan()->clanchar) {
            ch->send_line("{} is not a member of your clan.", victim->name);
            return;
        } /* If not in clan */
        victim->pcdata->pcclan.reset();
        act(fmt::format("{} removes {} from the {}", ch->name, victim->name, ch->clan()->name), ch, nullptr, victim,
            To::NotVict);
        victim->send_line("You have been discharged from the {}.", ch->clan()->name);
        ch->send_line("You remove {} from the {}.", victim->name, ch->clan()->name);
    }
}

void mote(Char *ch, ArgParser args, const int level_change) {
    if (ch->is_npc())
        return;
    if (!ch->pc_clan() || ch->pc_clan()->clanlevel < CLAN_LEADER) {
        ch->send_line("Huh?"); /* Cheesy cheat */
        return;
    }
    auto *victim = get_char_room(ch, args.shift());
    if (!victim) {
        ch->send_line("You can't see them here.");
        return;
    }
    if (victim == ch) {
        ch->send_line("What kind of wally are you?  You can't do that!");
        return;
    }
    if (!victim->pc_clan() || victim->clan()->clanchar != ch->clan()->clanchar) {
        ch->send_line("You don't have authority to promote or demote them.");
        return;
    }
    if ((victim->pc_clan()->clanlevel + level_change) > CLAN_HERO) {
        ch->send_line("You cannot make {} into another leader.", victim->name);
        return;
    }
    if ((victim->pc_clan()->clanlevel + level_change) < CLAN_MEMBER) {
        ch->send_line("You can't demote below member status.");
        return;
    }
    victim->pcdata->pcclan->clanlevel += level_change;
    act(fmt::format("$n is now {} of the {}.", victim->pc_clan()->level_name(), victim->clan()->name), victim);
    victim->send_line("You are now {} of the {}.", victim->pc_clan()->level_name(), victim->clan()->name);
}

void do_promote(Char *ch, ArgParser args) { mote(ch, args, 1); }

void do_demote(Char *ch, ArgParser args) { mote(ch, args, -1); }

void do_clanwho(Char *ch) {
    if (ch->is_npc())
        return;

    if (!ch->clan()) {
        ch->send_line("You have no clan.");
        return;
    }

    ch->send_line("|gCharacter name     |c|||g Clan level|w");
    ch->send_line("|c-------------------+-------------------------------|w");
    for (auto &d : ch->mud_.descriptors().all_visible_to(*ch)) {
        auto *wch = d.person();
        if (wch->clan() && wch->clan()->clanchar == ch->clan()->clanchar) {
            ch->send_line("{:19}|c|||w {}", wch->name, wch->pc_clan()->level_name());
        }
    }
}

void do_clanset(Char *ch, ArgParser args) {
    auto action = args.shift();
    if (action != "+" && action != "-" && !matches_start(action, "leader")) {
        ch->send_line("Syntax: clanset {+/-}  <player> <clan>\n\r        clanset leader <player>");
        return;
    }

    if (action == "+" || action == "-") {
        auto whom = args.shift();
        if (whom.empty()) {
            ch->send_line("You must provide a valid player name.");
            return;
        }
        auto *victim = get_char_world(ch, whom);
        if (!victim || victim->is_npc()) {
            ch->send_line("They're not here.");
            return;
        }
        if (victim->get_trust() > ch->get_trust()) {
            ch->send_line("You do not have the powers to do that.");
            return;
        }
        const Clan *clan_to_add_to{};
        auto clan_name = args.shift();
        if (action == "+") {
            if (victim->clan()) {
                ch->send_line("{} is already in a clan.", victim->name);
                return;
            }
            if (clan_name.empty()) {
                ch->send_line("You must provide a valid clan name.");
                return;
            }
            for (auto &clan : clantable)
                if (is_name(clan_name, clan.name)) {
                    clan_to_add_to = &clan;
                    break;
                }
            if (!clan_to_add_to || (matches(clan_name, "Implementor") && ch->get_trust() < IMPLEMENTOR)) {
                ch->send_line("That is not a valid clan name.");
                ch->send_line("Do 'help clanlist' to see those available.");
                return;
            }
            victim->pcdata->pcclan.emplace(PcClan{*clan_to_add_to});
            ch->send_to("You set {} as {} of the {}.\n\r", victim->name, victim->pc_clan()->level_name(),
                        victim->clan()->name);
            return;
        } else {
            if (!victim->clan()) {
                ch->send_line("{} is not a member of a clan.", victim->name);
                return;
            }
            auto clan_name = victim->clan()->name;
            victim->pcdata->pcclan.reset();
            ch->send_line("You remove {} from the {}.", victim->name, clan_name);
            return;
        }
    }
    if (matches_start(action, "leader")) {
        auto whom = args.shift();
        if (whom.empty()) {
            ch->send_line("You must provide a valid player name.");
            return;
        }
        auto *victim = get_char_world(ch, whom);
        if (!victim || victim->is_npc()) {
            ch->send_line("They're not here.");
            return;
        }
        if (victim->get_trust() > ch->get_trust()) {
            ch->send_line("You do not have the powers to do that.");
            return;
        }
        if (!victim->clan()) {
            ch->send_line("{} is not in a clan.", victim->name);
            return;
        }
        victim->pcdata->pcclan->clanlevel = CLAN_LEADER;
        ch->send_line("Ok.");
        return;
    }
    ch->send_line("Syntax: clanset {+/-}  <player> <clan>\n\r        clanset leader <player>");
}

/*
umm..has to check descriptor list for clan rank of all present
         and in that particular clan. Has to do this every time
         someone uses channel?
         if so then channel  is_active
         else
         ch->send_to("Your clan lacks the necessary broadcast nexus, causing your vain telepathy is lost upon the
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
