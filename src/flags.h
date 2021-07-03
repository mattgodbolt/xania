/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                    */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  flags.h:  a front end and management system for flags                */
/*                                                                       */
/*************************************************************************/

#pragma once

struct Char;

static constexpr auto INVALID_BIT = static_cast<unsigned long>(-1);

/* Takes: a template string of the form 'flag flag flag', or '*' for no flag,
 *	  ptr to a string of the form '+flagname -flagname flagname' to set, clear toggle in that article
 *	  current value of flag
 *	  the character attempting to modify the flag
 */
unsigned long flag_set(const char *format, const char *arg, unsigned long current_val, Char *ch);

char *print_flags(int value);

/*
 * Display the current flags
 */
void display_flags(const char *format, Char *ch, unsigned long current_val);
