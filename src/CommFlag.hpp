/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

#include "Flag.hpp"
#include "common/StandardBits.hpp"

#include <magic_enum.hpp>

enum class CommFlag : unsigned long {
    Quiet = A,
    Deaf = B,
    NoWiz = C,
    NoAuction = D,
    NoGossip = E,
    NoQuestion = F,
    NoMusic = G,
    NoGratz = H,
    NoAnnounce = I,
    NoPhilosophise = J,
    NoQwest = K,
    // Display Flags
    Compact = L,
    Brief = M,
    Prompt = N,
    Combine = O,
    // TelnetGa = P, -- No Longer Used, Probably Should Be Retired After A Pfile Sweep
    ShowAfk = Q,
    ShowDefence = R,
    /* 1 Flag Reserved, S */
    NoEmote = T,
    NoYell = U,
    NoTell = V,
    NoChannels = W,
    NoAllege = X,
    Affect = Y,
};

[[nodiscard]] constexpr auto to_int(const CommFlag flag) noexcept { return magic_enum::enum_integer<CommFlag>(flag); }

namespace CommFlags {

constexpr auto NumCommFlags = 23;
constexpr std::array<Flag, NumCommFlags> AllCommFlags = {{
    // clang-format off
    {to_int(CommFlag::Quiet), "quiet"},
    {to_int(CommFlag::Deaf), "deaf"},
    {to_int(CommFlag::NoWiz), "no_wiz"},
    {to_int(CommFlag::NoAuction), "no_action"},
    {to_int(CommFlag::NoGossip), "no_gossip"},
    {to_int(CommFlag::NoQuestion), "no_question"},
    {to_int(CommFlag::NoMusic), "no_music"},
    {to_int(CommFlag::NoGratz), "no_gratz"},
    {to_int(CommFlag::NoAnnounce), "no_allege"},
    {to_int(CommFlag::NoPhilosophise), "no_philosophise"},
    {to_int(CommFlag::NoQwest), "no_qwest"},
    {to_int(CommFlag::Compact), "compact"},
    {to_int(CommFlag::Brief), "brief"},
    {to_int(CommFlag::Prompt), "prompt"},
    {to_int(CommFlag::Combine), "combine"},
    {to_int(CommFlag::ShowAfk), "show_afk"},
    {to_int(CommFlag::ShowDefence), "show_def"},
    {to_int(CommFlag::NoEmote), "no_emote"},
    {to_int(CommFlag::NoYell), "no_yell"},
    {to_int(CommFlag::NoTell), "no_tell"},
    {to_int(CommFlag::NoChannels), "no_channels"},
    {to_int(CommFlag::NoAllege), "no_allege"},
    {to_int(CommFlag::Affect), "affect"}
    // clang-format on
}};

}