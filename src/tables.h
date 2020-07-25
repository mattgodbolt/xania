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
    char *name;
    char *short_name;
};

struct sex_type {
    char *name;
    int sex;
};

struct size_type {
    char *name;
};

struct item_type {
    int type;
    char *name;
};
struct weapon_type {
    char *name;
    sh_int vnum;
    sh_int type;
    sh_int *gsn;
};

/* game tables */
extern const struct position_type position_table[];
extern const struct sex_type sex_table[];
extern const struct size_type size_table[];

/* items */
extern const struct item_type item_table[];
extern const struct weapon_type weapon_table[];

/* flag tables */
extern const struct flag_type act_flags[];
extern const struct flag_type plr_flags[];
extern const struct flag_type affect_flags[];
extern const struct flag_type off_flags[];
extern const struct flag_type imm_flags[];
extern const struct flag_type res_flags[];
extern const struct flag_type vuln_flags[];
extern const struct flag_type form_flags[];
extern const struct flag_type part_flags[];
extern const struct flag_type comm_flags[];
extern const struct flag_type extra_flags[];
extern const struct flag_type wear_flags[];
extern const struct flag_type weapon_flags[];
extern const struct flag_type container_flags[];
extern const struct flag_type portal_flags[];
extern const struct flag_type room_flags[];
extern const struct flag_type exit_flags[];
/* Grim: doubly defined!?
extern	const	struct	flag_type	room_flags[];
 */
extern const struct flag_type weapontype_flags[];
extern const struct flag_type gate_flags[];
extern const struct flag_type furniture_flags[];
extern const struct flag_type mprog_flags[];

/* Enumerated tables */
/*extern	const	struct	enum_type	sector_type[];*/

/* Functions */

bool is_stat(const struct flag_type *flag_table);
