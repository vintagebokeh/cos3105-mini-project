#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <limits.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "src/models.h"   /* ขนาดของ ID, Title และ MAX_BOARD/MAX_TASK ต้องตรงกับ Server */
#include "src/protocol.h" /* คำสั่งและรหัสสถานะที่ Client/Server ใช้ร่วมกัน */

#define SERVER_HOST "127.0.0.1"
#define SERVER_PORT "3030"
#define REQUEST_SIZE 2048
#define RESPONSE_SIZE (128 * 1024)

typedef struct {
    char id[BOARDID_SIZE];
    char title[BOARD_TITLE_SIZE];
} BoardView;

typedef struct {
    char id[TASKID_SIZE];
    char title[TASK_TITLE_SIZE];
    int isFinish;
} TaskView;

typedef struct {
    BoardView entries[MAX_BOARD];
    int count;
} BoardList;

typedef struct {
    TaskView entries[MAX_TASK];
    int count;
} TaskList;

/* ================================================================
 * WORKFLOW 1: อ่านค่าจากคีย์บอร์ด (Input Helpers)
 * ใช้ fgets() เพื่อไม่ให้ตัวอักษรที่พิมพ์ผิดค้างใน stdin แบบ scanf()
 * ================================================================ */
static int readText(const char *prompt, char *out, size_t capacity)
{
    printf("%s", prompt);
    fflush(stdout);
    if (!fgets(out, (int)capacity, stdin))
        return -1; /* EOF: เช่น ผู้ใช้กด Ctrl+D */

    size_t len = strlen(out);
    if (len && out[len - 1] == '\n') {
        out[len - 1] = '\0';
    } else if (!feof(stdin)) {
        /* บรรทัดยาวเกิน buffer: ล้างส่วนที่เหลือ ไม่ส่งข้อมูลที่ถูกตัดไป Server */
        int ch;
        while ((ch = getchar()) != '\n' && ch != EOF) { }
        puts("Input is too long.");
        return 0;
    }

    /* Protocol ใช้ comma เป็นตัวแบ่ง จึงไม่รับ comma / newline ใน Title */
    if (strchr(out, ',') || strchr(out, '\r')) {
        puts("Comma and newline are not allowed in names.");
        return 0;
    }
    int any = 0;
    for (size_t i = 0; out[i]; ++i)
        if (out[i] != ' ' && out[i] != '\t') { any = 1; break; }
    if (!any) {
        puts("Input cannot be empty.");
        return 0;
    }
    return 1;
}

static int readMenuChoice(int maxOption)
{
    char line[64];
    for (;;) {
        printf("Choose [0-%d]: ", maxOption);
        fflush(stdout);
        if (!fgets(line, sizeof(line), stdin))
            return -1;
        if (!strchr(line, '\n') && !feof(stdin)) {
            int ch;
            while ((ch = getchar()) != '\n' && ch != EOF) { }
            puts("Invalid menu number.");
            continue;
        }
        char *end;
        errno = 0;
        long value = strtol(line, &end, 10);
        while (*end == ' ' || *end == '\t') ++end;
        if (errno == 0 && end != line && (*end == '\n' || *end == '\0') &&
            value >= 0 && value <= maxOption)
            return (int)value;
        puts("Invalid menu number. Please try again.");
    }
}

static int readName(const char *prompt, char *out, size_t capacity)
{
    for (;;) {
        int ok = readText(prompt, out, capacity);
        if (ok != 0) return ok;
    }
}

static int confirmDelete(void)
{
    char answer[16];
    printf("Confirm delete? [y/N]: ");
    fflush(stdout);
    if (!fgets(answer, sizeof(answer), stdin)) return 0;
    return answer[0] == 'y' || answer[0] == 'Y';
}

/* ================================================================
 * WORKFLOW 2: TCP หนึ่งคำสั่งต่อหนึ่ง Connection
 * 2.1 Connect -> 2.2 Send All -> 2.3 shutdown(write)
 * 2.4 Receive จน Server ปิด -> 2.5 Close
 * สำคัญ: TCP recv() ไม่รับประกันว่าจะได้ข้อความทั้งบรรทัดในครั้งเดียว
 * ================================================================ */
