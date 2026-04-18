#!/usr/bin/env bash
source "$(dirname "$0")/lib.sh"

print_header "add / ls-files"

start_test "add stages a single file"
new_repo add_one
echo "alpha" > a.txt
"$BOB" add a.txt >/dev/null
listing="$("$BOB" ls-files)"
assert_contains "$listing" "a.txt" "ls-files lists added file"
end_test

start_test "add stages multiple files in one call"
new_repo add_many
echo "1" > one.txt
echo "2" > two.txt
echo "3" > three.txt
"$BOB" add one.txt two.txt three.txt >/dev/null
listing="$("$BOB" ls-files)"
assert_contains "$listing" "one.txt"
assert_contains "$listing" "two.txt"
assert_contains "$listing" "three.txt"
end_test

start_test "ls-files is empty before any add"
new_repo ls_empty
listing="$("$BOB" ls-files)"
assert_eq "$listing" "" "no files staged"
end_test

summary
