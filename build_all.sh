
#!/usr/bin/env bash
set -euo pipefail

echo "=== Build COS3105 ==="

echo "[1/2] Compiling Server..."
gcc -Wall -Wextra -I./src \
    src/server.c src/models.c src/common.c \
    -o run_server -pthread -lrt

echo "[2/2] Compiling Client..."
gcc -Wall -Wextra client.c -o client

echo "=== BUILD SUCCESS ==="
