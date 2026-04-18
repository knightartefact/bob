#!/usr/bin/env bash
source "$(dirname "$0")/lib.sh"

print_header "merge"

start_test "fast-forward when base equals ours"
new_repo merge_ff
echo "v1" > f.txt
"$BOB" add f.txt >/dev/null
"$BOB" commit -m "base" >/dev/null
"$BOB" branch feature >/dev/null
"$BOB" checkout feature >/dev/null
echo "v2" > f.txt
"$BOB" add f.txt >/dev/null
"$BOB" commit -m "feature" >/dev/null
"$BOB" checkout main >/dev/null
out="$("$BOB" merge feature)"
assert_contains "$out" "Fast-forward" "fast-forward path"
content="$(cat f.txt)"
assert_eq "$content" "v2" "main now has feature contents"
end_test

start_test "merge of disjoint files produces no conflict"
new_repo merge_disjoint
echo "shared" > shared.txt
"$BOB" add shared.txt >/dev/null
"$BOB" commit -m "base" >/dev/null
"$BOB" branch feature >/dev/null
"$BOB" checkout feature >/dev/null
echo "feat" > only_feat.txt
"$BOB" add only_feat.txt >/dev/null
"$BOB" commit -m "add feat-only" >/dev/null
"$BOB" checkout main >/dev/null
echo "main" > only_main.txt
"$BOB" add only_main.txt >/dev/null
"$BOB" commit -m "add main-only" >/dev/null
out="$("$BOB" merge feature)"
assert_contains "$out" "Merge made" "merge commit produced"
assert_file_exists only_feat.txt
assert_file_exists only_main.txt
end_test

start_test "non-overlapping line edits in same file auto-merge"
new_repo merge_lines_clean
printf 'a\nb\nc\nd\ne\nf\ng\nh\ni\nj\n' > f.txt
"$BOB" add f.txt >/dev/null
"$BOB" commit -m "base" >/dev/null
"$BOB" branch feature >/dev/null
"$BOB" checkout feature >/dev/null
printf 'A\nB\nc\nd\ne\nf\ng\nh\ni\nj\n' > f.txt
"$BOB" add f.txt >/dev/null
"$BOB" commit -m "feature top" >/dev/null
"$BOB" checkout main >/dev/null
printf 'a\nb\nc\nd\ne\nf\ng\nh\nI\nJ\n' > f.txt
"$BOB" add f.txt >/dev/null
"$BOB" commit -m "main bottom" >/dev/null
out="$("$BOB" merge feature)"
assert_contains "$out" "Merge made" "clean line-level merge"
expected=$'A\nB\nc\nd\ne\nf\ng\nh\nI\nJ'
assert_file_eq "f.txt" "$expected" "both edits applied"
end_test

start_test "overlapping edits produce line-scoped conflict markers"
new_repo merge_lines_conflict
printf 'a\nb\nc\nd\ne\n' > f.txt
"$BOB" add f.txt >/dev/null
"$BOB" commit -m "base" >/dev/null
"$BOB" branch feature >/dev/null
"$BOB" checkout feature >/dev/null
printf 'a\nb\nFEAT\nd\ne\n' > f.txt
"$BOB" add f.txt >/dev/null
"$BOB" commit -m "feature" >/dev/null
"$BOB" checkout main >/dev/null
printf 'a\nb\nMAIN\nd\ne\n' > f.txt
"$BOB" add f.txt >/dev/null
"$BOB" commit -m "main" >/dev/null
out="$("$BOB" merge feature 2>&1)"
rc=$?
assert_nonzero "$rc" "merge with overlap returns nonzero"
assert_contains "$out" "CONFLICT" "conflict reported"
content="$(cat f.txt)"
assert_contains "$content" "<<<<<<<" "ours marker"
assert_contains "$content" "=======" "split marker"
assert_contains "$content" ">>>>>>>" "theirs marker"
assert_contains "$content" "MAIN" "ours value"
assert_contains "$content" "FEAT" "theirs value"
# Lines outside the conflict region should appear as plain text, not
# wrapped between markers — proving the markers are line-scoped.
first_line="$(printf '%s\n' "$content" | head -1)"
assert_eq "$first_line" "a" "untouched line 1 is plain"
last_line="$(printf '%s\n' "$content" | tail -1)"
assert_eq "$last_line" "e" "untouched line 5 is plain"
end_test

start_test "already-up-to-date when merging same commit"
new_repo merge_uptodate
echo "x" > f.txt
"$BOB" add f.txt >/dev/null
"$BOB" commit -m "init" >/dev/null
"$BOB" branch feature >/dev/null
out="$("$BOB" merge feature)"
assert_contains "$out" "Already up to date" "no-op merge"
end_test

# Helper: build a repo whose HEAD is a conflicted merge (MERGE_HEAD set,
# f.txt contains conflict markers). Used by the resolve-and-commit tests.
setup_conflicted_merge() {
    new_repo "$1"
    printf 'a\nb\nc\nd\ne\n' > f.txt
    "$BOB" add f.txt >/dev/null
    "$BOB" commit -m "base" >/dev/null
    "$BOB" branch feature >/dev/null
    "$BOB" checkout feature >/dev/null
    printf 'a\nb\nFEAT\nd\ne\n' > f.txt
    "$BOB" add f.txt >/dev/null
    "$BOB" commit -m "feature" >/dev/null
    "$BOB" checkout main >/dev/null
    printf 'a\nb\nMAIN\nd\ne\n' > f.txt
    "$BOB" add f.txt >/dev/null
    "$BOB" commit -m "main" >/dev/null
    "$BOB" merge feature 2>/dev/null
}

start_test "commit refuses to finalize merge while markers are unresolved"
setup_conflicted_merge merge_unresolved
# User stages the file but forgot to actually clean up the markers.
"$BOB" add f.txt >/dev/null
out="$("$BOB" commit -m "premature" 2>&1)"
rc=$?
assert_nonzero "$rc" "commit fails with unresolved conflicts"
assert_contains "$out" "unresolved conflicts" "diagnostic message"
assert_file_exists ".bob/MERGE_HEAD" "MERGE_HEAD still set after refusal"
end_test

start_test "resolving conflicts and committing creates a 2-parent merge commit"
setup_conflicted_merge merge_resolve
# Manually resolve: keep both edits as a hand-merged value.
printf 'a\nb\nMAIN+FEAT\nd\ne\n' > f.txt
"$BOB" add f.txt >/dev/null
"$BOB" commit -m "merge: resolved" >/dev/null
assert_file_absent ".bob/MERGE_HEAD" "MERGE_HEAD cleared on success"
log_out="$("$BOB" log)"
assert_contains "$log_out" "merge: resolved" "merge commit appears in log"
# Verify the new commit object has two parent lines via cat-file. The
# commit hex is the current HEAD of main.
head_hex="$(cat .bob/refs/heads/main)"
catfile_out="$("$BOB" cat-file "$head_hex")"
parent_count="$(printf '%s\n' "$catfile_out" | grep -c '^Parent: ')"
assert_eq "$parent_count" "2" "merge commit has exactly 2 parents"
end_test

summary
