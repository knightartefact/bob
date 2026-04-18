#!/usr/bin/env bash
source "$(dirname "$0")/lib.sh"

print_header "branch"

start_test "branch <name> creates a new ref"
new_repo branch_create
echo "x" > f.txt
"$BOB" add f.txt >/dev/null
"$BOB" commit -m "init" >/dev/null
"$BOB" branch feature >/dev/null
assert_file_exists ".bob/refs/heads/feature" "feature ref file"
end_test

start_test "branch (no args) lists branches with current marked"
new_repo branch_list
echo "x" > f.txt
"$BOB" add f.txt >/dev/null
"$BOB" commit -m "init" >/dev/null
"$BOB" branch feature >/dev/null
out="$("$BOB" branch)"
assert_contains "$out" "* main" "main is current"
assert_contains "$out" "feature" "feature listed"
end_test

start_test "creating an existing branch fails"
new_repo branch_dup
echo "x" > f.txt
"$BOB" add f.txt >/dev/null
"$BOB" commit -m "init" >/dev/null
"$BOB" branch feature >/dev/null
"$BOB" branch feature 2>/dev/null
rc=$?
assert_nonzero "$rc" "duplicate branch should fail"
end_test

summary
