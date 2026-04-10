#include "tree.h"
#include "object.h"
#include "utils.h"
#include "str/str.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static char *write_tree_recursive(const index_t *idx, int start, int end, int prefix_len)
{
    str_t data = strnewcap(256);
    int new_start = start;

    while (new_start < end) {
        const char *relpath = idx->entries[new_start].path + prefix_len;
        const char *slash = strchr(relpath, '/');

        if (slash == NULL) {
            str_t entry = strformat("%o %s", idx->entries[new_start].mode, relpath);
            strnconcat(&data, entry, strlength(entry));
            strpush(&data, '\0');
            strnconcat(&data, (const char *)idx->entries[new_start].sha1, 20);
            strfree(entry);
            new_start++;
        } else {
            int dir_len = (int)(slash - relpath);
            char dirname[INDEX_MAX_PATH] = {0};
            memcpy(dirname, relpath, dir_len);

            int new_prefix = prefix_len + dir_len + 1;
            int new_end = new_start;
            while (new_end < end
                && strncmp(idx->entries[new_end].path + prefix_len, dirname, dir_len) == 0
                && idx->entries[new_end].path[prefix_len + dir_len] == '/') {
                new_end++;
            }

            char *subtree_sha = write_tree_recursive(idx, new_start, new_end, new_prefix);
            if (subtree_sha == NULL) {
                strfree(data);
                return NULL;
            }

            str_t entry = strformat("40000 %s", dirname);
            strnconcat(&data, entry, strlength(entry));
            strpush(&data, '\0');
            strnconcat(&data, subtree_sha, 20);
            strfree(entry);
            free(subtree_sha);

            new_start = new_end;
        }
    }

    bob_object_t *tree = object_new("tree", data, strlength(data));
    char *digest = object_write(tree);
    object_free(tree);
    strfree(data);
    return digest;
}

char *tree_write(const index_t *idx)
{
    if (idx->count == 0) {
        bob_object_t *tree = object_new("tree", "", 0);
        char *digest = object_write(tree);
        object_free(tree);
        return digest;
    }
    return write_tree_recursive(idx, 0, idx->count, 0);
}

static size_t parse_raw_entry(const char *data, size_t size, size_t i, raw_tree_entry_t *raw)
{
    raw->mode_str = data + i;
    const char *space = memchr(raw->mode_str, ' ', size - i);
    if (space == NULL)
        return 0;
    raw->mode_len = space - raw->mode_str;

    raw->name = space + 1;
    const char *nul = memchr(raw->name, '\0', size - (raw->name - data));
    if (nul == NULL)
        return 0;

    raw->sha = (const unsigned char *)(nul + 1);
    if ((raw->sha + 20) - (const unsigned char *)data > (long)size)
        return 0;
    return (raw->sha + 20) - (const unsigned char *)data;
}

static str_t join_path(const char *prefix, const char *name)
{
    if (prefix[0] != '\0')
        return strformat("%s/%s", prefix, name);
    return strnew(name);
}

static int is_tree_mode(const raw_tree_entry_t *raw)
{
    return raw->mode_len == 5 && memcmp(raw->mode_str, "40000", 5) == 0;
}

static void flatten_blob_entry(const raw_tree_entry_t *raw, const char *prefix, tree_list_t *out)
{
    if (out->count >= INDEX_MAX_ENTRIES)
        return;
    tree_entry_t *entry = &out->entries[out->count];
    str_t path = join_path(prefix, raw->name);
    strncpy(entry->path, path, sizeof(entry->path) - 1);
    strfree(path);
    memcpy(entry->sha1, raw->sha, 20);
    entry->mode = strtoul(raw->mode_str, NULL, 8);
    out->count++;
}

static void flatten_tree_entry(const raw_tree_entry_t *raw, const char *prefix, tree_list_t *out)
{
    str_t subprefix = join_path(prefix, raw->name);
    char hex[41] = {0};
    sha2hex(raw->sha, hex);
    tree_flatten(hex, subprefix, out);
    strfree(subprefix);
}

int tree_flatten(const char *tree_hex, const char *prefix, tree_list_t *out)
{
    bob_object_t *obj = object_read(tree_hex);
    if (obj == NULL)
        return -1;

    size_t i = 0;
    while (i < obj->size) {
        raw_tree_entry_t raw;
        size_t next = parse_raw_entry(obj->data, obj->size, i, &raw);
        if (next == 0)
            break;
        if (is_tree_mode(&raw))
            flatten_tree_entry(&raw, prefix, out);
        else
            flatten_blob_entry(&raw, prefix, out);
        i = next;
    }
    object_free(obj);
    return 0;
}

const tree_entry_t *tree_list_find(const tree_list_t *list, const char *path)
{
    for (int i = 0; i < list->count; i++) {
        if (strcmp(list->entries[i].path, path) == 0)
            return &list->entries[i];
    }
    return NULL;
}
