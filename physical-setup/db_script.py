#!/usr/bin/env python3
import sqlite3
import os
import datetime

DB_PATH = os.path.abspath("../db/botnet.db")
os.makedirs(os.path.dirname(DB_PATH), exist_ok=True)

def init_and_seed():
    print(f"[*] Accessing SQLite database target footprint: {DB_PATH}")
    con = sqlite3.connect(DB_PATH)
    con.execute("PRAGMA journal_mode=WAL")
    
    con.execute("""
        CREATE TABLE IF NOT EXISTS tasks (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            device_id TEXT,
            task TEXT,
            status TEXT DEFAULT 'pending',
            created_at TIMESTAMP
        )
    """)
    
    con.execute("""
        CREATE TABLE IF NOT EXISTS results (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            task_id TEXT,
            device_id TEXT,
            result TEXT,
            received_at TIMESTAMP
        )
    """)
    
    now = datetime.datetime.now().isoformat()
    device_id = "dev-esp32mock01"
    
    con.execute("""
        INSERT OR REPLACE INTO devices (device_id, ip, cpu_load, memory, uptime, arch, last_seen)
        VALUES (?, '172.18.0.50', 0.0, 512.0, 0, 'XTensa_LX6', ?)
    """, (device_id, now))
    
    con.execute("DELETE FROM tasks WHERE device_id = ?", (device_id,))
    
    # Insert an 'ATTACK' command task to test the ESP32's internal parsing checks
    con.execute("""
        INSERT INTO tasks (device_id, task, status, created_at)
        VALUES (?, 'ATTACK', 'pending', ?)
    """, (device_id, now))
    
    con.commit()
    con.close()
    print("[+] Database initialized successfully. One 'ATTACK' action pending.")

if __name__ == "__main__":
    init_and_seed()
