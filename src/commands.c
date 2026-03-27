#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>

#include "repository.h"
#include "object.h"
#include "index.h"
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

int cmd_cat_file(const char *hex)
{
    bob_object_t *obj = object_read(hex);

    if (obj == NULL) {
        return -1;
    }
    printf("type: %s\n", obj->type);
    fwrite(obj->data, sizeof(char), obj->size, stdout);
    printf("\n");

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
    strncpy(entry.sha1, sha, sizeof(entry.sha1));
    entry.mode = st.st_mode;
    entry.mtime = st.st_mtime;
    entry.size = st.st_size;

    index_t index;
    index_read(&index);
    index_add(&index, &entry);
    index_write(&index);
    return 0;
}
