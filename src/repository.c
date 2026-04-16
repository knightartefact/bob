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
    fprintf(fp, "name=\nemail=\n");
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

int ref_resolve_head(char *out_ref, int size)
{
    FILE *fp = fopen(".bob/HEAD", "r");
    if (fp == NULL) {
        perror(".bob/HEAD");
        return -1;
    }
    char line[512] = {0};
    if (fgets(line, sizeof(line), fp) == NULL) {
        fclose(fp);
        return -1;
    }
    fclose(fp);
    line[strcspn(line, "\n")] = '\0';
    if (strncmp(line, "ref: ", 5) != 0) {
        fprintf(stderr, "HEAD is not a symbolic ref\nYou are in detached mode");
        return -1;
    }
    strncpy(out_ref, line + 5, size - 1);
    return 0;
}

int ref_read(const char *ref, char *out_hex)
{
    char path[512] = {0};
    snprintf(path, sizeof(path), ".bob/%s", ref);
    FILE *fp = fopen(path, "r");
    if (fp == NULL)
        return -1;
    if (fgets(out_hex, 41, fp) == NULL) {
        fclose(fp);
        return -1;
    }
    out_hex[strcspn(out_hex, "\n")] = '\0';
    fclose(fp);
    return 0;
}

int ref_update(const char *ref, const char *hex)
{
    char path[512] = {0};
    snprintf(path, sizeof(path), ".bob/%s", ref);
    FILE *fp = fopen(path, "w");
    if (fp == NULL) {
        perror(path);
        return -1;
    }
    fprintf(fp, "%s\n", hex);
    fclose(fp);
    return 0;
}
