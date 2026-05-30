import time
import json
import socket
import platform
import hashlib
import os
import requests


CONTROLLER_URL = os.getenv("CONTROLLER_URL", "http://controller:8000") # this has to be changed
DEVICE_ID = os.getenv("DEVICE_ID", "device-001") # this is very interesting maybe CNC sends id to device
SLEEP = 60    # change to lower for testing 


# -------------------------
# METRICS COLLECTION
# -------------------------

def get_cpu_load():
    with open("/proc/loadavg", "r") as f:
        return float(f.read().split()[0])


def get_memory_usage():
    with open("/proc/meminfo", "r") as f:
        data = f.read()

    mem_total = None
    mem_free = None

    for line in data.splitlines():
        if "MemTotal" in line:
            mem_total = int(line.split()[1])
        if "MemAvailable" in line:
            mem_free = int(line.split()[1])

    used_percent = 0
    if mem_total:
        used_percent = round((1 - mem_free / mem_total) * 100, 2)

    return used_percent


def get_uptime():
    with open("/proc/uptime", "r") as f:
        return float(f.read().split()[0])


def get_arch():
    return platform.machine()


def get_ip():
    try:
        return socket.gethostbyname(socket.gethostname())
    except:
        return "unknown"


def collect_metrics():
    return {
        "device_id": DEVICE_ID,
        "cpu_load": get_cpu_load(),
        "memory": get_memory_usage(),
        "uptime": get_uptime(),
        "arch": get_arch(),
        "ip": get_ip()
    }


# -------------------------
# TASK HANDLERS
# -------------------------

def handle_ping(params):
    target = params.get("target", "127.0.0.1")

    start = time.time()
    try:
        sock = socket.create_connection((target, params.get("port", 80)), timeout=2)
        sock.close()
        latency = (time.time() - start) * 1000

        return {
            "status": "ok",
            "latency_ms": round(latency, 2)
        }

    except Exception as e:
        return {
            "status": "fail",
            "error": str(e)
        }


def handle_benchmark(params):
    iterations = params.get("iterations", 200000)

    start = time.time()

    # simple CPU benchmark (safe)
    for _ in range(iterations):
        hashlib.sha256(b"test").hexdigest()

    duration = time.time() - start

    return {
        "status": "ok",
        "iterations": iterations,
        "seconds": round(duration, 3),
        "score": round(iterations / duration, 2)
    }


def handle_sleep(params):
    duration = params.get("seconds", 5)
    time.sleep(duration)

    return {
        "status": "slept",
        "seconds": duration
    }


TASK_HANDLERS = {
    "PING": handle_ping,
    "BENCHMARK": handle_benchmark,
    "SLEEP": handle_sleep,
}


# -------------------------
# COMMUNICATION
# -------------------------

def send_heartbeat():
    metrics = collect_metrics()

    try:
        requests.post(
            f"{CONTROLLER_URL}/heartbeat",
            json=metrics,
            timeout=3
        )
    except Exception as e:
        print("heartbeat error:", e)


def fetch_tasks():
    try:
        r = requests.get(
            f"{CONTROLLER_URL}/tasks",
            params={"device_id": DEVICE_ID},
            timeout=3
        )
        return r.json()
    except:
        return []


def send_result(task_id, result):
    payload = {
        "device_id": DEVICE_ID,
        "task_id": task_id,
        "result": result
    }

    try:
        requests.post(
            f"{CONTROLLER_URL}/result",
            json=payload,
            timeout=3
        )
    except Exception as e:
        print("result error:", e)


# -------------------------
# MAIN LOOP
# -------------------------

def main():
    print(f"[+] Agent started: {DEVICE_ID}")

    while True:
        send_heartbeat()

        tasks = fetch_tasks()

        for task in tasks:
            task_id = task.get("task_id")
            task_type = task.get("type")
            params = task.get("params", {})

            handler = TASK_HANDLERS.get(task_type)

            if handler:
                result = handler(params)
            else:
                result = {"status": "unknown_task"}

            send_result(task_id, result)

        time.sleep(SLEEP)


if __name__ == "__main__":
    main()
