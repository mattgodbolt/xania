/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once
struct Char;

// for the "practice" and "stat mob prac" commands.
namespace PracticeTabulator {

// Sends a table of victim's skills and their skill rating to ch.
void tabulate(Char *ch, const Char *victim);

// Sends a table of ch's skills and their skill rating to ch.
void tabulate(Char *ch);

}
