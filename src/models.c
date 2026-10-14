#include "models.h"

/*
 * [ADDED] หา Board ID ถัดไปจาก ID สูงสุดที่ยังมีอยู่
 * เหตุผล: เมื่อมี DELETE_BOARD แล้ว totalBoards + 1 อาจสร้าง ID ซ้ำได้
 */
static int getNextBoardId(Project *project)
{
    int maxId = 0;                                      // เก็บ ID สูงสุดที่พบ

    for (int i = 0; i < project->totalBoards; i++)      // เดินดู Board ที่ใช้งานจริงทั้งหมด
    {
        int currentId = atoi(project->boards[i].boardId);// แปลง boardId จาก char array เป็น int
        if (currentId > maxId)                          // ถ้า ID ปัจจุบันสูงกว่าเดิม
            maxId = currentId;                          // จำค่าใหม่เป็นค่าสูงสุด
    }

    return maxId + 1;                                   // ID ใหม่ = ค่าสูงสุด + 1
}

/*
 * [ADDED] หา Task ID ถัดไปภายใน Board เดียวกัน
 * Task ID จึง unique ภายใน Board ตามโครง model เดิม
 */
static int getNextTaskId(Board *board)
{
    int maxId = 0;                                      // เก็บ Task ID สูงสุดใน Board

    for (int i = 0; i < board->totalTasks; i++)         // เดินดู Task ที่ใช้งานจริง
    {
        int currentId = atoi(board->tasks[i].taskId);   // แปลง taskId เป็น int
        if (currentId > maxId)                          // ถ้าเจอ ID สูงกว่า
            maxId = currentId;                          // อัปเดตค่าสูงสุด
    }

    return maxId + 1;                                   // คืน ID ถัดไป
}

int loadProject(Project *project, char *boardFileName, char *taskFileName)
{
    if (readBoard(project, boardFileName) == 1)          // โหลด Board ก่อน เพราะ Task ต้องอ้าง boardId
        return 1;                                        // ถ้าอ่าน Board ไม่สำเร็จ ให้จบด้วย failure

    if (readAllTasks(project, taskFileName) == 1)        // โหลด Task แล้วกระจายเข้า Board ที่ตรงกัน
        return 1;                                        // ถ้าอ่าน Task ไม่สำเร็จ ให้จบด้วย failure

    return 0;                                            // โหลดข้อมูลสำเร็จ
}

int readBoard(Project *project, char *fileName)
{
    FILE *file_ptr = fopen(fileName, "r");              // เปิดไฟล์ Board เพื่ออ่าน

    if (file_ptr == NULL)                                // ถ้าไฟล์ยังไม่มี
    {
        printf("File not found. Creating a new one...\n");
        file_ptr = fopen(fileName, "w+");                // สร้างไฟล์ใหม่และเปิดให้อ่าน/เขียน
    }

    if (file_ptr == NULL)                                // ถ้ายังเปิดไฟล์ไม่ได้
        return 1;                                        // แจ้ง failure กลับ caller

    /* [FIX] buffer ต้องรองรับ title ที่กำหนดไว้สูงสุด 500 char */
    char buffer[BOARDID_SIZE + BOARD_TITLE_SIZE + 8];
    int i = 0;                                           // index ของ Board ที่กำลังโหลด
    int fd = fileno(file_ptr);                           // ขอ file descriptor เพื่อใช้ flock

    flock(fd, LOCK_EX);                                  // lock ไฟล์ระหว่างอ่าน

    while (fgets(buffer, sizeof(buffer), file_ptr) != NULL && i < MAX_BOARD)
    {
        buffer[strcspn(buffer, "\n")] = 0;              // ตัด newline ท้ายบรรทัด

        char *boardId = strtok(buffer, ",");             // token แรก = boardId
        char *boardTitle = strtok(NULL, ",");            // token ที่สอง = boardTitle

        if (boardId != NULL && boardTitle != NULL)        // รับเฉพาะบรรทัดที่ parse ได้ครบ
        {
            snprintf(project->boards[i].boardId,
                     sizeof(project->boards[i].boardId),
                     "%s", boardId);                    // copy boardId ลง Project

            snprintf(project->boards[i].boardTitle,
                     sizeof(project->boards[i].boardTitle),
                     "%s", boardTitle);                 // copy boardTitle ลง Project

            i++;                                         // เพิ่มจำนวน Board ที่โหลดสำเร็จ
        }
    }

    project->totalBoards = i;                            // บันทึกจำนวน Board จริงใน Shared Project

    flock(fd, LOCK_UN);                                  // ปลด file lock
    fclose(file_ptr);                                    // ปิดไฟล์
    return 0;                                            // สำเร็จ
}

