#include "trie.h"
#include <stdio.h>

static void printstring(void *trie, char *name, int lev) {
    char *str = (char *)trie_get(trie, name, lev);

    printf("String: %s\n", str);
}

static void fn(const char *name, int level, void *data, void *metadata) { printf("fn> %s\n", name); }

int main() {
    void *trie = trie_create(0);
    /*
            trie_add(trie, "foo", "Foo", 1);
            trie_add(trie, "bartap", "Bartap", 1);
            trie_add(trie, "bar", "Bar", 1);
            trie_add(trie, "bARson", "Barson", 1);
            trie_add(trie, "barstool", "Barstool", 1);
    */

    // By adding in read and ready only, we seem to prime the trie with
    // r [read]
    // r [read] -> e [read]
    // r [read] -> e [read] -> a [read]
    // r [read] -> e [read] -> a [read] -> d [read] -> nullptr
    //                                              |
    //												+> y [ready]
    trie_add(trie, "read", "read", 1);
    trie_add(trie, "ready", "ready", 1);

    trie_dump(trie, "trietest.out");

    trie_enumerate(trie, 0, 2, fn, nullptr);

    /*
    printstring(trie, "foo", 1);
    printstring(trie, "foob", 1);
    printstring(trie, "f", 1);
    printstring(trie, "bars", 1);
    printstring(trie, "", 1);
    printstring(trie, "BARt", 1);
    printstring(trie, "bartapfen", 1);
*/
    exit(0);
    return 1;
}
