#include <stdio.h>
#include "repository.h"
#include "object.h"
#include <stdlib.h>

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

int main(int ac, char **av)
{
    char buf[32768] = {0};
    int size = sizeof(buf);
    if (read_file(av[1], buf, &size)) {
        return -1;
    }
    bob_object_t *obj = object_new("blob", buf, size);
    char *digest = object_write(obj);
    object_free(obj);
    obj = NULL;
    obj = object_read(digest);
    printf("Object:\n    type: %s\n    size: %d\n    data: %s\n", obj->type, obj->size, obj->data);
    return 0;
}
