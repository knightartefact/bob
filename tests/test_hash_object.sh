#!/usr/bin/env bash
source "$(dirname "$0")/lib.sh"

print_header "hash-object / cat-file"

start_test "hash-object emits 40-char sha and stores blob"
new_repo hash
echo "hello world" > greet.txt
sha="$("$BOB" hash-object greet.txt)"
assert_eq "${#sha}" "40" "sha length"
prefix="${sha:0:2}"
suffix="${sha:2}"
assert_file_exists ".bob/objects/$prefix/$suffix" "blob file present"
end_test

start_test "cat-file emits blob type header and original contents"
new_repo catfile
printf 'roundtrip data\nline2\n' > data.txt
sha="$("$BOB" hash-object data.txt)"
out="$("$BOB" cat-file "$sha")"
assert_contains "$out" "type: blob" "header line"
assert_contains "$out" "roundtrip data" "first line"
assert_contains "$out" "line2" "second line"
end_test

start_test "hash-object handles empty file"
new_repo hash_empty
: > empty.txt
sha="$("$BOB" hash-object empty.txt)"
assert_eq "${#sha}" "40" "empty blob still produces 40-char sha"
prefix="${sha:0:2}"
suffix="${sha:2}"
assert_file_exists ".bob/objects/$prefix/$suffix" "empty blob persisted"
end_test

summary
