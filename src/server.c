#include "server.h"

#include "server.h"
#include <signal.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <fcntl.h>    /* For O_CREAT, O_RDWR constants */
#include <sys/stat.h> /* For mode constants like 0644 */

SOCKET socket_listen = -1;
Project *project = NULL;
sem_t *sem = NULL;

/*
 * Helper เล็ก ๆ สำหรับส่ง response หนึ่งบรรทัด
 * ยังใช้ send() ตามแนวทางเดิมของอาจารย์ ไม่เพิ่ม protocol layer ใหม่เกิน requirement
 */
//static void sendLine(SOCKET clientSocket, const char *text)
//{
//  send(clientSocket, text, strlen(text), 0);              // ส่งข้อความตามความยาว string
//}

static int sendLine(SOCKET clientSocket, const char *text)
{
    size_t length = strlen(text);
    size_t sent = 0;

    while (sent < length)
    {
        ssize_t n = send(clientSocket,
                         text + sent,
                         length - sent,
                         MSG_NOSIGNAL);

        if (n < 0 && errno == EINTR)
            continue;

        if (n <= 0)
            return -1;

        sent += (size_t)n;
    }

    return 0;
}

/* ================================================================
 * BOARD: ADD_BOARD
 * baseline ของอาจารย์ + แยก status ของ add/save ให้ชัดขึ้น
 * ================================================================ */

void handleAddBoard(CommandContext *ctx)
{
  char *boardTitle = strtok_r(NULL, COMMAND_SEPARATOR, &ctx->rawInput);

  char response[MESSAGE_SIZE];

  if (isEmptyOrSpace(boardTitle))
  {
    snprintf(response, sizeof(response), "%s%s%d\n", COMMAND_ADD_BOARD, COMMAND_SEPARATOR, STATUS_INVALID_ARGUMENTS);
    send(ctx->clientSocket, response, strlen(response), 0);
  }
  else
  {
    // 1. Wait for the lock (Block if another process is buying)
    sem_wait(sem);

    // 2. CRITICAL SECTION: Only one process can be here at a time
    int result = addBoard(ctx->project, boardTitle);
    int saveResult = 0;                                                   // เตรียมผลการ save

    saveBoard(ctx->project, BOARD_FILENAME);

    // 3. Release the lock
    sem_post(sem);
    if (result == 0)
    {
      snprintf(response, sizeof(response), "%s%s%d\n", COMMAND_ADD_BOARD, COMMAND_SEPARATOR, STATUS_OK);
      send(ctx->clientSocket, response, strlen(response), 0);
    }
    else
    {
      snprintf(response, sizeof(response), "%s%s%d\n", COMMAND_ADD_BOARD, COMMAND_SEPARATOR, STATUS_FAIL);
      send(ctx->clientSocket, response, strlen(response), 0);
    }
  }
}


void handleListBoard(CommandContext *ctx)
{
    char ids[MAX_BOARD][BOARDID_SIZE];
    char titles[MAX_BOARD][BOARD_TITLE_SIZE];
    char response[MESSAGE_SIZE];
    int count;

    // 1. Lock ก่อนอ่าน Shared Memory
    if (sem_wait(sem) == -1)
    {
        snprintf(response, sizeof(response),
                 "%s,%d,0\nEND\n",
                 COMMAND_LIST_BOARD, STATUS_FAIL);
        sendLine(ctx->clientSocket, response);
        return;
    }

    // 2. อ่านจำนวน Board
    count = ctx->project->totalBoards;

    if (count < 0 || count > MAX_BOARD)
    {
        sem_post(sem);
        snprintf(response, sizeof(response),
                 "%s,%d,0\nEND\n",
                 COMMAND_LIST_BOARD, STATUS_FAIL);
        sendLine(ctx->clientSocket, response);
        return;
    }

    // 3. Copy เฉพาะข้อมูลที่ต้องส่ง
    for (int i = 0; i < count; i++)
    {
        Board *board = &ctx->project->boards[i];

        snprintf(ids[i], sizeof(ids[i]),
                 "%s", board->boardId);

        snprintf(titles[i], sizeof(titles[i]),
                 "%s", board->boardTitle);
    }

    // 4. ปล่อย Lock ก่อนส่ง TCP
    sem_post(sem);

    // 5. ส่ง Header
    int status = (count == 0)
                 ? STATUS_EMPTY_BOARD : STATUS_OK;

    snprintf(response, sizeof(response),
             "%s,%d,%d\n",
             COMMAND_LIST_BOARD, status, count);

    sendLine(ctx->clientSocket, response);

    // 6. ส่ง Board Records
    for (int i = 0; i < count; i++)
    {
        snprintf(response, sizeof(response),
                 "%s,%s,%s\n",
                 RECORD_BOARD, ids[i], titles[i]);

        sendLine(ctx->clientSocket, response);
    }

    // 7. จบ Response
    snprintf(response, sizeof(response),
         "%s\n", RESPONSE_END);
    sendLine(ctx->clientSocket, response);
}

