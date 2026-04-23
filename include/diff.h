#ifndef DIFF_H_
#define DIFF_H_

#include "tree.h"

typedef enum { DIFF_ADDED, DIFF_DELETED, DIFF_MODIFIED } diff_type_t;

typedef struct diff_entry_s {
    char path[INDEX_MAX_PATH];
    diff_type_t type;
    uint8_t old_sha[20];
    uint8_t new_sha[20];
} diff_entry_t;

typedef struct diff_list_s {
    diff_entry_t entries[INDEX_MAX_ENTRIES];
    int count;
} diff_list_t;

void diff_trees(const tree_list_t *a, const tree_list_t *b, diff_list_t *out);
void diff_print_entry(const diff_entry_t *entry);

#endif /* !DIFF_H_ */
