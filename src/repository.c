#include "repository.h"

#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

static const char *bob_dirs[] = {
    ".bob",
    ".bob/objects",
    ".bob/refs",
    ".bob/refs/heads",
    NULL
};

static int create_dirs(int mode)
{
    for (int i = 0; bob_dirs[i]; i++) {
        if (mkdir(bob_dirs[i], mode) == -1 && errno != EEXIST) {
            char errmsg[4096] = {0};
            snprintf(errmsg, sizeof(errmsg), "%s: %s", "create_dirs", bob_dirs[i]);
            perror(errmsg);
            return -1;
        }
    }
    return 0;
}

static int create_head_file(void)
{
    FILE *fp = fopen(".bob/HEAD", "w");
    if (fp == NULL) {
        fprintf(stderr, "Couldn't create bob HEAD: %s\n", strerror(errno));
        return -1;
    }
    fprintf(fp, "ref: refs/heads/main\n");
    fclose(fp);
}

static int create_config_file(void)
{
    FILE *fp = fopen(".bob/config", "w");
    if (fp == NULL) {
        fprintf(stderr, "Couldn't create bob config file: %s\n", strerror(errno));
        return -1;
    }
    fclose(fp);
}

int create_repository(void)
{
    if (create_dirs(0755) == -1) {
        return -1;
    }
    if (create_head_file() == -1 || create_config_file() == -1) {
        return -1;
    }
    char abspath[4096] = {0};
    if (realpath(".bob", abspath) == NULL) {
        return -1;
    }
    printf("Initialized bob repository in %s\n", abspath);
    return 0;
}
