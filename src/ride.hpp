/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                 */
/*  See the header to file: merc.h for original code copyrights          */
/*************************************************************************/
#pragma once

class Char;

void unride_char(Char *ch, Char *pet);
void thrown_off(Char *ch, Char *pet);
void fallen_off_mount(Char *ch);
void do_ride(Char *ch, const char *argument);
void do_dismount(Char *ch);
