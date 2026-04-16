#ifndef REPOSITORY_H_
#define REPOSITORY_H_

int create_repository(void);
int ref_resolve_head(char *out_ref, int size);
int ref_read(const char *ref, char *out_hex);
int ref_update(const char *ref, const char *hex);

#endif /* !REPOSITORY_H_ */
