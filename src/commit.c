#include "commit.h"
#include "config.h"
#include "object.h"
#include "str/str.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

char *commit_create(const char *tree_hex, const char *parent_hex, const char *message)
{
    bob_config_t cfg = {0};
    if (config_read(&cfg) == -1)
        return NULL;
    if (cfg.name[0] == '\0' || cfg.email[0] == '\0') {
        fprintf(stderr, "missing name or email in .bob/config\n");
        return NULL;
    }

    time_t now = time(NULL);
    str_t body = strformat("tree %s\n", tree_hex);
    if (parent_hex != NULL) {
        str_t parent_line = strformat("parent %s\n", parent_hex);
        strconcatstr(&body, parent_line);
        strfree(parent_line);
    }
    str_t author = strformat("author %s <%s> %ld +0000\n", cfg.name, cfg.email, (long)now);
    str_t committer = strformat("committer %s <%s> %ld +0000\n", cfg.name, cfg.email, (long)now);
    strconcatstr(&body, author);
    strconcatstr(&body, committer);
    strconcat(&body, "\n");
    strconcat(&body, message);
    strconcat(&body, "\n");

    bob_object_t *obj = object_new("commit", body, strlength(body));
    char *digest = object_write(obj);

    object_free(obj);
    strfree(body);
    strfree(author);
    strfree(committer);
    return digest;
}
