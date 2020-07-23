// akey.cpp

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "akey.hpp"

const char *akey::getrndreply() {
    const char *defaultmsg = "Mmm, sounds interesting...";
    if (totalwates == 0)
        return defaultmsg;
    int rndnum = random(totalwates), total = 0;
#ifdef DEBUG
    printf("grr: rndnum=%d totalwates=%d\n", rndnum, totalwates);
#endif
    for (int i = 0; i < numreplys; i++) {
#ifdef DEBUG
        printf("i=%d total=%d wate=%d\n", i, total, replys[i].wate);
#endif
        if ((total += (replys[i].wate)) > rndnum) {
#ifdef DEBUG
            puts("grr f");
#endif

#ifdef REDUCEWATES
            if (replys[i].wate > 1) {
                totalwates--;
                replys[i].wate--;
            }
#endif
            return replys[i].sent;
        }
    }
#ifdef DEBUG
    puts("getrndreply should have found a string");
#endif
    return defaultmsg;
}

akey::akey() {
    logic = NULL;
    replys = NULL;
    numreplys = totalwates = 0;
#ifdef TEST
    // puts("constructor akey()");
#endif
}

int akey::addlogic(char *logicstr) { return (logic = strdup(logicstr)) != NULL; }

int akey::addreply(int w, char *r) {
#ifdef CHECKMEM
    static int called = 0;
    called++;
#endif
    if (replys == NULL) { // replys first time allocated
        if ((replys = (reply *)malloc((numreplys + 1) * sizeof(reply))) == NULL) {
#ifdef CHECKMEM
            printf("realloc error in addreply in call %d\n", called);
#endif
            return 0;
        }
    } else if ((replys = (reply *)realloc(replys, (numreplys + 1) * sizeof(reply))) == NULL) {
#ifdef CHECKMEM
        printf("realloc error in addreply in call %d\n", called);
#endif
        return 0;
    }
    totalwates += w;
    replys[numreplys].wate = w;
    if ((replys[numreplys].sent = strdup(r)) == NULL) {
#ifdef CHECKMEM
        puts("out of mem for strdup in addreply");
#endif
        return 0;
    }
#ifdef DEBUG
    printf("reply added:%s", replys[numreplys].sent);
#endif
    numreplys++;
    return 1;
}

akey::~akey(){
    /*  if(logic!=NULL) free(logic);

      for(int i=0;i<(numreplys-1);i++)
        {
      puts( "~akey() called" );
          free(replys[i].sent);
        }
        free(replys);    */
};
