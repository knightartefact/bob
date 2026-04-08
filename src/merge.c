#include "merge.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "object.h"
#include "commit.h"
#include "checkout.h"
#include "repository.h"
#include "utils.h"

#define MAX_ANCESTORS 1024

static int collect_ancestors(const char *hex, char ancestors[][41], int *count)
{
    char current[41] = {0};
    strncpy(current, hex, 40);

    while (current[0] != '\0' && *count < MAX_ANCESTORS) {
        memcpy(ancestors[*count], current, 41);
        (*count)++;
        bob_object_t *obj = object_read(current);
        if (obj == NULL)
            return -1;
        bob_commit_t commit;
        commit_parse(obj, &commit);
        object_free(obj);
        if (commit.parent_count > 0)
            memcpy(current, commit.parents[0], 41);
        else
            current[0] = '\0';
    }
    return 0;
}

int merge_find_base(const char *hex_a, const char *hex_b, char *out_hex)
{
    static char ancestors_a[MAX_ANCESTORS][41];
    int count_a = 0;

    if (collect_ancestors(hex_a, ancestors_a, &count_a) == -1)
        return -1;

    char current[41] = {0};
    strncpy(current, hex_b, 40);

    while (current[0] != '\0') {
        for (int i = 0; i < count_a; i++) {
            if (strcmp(current, ancestors_a[i]) == 0) {
                memcpy(out_hex, current, 41);
                return 0;
            }
        }
        bob_object_t *obj = object_read(current);
        if (obj == NULL)
            return -1;
        bob_commit_t commit;
        commit_parse(obj, &commit);
        object_free(obj);
        if (commit.parent_count > 0)
            memcpy(current, commit.parents[0], 41);
        else
            current[0] = '\0';
    }
    out_hex[0] = '\0';
    return -1;
}

static void add_merged(merge_result_t *result, const tree_entry_t *entry)
{
    if (result->merged.count >= INDEX_MAX_ENTRIES)
        return;
    result->merged.entries[result->merged.count] = *entry;
    result->merged.count++;
}

static void add_conflict(merge_result_t *result, const char *path,
                         const tree_entry_t *base, const tree_entry_t *ours,
                         const tree_entry_t *theirs)
{
    if (result->conflict_count >= INDEX_MAX_ENTRIES)
        return;
    merge_conflict_t *c = &result->conflicts[result->conflict_count];
    memset(c, 0, sizeof(*c));
    strncpy(c->path, path, sizeof(c->path) - 1);
    if (base) {
        memcpy(c->base_sha, base->sha1, 20);
        c->has_base = 1;
    }
    if (ours) {
        memcpy(c->ours_sha, ours->sha1, 20);
        c->has_ours = 1;
    }
    if (theirs) {
        memcpy(c->theirs_sha, theirs->sha1, 20);
        c->has_theirs = 1;
    }
    result->conflict_count++;
}

static void merge_both_present(merge_result_t *result, const char *path,
                               const tree_entry_t *base, const tree_entry_t *ours,
                               const tree_entry_t *theirs)
{
    if (sha_equal(ours->sha1, theirs->sha1))
        add_merged(result, ours);
    else if (base && sha_equal(base->sha1, ours->sha1))
        add_merged(result, theirs);
    else if (base && sha_equal(base->sha1, theirs->sha1))
        add_merged(result, ours);
    else
        add_conflict(result, path, base, ours, theirs);
}

static void merge_only_ours(merge_result_t *result, const char *path,
                            const tree_entry_t *base, const tree_entry_t *ours)
{
    if (base && sha_equal(base->sha1, ours->sha1))
        return;
    if (base)
        add_conflict(result, path, base, ours, NULL);
    else
        add_merged(result, ours);
}

static void merge_only_theirs(merge_result_t *result, const char *path,
                              const tree_entry_t *base, const tree_entry_t *theirs)
{
    if (base && sha_equal(base->sha1, theirs->sha1))
        return;
    if (base)
        add_conflict(result, path, base, NULL, theirs);
    else
        add_merged(result, theirs);
}

static void merge_entry(merge_result_t *result, const char *path,
                        const tree_list_t *base_tree, const tree_list_t *ours_tree,
                        const tree_list_t *theirs_tree)
{
    const tree_entry_t *base = tree_list_find(base_tree, path);
    const tree_entry_t *ours = tree_list_find(ours_tree, path);
    const tree_entry_t *theirs = tree_list_find(theirs_tree, path);

    if (ours && theirs)
        merge_both_present(result, path, base, ours, theirs);
    else if (ours)
        merge_only_ours(result, path, base, ours);
    else if (theirs)
        merge_only_theirs(result, path, base, theirs);
}

static const char *next_path(const tree_list_t *ours, int i,
                             const tree_list_t *theirs, int j)
{
    if (i >= ours->count)
        return theirs->entries[j].path;
    if (j >= theirs->count)
        return ours->entries[i].path;
    int cmp = strcmp(ours->entries[i].path, theirs->entries[j].path);
    return cmp <= 0 ? ours->entries[i].path : theirs->entries[j].path;
}

