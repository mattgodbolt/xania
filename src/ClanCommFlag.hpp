/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

#include "common/StandardBits.hpp"

#include <magic_enum/magic_enum.hpp>

enum class ClanCommFlag : unsigned int { ChannelOn = A, ChannelRevoked = B };

[[nodiscard]] constexpr auto to_int(const ClanCommFlag flag) noexcept {
    return magic_enum::enum_integer<ClanCommFlag>(flag);
}