static int sendRequest(const char *request, char *reply, size_t replySize)
{
    struct addrinfo hints, *addresses = NULL, *addr;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    int rc = getaddrinfo(SERVER_HOST, SERVER_PORT, &hints, &addresses);
    if (rc != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rc));
        return -1;
    }

    int fd = -1;
    for (addr = addresses; addr; addr = addr->ai_next) {
        fd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
        if (fd < 0) continue;
        if (connect(fd, addr->ai_addr, addr->ai_addrlen) == 0) break;
        close(fd);
        fd = -1;
    }
    freeaddrinfo(addresses);
    if (fd < 0) {
        perror("connect");
        return -1;
    }

    struct timeval timeout = { .tv_sec = 5, .tv_usec = 0 };
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    size_t length = strlen(request), sent = 0;
    while (sent < length) {
        ssize_t n = send(fd, request + sent, length - sent, 0);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) { perror("send"); close(fd); return -1; }
        sent += (size_t)n;
    }
    shutdown(fd, SHUT_WR);

    size_t received = 0;
    while (received + 1 < replySize) {
        ssize_t n = recv(fd, reply + received, replySize - received - 1, 0);
        if (n < 0 && errno == EINTR) continue;
        if (n < 0) { perror("recv"); close(fd); return -1; }
        if (n == 0) break;
        received += (size_t)n;
    }
    reply[received] = '\0';
    close(fd);
    if (received == 0 || received + 1 >= replySize) {
        fprintf(stderr, "Empty or oversized server response.\n");
        return -1;
    }
    return 0;
}

/* 2.6 สร้าง Protocol จาก Command และ Arguments โดยไม่ให้เกิน Buffer */
static int requestCommand(const char *command, const char *arg1, const char *arg2,
                          const char *arg3, char *reply, size_t replySize)
{
    char request[REQUEST_SIZE];
    int n = snprintf(request, sizeof(request), "%s", command);
    if (n < 0 || (size_t)n >= sizeof(request)) return -1;
    const char *args[] = {arg1, arg2, arg3};
    for (int i = 0; i < 3; ++i) {
        if (!args[i]) continue;
        size_t used = strlen(request);
        n = snprintf(request + used, sizeof(request) - used, ",%s", args[i]);
        if (n < 0 || (size_t)n >= sizeof(request) - used) return -1;
    }
    size_t used = strlen(request);
    if (used + 2 > sizeof(request)) return -1;
    request[used++] = '\n';
    request[used] = '\0';
    return sendRequest(request, reply, replySize);
}

/* ================================================================
 * WORKFLOW 3: แปลง Response ของ LIST_BOARD / LIST_TASK เป็น Array
 * Header: LIST_BOARD,status,count หรือ LIST_TASK,status,count
 * Records: BOARD,id,title / TASK,id,title,isFinish ตามด้วย END
 * ห้ามใช้ Array Index แทน ID ที่ Server กำหนด
 * ================================================================ */
static int parseNumber(const char *text, int *value)
{
    char *end;
    errno = 0;
    long n = strtol(text, &end, 10);
    if (errno || end == text || *end || n < 0 || n > INT_MAX) return -1;
    *value = (int)n;
    return 0;
}

static int parseListHeader(char *line, const char *expected, int maxCount,
                           int *status, int *count)
{
    char *save = NULL;
    char *command = strtok_r(line, ",", &save);
    char *statusText = strtok_r(NULL, ",", &save);
    char *countText = strtok_r(NULL, ",", &save);
    if (!command || !statusText || !countText || strcmp(command, expected) != 0 ||
        parseNumber(statusText, status) != 0 || parseNumber(countText, count) != 0 ||
        *count > maxCount) return -1;
    return 0;
}

static int parseBoards(char *reply, BoardList *boards)
{
    boards->count = 0;
    char *save = NULL;
    char *line = strtok_r(reply, "\n", &save);
    if (!line) return -1;
    int status = -1, count = 0;
    if (parseListHeader(line, COMMAND_LIST_BOARD, MAX_BOARD, &status, &count) != 0)
        return -1;
    if (status == STATUS_EMPTY_BOARD && count == 0) return 0;
    if (status != STATUS_OK) return -1;
    for (int i = 0; i < count; ++i) {
        line = strtok_r(NULL, "\n", &save);
        if (!line || strncmp(line, RECORD_BOARD ",", 6) != 0) return -1;
        char *id = line + 6;
        char *comma = strchr(id, ',');
        if (!comma) return -1;
        *comma++ = '\0';
        if (!*id || strlen(id) >= BOARDID_SIZE || strlen(comma) >= BOARD_TITLE_SIZE)
            return -1;
        snprintf(boards->entries[i].id, BOARDID_SIZE, "%s", id);
        snprintf(boards->entries[i].title, BOARD_TITLE_SIZE, "%s", comma);
        ++boards->count;
    }
    line = strtok_r(NULL, "\n", &save);
    return (line && strcmp(line, RESPONSE_END) == 0) ? 0 : -1;
}

