FROM ubuntu:20.04

# install necessary packages
RUN apt-get update -y && \
	apt-get install -y sudo telnetd systemctl && \
	apt-get clean

RUN adduser -gecos --disabled-password --shell /bin/bash telnet

# change telnet user password
RUN echo 'telnet:telnet' | chpasswd

EXPOSE 23

CMD systemctl start inetd; while [ true ]; do sleep 60; done
