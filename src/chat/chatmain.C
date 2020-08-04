
// chatmain.C
// Chris Busch (c) 1993
// please see chat.doc

#include "akey.hpp"
#include "allkeys.hpp"
#include "chatmain.hpp"
#include "eliza.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/////YOU MAY NOT change the next 2 lines.
const char eliza_title[] = "chat by Christopher Busch  Copyright (c)1993";
const char eliza_version[] = "version 1.0.0";

#ifdef UNIX
void randomize() { srand((int)time(nullptr)); }

int random(int x) { return rand() % x; }

#endif // UNIX

eliza chatter;

void startchat(const char *filename) {
    char buf[] = "";
    chatter.reducespaces(buf);
    chatter.loaddata(filename);
}

const char *dochat(char *talker, char *msg, char *target) { return chatter.process(talker, msg, target); }