/* ================================================================
 * BOARD: Update_BOARD
 * baseline ของอาจารย์ 
 * ================================================================ */

void handleUpdateBoard(CommandContext *ctx)
{
  char *boardId = strtok_r(NULL, COMMAND_SEPARATOR, &ctx->rawInput);
  char *boardTitle = strtok_r(NULL, COMMAND_SEPARATOR, &ctx->rawInput);
  strlen(boardTitle) >= BOARD_TITLE_SIZE;
  char response[MESSAGE_SIZE];

  if (isEmptyOrSpace(boardId) || isEmptyOrSpace(boardTitle))
  {
    snprintf(response, sizeof(response), "%s%s%d\n", COMMAND_UPDATE_BOARD, COMMAND_SEPARATOR, STATUS_INVALID_ARGUMENTS);
    send(ctx->clientSocket, response, strlen(response), 0);
  }
  else
  {
    int status = STATUS_OK;

    if (sem_wait(sem) == -1)
    {
        status = STATUS_FAIL;

    }
    else
    {
        if (hasBoard(ctx->project, boardId) == NULL)
        {
            status = STATUS_BOARD_NOT_FOUND;

        }
        else
        {
            int result = updateBoard(ctx->project, boardId, boardTitle);

            if (result != 0)
            {
                status = STATUS_UPDATE_BOARD_FAIL;
            }
            else
            {
                int saveResult = saveBoard(ctx->project, BOARD_FILENAME);

                if (saveResult != 0)
                {
                    status = STATUS_SAVE_BOARD_FAIL;
                }
            }
        }
        snprintf(response, sizeof(response), "%s%s%d%s%s\n", COMMAND_UPDATE_BOARD,
        COMMAND_SEPARATOR, status, COMMAND_SEPARATOR, statusName(status));
        sendLine(ctx->clientSocket, response);
                
        sem_post(sem);
    }
  }
}

void handleAddTask(CommandContext *ctx)
{
    char *boardId = strtok_r(NULL, COMMAND_SEPARATOR, &ctx->rawInput);
    char *taskTitle = strtok_r(NULL, COMMAND_SEPARATOR, &ctx->rawInput);

    char response[MESSAGE_SIZE];

    if (boardId == NULL || taskTitle == NULL ||
        isEmptyOrSpace(boardId) || isEmptyOrSpace(taskTitle))
    {
        snprintf(response, sizeof(response), "%s%s%d\n", COMMAND_ADD_TASK, COMMAND_SEPARATOR, STATUS_INVALID_ARGUMENTS);
        send(ctx->clientSocket, response, strlen(response), 0);
        return;
    }

    Board *board = hasBoard(ctx->project, boardId);

    if (board == NULL)
    {
        snprintf(response, sizeof(response), "%s%s%d\n", COMMAND_ADD_TASK, COMMAND_SEPARATOR, STATUS_BOARD_NOT_FOUND);
        send(ctx->clientSocket, response, strlen(response), 0);
        return;
    }

    // 1. Wait for the lock
    sem_wait(sem);

    // 2. CRITICAL SECTION
    int result = addTask(board, taskTitle);
    saveAllTasks(ctx->project, TASK_FILENAME);

    // 3. Release the lock
    sem_post(sem);

    if (result == 0)
    {
        snprintf(response, sizeof(response), "%s%s%d\n", COMMAND_ADD_TASK, COMMAND_SEPARATOR, STATUS_OK);
    }
    else
    {
        snprintf(response, sizeof(response), "%s%s%d\n", COMMAND_ADD_TASK, COMMAND_SEPARATOR, STATUS_FAIL);
    }
    send(ctx->clientSocket, response, strlen(response), 0);
}

int handleClient(SOCKET socket_client, char *read)
{
  // printf("line: %s\n", read);
  char response[MESSAGE_SIZE];
  char *saveptr;
  char *commandName = strtok_r(read, ",\n\r", &saveptr);

  if (commandName == NULL)
    return -1;

  CommandContext ctx = {
      .project = project,
      .clientSocket = socket_client,
      .rawInput = saveptr,
  };

  CommandEntry commandTable[] = {
      {COMMAND_ADD_BOARD, handleAddBoard},
      {COMMAND_LIST_BOARD, handleListBoard},
      {COMMAND_UPDATE_BOARD, handleUpdateBoard},
      {COMMAND_ADD_TASK, handleAddTask},
      {NULL, NULL} // Sentinel to mark the end
  };

  for (int i = 0; commandTable[i].commandName != NULL; i++)
  {
    if (strcmp(commandName, commandTable[i].commandName) == 0)
    {
      // Found the command! Execute its function.
      printf("Found command: %s\n", commandName);
      commandTable[i].handler(&ctx);
      return 0;
    }
  }
  snprintf(response, sizeof(response), "%s%s%d\n", COMMAND_UNKNOWN, COMMAND_SEPARATOR, STATUS_UNKNOWN);
  send(socket_client, response, strlen(response), 0);
  return -1;
}

