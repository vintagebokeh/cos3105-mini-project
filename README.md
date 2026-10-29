# COS3105 — Operating System Mini Project

**สถานะ 29 ตุลาคม 2569:** First-review build มี Board/Task CRUD ครบ 8 คำสั่ง และ Client Text Menu

ภาษา C ต่อยอด Client/Server ของอาจารย์ ฝึก TCP Socket, fork(), Shared Memory (mmap), Semaphore (sem_wait / sem_post) และ Save/Load

## เริ่มใช้งาน (Quick Start)

เปิด Codespaces สอง Terminal ที่ Root ของโปรเจกต์

**Terminal 1 — Server**
~~~bash
bash run_server.sh
~~~

**Terminal 2 — Client เมนูภาษาอังกฤษ**
~~~bash
bash run_client.sh
~~~

เมื่อ Compile แล้วจะรันซ้ำโดยไม่ Build ก็ได้:
~~~bash
./client
~~~

**โหมดคำสั่งเดี่ยว (CLI) ยังใช้ได้**
~~~bash
./client LIST_BOARD
./client ADD_BOARD "Example Board"
./client LIST_TASK 1
./client ADD_TASK 1 "Example Task"
./client UPDATE_TASK 1 1 "Updated Task"
./client DELETE_TASK 1 1
~~~

ตัวอย่าง ADD/UPDATE/DELETE ข้างต้นเปลี่ยนข้อมูลจริงเมื่อเปิด Server จาก Root ให้เลือก ID จริงจาก LIST ก่อน

## ลำดับการทำงาน (Workflow)

1. **Client Input:** readMenuChoice(), readText() รับคีย์บอร์ดและตรวจข้อมูล
2. **Client TCP:** sendRequest() เปิด Socket → ส่ง Request → shutdown(write) → รับ Response
3. **Response Parser:** parseBoards(), parseTasks() อ่านรายการหลายบรรทัดและ END
4. **Client Menu:** boardMenu() → taskMenu() เลือก Board/Task ตาม **ID จริง** ไม่ใช่เลขลำดับใน Array
5. **Server Dispatch:** handleClient() ค้น Command Table
6. **Server Critical Section:** Validate → sem_wait() → Model Operation → Save → sem_post() → Response
7. **Persistence:** เขียน board.txt/task.txt และ Load เมื่อ Restart Server

เมนูแสดง Disabled เมื่อไม่มี Board/Task ที่แก้ไขหรือลบได้ และ Refresh ข้อมูลจาก Server ทุกครั้ง

## Command ที่รองรับ

| Board | Task |
| --- | --- |
| ADD_BOARD | ADD_TASK |
| LIST_BOARD | LIST_TASK |
| UPDATE_BOARD | UPDATE_TASK |
| DELETE_BOARD | DELETE_TASK |

ชื่อ Board/Task ห้ามมี comma เพราะ Protocol ใช้ comma คั่น Field

## ทดสอบแบบไม่แตะข้อมูลจริง

~~~bash
bash build_all.sh
python3 src/tests/test_client_tui.py
~~~

Integration Test เปิด Server ใน Temporary Directory, ทดสอบ CLI, เมนู, TCP, Save/Load รวม **24 Checks** และลบข้อมูลทดลองเมื่อจบ

**ต้องไม่มี Server อื่นฟังพอร์ต 3030** ก่อนเริ่มชุดทดสอบ (ถ้ามี Test จะหยุด ไม่ส่งคำสั่งแก้ข้อมูล)

Unit Test ของ Model:
~~~bash
bash run_test.sh test_models
~~~

## ไฟล์ที่ควรไล่อ่านเพื่อเรียนรู้

- src/models.h และ src/models.c — โครงสร้าง Project → Board → Task, CRUD, Save/Load
- src/protocol.h — คำสั่งและ Status Code
- src/server.c — TCP, fork, Semaphore, Command Handlers
- client.c — Client TCP, Text Menu, Input Helpers พร้อมคอมเมนต์/Workflow ภาษาไทย
- src/tests/test_models.c — Model Unit Tests
- src/tests/test_client_tui.py — Client/Server Integration Test
- CHANGELOG_SERVER_CORE.md — แนวคิดการพัฒนาจาก Baseline ของอาจารย์
- TEST_COMMANDS.md — ตัวอย่างการทดสอบ TCP

## ข้อจำกัด

- Text Menu ยังไม่ใช่ Newt TUI แบบเต็ม
- หนึ่งคำสั่ง TCP ต่อหนึ่ง Connection ตาม Server ปัจจุบัน
- ผ่าน Smoke/Integration Tests แต่ยังไม่ใช่ Concurrency Stress Test
- การ Save ล้มเหลวอาจต้องเสริม Rollback ในอนาคต
