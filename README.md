# COS3105 Mini Project — Server Core Study Draft

This directory is a reviewable draft built to follow the teacher's existing architecture and complete only the stated Board/Task requirements before TUI work.

Read in this order:

1. `CHANGELOG_SERVER_CORE.md` — design and reasons
2. `src/protocol.h` — protocol vocabulary/status
3. `src/models.h` / `src/models.c` — data operations
4. `src/server.h` / `src/server.c` — server workflow/handlers
5. `src/tests/test_models.c` — Model unit tests
6. `TEST_COMMANDS.md` — netcat + C client tests
7. `client.c` — minimal client core before TUI

Build/test:

```bash
./run_test.sh test_models
./run_server.sh
```

In another terminal:

```bash
./run_client.sh
./client LIST_BOARD
```
