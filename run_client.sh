#!/usr/bin/env bash
set -euo pipefail

# ขั้นตอน 1: เข้าโฟลเดอร์โปรเจกต์ เพื่อให้ Compile ได้จากทุกตำแหน่ง
cd "$(dirname "$0")"

# ขั้นตอน 2: Compile Client ด้วย GCC (Build)
gcc -Wall -Wextra client.c -o client

# ขั้นตอน 3: รัน Client (Run)
# ไม่ส่ง Argument = เปิด Board/Task Menu
# ส่ง Argument เช่น LIST_BOARD = รันคำสั่ง TCP ครั้งเดียว
exec ./client "$@"
