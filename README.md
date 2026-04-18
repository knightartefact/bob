# bob

A small git-like version control tool, written from scratch in C as a learning
project. `bob` stores your project's history as content-addressed objects under
a `.bob/` directory and exposes a familiar set of commands: `add`, `commit`,
`branch`, `checkout`, `diff`, `merge`, and so on.

It is not a drop-in git replacement — the on-disk format is its own and the
feature set is small — but the workflow will look very familiar if you know
git.

## Build

```
cmake -B build
cmake --build build
```

The resulting binary is `build/bob`. Add it to your `PATH` or invoke directly.

Two runtime dependencies on host tools:

- `diff` — used by `bob diff` to render unified hunks.
- `diff3` — used by `bob merge` to do line-level 3-way merges. If absent, merge
  falls back to whole-file conflict markers.

## First-time setup

Inside any directory you want to track:

```
bob init
printf 'name=Your Name\nemail=you@example.com\n' > .bob/config
```

The config file is required — `bob commit` will refuse to run without `name`
and `email` set.

## Typical workflow

```
bob init                          # create .bob/ in the current directory
bob add hello.c                   # stage a file
bob commit -m "first commit"      # record a commit
bob status                        # show staged / unstaged / untracked

bob branch feature                # create a branch
bob checkout feature              # switch to it
# ...edit files...
bob add hello.c
bob commit -m "feature work"

bob checkout main
bob merge feature                 # merges feature into main
```

If `bob merge` reports `CONFLICT in N file(s)`, `.bob/MERGE_HEAD` is set and
the conflicted files contain `<<<<<<<` / `=======` / `>>>>>>>` markers around
the unstable line ranges. To finish the merge, edit those files until the
markers are gone, then:

```
bob add <resolved files>
bob commit -m "merge feature"     # produces a 2-parent merge commit
```

To abandon the merge instead, run `bob reset` — it clears `MERGE_HEAD` and
restores the working tree to HEAD.

## Commands

Porcelain (everyday use):

| command | description |
|---|---|
| `bob init` | create a new repository in the current directory |
| `bob status` | show staged, unstaged, and untracked changes |
| `bob add <files...>` | stage files |
| `bob commit -m <msg>` | record a commit (reads stdin if `-m` is omitted) |
| `bob log` | show commit history from HEAD |
| `bob graph` | show a simple ASCII graph of the current branch (latest commit on top) |
| `bob branch` | list branches (`*` marks the current one) |
| `bob branch <name>` | create a branch at HEAD |
| `bob checkout <branch-or-hex>` | switch branches or restore a commit |
| `bob diff <ref>` | show changes between HEAD and another ref |
| `bob merge <branch>` | merge another branch into the current one |
| `bob reset` | discard uncommitted changes; abort an in-progress merge |

Plumbing (low-level, mostly useful for debugging or scripting):

| command | description |
|---|---|
| `bob hash-object <file>` | compute a SHA1 and store the file as a blob |
| `bob cat-file <hex>` | print the type and contents of an object |
| `bob update-index <file>` | stage a single file (lower-level than `add`) |
| `bob ls-files` | list paths currently in the index |
| `bob write-tree` | write the index out as a tree object |
| `bob commit-tree <tree-hex> -m <msg> [-p <parent-hex>]` | create a commit object directly |

## Tests

End-to-end test scripts live in `tests/`. After building, run:

```
./tests/run.sh
```

Each `tests/test_*.sh` file is also runnable on its own — useful when iterating
on a single feature. Tests create their own scratch repos under `/tmp` and
clean up on exit.

## Repository layout

```
include/   public headers
src/       implementation
libs/str/  small dynamic-string library used internally
tests/     end-to-end shell tests
build/     CMake build output (created on first build)
```

After `bob init`, the per-repository state lives under `.bob/`:

```
.bob/HEAD               current branch ref or detached commit hex
.bob/refs/heads/<name>  branch tips
.bob/objects/xx/yyyy... content-addressed blobs, trees, commits
.bob/index              staged file list
.bob/MERGE_HEAD         present only during an in-progress merge
.bob/config             name and email used for commits
```
