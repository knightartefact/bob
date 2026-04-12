#include <stdio.h>
#include <string.h>

#include "commands.h"


int main(int ac, char **av)
{
    if (ac < 2) {
        fprintf(stderr, "Usage: bob <command> [args]\n");
        return -1;
    }
    if (strcmp(av[1], "init") == 0) {
        return cmd_init();
    }
    if ((strcmp(av[1], "hash-object") == 0) && (ac == 3)) {
        return cmd_hash_object(av[2]);
    }
    if ((strcmp(av[1], "cat-file") == 0) && (ac == 3)) {
        return cmd_cat_file(av[2]);
    }
    if ((strcmp(av[1], "update-index") == 0) && (ac == 3)) {
        return cmd_update_index(av[2]);
    }
    if ((strcmp(av[1], "ls-files") == 0)) {
        return cmd_ls_files();
    }
    fprintf(stderr, "bob: unknown command: %s\n", av[1]);
    return 0;
}
