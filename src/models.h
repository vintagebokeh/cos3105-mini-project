#ifndef MODELS_H 
#define MODELS_H

#include "common.h"

/* ขนาดข้อมูลที่อาจารย์กำหนดไว้ */
#define BOARDID_SIZE 16
#define BOARD_TITLE_SIZE 500
#define TASKID_SIZE 16
#define TASK_TITLE_SIZE 500
#define MAX_TASK 100
#define MAX_BOARD 100

/* ชื่อไฟล์ persistence ตาม baseline code */
#define BOARD_FILENAME "board.txt"
#define TASK_FILENAME "task.txt"

/* ----- Task model ----- */
typedef struct task Task;

struct task {
    char taskId[TASKID_SIZE];          // ID ของ Task
    char taskTitle[TASK_TITLE_SIZE];   // Title/เนื้อหาแบบง่ายตาม model เดิมของอาจารย์
    int isFinish;                      // สถานะงานเดิมที่อาจารย์เตรียมไว้
};

/* ----- Board model ----- */
typedef struct board Board;

struct board {
    char boardId[BOARDID_SIZE];        // ID ของ Board
    char boardTitle[BOARD_TITLE_SIZE]; // ชื่อ Board
    struct task tasks[MAX_TASK];       // Task ทั้งหมดที่อยู่ใน Board นี้
    int totalTasks;                    // จำนวน Task ที่ใช้งานจริง
};

/* ----- Project model ----- */
typedef struct project Project;

struct project {
    struct board boards[MAX_BOARD];    // Board ทั้งหมดของ Project
    int totalBoards;                   // จำนวน Board ที่ใช้งานจริง
};

/* ----- Persistence ----- */
int loadProject(Project *project, char *boardFileName, char *taskFileName);
int readBoard(Project *project, char *fileName);
int saveBoard(Project *project, char *fileName);
int readAllTasks(Project *project, char *fileName);
int saveAllTasks(Project *project, char *fileName);

/* ----- Create ----- */
int addBoard(Project *project, char *boardTitle);
int addTask(Board *board, char *taskTitle);

/* ----- Lookup ----- */
Board *hasBoard(Project *project, char *boardId);
Task *hasTask(Project *project, char *boardId, char *taskId);

/*
 * [ADDED] Requirement ใหม่จากเอกสารอาจารย์
 * LIST ไม่ได้เพิ่ม Model function เพราะ Project มี totalBoards/boards[]
 * และ Board มี totalTasks/tasks[] อยู่แล้ว Server สามารถอ่าน snapshot ได้โดยตรง
 */
int updateBoard(Project *project, char *boardId, char *boardTitle);
int deleteBoard(Project *project, char *boardId);
int updateTask(Project *project, char *boardId, char *taskId, char *taskTitle);
int deleteTask(Project *project, char *boardId, char *taskId);

#endif