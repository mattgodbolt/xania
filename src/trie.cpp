/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                 */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  trie.c: trie-based string lookup system                              */
/*                                                                       */
/*************************************************************************/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "trie.h"

#define NUM_CHILDREN 256

typedef struct trie trie_t;
typedef struct node node_t;
typedef struct leaf leaf_t;

struct trie {
    int allow_zerolength : 1;
    node_t *topnode;
};

enum node_type { tnode_twig, tnode_leaf };

struct node {
    int level;
    node_type type;
    union {
        node_t **children;
        leaf_t *leaf;
    } data;
};

struct leaf {
    int level;
    const char *name;
    void *value;
};

/* checks for the given string within the given node. */
static leaf_t *lookup_string(node_t *node, const char *name, int max_level) {
    if (!node) {
        return nullptr;
    }
    if (node->type == tnode_twig) {
        int i;
        void *result;
        node_t **children = node->data.children;

        if (name[0]) {
            int index = (int)(isalpha(name[0]) ? (name[0] | 0x20) : name[0]);
            result = lookup_string(children[index], name + 1, max_level);
            if (result) {
                return result;
            }
        } else {
            for (i = 0; i < NUM_CHILDREN; i++) {
                result = lookup_string(children[i], name, max_level);
                if (result) {
                    return result;
                }
            }
        }
    } else if (node->type == tnode_leaf) {
        leaf_t *leaf = node->data.leaf;
        if (leaf->level <= max_level) {
            return leaf;
        }
    }
    return nullptr;
}

/* looks up a given string within the trie. */
void *trie_get(void *trie, const char *name, int max_level) {
    trie_t *current = (trie_t *)trie;

    if (!current->allow_zerolength && !name[0]) {
        return nullptr;
    }

    leaf_t *leaf = lookup_string(current->topnode, name, max_level);
    if (leaf && !strncasecmp(leaf->name, name, strlen(name))) {
        if (leaf->level > max_level) {
            fprintf(stderr,
                    "SERIOUS BUG!  User of level %d almost ran command "
                    "\"%s\", which is level %d.\n",
                    max_level, leaf->name, leaf->level);
            return nullptr;
        }
        return leaf->value;
    }
    return nullptr;
}

/* destroys the given leaf. */
static void destroy_leaf(leaf_t *leaf) { free(leaf); }

/* destroys the given node. */
static void destroy_node(node_t *node) {
    if (node->type == tnode_twig) {
        for (int i = 1; i < NUM_CHILDREN; i++) {
            if (node->data.children[i]) {
                destroy_node(node->data.children[i]);
            }
        }
        free(node->data.children);
    } else if (node->type == tnode_leaf) {
        destroy_leaf(node->data.leaf);
    }
    free(node);
}

/* destroys the given trie. */
void trie_destroy(void *trie) {
    destroy_node(((trie_t *)trie)->topnode);
    free(trie);
}

/* creates a new twig node. */
static node_t *create_twignode(int level) {
    node_t *node = malloc(sizeof(node_t));
    if (node) {
        node->level = level;
        node->type = tnode_twig;
        node->data.children = calloc(NUM_CHILDREN, sizeof(node_t *));
        if (!node->data.children) {
            free(node);
            return nullptr;
        }
    }
    return node;
}

/* creates a new leaf node. */
static node_t *create_leafnode(const char *name, void *value, int level) {
    node_t *node = malloc(sizeof(node_t));
    leaf_t *leaf = malloc(sizeof(leaf_t));

    if (leaf && node) {
        leaf->name = name;
        leaf->value = value;
        leaf->level = level;
        node->level = 1;
        node->type = tnode_leaf;
        node->data.leaf = leaf;
        return node;
    }
    free(node);
    free(leaf);
    return nullptr;
}

/* inserts the given child into the given node. */
static void insert_node(node_t *node, node_t *child, int index) {
    node_t **children = node->data.children;

    children[index] = child;
    child->level = node->level + 1;
}

