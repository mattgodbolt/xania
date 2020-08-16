/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2020 Xania Development Team                                 */
/*  See the header to file: merc.h for original code copyrights          */
/*  Chat bot originally written by Chris Busch in 1993-5, this file is a */
/*  reimplementation of that work.                                       */
/*************************************************************************/
#include "Eliza.hpp"

using namespace chat;

Eliza chatter;

void startchat(const char *filename) { chatter.load_databases(filename); }

std::string dochat(std::string_view player_name, std::string_view msg, std::string_view npc_name) {
    return chatter.handle_player_message(player_name, msg, npc_name);
}
