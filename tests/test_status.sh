#!/usr/bin/env bash
source "$(dirname "$0")/lib.sh"

print_header "status"

start_test "status reports current branch"
new_repo status_branch
out="$("$BOB" status)"
assert_contains "$out" "On branch main" "branch line"
end_test

start_test "status flags untracked files"
new_repo status_untracked
echo "data" > new.txt
out="$("$BOB" status)"
assert_contains "$out" "Untracked files" "header"
assert_contains "$out" "new.txt" "filename listed"
end_test

start_test "status flags staged new file"
new_repo status_staged
echo "data" > f.txt
"$BOB" add f.txt >/dev/null
out="$("$BOB" status)"
assert_contains "$out" "Changes to be committed"
assert_contains "$out" "new file"
assert_contains "$out" "f.txt"
end_test

start_test "status flags unstaged modification after commit"
new_repo status_unstaged
echo "v1" > f.txt
"$BOB" add f.txt >/dev/null
"$BOB" commit -m "first" >/dev/null
sleep 1            # bump mtime past the indexed second
echo "v2" > f.txt
out="$("$BOB" status)"
assert_contains "$out" "Changes not staged for commit"
assert_contains "$out" "modified"
assert_contains "$out" "f.txt"
end_test

summary
