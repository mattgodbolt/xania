/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

// TODO convert to enum.
static inline constexpr auto BAN_PERMANENT = 1u;
static inline constexpr auto BAN_NEWBIES = 2u;
static inline constexpr auto BAN_PERMIT = 4u;
static inline constexpr auto BAN_PREFIX = 8u;
static inline constexpr auto BAN_SUFFIX = 16u;
static inline constexpr auto BAN_ALL = 32u;

extern bool check_ban(const char *site, int type);
