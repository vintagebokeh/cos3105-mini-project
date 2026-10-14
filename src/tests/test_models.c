#include <stdio.h>
#include <stdlib.h>
#include "models.h"
#include <assert.h>

/*
 * Unit Test ฝั่ง Models
 * ------------------------------------------------------------
 * เอกสารอาจารย์ระบุว่าจะดูการเขียน Test Models
 * ชุดนี้ต่อยอดจาก test_models.c เดิมให้ครอบคลุม Update/Delete ที่เพิ่มตาม requirement
 */

int main(){
    printf("Start testing all models\n");

    // printf("Testing Project\n");
    // printf("Testing add board and task to Project\n");
    // Project *project = calloc(1, sizeof(Project));

    Project *project = calloc(1, sizeof(Project));                       // สร้าง Project ว่างสำหรับ test
    assert(project != NULL);                                             // ต้องจอง memory สำเร็จ

    /* ----- Create Board ----- */
    assert(addBoard(project, "Board 1") == 0);                          // เพิ่ม Board 1
    assert(addBoard(project, "Board 2") == 0);                          // เพิ่ม Board 2
    assert(project->totalBoards == 2);                                  // ต้องมี 2 Board
    assert(strcmp(project->boards[0].boardTitle, "Board 1") == 0);      // ตรวจ title Board 1

    /* ----- Create Task ----- */
    assert(addTask(&project->boards[0], "Task 1") == 0);                // เพิ่ม Task 1 ใน Board 1
    assert(addTask(&project->boards[0], "Task 2") == 0);                // เพิ่ม Task 2 ใน Board 1
    assert(project->boards[0].totalTasks == 2);                          // Board 1 ต้องมี 2 Task

    /* ----- Update Board ----- */
    assert(updateBoard(project, "1", "Board 1 Updated") == 0);          // แก้ Board 1
    assert(strcmp(project->boards[0].boardTitle, "Board 1 Updated") == 0);

    /* ----- Update Task ----- */
    assert(updateTask(project, "1", "1", "Task 1 Updated") == 0);      // แก้ Task 1
    assert(strcmp(project->boards[0].tasks[0].taskTitle,
                  "Task 1 Updated") == 0);

    /* ----- Lookup ----- */
    assert(hasBoard(project, "1") != NULL);                              // Board 1 ต้องหาเจอ
    assert(hasTask(project, "1", "1") != NULL);                         // Task 1 ต้องหาเจอ

    /* ----- Delete Task ----- */
    assert(deleteTask(project, "1", "1") == 0);                         // ลบ Task 1
    assert(project->boards[0].totalTasks == 1);                          // เหลือ Task เดียว
    assert(strcmp(project->boards[0].tasks[0].taskTitle, "Task 2") == 0);// Task 2 ถูกเลื่อนมาปิดช่อง

    /* ----- Delete Board ----- */
    assert(deleteBoard(project, "1") == 0);                             // ลบ Board 1 และ Task ภายใน
    assert(project->totalBoards == 1);                                  // เหลือ Board 2
    assert(strcmp(project->boards[0].boardTitle, "Board 2") == 0);      // Board 2 ถูกเลื่อนมาช่อง 0

    /* ----- ID หลัง Delete ต้องไม่ชน ----- */
    assert(addBoard(project, "Board 3") == 0);                          // เพิ่ม Board ใหม่หลัง delete
    assert(strcmp(project->boards[1].boardId, "3") == 0);               // ID ใหม่ต้องเป็น 3 ไม่ใช่ 2 ซ้ำ

    /* ----- Persistence ----- */
    assert(saveBoard(project, BOARD_FILENAME) == 0);                     // save Board
    assert(saveAllTasks(project, TASK_FILENAME) == 0);                   // save Task

    free(project);                                                       // คืน memory test ชุดแรก

    project = calloc(1, sizeof(Project));                                // สร้าง Project ใหม่เพื่อ test load
    assert(project != NULL);
    assert(loadProject(project, BOARD_FILENAME, TASK_FILENAME) == 0);    // โหลดข้อมูลกลับจากไฟล์
    assert(project->totalBoards == 2);                                   // ต้องได้ Board 2 และ Board 3
    assert(strcmp(project->boards[0].boardTitle, "Board 2") == 0);
    assert(strcmp(project->boards[1].boardTitle, "Board 3") == 0);

    //if (project == NULL) 
    //{
    //    printf("calloc failed! sizeof(Project) = %zu\n", sizeof(Project));
    //    return 1;
    //}

    //addBoard(project, "Board 1");
    //assert(project->totalBoards == 1); // it should be 1
    //assert(isEmptyOrSpace(project->boards[0].boardId)==0); // it should not empty


    //addTask(&project->boards[0], "Task 1");
    //assert(project->boards[0].totalTasks == 1); // it should be 1
    //assert(isEmptyOrSpace(project->boards[0].tasks[0].taskId)==0); // it should not empty

    //printf("Testing save board to file\n");
    //saveBoard(project, BOARD_FILENAME);
    //assert(fileExists(BOARD_FILENAME) == 0);

    //printf("Testing save tasks to file\n");
    //saveAllTasks(project, TASK_FILENAME);
    //assert(fileExists(TASK_FILENAME) == 0);

    // 4. Clean up allocated memory
    /*free(project);*/
    /*
    printf("Testing load board and task to Project\n");
    project = calloc(1, sizeof(Project));
    if (project == NULL) {
        printf("calloc failed! sizeof(Project) = %zu\n", sizeof(Project));
        return 1;
    }
    
    printf("Testing load board from file\n");
    readBoard(project, BOARD_FILENAME);
    assert(project->totalBoards == 1); // it should be 1
    assert(isEmptyOrSpace(project->boards[0].boardId)==0); // it should not empty
    assert(strcmp(project->boards[0].boardTitle, "Board 1") == 0); // it should be Board 1

    printf("Testing load task from file\n");
    readAllTasks(project, TASK_FILENAME);
    assert(project->boards[0].totalTasks == 1); // it should be 1
    assert(isEmptyOrSpace(project->boards[0].tasks[0].taskId)==0); // it should not empty
    assert(strcmp(project->boards[0].tasks[0].taskTitle, "Task 1") == 0); // it should be Task 1
    */

    //free(project);

    //printf("Testing load project\n");
    //project = calloc(1, sizeof(Project));
    //if (project == NULL) 
    //{
    //    printf("calloc failed! sizeof(Project) = %zu\n", sizeof(Project));
    //    return 1;
    //}
    
    //printf("Testing load board from file\n");
    //readBoard(project, BOARD_FILENAME);
    //assert(project->totalBoards == 1); // it should be 1
    //assert(isEmptyOrSpace(project->boards[0].boardId)==0); // it should not empty
    //assert(strcmp(project->boards[0].boardTitle, "Board 1") == 0); // it should be Board 1

    //printf("Testing load task from file\n");
    //readAllTasks(project, TASK_FILENAME);
    //assert(project->boards[0].totalTasks == 1); // it should be 1
    //assert(isEmptyOrSpace(project->boards[0].tasks[0].taskId)==0); // it should not empty
    //assert(strcmp(project->boards[0].tasks[0].taskTitle, "Task 1") == 0); // it should be Task 1

    free(project);                                                       // คืน memory

    printf("All model tests passed successfully!\n");
    return 0;
}
