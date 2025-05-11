/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2025 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "Duel.hpp"

#include "Act.hpp"
#include "Char.hpp"
#include "Logging.hpp"
#include "fight.hpp"
#include "handler.hpp"
#include "lookup.h"
#include "magic.h"

namespace Duels {
constexpr sh_int InvitationTimeoutTicks = 2;
constexpr ush_int MinDuellingLevel = 20;
constexpr ush_int MaxDuellingLevelRange = 10;
constexpr auto invitation_expired = "Your invitation to duel has expired."sv;
constexpr auto duel_expired = "Your duel has taken longer than the Gods will allow and is deemed to be a draw."sv;

bool Duel::is_accepted() const noexcept { return state == State::Accepted; }

std::optional<std::string> validate_duellists(const Char &ch, const Char &challengee) {
    if (challengee.is_npc()) {
        return {"You can only duel other players!"};
    }
    if (&ch == &challengee) {
        return {"Suicide is a mortal sin."};
    }
    if (ch.is_immortal() || challengee.is_immortal()) {
        return {"Deities are not permitted to duel."};
    }
    if (ch.is_switched() || challengee.is_switched()) {
        return {"Creatures being controlled by a player are not permitted to duel."};
    }
    if (challengee.level < Duels::MinDuellingLevel
        || std::abs(ch.level - challengee.level) > Duels::MaxDuellingLevelRange) {
        return {fmt::format("You can duel other players who are at least level {} and within {} levels of you.",
                            Duels::MinDuellingLevel, Duels::MaxDuellingLevelRange)};
    }
    if (challengee.is_pos_preoccupied() || challengee.is_aff_charm()) {
        return {"They don't look ready to consider a duel at the moment."};
    }
    if (challengee.is_link_dead_pc()) {
        return {"They can't duel right now as they've been disconnected."};
    }
    return std::nullopt;
}

void send_invitation(Char *challenger, Char *challengee) {
    challenger->pcdata->duel.emplace(
        Duel{.rival{challengee}, .timeout_ticks{InvitationTimeoutTicks}, .state{State::Accepted}});
    challengee->pcdata->duel.emplace(
        Duel{.rival{challenger}, .timeout_ticks{InvitationTimeoutTicks}, .state{State::Invited}});
    act("You challenge $N to a duel to the death!\nYou must await their response.", challenger, nullptr, challengee,
        To::Char);
    act("$n has challenged you to a duel to the death!\nYou have about 60 seconds to consent by entering: duel $n",
        challenger, nullptr, challengee, To::Vict);
    act("$n has challenged $N to a duel to the death!", challenger, nullptr, challengee, To::NotVict);
}

void accept_invitation(Char *challenger, Char *challengee) {
    challenger->pcdata->duel->timeout_ticks = DuelTimeoutTicks;
    challengee->pcdata->duel->timeout_ticks = DuelTimeoutTicks;
    challenger->pcdata->duel->state = State::Accepted;
    challengee->pcdata->duel->state = State::Accepted;
    act("You consent to a duel with $N. Good luck!", challenger, nullptr, challengee, To::Char);
    act("$n consents to your invitation to duel. En garde!", challenger, nullptr, challengee, To::Vict);
    act("A duel between $n and $N begins.", challenger, nullptr, challengee, To::NotVict);
    const int strife_num = skill_lookup("strife");
    spell_strife_duelist(strife_num, MinDuellingLevel, challenger, SpellTarget(challengee));
    spell_strife_duelist(strife_num, MinDuellingLevel, challengee, SpellTarget(challenger));
}

// Does a Char already have a duel in progress? This will return true for a Char who's issued an invitation
// even if the rival hasn't accepted the invitation yet.
bool is_duel_accepted(const Char &ch) {
    auto &duel = ch.pcdata->duel;
    return duel && duel->is_accepted();
}

void issue_or_accept_invitation(Char *challenger, Char *challengee) {
    auto &challenger_duel = challenger->pcdata->duel;
    if (challenger_duel && !challenger_duel->is_accepted() && challenger_duel->rival != challengee) {
        challenger->send_line("You've already challenged {}. You can only duel one player at a time.",
                              challenger_duel->rival->name);
        return;
    }
    auto &challengee_duel = challengee->pcdata->duel;
    if (challengee_duel) {
        // If the challengee already invited the challenger to duel, accept the invitation.
        if (challengee_duel->rival == challenger) {
            accept_invitation(challenger, challengee);
        } else {
            // Note that the challengee may have an invitation from another challenger that they've not accepted
            // yet. The new challenger must wait for the invitation to clear.
            challenger->send_line("{} is duelling someone else, try again later.", challengee->name);
        }
    } else {
        send_invitation(challenger, challengee);
    }
}

void negotiate_duel(Char *challenger, Char *challengee) {
    if (is_duel_accepted(*challenger)) {
        challenger->send_line("You're already duelling {}. You can only duel one player at a time.",
                              challenger->pcdata->duel->rival->name);
        return;
    }
    issue_or_accept_invitation(challenger, challengee);
}

void terminate_duel(Char *ch, std::optional<std::string_view> char_msg, std::optional<std::string_view> rival_msg) {
    if (auto &duel = ch->pcdata->duel; duel) {
        if (char_msg) {
            ch->send_line(*char_msg);
        }
        Char *rival = duel->rival;
        duel.reset();
        if (rival && rival->pcdata->duel) {
            rival->pcdata->duel.reset();
            if (rival_msg) {
                rival->send_line(*rival_msg);
            }
        }
    }
}

void check_duel_timeout(Char *ch) {
    auto &duel = ch->pcdata->duel;
    if (!duel) {
        return;
    }
    if (--duel->timeout_ticks > 0) {
        return;
    }
    // If the rivals are dueling, they only have DuelTimeoutTicks to decide the winner.
    if (is_duel_in_progress(ch, duel->rival)) {
        // Passing false to stop_fighting() because it is possible that aggressive NPCs in the same room as the duelist
        // may also be attacking them and should continue to after the duel ends.
        if (ch->fighting == duel->rival) {
            stop_fighting(ch, false);
        }
        if (duel->rival->fighting == ch) {
            stop_fighting(duel->rival, false);
        }
        terminate_duel(ch, duel_expired, duel_expired);
    } else {
        // Invitation phase has expired.
        terminate_duel(ch, invitation_expired, invitation_expired);
    }
}

bool is_duel_in_progress(const Char *ch, const Char *rival) {
    if (ch->is_npc() || rival->is_npc() || !is_duel_accepted(*ch) || !is_duel_accepted(*rival)) {
        return false;
    }
    const auto &duel = ch->pcdata->duel;
    const auto &rival_duel = rival->pcdata->duel;
    return duel->is_accepted() && rival_duel->is_accepted() && duel->rival == rival;
}

std::optional<std::string_view> get_duel_rival_name(const Char *ch) {
    if (ch->is_pc()) {
        if (const auto duel = ch->pcdata->duel; duel) {
            return duel->rival->name;
        }
    }
    return std::nullopt;
}

}

void do_duel(Char *ch, ArgParser args) {
    if (is_safe_room(ch)) {
        ch->send_line("Duels aren't permitted here.");
        return;
    }
    if (args.empty()) {
        ch->send_line("Duel whom?");
        return;
    }
    auto *challengee = get_char_room(ch, args.shift());
    if (!challengee) {
        ch->send_line("There's nobody here by that name.");
        return;
    }
    if (auto failure_msg = Duels::validate_duellists(*ch, *challengee)) {
        ch->send_line(*failure_msg);
        return;
    }
    Duels::negotiate_duel(ch, challengee);
}