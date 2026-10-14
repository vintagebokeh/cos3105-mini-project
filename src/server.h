#ifndef SERVER_H 
#define SERVER_H


#define _POSIX_C_SOURCE 200112L
#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <netdb.h>
#include "models.h"
#include "common.h"
#include "protocol.h"


#define ISVALIDSOCKET(s) ((s) >= 0)
#define CLOSESOCKET(s) close(s)
#define SOCKET int
#define GETSOCKETERRNO() (errno)
#define SERVER_PORT "3030"
#define MESSAGE_SIZE 2000

typedef struct {
    Project *project;
    SOCKET clientSocket;
    char *rawInput;
} CommandContext;

// Define function pointer CommandHandler
typedef void (*CommandHandler)(CommandContext *ctx);

typedef struct {
    char *commandName;
    CommandHandler handler;
} CommandEntry;


// Server
/* ----- Board handlers ----- */
void handleAddBoard(CommandContext *ctx);       // baseline + ปรับ status ให้ชัดขึ้น
void handleListBoard(CommandContext *ctx);      // [ADDED] requirement: List board
void handleUpdateBoard(CommandContext *ctx);    // [ADDED] requirement: Update board
void handleDeleteBoard(CommandContext *ctx);    // [ADDED] requirement: Delete board

/* ----- Task handlers ----- */
void handleAddTask(CommandContext *ctx);        // baseline + ปรับ status ให้ชัดขึ้น
void handleListTask(CommandContext *ctx);       // [ADDED] requirement: List task from boardId
void handleUpdateTask(CommandContext *ctx);     // [ADDED] requirement: Update task
void handleDeleteTask(CommandContext *ctx);     // [ADDED] requirement: Delete task

// Common
int handleClient(SOCKET socket_client,char *read);
void handle_shutdown(int sig);
int setup(char *port);
#endif