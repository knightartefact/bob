#ifndef TREE_H_
#define TREE_H_

#include "index.h"

typedef struct tree_entry_s {
    char path[INDEX_MAX_PATH];
    uint8_t sha1[20];
    uint32_t mode;
} tree_entry_t;

typedef struct tree_list_s {
    tree_entry_t entries[INDEX_MAX_ENTRIES];
    int count;
} tree_list_t;

typedef struct raw_tree_entry_s {
    const char *mode_str;
    int mode_len;
    const char *name;
    const unsigned char *sha;
} raw_tree_entry_t;

char *tree_write(const index_t *idx);
int tree_flatten(const char *tree_hex, const char *prefix, tree_list_t *out);

#endif /* !TREE_H_ */
