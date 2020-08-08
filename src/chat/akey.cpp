// akey.cpp

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "akey.hpp"

const char *akey::getrndreply() {
    const char *defaultmsg = "Mmm, sounds interesting...";
    if (totalwates == 0)
        return defaultmsg;
    int rndnum = random(totalwates), total = 0;
    for (int i = 0; i < numreplys; i++) {
        if ((total += (replys[i].wate)) > rndnum) {
            return replys[i].sent;
        }
    }
    return defaultmsg;
}

akey::akey() {
    logic = nullptr;
    replys = nullptr;
    numreplys = totalwates = 0;
}

int akey::addlogic(char *logicstr) { return (logic = strdup(logicstr)) != nullptr; }

int akey::addreply(int w, char *r) {
    if (replys == nullptr) { // replys first time allocated
        if ((replys = (reply *)malloc((numreplys + 1) * sizeof(reply))) == nullptr) {
            return 0;
        }
    } else if ((replys = (reply *)realloc(replys, (numreplys + 1) * sizeof(reply))) == nullptr) {
        return 0;
    }
    totalwates += w;
    replys[numreplys].wate = w;
    if ((replys[numreplys].sent = strdup(r)) == nullptr) {
        return 0;
    }
    numreplys++;
    return 1;
}
