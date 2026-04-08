#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>

#include "repository.h"
#include "object.h"
#include "index.h"
#include "tree.h"
#include "commit.h"
#include "utils.h"
#include "status.h"
#include "checkout.h"
#include "diff.h"
#include "merge.h"

int cmd_init(void)
{
    create_repository();
}

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

int cmd_hash_object(const char *filepath)
{
    char buf[32768] = {0};
    int size = sizeof(buf);
    if (read_file(filepath, buf, &size)) {
        return -1;
    }
    bob_object_t *obj = object_new("blob", buf, size);
    char *digest = object_write(obj);
    if (digest == NULL) {
        return -1;
    }
    char hex[41] = {0};
    sha2hex(digest, hex);
    printf("%s\n", hex);
    object_free(obj);
    free(digest);
    return 0;
}

static void cat_blob(const bob_object_t *blob)
{
    fwrite(blob->data, sizeof(char), blob->size, stdout);
}

static void cat_tree(const bob_object_t *tree)
{
    size_t i = 0;

    while (i < tree->size) {
        // Parse mode (up to space)
        const char *mode = tree->data + i;
        const char *space = memchr(mode, ' ', tree->size - i);
        if (space == NULL)
            break;
        size_t mode_len = space - mode;

        // Parse name (up to null byte)
        const char *name = space + 1;
        const char *nul = memchr(name, '\0', tree->size - (name - tree->data));
        if (nul == NULL)
            break;
        size_t name_len = nul - name;

        // 20-byte binary SHA1 follows the null byte
        const unsigned char *sha = (const unsigned char *)(nul + 1);
        if ((sha + 20) - (const unsigned char *)tree->data > (long)tree->size)
            break;

        char hex[41] = {0};
        sha2hex(sha, hex);

        // Determine type from mode: "40000" is tree, otherwise blob
        const char *type = (mode_len == 5 && memcmp(mode, "40000", 5) == 0)
            ? "tree" : "blob";

        printf("%.*s %s %s\t%.*s\n",
            (int)mode_len, mode, type, hex, (int)name_len, name);

        i = (sha + 20) - (const unsigned char *)tree->data;
    }
}

int cmd_cat_file(const char *hex)
{
    bob_object_t *obj = object_read(hex);

    if (obj == NULL) {
        return -1;
    }
    if (strcmp(obj->type, "commit") == 0) {
        bob_commit_t commit;
        commit_parse(obj, &commit);
        commit_print(hex, &commit);
    } else {
        printf("type: %s\n", obj->type);
        if (strcmp(obj->type, "tree") == 0)
            cat_tree(obj);
        else
            cat_blob(obj);
    }

    object_free(obj);
    return 0;
}

int cmd_update_index(const char *filepath)
{
    struct stat st;
    if (stat(filepath, &st) == -1) {
        if (errno == ENOENT) {
            index_t index;
            index_read(&index);
            if (index_remove(&index, filepath) == -1) {
                fprintf(stderr, "pathspec '%s' did not match any files\n", filepath);
                return -1;
            }
            index_write(&index);
            return 0;
        }
        perror(filepath);
        return -1;
    }
    char buf[32768] = {0};
    int size = sizeof(buf);
    if (read_file(filepath, buf, &size) == -1) {
        fprintf(stderr, "Could not read %s file\n", filepath);
        return -1;
    }
    bob_object_t *obj = object_new("blob", buf, size);
    if (obj == NULL) {
        return -1;
    }
    char *sha = object_write(obj);
    if (sha == NULL) {
        object_free(obj);
        return -1;
    }
    object_free(obj);
    index_entry_t entry = {0};
    strncpy(entry.path, filepath, sizeof(entry.path));
    memcpy(entry.sha1, sha, sizeof(entry.sha1));
    entry.mode = st.st_mode;
    entry.mtime = st.st_mtime;
    entry.size = st.st_size;
    entry.path_len = strlen(filepath);

    index_t index;
    index_read(&index);
    index_add(&index, &entry);
    index_write(&index);
    return 0;
}

