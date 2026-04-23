#ifndef COMMIT_H_
#define COMMIT_H_

#include "object.h"

#define COMMIT_MAX_PARENTS 2

typedef struct bob_commit_s {
    char tree[41];
    char parents[COMMIT_MAX_PARENTS][41];
    int parent_count;
    char author[256];
    char committer[256];
    char message[4096];
} bob_commit_t;

int commit_parse(const bob_object_t *obj, bob_commit_t *commit);
void commit_print(const char *hex, const bob_commit_t *commit);
char *commit_create(const char *tree_hex, const char **parents, int parent_count, const char *message);

#endif /* !COMMIT_H_ */
