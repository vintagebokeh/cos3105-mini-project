# COS3105 Mini Project — Server Core Draft

> Study draft for review/presentation before merging into the user's branch.
> Baseline structure follows the teacher's existing `protocol.h`, `models.*`, `server.*`, `common.*` design.
> Added code is marked with `[ADDED]`; bug fixes required for reliable operation are marked `[FIX]`.

## Scope locked to teacher requirements

Board:
- ADD_BOARD (existing)
- LIST_BOARD (added)
- UPDATE_BOARD (added)
- DELETE_BOARD (added)

Task:
- ADD_TASK (existing)
- LIST_TASK by boardId (added)
- UPDATE_TASK by boardId + taskId (added)
- DELETE_TASK by boardId + taskId (added)

Not added yet:
- ADD_PROJECT / multi-project management
- GET_BOARD / GET_TASK
- soft delete / PACK
- PING / STATS
- TUI
- taskContent split
- task status command

Reason: these are outside the teacher's stated requirements for the current phase.

## Protocol design

### LIST_BOARD

Request:

```text
LIST_BOARD
```

Success:

```text
LIST_BOARD,0,<count>
BOARD,<boardId>,<boardTitle>
...
END
```

Empty:

```text
LIST_BOARD,202,0
END
```

### LIST_TASK

Request:

```text
LIST_TASK,<boardId>
```

Success:

```text
LIST_TASK,0,<count>
TASK,<taskId>,<taskTitle>
...
END
```

Board not found:

```text
LIST_TASK,201,0
END
```

Empty board:

```text
LIST_TASK,204,0
END
```

### UPDATE_BOARD

```text
UPDATE_BOARD,<boardId>,<newBoardTitle>
```

Success:

```text
UPDATE_BOARD,0
```

### DELETE_BOARD

```text
DELETE_BOARD,<boardId>
```

Success:

```text
DELETE_BOARD,0
```

### UPDATE_TASK

```text
UPDATE_TASK,<boardId>,<taskId>,<newTaskTitle>
```

Success:

```text
UPDATE_TASK,0
```

### DELETE_TASK

```text
DELETE_TASK,<boardId>,<taskId>
```

Success:

```text
DELETE_TASK,0
```

## Why LIST does not add a Model function

The existing model already stores:

```text
Project
├─ totalBoards
└─ boards[]
   ├─ boardId
   ├─ boardTitle
   ├─ totalTasks
   └─ tasks[]
```

`LIST_BOARD` therefore reads a snapshot from `project->totalBoards` and `project->boards[]`.
`LIST_TASK` finds the selected Board using the existing `hasBoard()` and then reads `totalTasks` and `tasks[]`.

The Server handler owns the protocol formatting; Model functions are added only where a data operation is actually missing (update/delete).

## Shared memory and synchronization

The teacher's `setup()` creates the Project with `mmap(... MAP_SHARED | MAP_ANONYMOUS ...)`, then calls `loadProject()` before `accept()`.

Therefore:
- Board/Task data in `Project` is shared across forked children.
- `ADD`, `UPDATE`, and `DELETE` modify shared state and must be inside `sem_wait()` / `sem_post()`.
- `LIST` also locks while taking a consistent snapshot, but releases the semaphore before network `send()`.

## Required fixes included

1. `recv()` now reserves one byte and writes `read[bytes_received] = '\0'` before treating the buffer as a C string.
2. `saveBoard()` writes only `project->totalBoards`, not all 100 array slots.
3. File read buffers were enlarged to support the model's 500-character title fields.
4. After physical delete, new IDs use max(existing ID) + 1 so IDs do not collide.

## Delete semantics

This draft implements **physical delete**, because the teacher requirement says Delete but does not specify soft delete/mark/pack.

- `DELETE_TASK` removes the Task from the in-memory array, shifts remaining tasks, decrements `totalTasks`, and rewrites `task.txt`.
- `DELETE_BOARD` removes the Board (including its nested Tasks), shifts remaining boards, decrements `totalBoards`, and rewrites both `board.txt` and `task.txt`.

Soft delete / PACK is intentionally deferred.
