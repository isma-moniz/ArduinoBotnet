#!/bin/bash

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 [number of containers]"
else
	if ! [[ "$1" =~ ^[0-9]+$ ]]; then
        echo "Usage: $0 [number of containers]"
        exit 1
    fi
	for ((i=0; i<"$1"; i++))
	do
		docker rm -f dev"$i"
	done
fi
