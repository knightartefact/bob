#!/usr/bin/env bash
source "$(dirname "$0")/lib.sh"

print_header "graph"

start_test "graph prints linear history with latest on top"
new_repo graph_linear
echo a > f.txt
"$BOB" add f.txt >/dev/null
"$BOB" commit -m "first" >/dev/null
echo b > f.txt
"$BOB" add f.txt >/dev/null
"$BOB" commit -m "second" >/dev/null
echo c > f.txt
"$BOB" add f.txt >/dev/null
"$BOB" commit -m "third" >/dev/null
out="$("$BOB" graph)"
# Order: third, second, first (latest first).
first_msg_line="$(printf '%s\n' "$out" | grep '^\*' | head -1)"
last_msg_line="$(printf '%s\n' "$out" | grep '^\*' | tail -1)"
assert_contains "$first_msg_line" "third" "latest commit on top"
assert_contains "$last_msg_line" "first" "earliest commit at bottom"
# All three commits present.
commit_count="$(printf '%s\n' "$out" | grep -c '^\*')"
assert_eq "$commit_count" "3" "three commit lines"
# Connector lines between commits.
pipe_count="$(printf '%s\n' "$out" | grep -c '^|$')"
assert_eq "$pipe_count" "2" "two connectors between three commits"
end_test

start_test "graph annotates merge commits"
new_repo graph_merge
echo a > f.txt
"$BOB" add f.txt >/dev/null
"$BOB" commit -m "base" >/dev/null
"$BOB" branch feature >/dev/null
"$BOB" checkout feature >/dev/null
echo b > g.txt
"$BOB" add g.txt >/dev/null
"$BOB" commit -m "feature work" >/dev/null
"$BOB" checkout main >/dev/null
echo c > h.txt
"$BOB" add h.txt >/dev/null
"$BOB" commit -m "main work" >/dev/null
"$BOB" merge feature >/dev/null
out="$("$BOB" graph)"
top_line="$(printf '%s\n' "$out" | grep '^\*' | head -1)"
assert_contains "$top_line" "(merge)" "merge commit tagged"
assert_contains "$top_line" "Merge branch" "merge commit subject"
# Walk follows first parent only — 'feature work' commit must NOT appear.
assert_not_contains "$out" "feature work" "second-parent commits not traversed"
assert_contains "$out" "main work" "first-parent line still visible"
assert_contains "$out" "base" "root commit still visible"
end_test

start_test "graph on a single-commit repo prints one line, no connector"
new_repo graph_single
echo a > f.txt
"$BOB" add f.txt >/dev/null
"$BOB" commit -m "only" >/dev/null
out="$("$BOB" graph)"
commit_count="$(printf '%s\n' "$out" | grep -c '^\*')"
assert_eq "$commit_count" "1" "one commit line"
pipe_count="$(printf '%s\n' "$out" | grep -c '^|$')"
assert_eq "$pipe_count" "0" "no connectors when only one commit"
end_test

start_test "graph on empty repo reports no commits"
new_repo graph_empty
out="$("$BOB" graph 2>&1)"
rc=$?
assert_nonzero "$rc" "graph before any commit fails"
assert_contains "$out" "no commits yet" "diagnostic message"
end_test

summary
