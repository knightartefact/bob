#include <stdlib.h>
#include <stdio.h>

#include "repository.h"
#include "object.h"

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
    printf("%s\n", digest);
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
