/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                 */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  trie.h: trie-based string lookup system                              */
/*                                                                       */
/*************************************************************************/

#ifndef __trie_h
#define __trie_h

typedef void(trie_enum_fn_t)(const char *name, int level, void *data, void *metadata);

typedef struct trielist {
    const char *name;
    void *value;
    int level;
} trielist_t;

void *trie_get(void *trie, const char *name, int max_level);
void trie_destroy(void *trie);
void trie_add(void *trie, const char *name, void *value, int level);
void trie_addlist(void *trie, trielist_t *list, int num);
void *trie_create(int allow_zerolength);
void trie_dump(void *trie, char *filename);
void trie_enumerate(void *trie, int min_level, int max_level, trie_enum_fn_t *ef, void *metadata);

#endif
