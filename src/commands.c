#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>

#include "repository.h"
#include "object.h"
#include "index.h"
#include "tree.h"
#include "commit.h"
#include "utils.h"

int cmd_init(void)
{
    create_repository();
}

static int read_file(const char *filename, char *out, int *size)
{
    FILE *fp = fopen(filename, "rb");

    if (fp == NULL) {
        perror(filename);
        return -1;
    }
    int read_size = fread(out, sizeof(*out), *size, fp);
    if (read_size == -1) {
        fprintf(stderr, "Could not read file %s\n", filename);
        return -1;
    }
    *size = read_size;
    return 0;
}

int cmd_hash_object(const char *filepath)
{
    char buf[32768] = {0};
    int size = sizeof(buf);
    if (read_file(filepath, buf, &size)) {
        return -1;
    }
    bob_object_t *obj = object_new("blob", buf, size);
    char *digest = object_write(obj);
    if (digest == NULL) {
        return -1;
    }
    char hex[41] = {0};
    sha2hex(digest, hex);
    printf("%s\n", hex);
    object_free(obj);
    free(digest);
    return 0;
}

static void cat_blob(const bob_object_t *blob)
{
    fwrite(blob->data, sizeof(char), blob->size, stdout);
}

static void cat_tree(const bob_object_t *tree)
{
    size_t i = 0;

    while (i < tree->size) {
        // Parse mode (up to space)
        const char *mode = tree->data + i;
        const char *space = memchr(mode, ' ', tree->size - i);
        if (space == NULL)
            break;
        size_t mode_len = space - mode;

        // Parse name (up to null byte)
        const char *name = space + 1;
        const char *nul = memchr(name, '\0', tree->size - (name - tree->data));
        if (nul == NULL)
            break;
        size_t name_len = nul - name;

        // 20-byte binary SHA1 follows the null byte
        const unsigned char *sha = (const unsigned char *)(nul + 1);
        if ((sha + 20) - (const unsigned char *)tree->data > (long)tree->size)
            break;

        char hex[41] = {0};
        sha2hex(sha, hex);

        // Determine type from mode: "40000" is tree, otherwise blob
        const char *type = (mode_len == 5 && memcmp(mode, "40000", 5) == 0)
            ? "tree" : "blob";

        printf("%.*s %s %s\t%.*s\n",
            (int)mode_len, mode, type, hex, (int)name_len, name);

        i = (sha + 20) - (const unsigned char *)tree->data;
    }
}

int cmd_cat_file(const char *hex)
{
    bob_object_t *obj = object_read(hex);

    if (obj == NULL) {
        return -1;
    }
    printf("type: %s\n", obj->type);
    if (strcmp(obj->type, "tree") == 0)
        cat_tree(obj);
    else
        cat_blob(obj);

    object_free(obj);
    return 0;
}

int cmd_update_index(const char *filepath)
{
    struct stat st;
    if (stat(filepath, &st) == -1) {
        perror(filepath);
        return -1;
    }
    char buf[32768] = {0};
    int size = sizeof(buf);
    if (read_file(filepath, buf, &size) == -1) {
        fprintf(stderr, "Could not read %s file\n", filepath);
        return -1;
    }
    bob_object_t *obj = object_new("blob", buf, size);
    if (obj == NULL) {
        return -1;
    }
    char *sha = object_write(obj);
    if (sha == NULL) {
        object_free(obj);
        return -1;
    }
    object_free(obj);
    index_entry_t entry = {0};
    strncpy(entry.path, filepath, sizeof(entry.path));
    memcpy(entry.sha1, sha, sizeof(entry.sha1));
    entry.mode = st.st_mode;
    entry.mtime = st.st_mtime;
    entry.size = st.st_size;
    entry.path_len = strlen(filepath);

    index_t index;
    index_read(&index);
    index_add(&index, &entry);
    index_write(&index);
    return 0;
}

int cmd_ls_files(void)
{
    index_t index = {0};
    index_read(&index);
    for (int i = 0; i < index.count; i++) {
        const index_entry_t *entry = &index.entries[i];
        char hex[41] = {0};
        sha2hex(entry->sha1, hex);
        printf("%o %s %s\n", entry->mode, hex, entry->path);
    }
    return 0;
}

int cmd_write_tree(void)
{
    index_t index = {0};
    index_read(&index);
    char *digest = tree_write(&index);
    if (digest == NULL) {
        return -1;
    }
    char hex[41] = {0};
    sha2hex((unsigned char *)digest, hex);
    printf("%s\n", hex);
    free(digest);
    return 0;
}

int cmd_commit_tree(const char *tree_hex, const char *message, const char *parent_hex)
{
    char *digest = commit_create(tree_hex, parent_hex, message);
    if (digest == NULL) {
        return -1;
    }
    char hex[41] = {0};
    sha2hex((unsigned char *)digest, hex);
    printf("%s\n", hex);
    free(digest);
    return 0;
}
