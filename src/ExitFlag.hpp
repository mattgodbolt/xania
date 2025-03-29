/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

#include "common/StandardBits.hpp"

#include <magic_enum/magic_enum.hpp>

enum class ExitFlag : unsigned int {
    // Bits for Room Exits.
    IsDoor = A,
    Closed = B,
    Locked = C,
    PickProof = F,
    PassProof = G,
    Easy = H,
    Hard = I,
    Infuriating = J,
    NoClose = K,
    NoLock = L,
    Trap = M,
    TrapDarts = N,
    TrapPoison = O,
    TrapExploding = P,
    TrapSleepgas = Q,
    TrapDeath = R,
    Secret = S
};

[[nodiscard]] constexpr auto to_int(const ExitFlag flag) noexcept { return magic_enum::enum_integer<ExitFlag>(flag); }
