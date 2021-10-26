/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                    */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  clan.h: organised player clans                                       */
/*                                                                       */
/*************************************************************************/

#pragma once

#include "ClanCommFlag.hpp"

#include <array>

// Change this for new clans
constexpr inline auto NUM_CLANS = 4;

// OK the real guts
constexpr inline auto CLAN_MAX = 4;
constexpr inline auto CLAN_MEMBER = 0;
constexpr inline auto CLAN_HERO = CLAN_MAX - 1;
constexpr inline auto CLAN_LEADER = CLAN_MAX;

// Clan is for an entire clan
struct Clan {
    const char *name;
    const char *whoname;
    const char clanchar;
    std::array<const char *, CLAN_MAX + 1> levelname;
    int recall_vnum;
    int entrance_vnum;
};

// PcClan is a structure all PC's in clans have
struct PcClan {
    const Clan &clan;
    int clanlevel{CLAN_MEMBER};
    unsigned int channelflags{to_int(ClanCommFlag::ChannelOn)};

    [[nodiscard]] const char *level_name() const { return clan.levelname[clanlevel]; }
};

extern const std::array<Clan, NUM_CLANS> clantable;
