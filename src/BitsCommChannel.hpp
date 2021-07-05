/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

#include "common/StandardBits.hpp"

// Bits for communication channels.
#define COMM_QUIET (A)
#define COMM_DEAF (B)
#define COMM_NOWIZ (C)
#define COMM_NOAUCTION (D)
#define COMM_NOGOSSIP (E)
#define COMM_NOQUESTION (F)
#define COMM_NOMUSIC (G)
#define COMM_NOGRATZ (H)
#define COMM_NOANNOUNCE (I)
#define COMM_NOPHILOSOPHISE (J)
#define COMM_NOQWEST (K)

// display flags
#define COMM_COMPACT (L)
#define COMM_BRIEF (M)
#define COMM_PROMPT (N)
#define COMM_COMBINE (O)
//#define COMM_TELNET_GA (P) -- no longer used, probably should be retired after a pfile sweep
#define COMM_SHOWAFK (Q)
#define COMM_SHOWDEFENCE (R)
/* 1 flag reserved, S */

/* penalties */
#define COMM_NOEMOTE (T)
#define COMM_NOSHOUT (U)
#define COMM_NOTELL (V)
#define COMM_NOCHANNELS (W)

#define COMM_NOALLEGE (X)
#define COMM_AFFECT (Y)
