#pragma once

void startchat(const char *);
char *dochat(char *response_buf, std::string_view player_name, std::string_view msg, std::string_view npc_name);
void chatperform(CHAR_DATA *ch, CHAR_DATA *victim, const char *msg);
void chatperformtoroom(const char *text, CHAR_DATA *ch);
