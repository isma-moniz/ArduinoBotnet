#!/bin/bash
# spins up as many containers as you want, yay!
# NOTE: you may have to run this script as root, and in the location of the Dockerfile

docker build -t dev_telnetd:1.0.0 .

if [ "$#" -ne 1 ]; then
	docker run -it -d --name dev dev_telnetd:1.0.0
else
	if ! [[ "$1" =~ ^[0-9]+$ ]]; then
        echo "Usage: $0 [number of containers]"
        exit 1
    fi
	for ((i=0; i<"$1"; i++))
	do
		docker run -it -d --name dev"$i" dev_telnetd:1.0.0
	done
fi
