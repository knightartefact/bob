#include "tree.h"
#include "object.h"
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
