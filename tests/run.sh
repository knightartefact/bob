#!/usr/bin/env bash
# Run all bob end-to-end tests sequentially and report a summary.
# Each test_*.sh file is a standalone script: it exits 0 if all its
# scenarios passed, non-zero otherwise. We just aggregate exit codes.

set -u

TESTS_DIR="$(cd "$(dirname "$0")" && pwd)"

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

total_files=0
failed_files=0
failed_names=()

for script in "$TESTS_DIR"/test_*.sh; do
    [[ -f "$script" ]] || continue
    name="$(basename "$script")"
    total_files=$((total_files + 1))
    bash "$script"
    rc=$?
    if [[ "$rc" -ne 0 ]]; then
        failed_files=$((failed_files + 1))
        failed_names+=("$name")
    fi
    echo
done

echo "${C_BOLD}===== summary =====${C_RESET}"
if [[ "$failed_files" -eq 0 ]]; then
    echo "${C_GREEN}all $total_files test files passed${C_RESET}"
    exit 0
fi
echo "${C_RED}$failed_files of $total_files test files failed:${C_RESET}"
for n in "${failed_names[@]}"; do
    echo "  - $n"
done
exit 1
