#include "status.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include <openssl/sha.h>

#include "str/str.h"

void status_staged(const index_t *idx, const tree_list_t *head)
{
    int printed_header = 0;

    for (int i = 0; i < idx->count; i++) {
        const index_entry_t *ie = &idx->entries[i];
        const tree_entry_t *te = tree_list_find(head, ie->path);
        const char *label = NULL;

        if (te == NULL)
            label = "new file";
        else if (memcmp(ie->sha1, te->sha1, 20) != 0)
            label = "modified";
        if (label == NULL)
            continue;
        if (!printed_header) {
            printf("Changes to be committed:\n");
            printed_header = 1;
        }
        printf("\t%s:   %s\n", label, ie->path);
    }
    for (int i = 0; i < head->count; i++) {
        if (index_find(idx, head->entries[i].path) == NULL) {
            if (!printed_header) {
                printf("Changes to be committed:\n");
                printed_header = 1;
            }
            printf("\tdeleted:    %s\n", head->entries[i].path);
        }
    }
    if (printed_header)
        printf("\n");
}

static int hash_file(const char *path, unsigned char *out_sha)
{
    FILE *fp = fopen(path, "rb");
    if (fp == NULL)
        return -1;
    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    rewind(fp);

    str_t content = strformat("blob %ld", fsize);
    strpush(&content, '\0');
    str_t filebuf = strnewcap(fsize);
    fread(filebuf, 1, fsize, fp);
    fclose(fp);
    strnconcat(&content, filebuf, fsize);

    SHA1((unsigned char *)content, strlength(content), out_sha);
    strfree(content);
    strfree(filebuf);
    return 0;
}

void status_unstaged(const index_t *idx)
{
    int printed_header = 0;

    for (int i = 0; i < idx->count; i++) {
        const index_entry_t *ie = &idx->entries[i];
        struct stat st;

        if (stat(ie->path, &st) == -1) {
            if (!printed_header) {
                printf("Changes not staged for commit:\n");
                printed_header = 1;
            }
            printf("\tdeleted:    %s\n", ie->path);
            continue;
        }
        if ((uint32_t)st.st_mtime == ie->mtime && (uint32_t)st.st_size == ie->size)
            continue;
        if (!printed_header) {
            printf("Changes not staged for commit:\n");
            printed_header = 1;
        }
        printf("\tmodified:   %s\n", ie->path);
    }
    if (printed_header)
        printf("\n");
}

void status_untracked(const filelist_t *files, const index_t *idx)
{
    int printed_header = 0;

    for (int i = 0; i < files->count; i++) {
        if (index_find(idx, files->paths[i]) != NULL)
            continue;
        if (!printed_header) {
            printf("Untracked files:\n");
            printed_header = 1;
        }
        printf("\t%s\n", files->paths[i]);
    }
    if (printed_header)
        printf("\n");
}
