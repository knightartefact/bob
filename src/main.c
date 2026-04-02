#include <stdio.h>
#include "repository.h"
#include "object.h"
#include <stdlib.h>

int main(int ac, char **av)
{
    bob_object_t *obj = object_new("blob", "Hello, this is another file!!", 29);
    char *digest = object_write(obj);
    free(digest);
    object_free(obj);
    return 0;
}
