#ifndef OBJECT_H_
#define OBJECT_H_

#include <stddef.h>

typedef struct bob_object_s {
    char *type;
    char *data;
    size_t size;
} bob_object_t;

bob_object_t *object_new(const char *type, const char *data, size_t size);
void object_free(bob_object_t *obj);

char *object_write(const bob_object_t *obj);
bob_object_t *object_read(const char *filename);

#endif /* !OBJECT_H_ */
