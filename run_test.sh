
#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")"

TEST_NAME="${1:-test_models}"
TEST_NAME="${TEST_NAME%.c}"

TEST_DIR="$PWD/model_test_run"
mkdir -p "$TEST_DIR"

echo "Compiling $TEST_NAME..."

gcc -Wall -Wextra -I./src \
    "src/tests/${TEST_NAME}.c" \
    src/models.c src/common.c \
    -o "$TEST_DIR/$TEST_NAME"

echo "Running Model Tests..."

(
    cd "$TEST_DIR"
    "./$TEST_NAME"
)

echo "TEST COMPLETED"
