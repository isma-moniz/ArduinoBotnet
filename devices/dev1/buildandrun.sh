#!/bin/bash

#NOTE: you may have to run this script as root

docker buildx build \
	--platform linux/arm64 \
	-t dev1_httpd:arm64 \
	--load .

sudo docker run -it --platform=linux/arm64 --name dev1 -p 8080:80 dev1_httpd:arm64
