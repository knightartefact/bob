#ifndef CHECKOUT_H_
#define CHECKOUT_H_

#include "index.h"
#include "tree.h"

void checkout_tree_to_index(const tree_list_t *tree, index_t *index);
void checkout_remove_old_files(const index_t *old_index, const index_t *new_index);
int checkout_write_files(index_t *index);

#endif /* !CHECKOUT_H_ */
