#include "checkout.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <libgen.h>
#include <unistd.h>

#include "object.h"
#include "utils.h"
#include "str/str.h"

void checkout_tree_to_index(const tree_list_t *tree, index_t *index)
{
    index->count = 0;
    for (int i = 0; i < tree->count && i < INDEX_MAX_ENTRIES; i++) {
        index_entry_t *ie = &index->entries[i];
        memset(ie, 0, sizeof(*ie));
        ie->mode = tree->entries[i].mode;
        memcpy(ie->sha1, tree->entries[i].sha1, 20);
        strncpy(ie->path, tree->entries[i].path, sizeof(ie->path) - 1);
        ie->path_len = strlen(tree->entries[i].path);
        index->count++;
    }
}

static void remove_empty_dirs(const char *filepath)
{
    str_t path = strnew(filepath);
    char *dir = dirname(path);
    rmdir(dir);
    strfree(path);
}

void checkout_remove_old_files(const index_t *old_index, const index_t *new_index)
{
    for (int i = 0; i < old_index->count; i++) {
        if (index_find(new_index, old_index->entries[i].path) != NULL)
            continue;
        remove(old_index->entries[i].path);
        remove_empty_dirs(old_index->entries[i].path);
    }
}

static int create_parent_dirs(const char *filepath)
{
    str_t path = strnew(filepath);
    char *dir = dirname(path);
    if (dir[0] != '.' || dir[1] != '\0') {
        mkdir(dir, 0755);
    }
    strfree(path);
    return 0;
}

static int write_blob_to_file(const char *path, const bob_object_t *blob)
{
    create_parent_dirs(path);
    FILE *fp = fopen(path, "wb");
    if (fp == NULL) {
        perror(path);
        return -1;
    }
    fwrite(blob->data, 1, blob->size, fp);
    fclose(fp);
    return 0;
}

static void update_entry_stat(index_entry_t *entry)
{
    struct stat st;
    if (stat(entry->path, &st) == 0) {
        entry->mtime = st.st_mtime;
        entry->size = st.st_size;
    }
}

int checkout_write_files(index_t *index)
{
    for (int i = 0; i < index->count; i++) {
        index_entry_t *ie = &index->entries[i];
        char hex[41] = {0};
        sha2hex(ie->sha1, hex);
        bob_object_t *blob = object_read(hex);
        if (blob == NULL) {
            fprintf(stderr, "checkout: could not read object %s\n", hex);
            return -1;
        }
        if (write_blob_to_file(ie->path, blob) == -1) {
            object_free(blob);
            return -1;
        }
        object_free(blob);
        update_entry_stat(ie);
    }
    return 0;
}
