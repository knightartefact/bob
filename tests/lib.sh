#!/usr/bin/env bash
# Shared helpers for bob end-to-end tests.
# Each test script sources this file, calls start_test/end_test pairs, and
# uses assert_* helpers in between. Scratch repos are created under /tmp and
# automatically removed via an EXIT trap.

set -u

BOB_TEST_LIB_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BOB="${BOB:-$BOB_TEST_LIB_DIR/../build/bob}"
if [[ ! -x "$BOB" ]]; then
    echo "FATAL: bob binary not found at $BOB (build it first)" >&2
    exit 2
fi

TESTS_RUN=0
TESTS_PASSED=0
TESTS_FAILED=0
CURRENT_TEST=""
CURRENT_FAILED=0
BOB_TEST_DIRS=()

if [[ -t 1 ]]; then
    C_RED=$'\033[31m'
    C_GREEN=$'\033[32m'
    C_BOLD=$'\033[1m'
    C_RESET=$'\033[0m'
else
    C_RED=""
    C_GREEN=""
    C_BOLD=""
    C_RESET=""
fi

start_test() {
    CURRENT_TEST="$1"
    CURRENT_FAILED=0
    TESTS_RUN=$((TESTS_RUN + 1))
}

end_test() {
    if [[ "$CURRENT_FAILED" -eq 0 ]]; then
        TESTS_PASSED=$((TESTS_PASSED + 1))
        echo "  ${C_GREEN}PASS${C_RESET} $CURRENT_TEST"
    else
        TESTS_FAILED=$((TESTS_FAILED + 1))
    fi
}

fail() {
    CURRENT_FAILED=1
    echo "  ${C_RED}FAIL${C_RESET} $CURRENT_TEST: $*" >&2
}

assert_eq() {
    if [[ "$1" != "$2" ]]; then
        fail "${3:-equality} expected '$2' got '$1'"
    fi
}

assert_contains() {
    if [[ "$1" != *"$2"* ]]; then
        fail "${3:-contains} expected to contain '$2', got '$1'"
    fi
}

assert_not_contains() {
    if [[ "$1" == *"$2"* ]]; then
        fail "${3:-not contains} expected not to contain '$2', got '$1'"
    fi
}

assert_file_exists() {
    if [[ ! -e "$1" ]]; then
        fail "${2:-file missing}: $1"
    fi
}

assert_file_absent() {
    if [[ -e "$1" ]]; then
        fail "${2:-file should be absent}: $1"
    fi
}

assert_file_eq() {
    local path="$1" expected="$2" msg="${3:-file contents}"
    if [[ ! -f "$path" ]]; then
        fail "$msg: file missing: $path"
        return
    fi
    local actual
    actual="$(cat "$path")"
    if [[ "$actual" != "$expected" ]]; then
        fail "$msg: contents mismatch for $path"
    fi
}

assert_zero() {
    if [[ "$1" -ne 0 ]]; then
        fail "${2:-expected exit 0} got $1"
    fi
}

assert_nonzero() {
    if [[ "$1" -eq 0 ]]; then
        fail "${2:-expected non-zero exit} got 0"
    fi
}

new_repo() {
    local label="${1:-repo}"
    local dir
    dir="$(mktemp -d "/tmp/bob_test_${label}_XXXXXX")"
    BOB_TEST_DIRS+=("$dir")
    cd "$dir"
    "$BOB" init >/dev/null
    printf 'name=tester\nemail=tester@example.com\n' > .bob/config
}

cleanup_repos() {
    cd /
    for d in "${BOB_TEST_DIRS[@]:-}"; do
        [[ -n "${d:-}" && -d "$d" ]] && rm -rf "$d"
    done
}

trap cleanup_repos EXIT

print_header() {
    echo "${C_BOLD}=== $1 ===${C_RESET}"
}

summary() {
    echo
    if [[ "$TESTS_FAILED" -eq 0 ]]; then
        echo "${C_GREEN}${TESTS_PASSED}/${TESTS_RUN} passed${C_RESET}"
        exit 0
    fi
    echo "${C_RED}${TESTS_FAILED}/${TESTS_RUN} failed${C_RESET}"
    exit 1
}
