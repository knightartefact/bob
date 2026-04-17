#ifndef MERGE_H_
#define MERGE_H_

#include "index.h"
#include "tree.h"
#include "str/str.h"

typedef struct merge_conflict_s {
    char path[INDEX_MAX_PATH];
    uint8_t base_sha[20];
    uint8_t ours_sha[20];
    uint8_t theirs_sha[20];
    int has_base;
    int has_ours;
    int has_theirs;
    str_t merged;
} merge_conflict_t;

typedef struct merge_result_s {
    tree_list_t merged;
    merge_conflict_t conflicts[INDEX_MAX_ENTRIES];
    int conflict_count;
} merge_result_t;

int merge_find_base(const char *hex_a, const char *hex_b, char *out_hex);
int merge_trees(const tree_list_t *base, const tree_list_t *ours,
                const tree_list_t *theirs, merge_result_t *result);
int merge_write_conflict(const merge_conflict_t *conflict);
void merge_apply_to_worktree(merge_result_t *result);
int merge_create_commit(const char *branch, const char *ours_hex,
                        const char *theirs_hex);
int merge_resolve_commit_tree(const char *hex, tree_list_t *out);
int merge_write_head(const char *hex);
int merge_read_head(char *out_hex);
void merge_clear_head(void);
int merge_has_conflicts(const index_t *idx);

#endif /* !MERGE_H_ */
