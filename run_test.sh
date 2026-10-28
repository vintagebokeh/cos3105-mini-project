
#!/usr/bin/env bash
set -euo pipefail

TEST_NAME="${1:-test_models}"
TEST_NAME="${TEST_NAME%.c}"

REPO_DIR="$PWD"
TEST_DIR="$(mktemp -d)"
trap 'rm -rf "$TEST_DIR"' EXIT

echo "Compiling $TEST_NAME..."

gcc -Wall -Wextra -I./src \
    "src/tests/${TEST_NAME}.c" \
    src/models.c src/common.c \
    -o "$TEST_DIR/$TEST_NAME"

echo "Running tests in isolated directory..."

(
    cd "$TEST_DIR"
    "./$TEST_NAME"
)

echo "TEST COMPLETED"
