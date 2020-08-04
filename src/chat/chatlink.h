#pragma once

void startchat(const char *);
char *dochat(const char *talker, const char *msg, const char *target);
void chatperform(CHAR_DATA *ch, CHAR_DATA *victim, const char *msg);
void chatperformtoroom(const char *text, CHAR_DATA *ch);
