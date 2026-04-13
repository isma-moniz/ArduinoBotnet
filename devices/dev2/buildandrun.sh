#!/bin/bash

#NOTE: you may have to run this script as root

docker buildx build \
	--platform linux/arm64 \
	-t dev2_alpine:arm64 \
	--load .

sudo docker run -it --platform=linux/arm64 --name dev2 -p 8080:80 dev2_alpine:arm64
