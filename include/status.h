#ifndef STATUS_H_
#define STATUS_H_

#include "index.h"
#include "tree.h"
#include "worktree.h"

void status_staged(const index_t *idx, const tree_list_t *head);
void status_unstaged(const index_t *idx);
void status_untracked(const filelist_t *files, const index_t *idx);

#endif /* !STATUS_H_ */
