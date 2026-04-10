#include "worktree.h"

#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "str/str.h"

static int should_skip(const char *name)
{
    return strcmp(name, ".") == 0
        || strcmp(name, "..") == 0
        || strcmp(name, ".bob") == 0;
}

static str_t build_fullpath(const char *dir, const char *name)
{
    return strformat("%s/%s", dir, name);
}

static str_t build_relpath(const char *prefix, const char *name)
{
    if (prefix[0] != '\0')
        return strformat("%s/%s", prefix, name);
    return strnew(name);
}

static void add_file(filelist_t *out, str_t relpath)
{
    if (out->count >= WORKTREE_MAX_FILES)
        return;
    strncpy(out->paths[out->count], relpath,
        sizeof(out->paths[out->count]) - 1);
    out->count++;
}

static void visit_entry(const char *dir, const char *prefix, const char *name, filelist_t *out)
{
    str_t fullpath = build_fullpath(dir, name);

    struct stat st;
    if (stat(fullpath, &st) == -1)
        strfree(fullpath);

    str_t relpath = build_relpath(prefix, name);
    if (S_ISDIR(st.st_mode))
        worktree_list(fullpath, relpath, out);
    else if (S_ISREG(st.st_mode))
        add_file(out, relpath);

    strfree(fullpath);
    strfree(relpath);
}

int worktree_list(const char *dir, const char *prefix, filelist_t *out)
{
    DIR *dp = opendir(dir);
    if (dp == NULL) {
        perror(dir);
        return -1;
    }

    struct dirent *ent;
    while ((ent = readdir(dp)) != NULL) {
        if (should_skip(ent->d_name))
            continue;
        if (out->count >= WORKTREE_MAX_FILES)
            break;
        visit_entry(dir, prefix, ent->d_name, out);
    }
    closedir(dp);
    return 0;
}