static int parseTasks(char *reply, TaskList *tasks)
{
    tasks->count = 0;
    char *save = NULL;
    char *line = strtok_r(reply, "\n", &save);
    if (!line) return -1;
    int status = -1, count = 0;
    if (parseListHeader(line, COMMAND_LIST_TASK, MAX_TASK, &status, &count) != 0)
        return -1;
    if (status == STATUS_EMPTY_TASK && count == 0) return 0;
    if (status != STATUS_OK) return -1;
    for (int i = 0; i < count; ++i) {
        line = strtok_r(NULL, "\n", &save);
        if (!line || strncmp(line, RECORD_TASK ",", 5) != 0) return -1;
        char *id = line + 5;
        char *comma = strchr(id, ',');
        if (!comma) return -1;
        *comma++ = '\0';
        char *lastComma = strrchr(comma, ',');
        if (!lastComma) return -1;
        *lastComma++ = '\0';
        int finished = 0;
        if (!*id || strlen(id) >= TASKID_SIZE || strlen(comma) >= TASK_TITLE_SIZE ||
            parseNumber(lastComma, &finished) != 0) return -1;
        snprintf(tasks->entries[i].id, TASKID_SIZE, "%s", id);
        snprintf(tasks->entries[i].title, TASK_TITLE_SIZE, "%s", comma);
        tasks->entries[i].isFinish = finished;
        ++tasks->count;
    }
    line = strtok_r(NULL, "\n", &save);
    return (line && strcmp(line, RESPONSE_END) == 0) ? 0 : -1;
}

/* ใช้ Buffer แยกแต่ละครั้ง เพราะ parser แก้ String ด้วย strtok_r() */
static int fetchBoards(BoardList *boards)
{
    static char reply[RESPONSE_SIZE];
    if (requestCommand(COMMAND_LIST_BOARD, NULL, NULL, NULL,
                       reply, sizeof(reply)) != 0) return -1;
    if (parseBoards(reply, boards) != 0) {
        puts("Invalid LIST_BOARD response from server.");
        return -1;
    }
    return 0;
}

static int fetchTasks(const char *boardId, TaskList *tasks)
{
    static char reply[RESPONSE_SIZE];
    if (requestCommand(COMMAND_LIST_TASK, boardId, NULL, NULL,
                       reply, sizeof(reply)) != 0) return -1;
    if (parseTasks(reply, tasks) != 0) {
        puts("Cannot load tasks (board may have been deleted).");
        return -1;
    }
    return 0;
}

static void showBoards(const BoardList *boards)
{
    printf("\nBOARDS (%d)\n------------------------------\n", boards->count);
    if (!boards->count) puts("(No boards)");
    for (int i = 0; i < boards->count; ++i)
        printf("%d. [%s] %s\n", i + 1, boards->entries[i].id,
               boards->entries[i].title);
}

static void showTasks(const TaskList *tasks)
{
    printf("\nTASKS (%d)\n------------------------------\n", tasks->count);
    if (!tasks->count) puts("(No tasks)");
    for (int i = 0; i < tasks->count; ++i)
        printf("%d. [%s] %s %s\n", i + 1, tasks->entries[i].id,
               tasks->entries[i].title, tasks->entries[i].isFinish ? "[DONE]" : "");
}

static int chooseBoard(const BoardList *boards)
{
    if (!boards->count) { puts("No boards. Please add a board first."); return -1; }
    showBoards(boards);
    puts("0. Back");
    int index = readMenuChoice(boards->count);
    return index <= 0 ? -1 : index - 1;
}

static int chooseTask(const TaskList *tasks)
{
    if (!tasks->count) { puts("No tasks. Please add a task first."); return -1; }
    showTasks(tasks);
    puts("0. Back");
    int index = readMenuChoice(tasks->count);
    return index <= 0 ? -1 : index - 1;
}

/* WORKFLOW 4: เปลี่ยนข้อมูล -> Server ตรวจแล้ว Save -> Client แสดง Status */
static void executeChange(const char *command, const char *arg1,
                          const char *arg2, const char *arg3)
{
    char response[4096];
    if (requestCommand(command, arg1, arg2, arg3,
                       response, sizeof(response)) != 0) {
        puts("Request failed. Check server connection.");
        return;
    }
    printf("Server: %s", response);
    if (!strchr(response, '\n')) putchar('\n');
}

/* ================================================================
 * WORKFLOW 5: TASK MENU - ต้องเลือก Board ก่อน จึงมี boardId จริง
 * เมนูเลขคงที่ แต่ Update/Delete Disabled หากไม่มี Task
 * ================================================================ */
