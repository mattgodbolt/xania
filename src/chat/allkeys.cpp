
// allkeys.cpp
// chris busch

#include "allkeys.hpp"

akeynode *allkeys::addkey() {
    akeynode *newone;
    if ((newone = new akeynode) == nullptr) {
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
    }
    return current;
}

allkeys::~allkeys() {
    if (first != nullptr) {
        reset();
    }
}
