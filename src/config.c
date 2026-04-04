#include "config.h"

#include <stdio.h>
#include <string.h>

int config_read(bob_config_t *cfg)
{
    FILE *fp = fopen(".bob/config", "r");

    if (fp == NULL) {
        perror(".bob/config");
        return -1;
    }

    char line[512];
    while (fgets(line, sizeof(line), fp) != NULL) {
        line[strcspn(line, "\n")] = '\0';
        char *eq = strchr(line, '=');
        if (eq == NULL)
            continue;
        *eq = '\0';
        const char *key = line;
        const char *value = eq + 1;
        if (strcmp(key, "name") == 0)
            strncpy(cfg->name, value, sizeof(cfg->name) - 1);
        else if (strcmp(key, "email") == 0)
            strncpy(cfg->email, value, sizeof(cfg->email) - 1);
    }
    fclose(fp);
    return 0;
}