/* attempts to add a leaf to a node. */
static void add_leaf(node_t *node, const char *name, node_t *leaf) {
    node_t **children = node->data.children;
    int index = (int)(isalpha(name[0]) ? (name[0] | 0x20) : name[0]);
    node_t *child = children[index];

    if (children[0] == nullptr) {
        insert_node(node, leaf, 0);
    }

    if (child && name[0]) {
        if (child->type == tnode_twig) {
            add_leaf(child, name + 1, leaf);
        } else {
            node_t *new_node = create_twignode(node->level + 1);
            if (!new_node) {
                return;
            }
            insert_node(node, new_node, index);
            add_leaf(new_node, child->data.leaf->name + new_node->level, child);
            add_leaf(new_node, name + 1, leaf);
        }
    } else {
        insert_node(node, leaf, index);
    }
}

/* adds a new entry to a trie. */
void trie_add(void *trie, const char *name, void *value, int level) {
    node_t *leaf = create_leafnode(name, value, level);
    if (!leaf) {
        fprintf(stderr, "Couldn't create leaf for token \"%s\".\n", name);
        return;
    }
    add_leaf(((trie_t *)trie)->topnode, name, leaf);
}

/* adds a list of entries to the given trie. */
void trie_addlist(void *trie, trielist_t *list, int num) {
    for (int i = 0; i < num; i++) {
        trie_add(trie, list[i].name, list[i].value, list[i].level);
    }
}

/* creates a new trie with the given options. */
void *trie_create(int allow_zerolength) {
    trie_t *trie = malloc(sizeof(trie_t));
    if (!trie) {
        return nullptr;
    }
    trie->allow_zerolength = allow_zerolength;
    trie->topnode = create_twignode(0);
    if (!trie->topnode) {
        free(trie);
        return nullptr;
    }
    return trie;
}

static void print_tabs(FILE *out, int num) {
    for (int i = 0; i < num; i++) {
        fprintf(out, "\t");
    }
}

/* dumps the node in text form. */
static void node_dump(FILE *out, node_t *node, int indent) {
    if (node->type == tnode_twig) {
        node_t **children = node->data.children;
        for (int i = 0; i < NUM_CHILDREN; i++) {
            if (children[i]) {
                print_tabs(out, indent);
                fprintf(out, "Node found at offset %d (%c), level %d.\n", i, i ? i : '0', children[i]->level);
                node_dump(out, children[i], indent + 1);
            }
        }
    } else {
        leaf_t *leaf = node->data.leaf;
        print_tabs(out, indent);
        fprintf(out, "leaf name \"%s\", value %p, level %d\n", leaf->name, leaf->value, leaf->level);
    }
}

/* dumps the trie in text form. */
void trie_dump(void *trie, char *filename) {
    FILE *out = fopen(filename, "w");
    if (!out) {
        fprintf(stderr, "Failed to open output file for dumping.\n");
        return;
    }
    fprintf(out, "Top level ->\n");
    node_dump(out, ((trie_t *)trie)->topnode, 1);
    fclose(out);
}

/* Enumerates the contents of the given node in the given level range, and call the function on each one. */
static void node_enumerate(node_t *node, int min_level, int max_level, trie_enum_fn_t *ef, void *metadata) {
    if (node->type == tnode_twig) {
        node_t **children = node->data.children;
        if (children[0] && children[0]->level > (int)strlen(children[0]->data.leaf->name))
            node_enumerate(children[0], min_level, max_level, ef, metadata);
        for (int i = 1; i < NUM_CHILDREN; i++) {
            if (children[i]) {
                node_enumerate(children[i], min_level, max_level, ef, metadata);
            }
        }
    } else {
        leaf_t *leaf = node->data.leaf;
        if (leaf->level >= min_level && leaf->level <= max_level) {
            ef(leaf->name, leaf->level, leaf->value, metadata);
        }
    }
}

/* Enumerates the contents of the trie between the given levels (inclusive), and calls the given
 * function for each one. */
void trie_enumerate(void *trie, int min_level, int max_level, trie_enum_fn_t *ef, void *metadata) {
    node_enumerate(((trie_t *)trie)->topnode, min_level, max_level, ef, metadata);
}
