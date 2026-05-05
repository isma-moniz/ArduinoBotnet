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
#include "loader.h"

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>

void print_usage(char* pname) {
	printf("\nUsage: %s <target ip> <port> <userfile> <passfile> <n thread> [options]\n\n"
		   "<userfile>\tUsername wordlist\n"
		   "<passdile>\tPassword wordlist\n"
		   "<n threads>\tNumber of parallel threads\n"
		   "\nOptions:\n\t-v\tVerbose mode\n\nExamples:\n"
		   "\t./loader 192.168.1.1 23 user.txt wordlist.txt 30\n"
		   "\t./loader 192.168.1.1 23 user.txt wordlist.txt 30 -v\n\n", pname);
}

int main(int argc, char *argv[]) {
	char *username, *password;
	uint8_t verbose = 0, found = 0;
	char line_user[BIGLINE];
	char line_password[BIGLINE];
	FILE *fd_user, *fd_pass;

	struct addrinfo *tel_addr = (struct addrinfo*)malloc(sizeof(struct addrinfo));
	memset(tel_addr, 0, sizeof(struct addrinfo));
	
	if ((argc != 6) && (argc != 7)) {
		print_usage(argv[0]);
		return(1);
	}
	printf("[+] Target: %s:%s\n", argv[1], argv[2]);

	int n_threads = atoi(argv[5]);

	struct telnet_config tdata[n_threads];
	pthread_t thread_ids[n_threads];

	if( (argc == 7) && (!strcmp(argv[6], "-v")) ) {
		verbose = 1;
	} else {
		print_usage(argv[1]);
		return(1);
	}

	if (getaddrinfo(argv[1], argv[2], NULL, &tel_addr) < 0) {
		perror("getaddrinfo error\n");
		return 1;
	}
	
	// initialize thread data
	for (int i = 0; i < n_threads; ++i) {

		// tel_addr is read only afaik, so all threads can share the same heap allocated one
		tdata[i].tel_addr = tel_addr;
		tdata[i].found = &found;
		tdata[i].tforked = 0;
		tdata[i].verbose = verbose;
	}

	username = line_user;
	password = line_password;

	if ((fd_user = fopen(argv[3], "r")) == NULL) {
		perror("username file io error\n");
		return(1);
	}

	if ((fd_pass = fopen(argv[4], "r")) == NULL) {
		perror("password file io error\n");
		return(1);
	}

	if ( getentry(fd_user, username) == NULL) {
		perror("username getentry error\n");
	}

	int i = 0;
	while (!found) {
		if (i == n_threads) {
			i = 0;
		}

		if (tdata[i].tforked == 0) {
			tdata[i].tforked = 1; // set busy
			
			// if ( (getrecord) ) *WIP*
		}
	}

	freeaddrinfo(tel_addr);
	return 0;
}

char* getentry(FILE* fd, char* line) {
	char* cut;

	bzero(line, BIGLINE);

	if (fgets(line, BIGLINE, fd)) {
		if ( (cut = strchr(line, '\n')) ) {
			*cut = '\0';
		}
		return line;
	}

	return NULL;
}

uint8_t getrecord(FILE* fd_user, char* username, FILE* fd_pass, char* password) {
	if (getentry(fd_pass, password) == NULL) {
		perror("password io error\n");
		return 1;
	}

	if (password[0] == '\0') {
		if (getentry(fd_user, username) == NULL) {
			perror("username io error\n");
			return 1;
		}
		if (username[0] == '\0') {
			perror("no more credentials to try\n");
			return 1;
		}
		fseek(fd_pass, 0, SEEK_SET);
		if (getentry(fd_pass, password) == NULL) {
			perror("password io error\n");
			return 1;
		}
	}
	return 0;
}
