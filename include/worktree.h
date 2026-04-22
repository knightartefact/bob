#ifndef WORKTREE_H_
#define WORKTREE_H_

#include "index.h"

#define WORKTREE_MAX_FILES 1024

typedef struct filelist_s {
    char paths[WORKTREE_MAX_FILES][INDEX_MAX_PATH];
    int count;
} filelist_t;

int worktree_list(const char *dir, const char *prefix, filelist_t *out);

#endif /* !WORKTREE_H_ */
