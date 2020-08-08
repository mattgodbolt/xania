// Chris Busch (c) 1993
// please see chat.doc
#include "Eliza.hpp"

using namespace chat;

Eliza chatter;

void startchat(const char *filename) { chatter.load_databases(filename); }

const char *dochat(char *response_buf, const char *player_name, const char *msg, const char *npc_name) {
    return chatter.handle_player_message(response_buf, player_name, msg, npc_name);
}
