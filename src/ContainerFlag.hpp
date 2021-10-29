/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

#include "common/StandardBits.hpp"

#include <magic_enum.hpp>

// Bits for for container flags (value[1]).
enum class ContainerFlag : unsigned int { Closeable = A, PickProof = B, Closed = C, Locked = D };

[[nodiscard]] constexpr auto to_int(const ContainerFlag flag) noexcept {
    return magic_enum::enum_integer<ContainerFlag>(flag);
}
