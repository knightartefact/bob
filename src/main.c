#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "commands.h"

static int read_file(const char *filename, char *out, int *size)
{
    FILE *fp = fopen(filename, "rb");

    if (fp == NULL) {
        perror(filename);
        return -1;
    }
    int read_size = fread(out, sizeof(*out), *size, fp);
    if (read_size == -1) {
        fprintf(stderr, "Could not read file %s\n", filename);
        return -1;
    }
    *size = read_size;
    return 0;
}

int main(int ac, char **av)
{
    if (ac < 2) {
        fprintf(stderr, "Usage: bob <command> [args]\n");
        return -1;
    }
    if (strcmp(av[1], "init") == 0) {
        return cmd_init();
    }
    fprintf(stderr, "bob: unknown command: %s\n", av[1]);
    return 0;
}
