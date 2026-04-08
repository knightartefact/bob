#include "index.h"

#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>

static uint32_t encode_u32(uint32_t hnum)
{
    return htonl(hnum);
}

static uint32_t decode_u32(uint32_t inum)
{
    return ntohl(inum);
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

static int index_read_entry(FILE *f, index_entry_t *entry)
{
    if (fread(&entry->mode, 4, 1, f) != 1 ||
        fread(&entry->mtime, 4, 1, f) != 1 ||
        fread(&entry->size, 4, 1, f) != 1 ||
        fread(&entry->sha1, 1, sizeof(entry->sha1), f) != sizeof(entry->sha1) ||
        fread(&entry->path, 1, sizeof(entry->path), f) != sizeof(entry->path)) {
        return -1;
    }

    entry->mode = decode_u32(entry->mode);
    entry->mtime = decode_u32(entry->mtime);
    entry->size = decode_u32(entry->size);
    return 0;
}

int index_read(index_t *idx)
{
    idx->count = 0;

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
        index_entry_t *entry = &idx->entries[i];
        if (index_read_entry(f, entry) == -1) {
            fclose(f);
            return -1;
        }
    }
    idx->count = num_entries;
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
    fwrite(entry->path, 1, sizeof(entry->path), f);
}

static void index_write_entries(FILE *f, const index_t *idx)
{
    for (int i = 0; i < idx->count; i++) {
        index_entry_t e = idx->entries[i];
        index_write_entry(f, &e);
    }
}

int index_write(const index_t *idx)
{
    FILE *f = fopen(".bob/index", "wb");
    if (!f) {
        perror("index_write");
        return -1;
    }
    index_write_header(f, idx->count);
    index_write_entries(f, idx);
    fclose(f);
    return 0;
}

void index_add(index_t *idx, const index_entry_t *entry)
{
    for (int i = 0; i < idx->count; i++) {
        if (strcmp(idx->entries[i].path, entry->path) == 0) {
            idx->entries[i] = *entry;
            return;
        }
    }
    if (idx->count < INDEX_MAX_ENTRIES)
        idx->entries[idx->count++] = *entry;
}