int saveBoard(Project *project, char *fileName)
{
    FILE *file_ptr = fopen(fileName, "w");              // เปิดแบบ w เพื่อเขียน snapshot ใหม่ทั้งไฟล์

    if (file_ptr == NULL)                                // ถ้าเปิดไฟล์ไม่ได้
    {
        printf("Error opening file!\n");
        return 1;                                        // แจ้ง failure
    }

    int fd = fileno(file_ptr);                           // ขอ file descriptor
    flock(fd, LOCK_EX);                                  // lock ไฟล์ระหว่างเขียน

    Board *boards = project->boards;                     // pointer ไปยัง Board array ใน Project

    /* [FIX] เขียนเฉพาะ Board ที่ใช้งานจริง ไม่ใช่ MAX_BOARD ทั้ง 100 ช่อง */
    for (int i = 0; i < project->totalBoards; i++)
    {
        fprintf(file_ptr, "%s,%s\n",
                boards[i].boardId,
                boards[i].boardTitle);                   // เขียน boardId,boardTitle หนึ่งบรรทัด
    }

    flock(fd, LOCK_UN);                                  // ปลด file lock
    fclose(file_ptr);                                    // ปิดไฟล์
    return 0;                                            // สำเร็จ
}

int readAllTasks(Project *project, char *fileName)
{
    FILE *file_ptr = fopen(fileName, "r");              // เปิดไฟล์ Task เพื่ออ่าน

    if (file_ptr == NULL)                                // ถ้ายังไม่มีไฟล์
    {
        printf("File not found. Creating a new one...\n");
        file_ptr = fopen(fileName, "w+");                // สร้างไฟล์ใหม่
    }

    if (file_ptr == NULL)                                // ถ้าเปิด/สร้างไม่ได้
        return 1;                                        // แจ้ง failure

    /* [FIX] buffer รองรับ taskTitle สูงสุดตาม model */
    char buffer[BOARDID_SIZE + TASKID_SIZE + TASK_TITLE_SIZE + 32];
    int fd = fileno(file_ptr);                           // ขอ file descriptor
    flock(fd, LOCK_EX);                                  // lock ไฟล์ระหว่างอ่าน

    while (fgets(buffer, sizeof(buffer), file_ptr) != NULL)
    {
        buffer[strcspn(buffer, "\n")] = 0;              // ตัด newline

        char *boardId = strtok(buffer, ",");             // token 1 = boardId
        char *taskId = strtok(NULL, ",");                // token 2 = taskId
        char *taskTitle = strtok(NULL, ",");             // token 3 = taskTitle
        char *isFinishStr = strtok(NULL, ",");           // token 4 = isFinish

        if (boardId == NULL || taskId == NULL ||
            taskTitle == NULL || isFinishStr == NULL)
            continue;                                    // ข้ามบรรทัด malformed

        Board *targetBoard = hasBoard(project, boardId);  // หา Board ที่ Task นี้สังกัด

        if (targetBoard == NULL)                          // ถ้าไม่มี Board เป้าหมาย
            continue;                                    // ข้าม Task นี้

        if (targetBoard->totalTasks < MAX_TASK)           // ป้องกัน array เต็ม
        {
            int i = targetBoard->totalTasks;              // index ช่องว่างถัดไป

            snprintf(targetBoard->tasks[i].taskId,
                     sizeof(targetBoard->tasks[i].taskId),
                     "%s", taskId);                     // copy taskId

            snprintf(targetBoard->tasks[i].taskTitle,
                     sizeof(targetBoard->tasks[i].taskTitle),
                     "%s", taskTitle);                  // copy taskTitle

            targetBoard->tasks[i].isFinish = atoi(isFinishStr); // แปลงสถานะกลับเป็น int
            targetBoard->totalTasks++;                    // เพิ่มจำนวน Task ใน Board
        }
    }

    flock(fd, LOCK_UN);                                  // ปลด file lock
    fclose(file_ptr);                                    // ปิดไฟล์
    return 0;                                            // สำเร็จ
}

