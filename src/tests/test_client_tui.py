#!/usr/bin/env python3
"""COS3105 Client TUI integration smoke test.

วิธีทำงาน (Workflow):
1. สร้างพื้นที่ทดสอบแยกจาก board.txt/task.txt จริง
2. เปิด Server Process ใหม่ในพื้นที่แยก
3. ทดสอบ CLI Commands และ Response ของ TCP
4. จำลองการกดเมนู Board/Task ผ่าน stdin ของ Client
5. Restart Server แล้วตรวจข้อมูล Persistence
6. ปิด Process และลบไฟล์ทดสอบอัตโนมัติ
"""
import os
import signal
import socket
import subprocess
import tempfile
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SERVER = ROOT / "run_server"
CLIENT = ROOT / "client"
PORT = 3030
passed = 0


def check(label: str, condition: bool, evidence: str = "") -> None:
    global passed
    if not condition:
        raise AssertionError(f"FAIL: {label}\n{evidence[-3000:]}")
    passed += 1
    print(f"PASS {passed:02}: {label}", flush=True)


def free_port() -> bool:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        return sock.connect_ex(("127.0.0.1", PORT)) != 0


def start_server(cwd: str):
    log = open(Path(cwd) / "server.log", "ab")
    # แยก Process Group เพราะ shutdown ของ Server มี kill(0, SIGTERM)
    server = subprocess.Popen(
        [str(SERVER)], cwd=cwd, stdout=log, stderr=subprocess.STDOUT,
        start_new_session=True
    )
    log.close()
    for _ in range(50):
        if server.poll() is not None:
            raise RuntimeError("Server terminated during startup")
        if not free_port():
            return server
        time.sleep(0.1)
    stop_server(server)
    raise RuntimeError("Server did not start listening on port 3030")


def stop_server(server) -> None:
    if server is None or server.poll() is not None:
        return
    try:
        os.killpg(server.pid, signal.SIGTERM)
    except ProcessLookupError:
        pass
    try:
        server.wait(timeout=3)
    except subprocess.TimeoutExpired:
        try:
            os.killpg(server.pid, signal.SIGKILL)
        except ProcessLookupError:
            pass
        server.wait(timeout=3)


def cli(*args: str) -> str:
    out = subprocess.run(
        [str(CLIENT), *args], capture_output=True, text=True,
        timeout=10, check=True
    )
    return out.stdout


def main() -> None:
    if not SERVER.is_file() or not CLIENT.is_file():
        raise RuntimeError("Run bash build_all.sh first")
    if not free_port():
        raise RuntimeError(
            "Port 3030 is already in use. Stop that Server first; "
            "not safe to write to possibly real data."
        )

    with tempfile.TemporaryDirectory(prefix="cos3105-client-smoke-") as folder:
        server = None
        try:
            server = start_server(folder)

            # CLI ทดสอบคำสั่งพื้นฐานก่อนเริ่มเมนู
            check("Empty board list", "LIST_BOARD,202,0" in cli("LIST_BOARD"))
            check("Add Board A", "ADD_BOARD,0" in cli("ADD_BOARD", "Board A"))
            check("Add Board B", "ADD_BOARD,0" in cli("ADD_BOARD", "Board B"))
            boards = cli("LIST_BOARD")
            check("Board IDs are assigned by Server",
                  "BOARD,1,Board A" in boards and "BOARD,2,Board B" in boards, boards)

            check("Missing Board rejected",
                  "ADD_TASK,201" in cli("ADD_TASK", "999", "Orphan"))
            check("Add Task", "ADD_TASK,0" in cli("ADD_TASK", "2", "First Task"))
            check("Read Task", "TASK,1,First Task,0" in cli("LIST_TASK", "2"))
            check("Update Task", "UPDATE_TASK,0" in
                  cli("UPDATE_TASK", "2", "1", "First Updated"))
            check("Task title updated",
                  "TASK,1,First Updated,0" in cli("LIST_TASK", "2"))
            check("Delete Task", "DELETE_TASK,0" in
                  cli("DELETE_TASK", "2", "1"))
            check("Task list now empty", "LIST_TASK,204,0" in cli("LIST_TASK", "2"))
            check("Missing Task rejected",
                  "UPDATE_TASK,203" in cli("UPDATE_TASK", "2", "999", "Missing"))

            # จำลองคีย์บอร์ด: เลือกเมนูผิดหนึ่งครั้ง แล้วเข้าสู่ Board ID 2
            # เพิ่ม/แสดง/แก้/ลบ Task, กลับออกมาแก้/ลบ Board ID 1
            keys = (
                "x\n"      # เมนูไม่ใช่ตัวเลข ต้องไม่เกิดคำสั่ง TCP
                "5\n2\n"  # Select Board > เลือกลำดับ 2 (Board ID 2)
                "2\nInteractive Task\n"
                "1\n"
                "3\n1\nEdited Task\n"
                "1\n"
                "4\n1\ny\n"
                "0\n"      # กลับ Board Menu
                "3\n1\nBoard A Updated\n"
                "4\n1\ny\n"
                "0\n"
            )
            run = subprocess.run(
                [str(CLIENT)], input=keys, capture_output=True, text=True,
                timeout=25
            )
            check("Interactive menu exits normally", run.returncode == 0,
                  run.stdout + run.stderr)
            check("Invalid keyboard choice handled",
                  "Invalid menu number" in run.stdout, run.stdout)
            check("Dynamic Task menu and ID selection",
                  "TASK MENU:" in run.stdout and
                  "Board ID 2" in run.stdout and
                  "Server: ADD_TASK,0" in run.stdout, run.stdout)
            check("Interactive Update/Delete Task",
                  "Server: UPDATE_TASK,0" in run.stdout and
                  "Server: DELETE_TASK,0" in run.stdout, run.stdout)
            check("Interactive Update/Delete Board",
                  "Server: UPDATE_BOARD,0" in run.stdout and
                  "Server: DELETE_BOARD,0" in run.stdout, run.stdout)
            check("Disabled option shown with no Tasks",
                  "Update Task [Disabled]" in run.stdout, run.stdout)

            after = cli("LIST_BOARD")
            check("Only Board ID 2 remains",
                  "BOARD,2,Board B" in after and
                  "BOARD,1," not in after, after)
            check("Task deletion persisted in memory",
                  "LIST_TASK,204,0" in cli("LIST_TASK", "2"))

            # ปิดแล้วเริ่ม Server ใหม่: ตรวจไฟล์ที่สร้างใน Temporary Directory
            stop_server(server)
            server = None
            check("Test board file exists", (Path(folder) / "board.txt").is_file())
            check("Test task file exists", (Path(folder) / "task.txt").is_file())
            server = start_server(folder)
            persisted = cli("LIST_BOARD")
            check("Board survives Server restart",
                  "BOARD,2,Board B" in persisted and "BOARD,1," not in persisted,
                  persisted)
            check("Task deletion survives Server restart",
                  "LIST_TASK,204,0" in cli("LIST_TASK", "2"))
        finally:
            stop_server(server)

    print(f"\nCLIENT TUI + TCP + PERSISTENCE: {passed} checks passed")
    print("Test data stayed inside a temporary directory.")


if __name__ == "__main__":
    main()
