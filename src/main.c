#include <stdio.h>
#include <stdlib.h>
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
    if (strcmp(av[1], "add") == 0 && ac >= 3) {
        return cmd_add(ac - 2, (const char **)(av + 2));
    }
    if ((strcmp(av[1], "write-tree") == 0)) {
        return cmd_write_tree();
    }
    if (strcmp(av[1], "log") == 0) {
        return cmd_log();
    }
    if (strcmp(av[1], "status") == 0) {
        return cmd_status();
    }
    if (strcmp(av[1], "commit") == 0) {
        const char *message = NULL;
        for (int i = 2; i < ac - 1; i++) {
            if (strcmp(av[i], "-m") == 0) {
                message = av[++i];
                break;
            }
        }
        if (message == NULL) {
            char buf[4096] = {0};
            int len = fread(buf, 1, sizeof(buf) - 1, stdin);
            if (len <= 0) {
                fprintf(stderr, "Usage: bob commit -m <message>\n");
                return -1;
            }
            return cmd_commit(buf);
        }
        return cmd_commit(message);
    }
    if (strcmp(av[1], "commit-tree") == 0 && ac >= 4) {
        const char *tree_hex = av[2];
        const char *message = NULL;
        const char *parent_hex = NULL;
        for (int i = 3; i < ac - 1; i++) {
            if (strcmp(av[i], "-m") == 0)
                message = av[++i];
            else if (strcmp(av[i], "-p") == 0)
                parent_hex = av[++i];
        }
        if (message == NULL) {
            fprintf(stderr, "Usage: bob commit-tree <tree-sha> -m <message> [-p <parent-sha>]\n");
            return -1;
        }
        return cmd_commit_tree(tree_hex, message, parent_hex);
    }
    fprintf(stderr, "bob: unknown command: %s\n", av[1]);
    return 0;
}
