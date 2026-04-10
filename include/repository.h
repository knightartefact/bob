#ifndef REPOSITORY_H_
#define REPOSITORY_H_

int create_repository(void);
int ref_resolve_head(char *out_ref, int size);
int ref_read(const char *ref, char *out_hex);
int ref_update(const char *ref, const char *hex);
int ref_resolve_commit(const char *arg, char *out_hex);
int ref_is_branch(const char *arg);
int ref_write_head(const char *value);

#endif /* !REPOSITORY_H_ */
