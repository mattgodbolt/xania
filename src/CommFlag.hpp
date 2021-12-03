/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

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
