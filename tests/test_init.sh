#!/usr/bin/env bash
source "$(dirname "$0")/lib.sh"

print_header "init"

start_test "init creates .bob layout"
new_repo init
assert_file_exists ".bob"
assert_file_exists ".bob/HEAD"
assert_file_exists ".bob/objects"
assert_file_exists ".bob/refs/heads"
end_test

start_test "init HEAD points at refs/heads/main"
new_repo init_head
head_contents="$(cat .bob/HEAD)"
assert_contains "$head_contents" "refs/heads/main" "HEAD references main"
end_test

start_test "init prints initialized message"
dir="$(mktemp -d /tmp/bob_test_init_msg_XXXXXX)"
BOB_TEST_DIRS+=("$dir")
cd "$dir"
out="$("$BOB" init 2>&1)"
assert_contains "$out" "Initialized" "init prints success"
end_test

summary
