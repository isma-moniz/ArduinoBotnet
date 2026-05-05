/**
 * This is a program for bruteforcing telnet credentials. I am roughly basing it on
 * https://github.com/petrocchi/brute_telnet . Additionally, portions of the original code
 * may be found in loader.h.
 *
 * I have modified it in various ways, and attach below a copy of the original
 * license and author information.
 *
 * Modification authorship:
 * 
 * DATE:	05/02/2026
 * AUTHOR: 	Ismael Moniz
 * EMAIL:   hismamoniz@gmail.com
 * WEBSITE: https://ismasglade.com
 */
/*
 * brute_telnet.c - Telnet bruteforce, for penetration test.
 *
 * THIS TOOL IS FOR LEGAL PURPOSES ONLY!
 *
 * brute force crack the remote authentication service telnet, parallel
 * connection.
 *
 * Copyright (C) 2017 Luca Petrocchi <petrocchi@myoffset.me>
 *
 * DATE:	16/02/2017
 * AUTHOR:	Luca Petrocchi
 * EMAIL:	petrocchi@myoffset.me
 * WEBSITE	https://myoffset.me/
 * URL:		https://github.com/petrocchi
 *
 *
 *
 * brute_telnet is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * brute_telnet is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 *
 * For compile: gcc -o brute_telnet brute_telnet.c -pthread
 *
 */
#pragma once

#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>

#define BIGLINE 32 // let's try to avoid using this
#define DO 0xfd
#define WONT 0xfc
#define WILL 0xfb
#define DONT 0xfe
#define CMD 0xff
#define CMD_ECHO 1
#define CMD_WINDOW_SIZE 31
#define BUFLEN 20
#define BUFF_R_SIZE 2048
#define KNOWN_LOGIN_SIZE 3
#define KNOWN_PSW_SIZE 3
#define KNOWN_PRT_SIZE 6
#define KNOWN_BAD_SIZE 3

struct telnet_config {
	struct addrinfo *tel_addr;
	char* username;
	char* password;
	uint8_t* found;
	uint8_t verbose;
	uint16_t tforked;
};

void print_usage(char* pname);
char* getentry(FILE* fd, char* line);
uint8_t getrecord(FILE* fd_user, char* username, FILE* fd_pass, char* password);
uint8_t trycredentials(int sockfd, char *username, char *password);
void negotiate(int sock, unsigned char* buf, int len);
ssize_t sendch(int sockfd, char* buf_all);
int parse(char* buff, int type);