int merge_trees(const tree_list_t *base, const tree_list_t *ours,
                const tree_list_t *theirs, merge_result_t *result)
{
    memset(result, 0, sizeof(*result));

    int i = 0, j = 0;
    while (i < ours->count || j < theirs->count) {
        const char *path = next_path(ours, i, theirs, j);
        merge_entry(result, path, base, ours, theirs);

        if (i < ours->count && strcmp(ours->entries[i].path, path) == 0)
            i++;
        if (j < theirs->count && strcmp(theirs->entries[j].path, path) == 0)
            j++;
    }
    return 0;
}

static int read_blob_data(const uint8_t *sha, char **out, size_t *out_size)
{
    char hex[41] = {0};
    sha2hex(sha, hex);
    bob_object_t *obj = object_read(hex);
    if (obj == NULL)
        return -1;
    *out = obj->data;
    *out_size = obj->size;
    obj->data = NULL;
    object_free(obj);
    return 0;
}

int merge_write_conflict(const merge_conflict_t *conflict)
{
    FILE *fp = fopen(conflict->path, "wb");
    if (fp == NULL) {
        perror(conflict->path);
        return -1;
    }

    char *ours_data = NULL, *theirs_data = NULL;
    size_t ours_size = 0, theirs_size = 0;

    if (conflict->has_ours)
        read_blob_data(conflict->ours_sha, &ours_data, &ours_size);
    if (conflict->has_theirs)
        read_blob_data(conflict->theirs_sha, &theirs_data, &theirs_size);

    fprintf(fp, "<<<<<<< ours\n");
    if (ours_data)
        fwrite(ours_data, 1, ours_size, fp);
    fprintf(fp, "=======\n");
    if (theirs_data)
        fwrite(theirs_data, 1, theirs_size, fp);
    fprintf(fp, ">>>>>>> theirs\n");

    fclose(fp);
    free(ours_data);
    free(theirs_data);
    return 0;
}

int merge_resolve_commit_tree(const char *hex, tree_list_t *out)
{
    bob_object_t *obj = object_read(hex);
    if (obj == NULL)
        return -1;
    bob_commit_t c;
    commit_parse(obj, &c);
    object_free(obj);
    return tree_flatten(c.tree, "", out);
}

void merge_apply_to_worktree(const merge_result_t *result)
{
    index_t old_index = {0};
    index_read(&old_index);

    index_t new_index = {0};
    checkout_tree_to_index(&result->merged, &new_index);
    index_write(&new_index);

    checkout_remove_old_files(&old_index, &new_index);
    checkout_write_files(&new_index);
    index_write(&new_index);

    for (int i = 0; i < result->conflict_count; i++)
        merge_write_conflict(&result->conflicts[i]);
}

int merge_create_commit(const char *branch, const char *ours_hex,
                        const char *theirs_hex)
{
    index_t index = {0};
    index_read(&index);
    char *tree_digest = tree_write(&index);
    if (tree_digest == NULL)
        return -1;
    char tree_hex[41] = {0};
    sha2hex((unsigned char *)tree_digest, tree_hex);
    free(tree_digest);

    const char *parents[] = { ours_hex, theirs_hex };
    char msg[512] = {0};
    snprintf(msg, sizeof(msg), "Merge branch '%s'", branch);
    char *commit_digest = commit_create(tree_hex, parents, 2, msg);
    if (commit_digest == NULL)
        return -1;
    char commit_hex[41] = {0};
    sha2hex((unsigned char *)commit_digest, commit_hex);
    free(commit_digest);

    char ref[256] = {0};
    int head_type = ref_resolve_head(ref, sizeof(ref));
    if (head_type == 1)
        ref_write_head(commit_hex);
    else
        ref_update(ref, commit_hex);

    printf("Merge made: %.7s\n", commit_hex);
    return 0;
}

int merge_write_head(const char *hex)
{
    FILE *fp = fopen(".bob/MERGE_HEAD", "w");
    if (fp == NULL) {
        perror(".bob/MERGE_HEAD");
        return -1;
    }
    fprintf(fp, "%s\n", hex);
    fclose(fp);
    return 0;
}

int merge_read_head(char *out_hex)
{
    FILE *fp = fopen(".bob/MERGE_HEAD", "r");
    if (fp == NULL)
        return -1;
    if (fgets(out_hex, 41, fp) == NULL) {
        fclose(fp);
        return -1;
    }
    out_hex[strcspn(out_hex, "\n")] = '\0';
    fclose(fp);
    return 0;
}

void merge_clear_head(void)
{
    remove(".bob/MERGE_HEAD");
}

static int file_has_markers(const char *path)
{
    FILE *fp = fopen(path, "r");
    if (fp == NULL)
        return 0;
    char line[1024];
    while (fgets(line, sizeof(line), fp) != NULL) {
        if (strncmp(line, "<<<<<<<", 7) == 0) {
            fclose(fp);
            return 1;
        }
    }
    fclose(fp);
    return 0;
}

int merge_has_conflicts(const index_t *idx)
{
    for (int i = 0; i < idx->count; i++) {
        if (file_has_markers(idx->entries[i].path))
            return 1;
    }
    return 0;
}
