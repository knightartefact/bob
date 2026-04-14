#include "object.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/stat.h>

#include <openssl/sha.h>

#include "utils.h"
#include "str/str.h"

static str_t create_header(const char *type, size_t size)
{
    return strformat("%s %u", type, size);
}

static str_t object_path(const char *digest)
{
    return strformat(".bob/objects/%.2s/%s", digest, digest + 2);
}

static void object_create_dir(const char *obj_path)
{
    str_t path = strnew(obj_path);
    char *dir = dirname(path);
    mkdir(dir, 0755);
    strfree(path);
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

static int object_write_file(str_t filepath, str_t data)
{
    if (access(filepath, F_OK) == 0) {
        return 0;
    }
    FILE *fp = object_create_file(filepath);
    if (fp == NULL) {
        return -1;
    }
    fwrite(data, sizeof(*data), strlength(data), fp);
    fclose(fp);
    return 0;
}

bob_object_t *object_new(const char *type, const char *data, size_t size)
{
    bob_object_t *obj = malloc(sizeof(*obj));

    if (obj == NULL) {
        fprintf(stderr, "Could not create object\n");
        return NULL;
    }
    obj->type = strdup(type);
    obj->data = malloc(size + 1);
    if (obj->data == NULL) {
        free(obj->type);
        free(obj);
        return NULL;
    }
    memcpy(obj->data, data, size);
    obj->data[size] = '\0';
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

char *object_hash_data(str_t data)
{
    char *digest = calloc(20, sizeof(char));
    if (digest == NULL) {
        return NULL;
    }
    SHA1(data, strlength(data), digest);
    return digest;
}

char *object_write(const bob_object_t *obj)
{
    str_t header = create_header(obj->type, obj->size);
    strpush(&header, '\0');
    strnconcat(&header, obj->data, obj->size);

    char *digest = object_hash_data(header);

    char filename[41] = {0};
    sha2hex(digest, filename);
    str_t filepath = object_path(filename);
    object_write_file(filepath, header);

    strfree(header);
    strfree(filepath);
    return digest;
}

static str_t object_read_file(const char *filepath)
{
    FILE *fp = fopen(filepath, "rb");

    if (fp == NULL) {
        perror(filepath);
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    size_t filesize = ftell(fp);
    rewind(fp);
    str_t data = strnewlen("", filesize);

    if (data == NULL) {
        fclose(fp);
        return NULL;
    }
    fread(data, filesize, sizeof(*data), fp);
    fclose(fp);
    return data;
}

static int strchindex(const char *str, char c)
{
    char *strend = strchr(str, c);

    if (strend == NULL) {
        fprintf(stderr, "Object header is corrupt\n");
        return -1;
    }
    return (int)(strend - str);
}

static bob_object_t *object_parse_object(const str_t buf)
{
    int type_len = strchindex(buf, ' ');
    if (type_len == -1) {
        fprintf(stderr, "Object type is corrupted\n");
        return NULL;
    }
    int size_len = strchindex(buf + type_len + 1, '\0');
    if (size_len == -1) {
        fprintf(stderr, "Object size is corrupted\n");
        return NULL;
    }

    char type[64] = {0};
    memcpy(type, buf, type_len);
    char obj_size[64] = {0};
    memcpy(obj_size, buf + type_len + 1, size_len);
    int size = atoi(obj_size);
    int header_len = type_len + 1 + size_len;
    const char *body = buf + header_len + 1;

    return object_new(type, body, size);
}

bob_object_t *object_read(const char *filename)
{
    str_t path = object_path(filename);
    str_t data = object_read_file(path);

    if (data == NULL) {
        return NULL;
    }

    bob_object_t *obj = object_parse_object(data);
    strfree(data);
    strfree(path);
    return obj;
}
