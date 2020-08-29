/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2020 Xania Development Team                                 */
/*  See the header to file: merc.h for original code copyrights          */
/*  Chat bot originally written by Chris Busch in 1993-5, this file is a */
/*  reimplementation of that work.                                       */
/*************************************************************************/
#pragma once

void startchat(const char *);
std::string dochat(std::string_view player_name, std::string_view msg, std::string_view npc_name);
void chatperform(Char *to_npc, Char *from_player, std::string_view msg);
void chatperformtoroom(std::string_view text, Char *ch);
