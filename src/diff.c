#include "diff.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "object.h"
#include "utils.h"
#include "str/str.h"

void diff_trees(const tree_list_t *a, const tree_list_t *b, diff_list_t *out)
{
    out->count = 0;
    int i = 0;
    int j = 0;

    while (i < a->count && j < b->count) {
        int cmp = strcmp(a->entries[i].path, b->entries[j].path);
        diff_entry_t *de = &out->entries[out->count];

        if (cmp < 0) {
            strncpy(de->path, a->entries[i].path, sizeof(de->path) - 1);
            de->type = DIFF_DELETED;
            memcpy(de->old_sha, a->entries[i].sha1, 20);
            out->count++;
            i++;
        } else if (cmp > 0) {
            strncpy(de->path, b->entries[j].path, sizeof(de->path) - 1);
            de->type = DIFF_ADDED;
            memcpy(de->new_sha, b->entries[j].sha1, 20);
            out->count++;
            j++;
        } else {
            if (memcmp(a->entries[i].sha1, b->entries[j].sha1, 20) != 0) {
                strncpy(de->path, a->entries[i].path, sizeof(de->path) - 1);
                de->type = DIFF_MODIFIED;
                memcpy(de->old_sha, a->entries[i].sha1, 20);
                memcpy(de->new_sha, b->entries[j].sha1, 20);
                out->count++;
            }
            i++;
            j++;
        }
    }
    while (i < a->count) {
        diff_entry_t *de = &out->entries[out->count];
        strncpy(de->path, a->entries[i].path, sizeof(de->path) - 1);
        de->type = DIFF_DELETED;
        memcpy(de->old_sha, a->entries[i].sha1, 20);
        out->count++;
        i++;
    }
    while (j < b->count) {
        diff_entry_t *de = &out->entries[out->count];
        strncpy(de->path, b->entries[j].path, sizeof(de->path) - 1);
        de->type = DIFF_ADDED;
        memcpy(de->new_sha, b->entries[j].sha1, 20);
        out->count++;
        j++;
    }
}

static int write_tmp(const char *path, const char *data, size_t size)
{
    FILE *fp = fopen(path, "wb");
    if (fp == NULL)
        return -1;
    fwrite(data, 1, size, fp);
    fclose(fp);
    return 0;
}

static void diff_blobs(const char *path,
                       const char *a_data, size_t a_size,
                       const char *b_data, size_t b_size)
{
    write_tmp("/tmp/bob_diff_a", a_data, a_size);
    write_tmp("/tmp/bob_diff_b", b_data, b_size);

    str_t cmd = strformat("diff -u --label a/%s --label b/%s /tmp/bob_diff_a /tmp/bob_diff_b",
                          path, path);
    system(cmd);
    strfree(cmd);

    remove("/tmp/bob_diff_a");
    remove("/tmp/bob_diff_b");
}

void diff_print_entry(const diff_entry_t *entry)
{
    if (entry->type == DIFF_ADDED) {
        char hex[41] = {0};
        sha2hex(entry->new_sha, hex);
        bob_object_t *blob = object_read(hex);
        if (blob == NULL)
            return;
        diff_blobs(entry->path, "", 0, blob->data, blob->size);
        object_free(blob);
    } else if (entry->type == DIFF_DELETED) {
        char hex[41] = {0};
        sha2hex(entry->old_sha, hex);
        bob_object_t *blob = object_read(hex);
        if (blob == NULL)
            return;
        diff_blobs(entry->path, blob->data, blob->size, "", 0);
        object_free(blob);
    } else {
        char old_hex[41] = {0};
        char new_hex[41] = {0};
        sha2hex(entry->old_sha, old_hex);
        sha2hex(entry->new_sha, new_hex);
        bob_object_t *old_blob = object_read(old_hex);
        bob_object_t *new_blob = object_read(new_hex);
        if (old_blob == NULL || new_blob == NULL) {
            object_free(old_blob);
            object_free(new_blob);
            return;
        }
        diff_blobs(entry->path, old_blob->data, old_blob->size,
                   new_blob->data, new_blob->size);
        object_free(old_blob);
        object_free(new_blob);
    }
}
