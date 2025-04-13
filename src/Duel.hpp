/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2025 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once
#include "Types.hpp"

#include <optional>
#include <string_view>

namespace Duels {

constexpr auto DuelTimeoutTicks = 10u;

enum class State { Invited, Accepted };

// Represents one half of a duelling negotiation. This is initialized in the PcData.duel of a challenger _and_
// their rival when a challenger types duel <rival>.
struct Duel {
    // Returns true if this half of the negotiation has been accepted.
    // A Char sending an invitation has implicitly accepted the duel too.
    [[nodiscard]] bool is_accepted() const noexcept;

    Char *rival;
    sh_int timeout_ticks;
    State state;
};

void terminate_duel(Char *ch, std::optional<std::string_view> char_msg, std::optional<std::string_view> rival_msg);

// While awaiting consent from the rival, timeout_ticks counts down every tick and when it hits zero, the Duel expires
// and is neutralized on both sides of the negotiation. If the duel has been accepted by both rivals,
// they have DuelTimeoutTicks to determine the winner.
void check_duel_timeout(Char *ch);
// Returns true if a duel has been accepted by both rivals.
bool is_duel_in_progress(const Char *ch, const Char *rival);
std::optional<std::string_view> get_duel_rival_name(const Char *ch);

}
