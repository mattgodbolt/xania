/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                    */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  prog.h:  programmable mobiles header                                 */
/*                                                                       */
/*************************************************************************/

#pragma once

#include "merc.h"

/* typedefs */
typedef char EXEC;

/* defines */

#define SIG_TICK 0
#define SIG_MOBENTER 1
#define SIG_MOBLEAVE 2

/* NB on MOB_LEAVE you *must not* try anything against the mob!!!! It is
   for informational purposes only.  Hence *never* do a 'kill $victim' during
   a MOB_LEAVE signal call */

#define SIG_MOBFIGHT 3
#define SIG_MOBENDFIGHT 4
#define SIG_OBJENTER 5
#define SIG_OBJLEAVE 6
#define SIG_DOOROPEN 7
#define SIG_DOORCLOSE 8
#define SIG_SPOKENTO 9
#define SIG_OBJGIVEN 10
#define SIG_MAGIC 11
#define SIG_SLEEP 12
#define SIG_LOWLEVEL 13
#define SIG_PAYED 14

/* Structures */

struct mob_function {
    char *name;
    char *(*function)(CHAR_DATA *ch, char *args, char *line);
};

/* functions */
char *label_lookup(CHAR_DATA *ch, char *label);
void parse_line(CHAR_DATA *ch, char *dest, char *source);
void mobile_signal(CHAR_DATA *ch, sh_int signal);
void mobile_init(CHAR_DATA *ch);
void mobile_run(CHAR_DATA *ch, char *line);

/* mobdo_functions */
#define DECLARE_MOBDO(fn) char *fn(CHAR_DATA *ch, char *args, char *line)
DECLARE_MOBDO(mobdo_signal);
DECLARE_MOBDO(mobdo_parse);
DECLARE_MOBDO(mobdo_ifstring);
DECLARE_MOBDO(mobdo_ifinstring);
DECLARE_MOBDO(mobdo_ifnotinstring);
DECLARE_MOBDO(mobdo_ifname);
DECLARE_MOBDO(mobdo_ifnotstring);
DECLARE_MOBDO(mobdo_ifnotname);
DECLARE_MOBDO(mobdo_ifrange);
DECLARE_MOBDO(mobdo_ifge);
DECLARE_MOBDO(mobdo_ifle);
DECLARE_MOBDO(mobdo_ifgt);
DECLARE_MOBDO(mobdo_iflt);
DECLARE_MOBDO(mobdo_ifeq);
DECLARE_MOBDO(mobdo_ifne);
DECLARE_MOBDO(mobdo_return);
DECLARE_MOBDO(mobdo_goto);
DECLARE_MOBDO(mobdo_chance);
DECLARE_MOBDO(mobdo_scan);
DECLARE_MOBDO(mobdo_scanon);
DECLARE_MOBDO(mobdo_set);
DECLARE_MOBDO(mobdo_seteval);