static void taskMenu(const BoardView *selected)
{
    for (;;) {
        TaskList tasks;
        if (fetchTasks(selected->id, &tasks) != 0) return;
        printf("\n=== TASK MENU: %s [Board ID %s] ===\n", selected->title, selected->id);
        printf("1. List Tasks (%d)\n", tasks.count);
        puts("2. Add Task");
        printf("3. Update Task %s\n", tasks.count ? "" : "[Disabled]");
        printf("4. Delete Task %s\n", tasks.count ? "" : "[Disabled]");
        puts("0. Back to Board Menu");
        int choice = readMenuChoice(4);
        if (choice <= 0) return;
        if (choice == 1) { showTasks(&tasks); continue; }
        if (choice >= 3 && tasks.count == 0) {
            puts("No tasks available for this command.");
            continue;
        }
        char name[TASK_TITLE_SIZE];
        if (choice == 2) {
            if (readName("Task title: ", name, sizeof(name)) < 0) return;
            executeChange(COMMAND_ADD_TASK, selected->id, name, NULL);
        } else {
            int index = chooseTask(&tasks);
            if (index < 0) continue;
            const char *taskId = tasks.entries[index].id;
            if (choice == 3) {
                if (readName("New task title: ", name, sizeof(name)) < 0) return;
                executeChange(COMMAND_UPDATE_TASK, selected->id, taskId, name);
            } else if (choice == 4 && confirmDelete()) {
                executeChange(COMMAND_DELETE_TASK, selected->id, taskId, NULL);
            }
        }
    }
}

/* WORKFLOW 6: BOARD MENU - แสดงจำนวนจริงที่ Server ส่งทุกครั้ง */
static int boardMenu(void)
{
    for (;;) {
        BoardList boards;
        if (fetchBoards(&boards) != 0) {
            puts("Cannot read boards. Is the server running on port 3030?");
            return 1;
        }
        puts("\n=== COS3105 MINI PROJECT ===");
        printf("1. List Boards (%d)\n", boards.count);
        puts("2. Add Board");
        printf("3. Update Board %s\n", boards.count ? "" : "[Disabled]");
        printf("4. Delete Board %s\n", boards.count ? "" : "[Disabled]");
        printf("5. Select Board %s\n", boards.count ? "" : "[Disabled]");
        puts("0. Exit");
        int choice = readMenuChoice(5);
        if (choice <= 0) return 0;
        if (choice == 1) { showBoards(&boards); continue; }
        if (choice >= 3 && boards.count == 0) {
            puts("No boards available for this command.");
            continue;
        }
        char name[BOARD_TITLE_SIZE];
        if (choice == 2) {
            if (readName("Board title: ", name, sizeof(name)) < 0) return 0;
            executeChange(COMMAND_ADD_BOARD, name, NULL, NULL);
        } else {
            int index = chooseBoard(&boards);
            if (index < 0) continue;
            const char *boardId = boards.entries[index].id;
            if (choice == 3) {
                if (readName("New board title: ", name, sizeof(name)) < 0) return 0;
                executeChange(COMMAND_UPDATE_BOARD, boardId, name, NULL);
            } else if (choice == 4 && confirmDelete()) {
                executeChange(COMMAND_DELETE_BOARD, boardId, NULL, NULL);
            } else if (choice == 5) {
                taskMenu(&boards.entries[index]);
            }
        }
    }
}

/* WORKFLOW 7: คงโหมด CLI เดิมไว้ใช้กับ Smoke Test และ Netcat เปรียบเทียบ */
static int commandLineMode(int argc, char *argv[])
{
    char request[REQUEST_SIZE] = "";
    for (int i = 1; i < argc; ++i) {
        if (strpbrk(argv[i], ",\r\n")) {
            fprintf(stderr, "Arguments cannot contain comma or newline.\n");
            return 1;
        }
        size_t used = strlen(request);
        int n = snprintf(request + used, sizeof(request) - used,
                         "%s%s", i == 1 ? "" : ",", argv[i]);
        if (n < 0 || (size_t)n >= sizeof(request) - used) {
            fprintf(stderr, "Request too long.\n");
            return 1;
        }
    }
    size_t len = strlen(request);
    if (len + 2 > sizeof(request)) return 1;
    request[len] = '\n';
    request[len + 1] = '\0';
    static char response[RESPONSE_SIZE];
    if (sendRequest(request, response, sizeof(response)) != 0) return 1;
    fputs(response, stdout);
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc > 1) return commandLineMode(argc, argv);
    return boardMenu();
}