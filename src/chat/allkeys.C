
// allkeys.cpp
// chris busch

#include "allkeys.hpp"
#include <stdio.h> //for puts

akeynode *allkeys::addkey() {
    akeynode *newone;
    if ((newone = new akeynode) == nullptr) {
#ifdef CHECKMEM
        puts("out of mem in allkeys::addkey()");
#endif
        return nullptr;
    }
    if (top != nullptr)
        top->next = newone;
    current = top = newone;
    top->next = nullptr;
    if (numkeys == 0)
        first = top;
    numkeys++;
    return current;
}

akeynode *allkeys::reset() { return (current = first); }

akeynode *allkeys::advance() {
    if (current != nullptr) {
        current = current->next;
    } else {
#ifdef DEBUG
        puts("advanced beyond allkeys list");
#endif
    }
    return current;
}

allkeys::~allkeys() {
    if (first != nullptr) {
        reset();
        /*		do{
                                delete current;
                                }while(advance()!=nullptr); */
    }
}
