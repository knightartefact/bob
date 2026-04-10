#include "commit.h"
#include "config.h"
#include "object.h"
#include "str/str.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static void parse_hex_field(const char *line, int len,
    const char *prefix, int prefix_len, int expected_min, char *out)
{
    if (strncmp(line, prefix, prefix_len) == 0 && len >= expected_min)
        memcpy(out, line + prefix_len, 40);
}

static void parse_str_field(const char *line, int len,
    const char *prefix, int prefix_len, char *out, int out_size)
{
    if (strncmp(line, prefix, prefix_len) != 0)
        return;
    int val_len = len - prefix_len;
    if (val_len > out_size - 1)
        val_len = out_size - 1;
    memcpy(out, line + prefix_len, val_len);
}

static void parse_message(const char *start, const char *end, char *out,
    int out_size)
{
    int msg_len = end - start;
    if (msg_len <= 0)
        return;
    if (start[msg_len - 1] == '\n')
        msg_len--;
    if (msg_len > out_size - 1)
        msg_len = out_size - 1;
    memcpy(out, start, msg_len);
}

static void parse_header_line(const char *line, int len, bob_commit_t *commit)
{
    parse_hex_field(line, len, "tree ", 5, 45, commit->tree);
    parse_hex_field(line, len, "parent ", 7, 47, commit->parent);
    parse_str_field(line, len, "author ", 7, commit->author, sizeof(commit->author));
    parse_str_field(line, len, "committer ", 10, commit->committer, sizeof(commit->committer));
}

int commit_parse(const bob_object_t *obj, bob_commit_t *commit)
{
    memset(commit, 0, sizeof(*commit));
    const char *line = obj->data;
    const char *end = obj->data + obj->size;

    while (line < end) {
        const char *eol = memchr(line, '\n', end - line);
        if (eol == NULL)
            eol = end;
        int len = eol - line;

        if (len == 0) {
            parse_message(eol + 1, end, commit->message, sizeof(commit->message));
            break;
        }
        parse_header_line(line, len, commit);
        line = eol + 1;
    }
    return 0;
}

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
