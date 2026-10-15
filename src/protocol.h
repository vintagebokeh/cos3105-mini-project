#ifndef PROTOCOL_H
#define PROTOCOL_H

/*
 * PROTOCOL.H
 * ----------------------------------------
 * ไฟล์นี้เก็บ "ภาษากลาง" ที่ Client และ Server ตกลงใช้ร่วมกัน
 * เช่น ชื่อคำสั่ง และรหัสสถานะที่ Server ส่งกลับไปให้ Client
 *
 * NOTE: ส่วน LIST/UPDATE/DELETE ด้านล่างเป็นโค้ดเพิ่มเติมจาก baseline ของอาจารย์
 * เพื่อทำ requirement ให้ครบตามเอกสาร Mini Project
 */

/* ----- Board commands ----- */
#define COMMAND_ADD_BOARD     "ADD_BOARD"      // เพิ่ม Board
#define COMMAND_LIST_BOARD    "LIST_BOARD"     // [ADDED] แสดง Board ทั้งหมด
#define COMMAND_UPDATE_BOARD  "UPDATE_BOARD"   // [ADDED] แก้ชื่อ Board
#define COMMAND_DELETE_BOARD  "DELETE_BOARD"   // [ADDED] ลบ Board

/* ----- Task commands ----- */
#define COMMAND_ADD_TASK      "ADD_TASK"       // เพิ่ม Task เข้า Board
#define COMMAND_LIST_TASK     "LIST_TASK"      // [ADDED] แสดง Task ของ Board ตาม boardId
#define COMMAND_UPDATE_TASK   "UPDATE_TASK"    // [ADDED] แก้ Task ตาม boardId + taskId
#define COMMAND_DELETE_TASK   "DELETE_TASK"    // [ADDED] ลบ Task ตาม boardId + taskId

/* ตัวคั่น argument ใน protocol เช่น ADD_TASK,1,Task 1 */
#define COMMAND_SEPARATOR ","

/* ใช้ตอบเมื่อ Client ส่ง command ที่ Server ไม่รู้จัก */
#define COMMAND_UNKNOWN "UNKNOWN"

/* Record type สำหรับ response แบบ list */
#define RECORD_BOARD "BOARD"                  // [ADDED] ใช้ขึ้นต้นแต่ละ Board record
#define RECORD_TASK  "TASK"                   // [ADDED] ใช้ขึ้นต้นแต่ละ Task record
#define RESPONSE_END "END"                    // [ADDED] ใช้บอกจบ response แบบหลาย record

/*
 * ResponseStatus คือ status ที่ส่ง "ผ่าน Protocol" จาก Server -> Client
 * ไม่ใช่ return code ภายใน Model โดยตรง ถึงแม้บางค่าจะใช้เลขเดียวกันได้
 */

typedef enum {
    // --- 0xXX: Success & Basic ---
    STATUS_OK = 0,
    STATUS_FAIL = 1,
    STATUS_UNKNOWN = 3,

    // --- 2xXX: Client/Request Errors (200+) ---
    STATUS_INVALID_ARGUMENTS = 200,
    STATUS_BOARD_NOT_FOUND = 201,
    STATUS_EMPTY_BOARD = 202,
    STATUS_TASK_NOT_FOUND = 203,
    STATUS_EMPTY_TASK = 204,

    // --- 5xXX: Server/System Errors (500+) ---
    STATUS_SAVE_BOARD_FAIL = 500,
    STATUS_ADD_BOARD_FAIL = 501,
    STATUS_SAVE_TASK_FAIL = 502,
    STATUS_ADD_TASK_FAIL = 503,

    /* [ADDED] ใช้แยก failure ของ requirement ใหม่ให้ debug ได้ชัดขึ้น */
    STATUS_UPDATE_BOARD_FAIL = 504,
    STATUS_DELETE_BOARD_FAIL = 505,
    STATUS_UPDATE_TASK_FAIL = 506,
    STATUS_DELETE_TASK_FAIL = 507,
    STATUS_SAVE_BOARD_AND_TASK_FAIL = 508,
} ResponseStatus;

#endif