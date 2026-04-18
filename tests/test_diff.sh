#!/usr/bin/env bash
source "$(dirname "$0")/lib.sh"

print_header "diff"

start_test "diff between two branches shows modified lines"
new_repo diff_basic
printf 'one\ntwo\nthree\n' > f.txt
"$BOB" add f.txt >/dev/null
"$BOB" commit -m "base" >/dev/null
"$BOB" branch feature >/dev/null
"$BOB" checkout feature >/dev/null
printf 'one\nTWO\nthree\n' > f.txt
"$BOB" add f.txt >/dev/null
"$BOB" commit -m "feature" >/dev/null
"$BOB" checkout main >/dev/null
out="$("$BOB" diff feature)"
assert_contains "$out" "f.txt" "path appears"
assert_contains "$out" "-two" "old line removed"
assert_contains "$out" "+TWO" "new line added"
end_test

start_test "diff shows added file as new"
new_repo diff_added
echo "x" > base.txt
"$BOB" add base.txt >/dev/null
"$BOB" commit -m "base" >/dev/null
"$BOB" branch feature >/dev/null
"$BOB" checkout feature >/dev/null
echo "fresh" > new.txt
"$BOB" add new.txt >/dev/null
"$BOB" commit -m "add new" >/dev/null
"$BOB" checkout main >/dev/null
out="$("$BOB" diff feature)"
assert_contains "$out" "new.txt" "added path appears"
assert_contains "$out" "+fresh" "new content appears as added"
end_test

start_test "diff against identical branch is empty"
new_repo diff_same
echo "stable" > f.txt
"$BOB" add f.txt >/dev/null
"$BOB" commit -m "base" >/dev/null
"$BOB" branch feature >/dev/null
out="$("$BOB" diff feature)"
assert_eq "$out" "" "no output when trees match"
end_test

summary
