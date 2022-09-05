/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

struct Char;

void advance_level(Char *ch);
void gain_exp(Char *ch, int gain);
void update_handler();
bool is_safe_sentient(Char *ch, const Char *victim);
