/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                    */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  olc_room.h:  adapted from TheMoog's original                         */
/*                                                                       */
/*************************************************************************/

#pragma once

#include "merc.h"

extern char *const dir_name[];
extern sh_int const rev_dir[];
extern AREA_DATA *area_first;
extern AREA_DATA *area_last;
extern ROOM_INDEX_DATA *room_index_hash[MAX_KEY_HASH];

/* info flags */
#define OLC_R_EXTRAS 1