int cmd_ls_files(void)
{
    index_t index = {0};
    index_read(&index);
    for (int i = 0; i < index.count; i++) {
        const index_entry_t *entry = &index.entries[i];
        char hex[41] = {0};
        sha2hex(entry->sha1, hex);
        printf("%o %s %s\n", entry->mode, hex, entry->path);
    }
    return 0;
}

int cmd_write_tree(void)
{
    index_t index = {0};
    index_read(&index);
    char *digest = tree_write(&index);
    if (digest == NULL) {
        return -1;
    }
    char hex[41] = {0};
    sha2hex((unsigned char *)digest, hex);
    printf("%s\n", hex);
    free(digest);
    return 0;
}

int cmd_commit_tree(const char *tree_hex, const char *message, const char *parent_hex)
{
    const char *parents[] = { parent_hex };
    int parent_count = parent_hex ? 1 : 0;
    char *digest = commit_create(tree_hex, parents, parent_count, message);
    if (digest == NULL) {
        return -1;
    }
    char hex[41] = {0};
    sha2hex((unsigned char *)digest, hex);
    printf("%s\n", hex);
    free(digest);
    return 0;
}

int cmd_add(int count, const char **files)
{
    for (int i = 0; i < count; i++) {
        if (cmd_update_index(files[i]) == -1)
            return -1;
    }
    return 0;
}

int cmd_log(void)
{
    char ref[256] = {0};
    int head_type = ref_resolve_head(ref, sizeof(ref));
    if (head_type == -1)
        return -1;
    char hex[41] = {0};
    if (head_type == 1) {
        strncpy(hex, ref, 40);
    } else if (ref_read(ref, hex) == -1) {
        fprintf(stderr, "no commits yet\n");
        return -1;
    }

    while (hex[0] != '\0') {
        bob_object_t *obj = object_read(hex);
        if (obj == NULL)
            return -1;
        if (strcmp(obj->type, "commit") != 0) {
            fprintf(stderr, "%s is not a commit\n", hex);
            object_free(obj);
            return -1;
        }

        bob_commit_t commit;
        commit_parse(obj, &commit);
        commit_print(hex, &commit);

        object_free(obj);
        if (commit.parent_count > 0)
            memcpy(hex, commit.parents[0], 41);
        else
            hex[0] = '\0';
    }
    return 0;
}

int cmd_commit(const char *message)
{
    index_t index = {0};
    index_read(&index);
    if (index.count == 0) {
        fprintf(stderr, "nothing to commit (empty index)\n");
        return 0;
    }
    char *tree_digest = tree_write(&index);
    if (tree_digest == NULL) {
        return -1;
    }
    char tree_hex[41] = {0};
    sha2hex((unsigned char *)tree_digest, tree_hex);
    free(tree_digest);

    char ref[256] = {0};
    int head_type = ref_resolve_head(ref, sizeof(ref));
    if (head_type == -1) {
        return -1;
    }
    char parent_hex[41] = {0};
    char merge_hex[41] = {0};
    int is_merge = merge_read_head(merge_hex) == 0;

    if (is_merge && merge_has_conflicts(&index)) {
        fprintf(stderr, "cannot commit: unresolved conflicts\n");
        return -1;
    }

    const char *parents[COMMIT_MAX_PARENTS] = {0};
    int parent_count = 0;
    if (head_type == 1) {
        strncpy(parent_hex, ref, 40);
        parents[parent_count++] = parent_hex;
    } else if (ref_read(ref, parent_hex) == 0) {
        parents[parent_count++] = parent_hex;
    }
    if (is_merge)
        parents[parent_count++] = merge_hex;

    char *commit_digest = commit_create(tree_hex, parents, parent_count, message);
    if (commit_digest == NULL) {
        return -1;
    }
    char commit_hex[41] = {0};
    sha2hex((unsigned char *)commit_digest, commit_hex);
    free(commit_digest);

    if (head_type == 1) {
        ref_write_head(commit_hex);
    } else if (ref_update(ref, commit_hex) == -1) {
        return -1;
    }

    if (head_type == 1) {
        printf("[detached %.7s] %s\n", commit_hex, message);
    } else {
        const char *branch = strrchr(ref, '/');
        branch = branch ? branch + 1 : ref;
        printf("[%s %.7s] %s\n", branch, commit_hex, message);
    }
    if (is_merge)
        merge_clear_head();
    return 0;
}

