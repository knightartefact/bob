#!/usr/bin/env bash
source "$(dirname "$0")/lib.sh"

print_header "checkout"

start_test "checkout switches branches and updates working tree"
new_repo checkout_switch
echo "base" > f.txt
"$BOB" add f.txt >/dev/null
"$BOB" commit -m "base" >/dev/null
"$BOB" branch feature >/dev/null
"$BOB" checkout feature >/dev/null
echo "from feature" > f.txt
"$BOB" add f.txt >/dev/null
"$BOB" commit -m "feature edit" >/dev/null
"$BOB" checkout main >/dev/null
content="$(cat f.txt)"
assert_eq "$content" "base" "main keeps original content"
"$BOB" checkout feature >/dev/null
content="$(cat f.txt)"
assert_eq "$content" "from feature" "feature has its own content"
end_test

start_test "checkout updates HEAD ref"
new_repo checkout_head
echo "x" > f.txt
"$BOB" add f.txt >/dev/null
"$BOB" commit -m "init" >/dev/null
"$BOB" branch feature >/dev/null
"$BOB" checkout feature >/dev/null
head_contents="$(cat .bob/HEAD)"
assert_contains "$head_contents" "refs/heads/feature" "HEAD now references feature"
end_test

start_test "checkout removes files added on the other branch"
new_repo checkout_remove
echo "shared" > shared.txt
"$BOB" add shared.txt >/dev/null
"$BOB" commit -m "base" >/dev/null
"$BOB" branch feature >/dev/null
"$BOB" checkout feature >/dev/null
echo "extra" > only_feature.txt
"$BOB" add only_feature.txt >/dev/null
"$BOB" commit -m "add only_feature" >/dev/null
"$BOB" checkout main >/dev/null
assert_file_absent "only_feature.txt" "feature-only file should be gone on main"
end_test

summary