int saveAllTasks(Project *project, char *fileName)
{
    FILE *file_ptr = fopen(fileName, "w");              // เปิด task file เพื่อเขียน snapshot ใหม่

    if (file_ptr == NULL)                                // ถ้าเปิดไม่ได้
    {
        printf("Error opening file!\n");
        return 1;                                        // แจ้ง failure
    }

    int fd = fileno(file_ptr);                           // ขอ file descriptor
    flock(fd, LOCK_EX);                                  // lock ไฟล์

    for (int b = 0; b < project->totalBoards; b++)       // เดินทุก Board ที่ใช้งานจริง
    {
        Board *board = &project->boards[b];              // pointer ไป Board ปัจจุบัน

        for (int i = 0; i < board->totalTasks; i++)      // เดินทุก Task ใน Board
        {
            fprintf(file_ptr, "%s,%s,%s,%d\n",
                    board->boardId,
                    board->tasks[i].taskId,
                    board->tasks[i].taskTitle,
                    board->tasks[i].isFinish);           // เขียน Task พร้อม boardId ที่สังกัด
        }
    }

    flock(fd, LOCK_UN);                                  // ปลด file lock
    fclose(file_ptr);                                    // ปิดไฟล์
    return 0;                                            // สำเร็จ
}

int addBoard(Project *project, char *boardTitle)
{
    if (project == NULL || boardTitle == NULL)            // ตรวจ pointer ขั้นพื้นฐาน
        return 1;                                         // input ไม่พร้อม

    if (project->totalBoards >= MAX_BOARD)                // ถ้า Board เต็มตาม array
        return 1;                                         // เพิ่มไม่ได้

    Board *board = &project->boards[project->totalBoards];// ใช้ช่องว่างถัดไป
    memset(board, 0, sizeof(Board));                      // ล้างข้อมูลเก่าของช่องนี้

    int nextId = getNextBoardId(project);                 // [ADDED] หา ID ใหม่ที่ไม่ชนหลัง delete

    snprintf(board->boardId,
             sizeof(board->boardId),
             "%d", nextId);                             // บันทึก ID เป็น char array

    snprintf(board->boardTitle,
             sizeof(board->boardTitle),
             "%s", boardTitle);                         // บันทึกชื่อ Board

    project->totalBoards++;                               // เพิ่มจำนวน Board จริง
    return 0;                                             // สำเร็จ
}

int addTask(Board *board, char *taskTitle)
{
    if (board == NULL || taskTitle == NULL)               // ตรวจ pointer ขั้นพื้นฐาน
        return 1;                                         // input ไม่พร้อม

    if (board->totalTasks >= MAX_TASK)                    // ถ้า Task เต็ม
        return 1;                                         // เพิ่มไม่ได้

    Task *task = &board->tasks[board->totalTasks];        // ใช้ช่องว่างถัดไป
    memset(task, 0, sizeof(Task));                        // ล้างค่าค้างเดิม

    int nextId = getNextTaskId(board);                    // [ADDED] หา ID ถัดไปที่ไม่ชนหลัง delete

    snprintf(task->taskId,
             sizeof(task->taskId),
             "%d", nextId);                             // บันทึก taskId

    snprintf(task->taskTitle,
             sizeof(task->taskTitle),
             "%s", taskTitle);                          // บันทึก taskTitle

    task->isFinish = 0;                                   // Task ใหม่เริ่มต้นเป็นยังไม่เสร็จ
    board->totalTasks++;                                  // เพิ่มจำนวน Task จริง
    return 0;                                             // สำเร็จ
}

Board *hasBoard(Project *project, char *boardId)
{
    if (project == NULL || boardId == NULL)               // ถ้า input ใช้ไม่ได้
        return NULL;                                      // หาไม่ได้

    for (int i = 0; i < project->totalBoards; i++)        // เดินทุก Board
    {
        if (strcmp(project->boards[i].boardId, boardId) == 0)
            return &project->boards[i];                   // เจอแล้วคืน pointer ไป Board นั้น
    }

    return NULL;                                          // ไม่พบ Board
}

