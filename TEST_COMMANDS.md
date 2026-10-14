# COS3105 Server Core — Terminal/API Test Commands

## Important

This mini project is a **raw TCP protocol**, not HTTP.
Do **not** use `curl` for these tests. `curl` sends HTTP such as `GET / HTTP/1.1`, which is why the browser/forwarded port previously produced HTTP traffic and 502 behavior.

The equivalent manual client for this project is `nc` (netcat).

Open two terminals:

- Terminal A: run the server
- Terminal B: send client requests

## Terminal A

```bash
./run_server.sh
```

Expected startup:

```text
Configuring local address...
Creating socket...
Binding socket to local address...
Binding to port: 3030
Listening...
Waiting for connections...
```

## Terminal B — Board commands

### 1. List boards

```bash
printf 'LIST_BOARD\n' | nc -N 127.0.0.1 3030
```

### 2. Add board

```bash
printf 'ADD_BOARD,Board 1\n' | nc -N 127.0.0.1 3030
```

### 3. Add another board

```bash
printf 'ADD_BOARD,Board 2\n' | nc -N 127.0.0.1 3030
```

### 4. Update board 1

```bash
printf 'UPDATE_BOARD,1,Board 1 Updated\n' | nc -N 127.0.0.1 3030
```

### 5. Delete board 2

```bash
printf 'DELETE_BOARD,2\n' | nc -N 127.0.0.1 3030
```

## Terminal B — Task commands

### 6. List tasks in board 1

```bash
printf 'LIST_TASK,1\n' | nc -N 127.0.0.1 3030
```

### 7. Add task to board 1

```bash
printf 'ADD_TASK,1,Task 1\n' | nc -N 127.0.0.1 3030
```

### 8. Add another task

```bash
printf 'ADD_TASK,1,Task 2\n' | nc -N 127.0.0.1 3030
```

### 9. Update task 1 in board 1

```bash
printf 'UPDATE_TASK,1,1,Task 1 Updated\n' | nc -N 127.0.0.1 3030
```

### 10. Delete task 2 in board 1

```bash
printf 'DELETE_TASK,1,2\n' | nc -N 127.0.0.1 3030
```

## Error-path checks

### Board not found

```bash
printf 'LIST_TASK,999\n' | nc -N 127.0.0.1 3030
```

Expected header:

```text
LIST_TASK,201,0
```

### Task not found

```bash
printf 'UPDATE_TASK,1,999,No such task\n' | nc -N 127.0.0.1 3030
```

Expected:

```text
UPDATE_TASK,203
```

### Unknown command

```bash
printf 'HELLO\n' | nc -N 127.0.0.1 3030
```

Expected:

```text
UNKNOWN,3
```

## Client Core (C, before TUI)

Compile once:

```bash
./run_client.sh
```

Then the same protocol can be sent with the C client:

```bash
./client LIST_BOARD
./client ADD_BOARD "Board 1"
./client UPDATE_BOARD 1 "Board 1 Updated"
./client DELETE_BOARD 1

./client LIST_TASK 1
./client ADD_TASK 1 "Task 1"
./client UPDATE_TASK 1 1 "Task 1 Updated"
./client DELETE_TASK 1 1
```

This `client.c` is intentionally non-TUI. The next phase can place a TUI menu above this client/network layer.

## No ADD_PROJECT command in this phase

The teacher's current model contains one root `Project` object that owns Boards; the requirement does not define Create/Update/Delete Project commands. Therefore this draft does not invent an `ADD_PROJECT` command.
