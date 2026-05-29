#!/bin/bash

#NOTE: you may have to run this script as root

docker buildx build \
	#	--platform linux/arm64 \ (let's keep it simple for now)
	-t dev_telnetd:1.0.0 \
	--load .

sudo docker run -it --name dev_telnet dev_telnetd:1.0.0
