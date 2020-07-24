/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                    */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  flags.h:  a front end and management system for flags                */
/*                                                                       */
/*************************************************************************/

#pragma once

typedef struct char_data CHAR_DATA;

#define INVALID_BIT (-1)

/* Takes: a template string of the form 'flag flag flag', or '*' for no flag,
 *	  ptr to a string of the form '+flagname -flagname flagname' to set, clear toggle in that article
 *	  current value of flag
 *	  the character attempting to modify the flag
 */
long flag_set(char *template, char *arg, long current_val, CHAR_DATA *ch);
/*
 * Display the current flags
 */
void display_flags(char *template, CHAR_DATA *ch, int current_val);
