#ifndef INDEX_H_
#define INDEX_H_

#include <stdint.h>

#define INDEX_MAGIC "BOB"
#define INDEX_MAX_PATH 256
#define INDEX_MAX_ENTRIES 1024

typedef struct index_entry_s {
    uint32_t mode;
    uint32_t mtime;
    uint32_t size;
    uint8_t sha1[20];
    uint16_t path_len;
    char path[INDEX_MAX_PATH];
} index_entry_t;

typedef struct index_s {
    index_entry_t entries[INDEX_MAX_ENTRIES];
    int count;
} index_t;

int index_read(index_t *idx);
int index_write(const index_t *idx);
void index_add(index_t *idx, const index_entry_t *entry);

#endif /* !INDEX_H_ */
