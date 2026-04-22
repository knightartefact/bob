#include "index.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>

static uint32_t encode_u32(uint32_t hnum)
{
    return htonl(hnum);
}

static uint32_t decode_u32(uint32_t inum)
{
    return ntohl(inum);
}

static uint16_t encode_u16(uint16_t hnum)
{
    return htons(hnum);
}

static uint16_t decode_u16(uint16_t inum)
{
    return ntohs(inum);
}

static int index_read_header(FILE *f)
{
    char magic[4] = {0};

    if (fread(&magic, 1, sizeof(INDEX_MAGIC), f) != 4 ||
        memcmp(magic, INDEX_MAGIC, sizeof(INDEX_MAGIC))) {
        fprintf(stderr, "index: bad magic number, header is corrupted!\n");
        return -1;
    }

    uint32_t raw_count = 0;
    if (fread(&raw_count, 4, 1, f) != 1) {
        fprintf(stderr, "index: invalid count read\n");
        return -1;
    }
    uint32_t num_entries = decode_u32(raw_count);
    if (num_entries > INDEX_MAX_ENTRIES) {
        fprintf(stderr, "index: entry count too large\n");
        return -1;
    }
    return num_entries;
}

static int index_read_entry_stat(FILE *f, index_entry_t *entry)
{
    if (fread(&entry->mode, 4, 1, f) != 1 ||
        fread(&entry->mtime, 4, 1, f) != 1 ||
        fread(&entry->size, 4, 1, f) != 1) {
        return -1;
    }
    entry->mode = decode_u32(entry->mode);
    entry->mtime = decode_u32(entry->mtime);
    entry->size = decode_u32(entry->size);
    return 0;
}

static int index_read_entry(FILE *f, index_entry_t *entry)
{
    if (index_read_entry_stat(f, entry) == -1) {
        return -1;
    }
    fread(&entry->sha1, 1, sizeof(entry->sha1), f) != sizeof(entry->sha1);
    fread(&entry->path_len, sizeof(entry->path_len), 1, f);
    entry->path_len = decode_u16(entry->path_len);
    fread(&entry->path, sizeof(*entry->path), entry->path_len, f) != sizeof(entry->path);
    entry->path[entry->path_len] = 0;
    return 0;
}

int index_read(index_t *index)
{
    index->count = 0;

    FILE *f = fopen(".bob/index", "rb");
    if (!f) {
        return 0;
    }
    int num_entries = index_read_header(f);
    if (num_entries == -1) {
        fclose(f);
        return -1;
    }
    for (uint32_t i = 0; i < num_entries; i++) {
        index_entry_t *entry = &index->entries[i];
        if (index_read_entry(f, entry) == -1) {
            fclose(f);
            return -1;
        }
    }
    index->count = num_entries;
    fclose(f);
    return 0;
}

static void index_write_header(FILE *f, uint32_t num_entries)
{
    fwrite(INDEX_MAGIC, 1, sizeof(INDEX_MAGIC), f);
    uint32_t count = encode_u32(num_entries);
    fwrite(&count, 4, 1, f);
}

static void index_write_entry(FILE *f, const index_entry_t *entry)
{
    uint32_t word = encode_u32(entry->mode);
    fwrite(&word, 4, 1, f);
    word = encode_u32(entry->mtime);
    fwrite(&word, 4, 1, f);
    word = encode_u32(entry->size);
    fwrite(&word, 4, 1, f);
    fwrite(entry->sha1, 1, sizeof(entry->sha1), f);
    uint16_t path_len = encode_u16(entry->path_len);
    fwrite(&path_len, sizeof(path_len), 1, f);
    fwrite(entry->path, 1, entry->path_len, f);
}

static void index_write_entries(FILE *f, const index_t *index)
{
    for (int i = 0; i < index->count; i++) {
        index_entry_t e = index->entries[i];
        index_write_entry(f, &e);
    }
}

int index_write(const index_t *index)
{
    FILE *f = fopen(".bob/index", "wb");
    if (!f) {
        perror("index_write");
        return -1;
    }
    index_write_header(f, index->count);
    index_write_entries(f, index);
    fclose(f);
    return 0;
}

void index_add(index_t *index, const index_entry_t *entry)
{
    for (int i = 0; i < index->count; i++) {
        if (strcmp(index->entries[i].path, entry->path) == 0) {
            index->entries[i] = *entry;
            return;
        }
    }
    if (index->count < INDEX_MAX_ENTRIES)
        index->entries[index->count++] = *entry;
        index_sort(index);
}

static int index_entry_cmp(const void *lhs, const void *rhs)
{
    const index_entry_t *left_entry = lhs;
    const index_entry_t *right_entry = rhs;
    return strncmp(left_entry->path, right_entry->path, INDEX_MAX_PATH);
}

void index_sort(index_t *index)
{
    qsort(index->entries, index->count, sizeof(index_entry_t), index_entry_cmp);
}

const index_entry_t *index_find(const index_t *idx, const char *path)
{
    for (int i = 0; i < idx->count; i++) {
        if (strcmp(idx->entries[i].path, path) == 0)
            return &idx->entries[i];
    }
    return NULL;
}
