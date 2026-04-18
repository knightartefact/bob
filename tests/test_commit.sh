#!/usr/bin/env bash
source "$(dirname "$0")/lib.sh"

print_header "commit / log / write-tree"

start_test "commit -m records a commit visible in log"
new_repo commit_basic
echo "first" > f.txt
"$BOB" add f.txt >/dev/null
"$BOB" commit -m "first commit" >/dev/null
log_out="$("$BOB" log)"
assert_contains "$log_out" "first commit" "log shows message"
end_test

start_test "commit reads message from stdin when -m is absent"
new_repo commit_stdin
echo "stdin-msg-test" > f.txt
"$BOB" add f.txt >/dev/null
echo "from stdin" | "$BOB" commit >/dev/null
log_out="$("$BOB" log)"
assert_contains "$log_out" "from stdin" "log shows stdin-supplied message"
end_test

start_test "two commits chain via parent in log"
new_repo commit_chain
echo "v1" > f.txt
"$BOB" add f.txt >/dev/null
"$BOB" commit -m "first" >/dev/null
echo "v2" > f.txt
"$BOB" add f.txt >/dev/null
"$BOB" commit -m "second" >/dev/null
log_out="$("$BOB" log)"
assert_contains "$log_out" "first"
assert_contains "$log_out" "second"
end_test

start_test "write-tree returns a 40-char sha"
new_repo write_tree
echo "x" > f.txt
"$BOB" add f.txt >/dev/null
sha="$("$BOB" write-tree)"
assert_eq "${#sha}" "40" "tree sha length"
prefix="${sha:0:2}"
suffix="${sha:2}"
assert_file_exists ".bob/objects/$prefix/$suffix" "tree object persisted"
end_test

summary
