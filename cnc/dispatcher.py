import subprocess;
import json;
import io;
import inspect;
import os;

path = os.path.dirname(inspect.stack()[0][1])

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

if __name__ == "__main__":
    main()
