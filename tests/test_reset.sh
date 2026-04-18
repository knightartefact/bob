#!/usr/bin/env bash
source "$(dirname "$0")/lib.sh"

print_header "reset"

start_test "reset discards unstaged modifications"
new_repo reset_unstaged
echo "committed" > f.txt
"$BOB" add f.txt >/dev/null
"$BOB" commit -m "init" >/dev/null
echo "scratch" > f.txt
"$BOB" reset >/dev/null
content="$(cat f.txt)"
assert_eq "$content" "committed" "file restored to HEAD content"
end_test

start_test "reset discards staged changes"
new_repo reset_staged
echo "v1" > f.txt
"$BOB" add f.txt >/dev/null
"$BOB" commit -m "init" >/dev/null
echo "v2" > f.txt
"$BOB" add f.txt >/dev/null
"$BOB" reset >/dev/null
content="$(cat f.txt)"
assert_eq "$content" "v1" "staged change rolled back"
status_out="$("$BOB" status)"
assert_not_contains "$status_out" "Changes to be committed" "no staged changes after reset"
end_test

start_test "reset removes files added on top of HEAD"
new_repo reset_added
echo "kept" > kept.txt
"$BOB" add kept.txt >/dev/null
"$BOB" commit -m "init" >/dev/null
echo "extra" > extra.txt
"$BOB" add extra.txt >/dev/null
"$BOB" reset >/dev/null
assert_file_exists kept.txt "committed file remains"
assert_file_absent extra.txt "uncommitted-add file removed"
listing="$("$BOB" ls-files)"
assert_not_contains "$listing" "extra.txt" "extra.txt no longer indexed"
end_test

start_test "reset aborts an in-progress merge"
new_repo reset_abort_merge
printf 'a\nb\nc\n' > f.txt
"$BOB" add f.txt >/dev/null
"$BOB" commit -m "base" >/dev/null
"$BOB" branch feature >/dev/null
"$BOB" checkout feature >/dev/null
printf 'a\nFEAT\nc\n' > f.txt
"$BOB" add f.txt >/dev/null
"$BOB" commit -m "feat" >/dev/null
"$BOB" checkout main >/dev/null
printf 'a\nMAIN\nc\n' > f.txt
"$BOB" add f.txt >/dev/null
"$BOB" commit -m "main" >/dev/null
"$BOB" merge feature 2>/dev/null
rc=$?
assert_nonzero "$rc" "merge produced a conflict"
assert_file_exists ".bob/MERGE_HEAD" "MERGE_HEAD set during conflict"
"$BOB" reset >/dev/null
assert_file_absent ".bob/MERGE_HEAD" "MERGE_HEAD cleared by reset"
content="$(cat f.txt)"
assert_not_contains "$content" "<<<<<<<" "no conflict markers after reset"
expected=$'a\nMAIN\nc'
assert_eq "$content" "$expected" "file restored to HEAD content"
end_test

start_test "reset leaves untracked files alone"
new_repo reset_untracked
echo "tracked" > tracked.txt
"$BOB" add tracked.txt >/dev/null
"$BOB" commit -m "init" >/dev/null
echo "outsider" > untracked.txt
"$BOB" reset >/dev/null
assert_file_exists untracked.txt "untracked file survives reset"
content="$(cat untracked.txt)"
assert_eq "$content" "outsider" "untracked content preserved"
end_test

start_test "reset on empty repo reports no commits"
new_repo reset_empty
out="$("$BOB" reset 2>&1)"
rc=$?
assert_nonzero "$rc" "reset before any commit fails"
assert_contains "$out" "no commits yet" "diagnostic message"
end_test

summary
