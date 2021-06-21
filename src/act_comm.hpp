/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

#include <string_view>

class Char;

void announce(std::string_view buf, const Char *ch);
void add_follower(Char *ch, Char *master);
void stop_follower(Char *ch);
void nuke_pets(Char *ch);
void die_follower(Char *ch);
bool is_same_group(const Char *ach, const Char *bch);
