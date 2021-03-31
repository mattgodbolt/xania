/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                 */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  tables.h: headers for various tables of constants                    */
/*                                                                       */
/*************************************************************************/
/***************************************************************************
 *	ROM 2.4 is copyright 1993-1996 Russ Taylor			   *
 *	ROM has been brought to you by the ROM consortium		   *
 *	    Russ Taylor (rtaylor@pacinfo.com)				   *
 *	    Gabrielle Taylor (gtaylor@pacinfo.com)			   *
 *	    Brian Moore (rom@rom.efn.org)				   *
 *	By using this code, you have agreed to follow the terms of the	   *
 *	ROM license, in the file Rom24/doc/rom.license			   *
 ***************************************************************************/

#pragma once

#include "merc.h"

struct position_type {
    const char *name;
    const char *short_name;
};

struct size_type {
    const char *name;
};

struct item_type {
    int type;
    const char *name;
};
struct weapon_type {
    const char *name;
    sh_int vnum;
    sh_int type;
    sh_int *gsn;
};

/* game tables */
extern const struct position_type position_table[];
extern const struct size_type size_table[];

/* items */
extern const struct item_type item_table[];
extern const struct weapon_type weapon_table[];
