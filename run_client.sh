#!/usr/bin/env bash

# Compile Client Core (ยังไม่ใช่ TUI)
gcc -Wall -Wextra client.c -o client

if [ $? -eq 0 ]; then
    echo "Client compiled: ./client"
fi
