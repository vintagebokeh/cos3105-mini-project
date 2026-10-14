#define _POSIX_C_SOURCE 200112L

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SERVER_HOST "127.0.0.1"
#define SERVER_PORT "3030"
#define BUFFER_SIZE 2048

/*
 * CLIENT CORE (ยังไม่ใช่ TUI)
 * ------------------------------------------------------------
 * จุดประสงค์ของไฟล์นี้คือใช้ทดแทน nc ในรอบถัดไป
 * รับ token จาก command line แล้วประกอบเป็น protocol ด้วย comma
 * เช่น:
 *   ./client LIST_BOARD
 *   ./client ADD_BOARD "Board 2"
 *   ./client ADD_TASK 1 "Task 1"
 *
 * Server ปัจจุบันรองรับหนึ่ง command ต่อ connection ได้ดีอยู่แล้ว
 * Client จึง shutdown ฝั่ง write หลังส่ง request แล้วอ่าน response จน Server ปิด connection
 */
int main(int argc, char *argv[])
{
    if (argc < 2)                                                       // ต้องมีอย่างน้อยชื่อ command
    {
        fprintf(stderr, "Usage: %s COMMAND [arg1] [arg2] [arg3]\n", argv[0]);
        return 1;
    }

    char request[BUFFER_SIZE];                                          // buffer สำหรับประกอบ protocol request
    request[0] = '\0';                                                  // เริ่มจาก empty C string

    for (int i = 1; i < argc; i++)                                     // เดิน argument ที่ user พิมพ์
    {
        if (i > 1)                                                      // token หลังตัวแรกต้องคั่นด้วย comma
            strncat(request, ",", sizeof(request) - strlen(request) - 1);

        strncat(request, argv[i], sizeof(request) - strlen(request) - 1);// ต่อ token เข้า request
    }

    strncat(request, "\n", sizeof(request) - strlen(request) - 1);      // ปิด request ด้วย newline

    struct addrinfo hints;                                              // เงื่อนไขในการ resolve server address
    memset(&hints, 0, sizeof(hints));                                   // clear struct
    hints.ai_family = AF_INET;                                          // IPv4
    hints.ai_socktype = SOCK_STREAM;                                    // TCP

    struct addrinfo *server_address = NULL;                             // result จาก getaddrinfo

    if (getaddrinfo(SERVER_HOST, SERVER_PORT, &hints, &server_address) != 0)
    {
        fprintf(stderr, "getaddrinfo() failed\n");
        return 1;
    }

    int clientSocket = socket(server_address->ai_family,
                              server_address->ai_socktype,
                              server_address->ai_protocol);             // สร้าง TCP socket ฝั่ง Client

    if (clientSocket < 0)
    {
        perror("socket");
        freeaddrinfo(server_address);
        return 1;
    }

    if (connect(clientSocket,
                server_address->ai_addr,
                server_address->ai_addrlen) < 0)                        // เชื่อมไป Server :3030
    {
        perror("connect");
        close(clientSocket);
        freeaddrinfo(server_address);
        return 1;
    }

    freeaddrinfo(server_address);                                       // address result ใช้เสร็จแล้ว

    send(clientSocket, request, strlen(request), 0);                     // ส่ง protocol request

    /*
     * บอก Server ว่า Client จะไม่ส่งข้อมูลเพิ่มแล้ว แต่ยังรออ่าน response ได้
     * Child Server จะวน recv อีกครั้ง แล้วเจอ EOF -> close -> Client อ่านจนจบได้
     */
    shutdown(clientSocket, SHUT_WR);

    char response[BUFFER_SIZE];                                         // buffer สำหรับรับ response
    int bytes_received;                                                 // จำนวน byte ที่รับแต่ละรอบ

    while ((bytes_received = recv(clientSocket,
                                  response,
                                  sizeof(response) - 1,
                                  0)) > 0)
    {
        response[bytes_received] = '\0';                                // ทำให้เป็น C string
        printf("%s", response);                                        // แสดง response ตามที่ Server ส่งมา
    }

    close(clientSocket);                                                // ปิด socket เมื่ออ่าน response จบ
    return 0;
}
