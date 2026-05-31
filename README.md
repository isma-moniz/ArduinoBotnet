# Idea

Have all possible functions as task payloads. Scanning is a task payload, loading is a task payload, yada yada. Even in the initial computer.
Every device is treated as a node, even the one initiating the attack. Never run payloads by themselves, simply use the CnC to issue commands to nodes,
even to the initial one. Simply runs the C executables and stores results and whatever in a central file. This is the most extensible architecture.

## Current implementation status (PLEASE READ ME IF YOU'RE ONE OF THE DEVS)

- [x] Loader -> bruteforces telnet connection to a given ip
    - [x] Loader reports ip:username:pass to report.py collection server

- [x] dispatch.py connects with retrieved telnet credentials to provided ip and "infects" with bot
    - [ ] automatically selects uninfected ips to infect
    - [ ] has meaningful management capabilities (start infection from 0, orchestrating scanner->loader->infection)
    - [ ] interacts with the API endpoints somehow in order to manage the devices (the agent in the infected devices will constantly
    pull for instructions and update the status, but the dispatcher should serve as a higher level wrapper over the api.py to issue orders)

- [x] agent.sh (bot) when deployed to a device pulls necessary payloads from server
    - [x] agent.sh stores the device id locally in the device and is aware of it, and uses it in requests to the server
    - [x] agent.sh marks the device as infected locally (in-device)
    - [ ] agent.sh does the management loop (periodically checking for instructions, marking busy, etc.)
    - [ ] agent.sh signals the device as infected to the server (the server should do this upon receiving a get request from an agent with the id)

- [x] Scanner -> scans for ips in subnet and tests if they have port 23 open (telnet default)
    - [x] Reports them to report.py collection server in a similar manner to loader 
    - [x] dispatcher.py calls loader on non-bruted scanned devices
