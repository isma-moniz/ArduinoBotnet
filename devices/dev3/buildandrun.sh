#!/bin/bash

#NOTE: you may have to run this script as root

docker buildx build \
	--platform linux/arm64 \
	-t dev3_telnetd:arm64 \
	--load .

sudo docker run -it --platform=linux/arm64 --name dev3 dev3_telnetd:arm64
