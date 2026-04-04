#ifndef COMMANDS_H_
#define COMMANDS_H_

// Procelain commands
int cmd_init(void);
int cmd_add(int count, const char **files);
int cmd_commit(const char *message);

// Plumbing commands
int cmd_hash_object(const char *filepath);
int cmd_cat_file(const char *hex);
int cmd_update_index(const char *filepath);
int cmd_ls_files(void);
int cmd_write_tree(void);
int cmd_commit_tree(const char *tree_hex, const char *message, const char *parent_hex);

#endif /* !COMMANDS_H_ */
