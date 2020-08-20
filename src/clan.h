/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                    */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  clan.h: organised player clans                                       */
/*                                                                       */
/*************************************************************************/

#pragma once

#include <array>

// Change this for new clans
constexpr inline auto NUM_CLANS = 4;

// Bits in the clan channelflags
constexpr inline auto CLANCHANNEL_ON = 1u;
constexpr inline auto CLANCHANNEL_NOCHANNED = 2u;

// OK the real guts
constexpr inline auto CLAN_MAX = 4;
constexpr inline auto CLAN_MEMBER = 0;
constexpr inline auto CLAN_HERO = CLAN_MAX - 1;
constexpr inline auto CLAN_LEADER = CLAN_MAX;

// CLAN is for an entire clan
struct CLAN {
    const char *name;
    const char *whoname;
    const char clanchar;
    const char *levelname[CLAN_MAX + 1];
    int recall_vnum;
    int entrance_vnum;
};

// PCCLAN is a structure all PC's in clans have
struct PCCLAN {
    const CLAN *clan{};
    int clanlevel{CLAN_MEMBER};
    unsigned int channelflags{CLANCHANNEL_ON};
    const char *level_name() const { return clan->levelname[clanlevel]; }
};

extern const std::array<CLAN, NUM_CLANS> clantable;
