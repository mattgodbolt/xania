// Chris Busch (c) 1993
// please see chat.doc
#include "Eliza.hpp"

using namespace chat;

Eliza chatter;

void startchat(const char *filename) { chatter.load_databases(filename); }

const char *dochat(char *response_buf, std::string_view player_name, std::string_view msg, std::string_view npc_name) {
    return chatter.handle_player_message(response_buf, player_name, msg, npc_name);
}