void handle_shutdown(int sig)
{
  if (sig == SIGINT)
  {
    printf("\nCaught Ctrl+C! Cleaning up...\n");
  }
  else if (sig == SIGTERM)
  {
    printf("\nTermination request received...\n");
  }
  // To prevent an infinite loop of signals
  signal(SIGINT, SIG_IGN);

  printf("Closing listening socket...\n");
  CLOSESOCKET(socket_listen);
  socket_listen = -1;

  printf("Cleaning up semaphore...\n");
  sem_close(sem);
  sem_unlink("/store_lock");

  // 2. Kill all child processes in the group
  // Passing 0 as the PID sends the signal to everyone in the process group
  kill(0, SIGTERM);

  exit(0);
}

int setup(char *port)
{
  printf("Configuring local address...\n");
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  // init semaphore
  sem = sem_open("/store_lock", O_CREAT, 0644, 1);

  // Init project using shared memory (mmap)
  project = mmap(NULL, sizeof(Project), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  loadProject(project, BOARD_FILENAME, TASK_FILENAME);

  struct addrinfo *bind_address;
  getaddrinfo(0, port, &hints, &bind_address);

  printf("Creating socket...\n");

  socket_listen = socket(bind_address->ai_family, bind_address->ai_socktype,
                         bind_address->ai_protocol);
  if (!ISVALIDSOCKET(socket_listen))
  {
    fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
    return 1;
  }

  // Allow socket address reuse
  int yes = 1;
  if (setsockopt(socket_listen, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) <
      0)
  {
    fprintf(stderr, "setsockopt() failed. (%d)\n", GETSOCKETERRNO());
    return 1;
  }

  printf("Binding socket to local address...\n");
  printf("Binding to port: %s\n", SERVER_PORT);
  if (bind(socket_listen, bind_address->ai_addr, bind_address->ai_addrlen))
  {
    fprintf(stderr, "bind() failed. (%d)\n", GETSOCKETERRNO());
    return 1;
  }
  freeaddrinfo(bind_address);

  printf("Listening...\n");
  if (listen(socket_listen, 10) < 0)
  {
    fprintf(stderr, "listen() failed. (%d)\n", GETSOCKETERRNO());
    return 1;
  }

  printf("Waiting for connections...\n");
  while (1)
  {
    struct sockaddr_storage client_address;
    socklen_t client_len = sizeof(client_address);
    SOCKET socket_client =
        accept(socket_listen, (struct sockaddr *)&client_address, &client_len);
    if (!ISVALIDSOCKET(socket_client))
    {
      // If we are shutting down, don't print an error
      if (socket_listen == -1)
      {
        return 0;
      }
      fprintf(stderr, "accept() failed. (%d)\n", GETSOCKETERRNO());
      return 1;
    }

    char address_buffer[100];
    getnameinfo((struct sockaddr *)&client_address, client_len, address_buffer,
                sizeof(address_buffer), 0, 0, NI_NUMERICHOST);
    printf("New connection from %s\n", address_buffer);

    int pid = fork();
    if (pid == 0)
    { // child process
      CLOSESOCKET(socket_listen);

      while (1)
      {
        char read[1024];

        int bytes_received =
            recv(socket_client, read, sizeof(read) - 1, 0);

        if (bytes_received <= 0)
        {
            CLOSESOCKET(socket_client);
            exit(0);
        }

        read[bytes_received] = '\0';

        /*
        int bytes_received = recv(socket_client, read, 1024, 0);
        if (bytes_received < 1)
        {
          printf("Client disconnected.\n");

          CLOSESOCKET(socket_client);
          exit(0);
        }
        int j;
        for (j = 0; j < bytes_received; ++j)
          read[j] = read[j];
        */
        printf("%s\n", read);
        replace_char(read, '\n', '\0');
        handleClient(socket_client, read);
      }
    }
    CLOSESOCKET(socket_client);
  } // while(1)

  printf("Finished.\n");
  return 0;
}

int main()
{

  signal(SIGINT, handle_shutdown);  // Catches CTRL+C
  signal(SIGTERM, handle_shutdown); // Catches kill command
  setup(SERVER_PORT);

  return 0;
}