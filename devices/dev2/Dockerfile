FROM alpine:latest

# install necessary packages
RUN apk add --no-cache openssh-server \
	&& apk add --no-cache lscpu \
	&& apk add --no-cache apache2 \
	&& apk add --no-cache sed

# enable password ssh root login (oh my god this is so retarded)
RUN sed -i 's/#\?\(PermitRootLogin\s*\).*$/\1 yes/' /etc/ssh/sshd_config \
	&& ssh-keygen -A \
	&& echo 'root:password' | chpasswd

ENTRYPOINT /usr/sbin/sshd && /bin/sh
