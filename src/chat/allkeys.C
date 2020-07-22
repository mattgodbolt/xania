
// allkeys.cpp
// chris busch

#include "allkeys.hpp"
#include <stdio.h> //for puts

akeynode *allkeys::addkey() {
    akeynode *newone;
    if ((newone = new akeynode) == NULL) {
#ifdef CHECKMEM
        puts("out of mem in allkeys::addkey()");
#endif
        return NULL;
    }
    if (top != NULL)
        top->next = newone;
    current = top = newone;
    top->next = NULL;
    if (numkeys == 0)
        first = top;
    numkeys++;
    return current;
}

akeynode *allkeys::reset() { return (current = first); }

akeynode *allkeys::advance() {
    if (current != NULL) {
        current = current->next;
    } else {
#ifdef DEBUG
        puts("advanced beyond allkeys list");
#endif
    }
    return current;
}

allkeys::~allkeys() {
    if (first != NULL) {
        reset();
        /*		do{
                                delete current;
                                }while(advance()!=NULL); */
    }
}
