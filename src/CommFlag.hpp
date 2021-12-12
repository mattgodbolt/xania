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

using CF = Flag<CommFlag>;
constexpr auto Quiet = CF{CommFlag::Quiet, "quiet"};
constexpr auto Deaf = CF{CommFlag::Deaf, "deaf"};
constexpr auto NoWiz = CF{CommFlag::NoWiz, "no_wiz"};
constexpr auto NoAuction = CF{CommFlag::NoAuction, "no_action"};
constexpr auto NoGossip = CF{CommFlag::NoGossip, "no_gossip"};
constexpr auto NoQuestion = CF{CommFlag::NoQuestion, "no_question"};
constexpr auto NoMusic = CF{CommFlag::NoMusic, "no_music"};
constexpr auto NoGratz = CF{CommFlag::NoGratz, "no_gratz"};
constexpr auto NoAnnounce = CF{CommFlag::NoAnnounce, "no_allege"};
constexpr auto NoPhilosophise = CF{CommFlag::NoPhilosophise, "no_philosophise"};
constexpr auto NoQwest = CF{CommFlag::NoQwest, "no_qwest"};
constexpr auto Compact = CF{CommFlag::Compact, "compact"};
constexpr auto Brief = CF{CommFlag::Brief, "brief"};
constexpr auto Prompt = CF{CommFlag::Prompt, "prompt"};
constexpr auto Combine = CF{CommFlag::Combine, "combine"};
constexpr auto ShowAfk = CF{CommFlag::ShowAfk, "show_afk"};
constexpr auto ShowDefence = CF{CommFlag::ShowDefence, "show_def"};
constexpr auto NoEmote = CF{CommFlag::NoEmote, "no_emote"};
constexpr auto NoYell = CF{CommFlag::NoYell, "no_yell"};
constexpr auto NoTell = CF{CommFlag::NoTell, "no_tell"};
constexpr auto NoChannels = CF{CommFlag::NoChannels, "no_channels"};
constexpr auto NoAllege = CF{CommFlag::NoAllege, "no_allege"};
constexpr auto Affect = CF{CommFlag::Affect, "affect"};

constexpr auto NumCommFlags = 23;
constexpr std::array<Flag<CommFlag>, NumCommFlags> AllCommFlags = {
    {Quiet,       Deaf,           NoWiz,   NoAuction, NoGossip,   NoQuestion, NoMusic, NoGratz,
     NoAnnounce,  NoPhilosophise, NoQwest, Compact,   Brief,      Prompt,     Combine, ShowAfk,
     ShowDefence, NoEmote,        NoYell,  NoTell,    NoChannels, NoAllege,   Affect}};

}