int cmd_status(void)
{
    char ref[256] = {0};
    int head_type = ref_resolve_head(ref, sizeof(ref));
    if (head_type == -1)
        return -1;
    if (head_type == 1) {
        printf("HEAD detached at %.7s\n", ref);
    } else {
        const char *branch = strrchr(ref, '/');
        branch = branch ? branch + 1 : ref;
        printf("On branch %s\n", branch);
    }

    index_t index = {0};
    index_read(&index);

    tree_list_t head_tree = {0};
    char head_hex[41] = {0};
    if (head_type == 1) {
        strncpy(head_hex, ref, 40);
    } else if (ref_read(ref, head_hex) != 0) {
        head_hex[0] = '\0';
    }
    if (head_hex[0] != '\0') {
        bob_object_t *obj = object_read(head_hex);
        if (obj == NULL) {
            return -1;
        }
        bob_commit_t commit;
        commit_parse(obj, &commit);
        object_free(obj);
        if (commit.tree[0] != '\0')
            tree_flatten(commit.tree, "", &head_tree);
    }

    status_staged(&index, &head_tree);
    status_unstaged(&index);

    filelist_t files = {0};
    worktree_list(".", "", &files);
    status_untracked(&files, &index);
    return 0;
}

int cmd_checkout(const char *arg)
{
    char hex[41] = {0};
    ref_resolve_commit(arg, hex);
    bob_object_t *obj = object_read(hex);
    if (obj == NULL) {
        fprintf(stderr, "checkout: could not read commit %s\n", arg);
        return -1;
    }
    if (strcmp(obj->type, "commit") != 0) {
        fprintf(stderr, "checkout: %s is not a commit\n", hex);
        object_free(obj);
        return -1;
    }
    bob_commit_t commit;
    commit_parse(obj, &commit);
    object_free(obj);

    tree_list_t tree_list = {0};
    if (tree_flatten(commit.tree, "", &tree_list) == -1)
        return -1;

    index_t old_index = {0};
    index_read(&old_index);

    index_t new_index = {0};
    checkout_tree_to_index(&tree_list, &new_index);
    index_write(&new_index);

    checkout_remove_old_files(&old_index, &new_index);
    if (checkout_write_files(&new_index) == -1)
        return -1;
    index_write(&new_index);

    if (ref_is_branch(arg)) {
        char head_val[512] = {0};
        snprintf(head_val, sizeof(head_val), "ref: refs/heads/%s", arg);
        ref_write_head(head_val);
    } else {
        ref_write_head(hex);
    }
    return 0;
}

int cmd_branch(const char *name)
{
    char ref[256] = {0};
    int head_type = ref_resolve_head(ref, sizeof(ref));
    if (head_type == -1)
        return -1;
    char hex[41] = {0};
    if (head_type == 1) {
        strncpy(hex, ref, 40);
    } else if (ref_read(ref, hex) == -1) {
        fprintf(stderr, "no commits yet\n");
        return -1;
    }
    char branch_ref[256] = {0};
    snprintf(branch_ref, sizeof(branch_ref), "refs/heads/%s", name);
    if (ref_is_branch(name)) {
        fprintf(stderr, "branch '%s' already exists\n", name);
        return -1;
    }
    return ref_update(branch_ref, hex);
}

