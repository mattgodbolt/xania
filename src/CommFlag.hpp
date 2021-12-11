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
constexpr std::array<Flag<CommFlag>, NumCommFlags> AllCommFlags = {{
    // clang-format off
    {CommFlag::Quiet, "quiet"},
    {CommFlag::Deaf, "deaf"},
    {CommFlag::NoWiz, "no_wiz"},
    {CommFlag::NoAuction, "no_action"},
    {CommFlag::NoGossip, "no_gossip"},
    {CommFlag::NoQuestion, "no_question"},
    {CommFlag::NoMusic, "no_music"},
    {CommFlag::NoGratz, "no_gratz"},
    {CommFlag::NoAnnounce, "no_allege"},
    {CommFlag::NoPhilosophise, "no_philosophise"},
    {CommFlag::NoQwest, "no_qwest"},
    {CommFlag::Compact, "compact"},
    {CommFlag::Brief, "brief"},
    {CommFlag::Prompt, "prompt"},
    {CommFlag::Combine, "combine"},
    {CommFlag::ShowAfk, "show_afk"},
    {CommFlag::ShowDefence, "show_def"},
    {CommFlag::NoEmote, "no_emote"},
    {CommFlag::NoYell, "no_yell"},
    {CommFlag::NoTell, "no_tell"},
    {CommFlag::NoChannels, "no_channels"},
    {CommFlag::NoAllege, "no_allege"},
    {CommFlag::Affect, "affect"}
    // clang-format on
}};

}