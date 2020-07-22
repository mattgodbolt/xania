/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                    */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  clan.h: organised player clans                                       */
/*                                                                       */
/*************************************************************************/

#ifndef _clan_h
#define _clan_h

/* Change this for new clans */
#define NUM_CLANS 4

/* Bits in the clan channelflags */
#define CLANCHANNEL_ON 1
#define CLANCHANNEL_NOCHANNED 2

/* OK the real guts */

#define CLAN_MAX 4
#define CLAN_MEMBER 0
#define CLAN_HERO CLAN_MAX - 1
#define CLAN_LEADER CLAN_MAX

/* CLAN is for an entire clan */

typedef struct _clan {
    char *name;
    char *whoname;
    char clanchar;
    char *levelname[CLAN_MAX + 1];
    int recall_vnum;
    int entrance_vnum;
} CLAN;

/* PCCLAN is a structure all PC's in clans have */
typedef struct _pcclan {
    CLAN *clan;
    int clanlevel;
    int channelflags;
} PCCLAN;

extern const CLAN clantable[NUM_CLANS];

#endif
/* clan_h */
