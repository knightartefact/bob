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

static void parse_parent_line(const char *line, int len, bob_commit_t *commit)
{
    if (strncmp(line, "parent ", 7) != 0 || len < 47)
        return;
    if (commit->parent_count >= COMMIT_MAX_PARENTS)
        return;
    memcpy(commit->parents[commit->parent_count], line + 7, 40);
    commit->parent_count++;
}

static void parse_header_line(const char *line, int len, bob_commit_t *commit)
{
    parse_hex_field(line, len, "tree ", 5, 45, commit->tree);
    parse_parent_line(line, len, commit);
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

static void print_identity(const char *label, const char *field)
{
    if (field[0] == '\0')
        return;
    const char *email_end = strchr(field, '>');
    if (email_end)
        printf("%s %.*s\n", label, (int)(email_end - field + 1), field);
    else
        printf("%s %s\n", label, field);
}

static void print_date(const char *field)
{
    if (field[0] == '\0')
        return;
    const char *email_end = strchr(field, '>');
    if (email_end == NULL)
        return;
    long timestamp = strtol(email_end + 2, NULL, 10);
    if (timestamp == 0)
        return;
    time_t t = (time_t)timestamp;
    struct tm *tm = gmtime(&t);
    char buf[64] = {0};
    strftime(buf, sizeof(buf), "%a %b %d %H:%M:%S %Y %z", tm);
    printf("Date: %s\n", buf);
}

void commit_print(const char *hex, const bob_commit_t *commit)
{
    printf("commit %s\n", hex);
    for (int i = 0; i < commit->parent_count; i++)
        printf("Parent: %s\n", commit->parents[i]);
    print_identity("Author", commit->author);
    print_date(commit->author);
    print_identity("Commit", commit->committer);
    print_date(commit->committer);
    printf("\n");
    if (commit->message[0] != '\0')
        printf("%s\n", commit->message);
    printf("\n");
}

char *commit_create(const char *tree_hex, const char **parents, int parent_count, const char *message)
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
    for (int i = 0; i < parent_count; i++) {
        str_t parent_line = strformat("parent %s\n", parents[i]);
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
