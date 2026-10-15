#include "common.h"
#include "protocol.h"

int isEmptyOrSpace(const char *str) 
{
    if (str == NULL || *str == '\0') 
       return 1; // ว่างเปล่าแน่นอน
    
    while (*str) 
    {
        if (!isspace((unsigned char)*str)) 
        {
            return 0; // เจอตัวอักษรที่ไม่ใช่ช่องว่าง (แปลว่าไม่ว่างจริง)
        }
        str++;
    }

    return 1; // วนจนจบแล้วเจอแต่ช่องว่าง
}

char* replace_char(char* str, char find, char replace)
{
    char *current_pos = strchr(str,find);

    while (current_pos) 
    {
        *current_pos = replace;
        current_pos = strchr(current_pos,find);
    }

    return str;
}

int isNumeric(const char *str) 
{
    if (str == NULL || *str == '\0') 
       return 0;

    int i = 0;
    // ตรวจสอบเครื่องหมายลบที่ตำแหน่งแรก
    if (str[0] == '-') 
    {
        // ถ้ามีแค่เครื่องหมายลบตัวเดียวแบบ "-" ให้ถือว่าไม่ใช่ตัวเลข
        if (str[1] == '\0') return 0;
        i = 1; // เริ่มตรวจตัวเลขที่ตำแหน่งถัดไป
    }

    for (; str[i] != '\0'; i++) 
    {
        if (!isdigit(str[i])) 
        {
            return 0; 
        }
    }

    return 1;
}

int isFloat(const char *str) 
{
    if (str == NULL || *str == '\0') 
       return 0;

    int i = 0;
    int dot_count = 0;

    // 1. เช็คเครื่องหมายลบที่ตำแหน่งแรก
    if (str[i] == '-') 
    {
        if (str[i + 1] == '\0') 
           return 0; // มีแค่ "-" ไม่ได้
        i++;
    }

    // 2. วนลูปเช็คตัวอักษรที่เหลือ
    for (; str[i] != '\0'; i++) 
    {
        if (str[i] == '.') 
        {
            dot_count++;
            if (dot_count > 1) 
               return 0; // มีจุดเกิน 1 จุดไม่ได้
        } 
        else if (!isdigit(str[i])) 
        {
            return 0; // เจอตัวอักษรอื่นที่ไม่ใช่ตัวเลข
        }
    }
    
    // กรณีพิมพ์แค่ "." ก็ให้ถือว่าไม่ใช่เลข
    if (dot_count == 1 && strlen(str) == 1) 
       return 0;

    return 1;
}

int fileExists(const char *filename) 
{
    FILE *file = fopen(filename, "r");
    if (file != NULL) 
    {
        fclose(file);
        return 0;
    }
    
    return 1;
}

const char *statusName(int status)
{
    switch (status)
    {
        case STATUS_OK:                return "OK";
        case STATUS_FAIL:              return "FAIL";
        case STATUS_UNKNOWN:           return "UNKNOWN_COMMAND";

        case STATUS_INVALID_ARGUMENTS: return "INVALID_ARGUMENTS";
        case STATUS_BOARD_NOT_FOUND:   return "BOARD_NOT_FOUND";
        case STATUS_EMPTY_BOARD:       return "EMPTY_BOARD";
        case STATUS_TASK_NOT_FOUND:    return "TASK_NOT_FOUND";
        case STATUS_EMPTY_TASK:        return "EMPTY_TASK";

        case STATUS_SAVE_BOARD_FAIL:   return "SAVE_BOARD_FAIL";
        case STATUS_ADD_BOARD_FAIL:    return "ADD_BOARD_FAIL";
        case STATUS_SAVE_TASK_FAIL:    return "SAVE_TASK_FAIL";
        case STATUS_ADD_TASK_FAIL:     return "ADD_TASK_FAIL";

        case STATUS_UPDATE_BOARD_FAIL: return "UPDATE_BOARD_FAIL";
        case STATUS_DELETE_BOARD_FAIL: return "DELETE_BOARD_FAIL";
        case STATUS_UPDATE_TASK_FAIL:  return "UPDATE_TASK_FAIL";
        case STATUS_DELETE_TASK_FAIL:  return "DELETE_TASK_FAIL";
        case STATUS_SAVE_BOARD_AND_TASK_FAIL: return "SAVE_BOARD_AND_TASK_FAIL";
        case STATUS_ADD_BOARD_AND_SAVE_FAIL: return "STATUS_ADD_BOARD_AND_SAVE_FAIL";

        default:                       return "UNKNOWN_STATUS";
    }
}