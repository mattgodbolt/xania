/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                    */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  challeng.h: supervised player combat. Originally by Rohan and        */
/*              Wandera. Revised by Oshea 26/8/96                        */
/*************************************************************************/

#pragma once
#include "merc.h"

/* To keep things general. */
#define CHAL_ROOM 30011
#define CHAL_PREP 30012

int fighting_duel(CHAR_DATA *ch, CHAR_DATA *victim);
int in_duel(CHAR_DATA *ch);
void do_chal_tick(void);
int do_check_chal(CHAR_DATA *ch);
void raw_kill(CHAR_DATA *victim);