int cmd_list_branches(void)
{
    char ref[256] = {0};
    int head_type = ref_resolve_head(ref, sizeof(ref));

    DIR *dp = opendir(".bob/refs/heads");
    if (dp == NULL) {
        perror(".bob/refs/heads");
        return -1;
    }
    struct dirent *ent;
    while ((ent = readdir(dp)) != NULL) {
        if (ent->d_name[0] == '.')
            continue;
        char branch_ref[256] = {0};
        snprintf(branch_ref, sizeof(branch_ref), "refs/heads/%s", ent->d_name);
        int is_current = head_type == 0 && strcmp(ref, branch_ref) == 0;
        printf("%s %s\n", is_current ? "*" : " ", ent->d_name);
    }
    closedir(dp);
    return 0;
}

static int resolve_head_tree(tree_list_t *out)
{
    char ref[256] = {0};
    int head_type = ref_resolve_head(ref, sizeof(ref));
    if (head_type == -1)
        return -1;
    char hex[41] = {0};
    if (head_type == 1) {
        strncpy(hex, ref, 40);
    } else if (ref_read(ref, hex) == -1) {
        fprintf(stderr, "no commits yet\n");
        return -1;
    }
    bob_object_t *obj = object_read(hex);
    if (obj == NULL)
        return -1;
    bob_commit_t commit;
    commit_parse(obj, &commit);
    object_free(obj);
    return tree_flatten(commit.tree, "", out);
}

static int resolve_arg_tree(const char *arg, tree_list_t *out)
{
    char hex[41] = {0};
    ref_resolve_commit(arg, hex);
    bob_object_t *obj = object_read(hex);
    if (obj == NULL)
        return -1;
    bob_commit_t commit;
    commit_parse(obj, &commit);
    object_free(obj);
    return tree_flatten(commit.tree, "", out);
}

int cmd_diff(const char *arg)
{
    tree_list_t ours = {0};
    if (resolve_head_tree(&ours) == -1)
        return -1;
    tree_list_t theirs = {0};
    if (resolve_arg_tree(arg, &theirs) == -1)
        return -1;
    diff_list_t diff = {0};
    diff_trees(&ours, &theirs, &diff);
    for (int i = 0; i < diff.count; i++)
        diff_print_entry(&diff.entries[i]);
    return 0;
}

static int resolve_head_hex(char *out_hex)
{
    char ref[256] = {0};
    int head_type = ref_resolve_head(ref, sizeof(ref));
    if (head_type == -1)
        return -1;
    if (head_type == 1) {
        strncpy(out_hex, ref, 40);
    } else if (ref_read(ref, out_hex) == -1) {
        fprintf(stderr, "no commits yet\n");
        return -1;
    }
    return 0;
}

int cmd_merge(const char *branch)
{
    char ours_hex[41] = {0};
    if (resolve_head_hex(ours_hex) == -1)
        return -1;
    char theirs_hex[41] = {0};
    ref_resolve_commit(branch, theirs_hex);

    if (strcmp(ours_hex, theirs_hex) == 0) {
        printf("Already up to date.\n");
        return 0;
    }

    char base_hex[41] = {0};
    if (merge_find_base(ours_hex, theirs_hex, base_hex) == -1) {
        fprintf(stderr, "merge: no common ancestor\n");
        return -1;
    }
    if (strcmp(base_hex, ours_hex) == 0) {
        printf("Fast-forward\n");
        return cmd_checkout(branch);
    }

    tree_list_t base_tree = {0}, ours_tree = {0}, theirs_tree = {0};
    merge_resolve_commit_tree(base_hex, &base_tree);
    resolve_head_tree(&ours_tree);
    resolve_arg_tree(branch, &theirs_tree);

    merge_result_t result = {0};
    merge_trees(&base_tree, &ours_tree, &theirs_tree, &result);
    merge_apply_to_worktree(&result);

    if (result.conflict_count > 0) {
        merge_write_head(theirs_hex);
        printf("CONFLICT in %d file(s):\n", result.conflict_count);
        for (int i = 0; i < result.conflict_count; i++)
            printf("\t%s\n", result.conflicts[i].path);
        printf("Automatic merge failed; fix conflicts and then commit.\n");
        return 1;
    }
    return merge_create_commit(branch, ours_hex, theirs_hex);
}
