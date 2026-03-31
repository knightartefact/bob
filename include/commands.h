#ifndef COMMANDS_H_
#define COMMANDS_H_

int cmd_init(void);
int cmd_hash_object(const char *filepath);
int cmd_cat_file(const char *hex);
int cmd_update_index(const char *filepath);
int cmd_ls_files(void);

#endif /* !COMMANDS_H_ */