Task *hasTask(Project *project, char *boardId, char *taskId)
{
    Board *board = hasBoard(project, boardId);            // หา Board ก่อน

    if (board == NULL || taskId == NULL)                  // Board ไม่มี หรือ taskId ไม่มี
        return NULL;                                      // หา Task ไม่ได้

    for (int i = 0; i < board->totalTasks; i++)           // เดิน Task ใน Board เท่านั้น
    {
        if (strcmp(board->tasks[i].taskId, taskId) == 0)
            return &board->tasks[i];                      // เจอแล้วคืน pointer ไป Task
    }

    return NULL;                                          // ไม่พบ Task
}

/* [ADDED] Requirement: Update board */
int updateBoard(Project *project, char *boardId, char *boardTitle)
{
    Board *board = hasBoard(project, boardId);            // หา Board ที่ต้องแก้

    if (board == NULL || boardTitle == NULL)              // ถ้า Board ไม่เจอหรือ title ไม่พร้อม
        return 1;                                         // update ไม่สำเร็จ

    snprintf(board->boardTitle,
             sizeof(board->boardTitle),
             "%s", boardTitle);                         // เขียนชื่อใหม่ทับชื่อเดิม

    return 0;                                             // update สำเร็จ
}

/* [ADDED] Requirement: Delete board แบบลบจริงจาก array */
int deleteBoard(Project *project, char *boardId)
{
    if (project == NULL || boardId == NULL)               // ตรวจ input
        return 1;                                         // ใช้งานไม่ได้

    int targetIndex = -1;                                 // -1 หมายถึงยังหา Board ไม่เจอ

    for (int i = 0; i < project->totalBoards; i++)        // หา index ของ Board เป้าหมาย
    {
        if (strcmp(project->boards[i].boardId, boardId) == 0)
        {
            targetIndex = i;                              // จำ index ที่พบ
            break;                                        // หยุดค้นหา
        }
    }

    if (targetIndex == -1)                                // ถ้าไม่พบ Board
        return 1;                                         // delete ไม่สำเร็จ

    for (int i = targetIndex; i < project->totalBoards - 1; i++)
        project->boards[i] = project->boards[i + 1];      // เลื่อน Board ด้านหลังมาปิดช่องว่าง

    project->totalBoards--;                               // ลดจำนวน Board จริง
    memset(&project->boards[project->totalBoards],
           0,
           sizeof(Board));                                // ล้างช่องท้ายที่ไม่ใช้งานแล้ว

    return 0;                                             // delete สำเร็จ
}

/* [ADDED] Requirement: Update task */
int updateTask(Project *project, char *boardId, char *taskId, char *taskTitle)
{
    Task *task = hasTask(project, boardId, taskId);       // หา Task จาก boardId + taskId

    if (task == NULL || taskTitle == NULL)                // ถ้า Task ไม่เจอหรือ title ไม่พร้อม
        return 1;                                         // update ไม่สำเร็จ

    snprintf(task->taskTitle,
             sizeof(task->taskTitle),
             "%s", taskTitle);                          // เขียนชื่อใหม่ทับชื่อเดิม

    return 0;                                             // update สำเร็จ
}

/* [ADDED] Requirement: Delete task แบบลบจริงจาก array */
int deleteTask(Project *project, char *boardId, char *taskId)
{
    Board *board = hasBoard(project, boardId);            // หา Board ที่ Task สังกัด

    if (board == NULL || taskId == NULL)                  // ถ้า Board ไม่เจอหรือ taskId ไม่พร้อม
        return 1;                                         // delete ไม่สำเร็จ

    int targetIndex = -1;                                 // index ของ Task เป้าหมาย

    for (int i = 0; i < board->totalTasks; i++)           // เดิน Task ใน Board
    {
        if (strcmp(board->tasks[i].taskId, taskId) == 0)
        {
            targetIndex = i;                              // จำ index ที่พบ
            break;                                        // หยุดค้นหา
        }
    }

    if (targetIndex == -1)                                // ไม่พบ Task
        return 1;                                         // delete ไม่สำเร็จ

    for (int i = targetIndex; i < board->totalTasks - 1; i++)
        board->tasks[i] = board->tasks[i + 1];            // เลื่อน Task ด้านหลังมาปิดช่องว่าง

    board->totalTasks--;                                  // ลดจำนวน Task จริง
    memset(&board->tasks[board->totalTasks],
           0,
           sizeof(Task));                                 // ล้างช่องท้ายที่ไม่ใช้งานแล้ว

    return 0;                                             // delete สำเร็จ
}
