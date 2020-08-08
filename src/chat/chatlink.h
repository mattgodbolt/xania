#pragma once

void startchat(const char *);
char *dochat(char *response_buf, const char *player_name, const char *msg, const char *npc_name);
void chatperform(CHAR_DATA *ch, CHAR_DATA *victim, const char *msg);
void chatperformtoroom(const char *text, CHAR_DATA *ch);
