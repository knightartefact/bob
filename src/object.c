#include "object.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/stat.h>

#include <openssl/sha.h>

static size_t create_header(const char *type, size_t size, char *header, size_t header_size)
{
    return snprintf(header, header_size, "%s %u", type, size);
}

static void sha2hex(const unsigned char *digest, char *out)
{
    for (int i = 0; i < 20; i++)
        sprintf(out + i * 2, "%02x", digest[i]);
    out[40] = '\0';
}

static void object_path(const char *digest, char *out, size_t size)
{
    snprintf(out, size, ".bob/objects/%.2s/%s", digest, digest + 2);
}

static void object_create_dir(const char *obj_path)
{
    char *cpy = strdup(obj_path);
    char *dir = dirname(cpy);
    mkdir(dir, 0755);
    free(cpy);
}

static FILE *object_create_file(const char *filepath)
{
    object_create_dir(filepath);
    FILE *fp = fopen(filepath, "wb");
    if (fp == NULL) {
        perror(filepath);
        return NULL;
    }
    return fp;
}

static int object_write_file(const char *filepath, const char *data, size_t size)
{
    if (access(filepath, F_OK) == 0) {
        return 0;
    }
    FILE *fp = object_create_file(filepath);
    if (fp == NULL) {
        return -1;
    }
    fwrite(data, sizeof(*data), size, fp);
    fclose(fp);
    return 0;
}

bob_object_t *object_new(char *type, char *data, size_t size)
{
    bob_object_t *obj = malloc(sizeof(*obj));

    if (obj == NULL) {
        fprintf(stderr, "Could not create object\n");
        return NULL;
    }
    obj->type = strdup(type);
    obj->data = strdup(data);
    obj->size = size;
    return obj;
}

void object_free(bob_object_t *obj)
{
    if (!obj) {
        return;
    }
    free(obj->type);
    free(obj->data);
    free(obj);
}

char *object_write(const bob_object_t *obj)
{
    char header[4096] = {0};
    size_t header_len = create_header(obj->type, obj->size, header, sizeof(header));
    size_t buflen = header_len + 1 + obj->size;
    char *buf = calloc(buflen, sizeof(*buf));

    if (buf == NULL) {
        perror("object_write");
        return NULL;
    }
    memmove(buf, header, header_len);
    memmove(buf + header_len + 1, obj->data, obj->size);

    char digest[20] = {0};
    SHA1(buf, buflen, digest);

    char *filename = calloc(41, sizeof(char));
    sha2hex(digest, filename);
    char filepath[4096] = {0};
    object_path(filename, filepath, sizeof(filepath));
    object_write_file(filepath, buf, buflen);

    free(buf);
    return filename;
}
