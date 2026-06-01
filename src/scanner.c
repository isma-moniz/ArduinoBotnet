#define _GNU_SOURCE
#include "scanner.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <netdb.h>
#include <stdlib.h>
#include <time.h>

const char* report_ip = "172.18.0.1"; // WARNING: hardcoded for now
const char* report_port = "5000";

static uint32_t random_ip_in_subnet(uint32_t subnet, uint32_t mask) 
{
    uint32_t host_part = rand() & ~mask;
 
    // ignore broadcast and network itself
    if (host_part == 0 || host_part == ~mask) host_part = 1;
    return (subnet & mask) | host_part;
}

static uint32_t prefix_to_mask(int prefix_len) {
    if (prefix_len == 0) return 0;
    return (~0U) << (32 - prefix_len);
}

// returns 0 for open, -1 for nope, 1 for error
int check_open(char* ip, char* port) {
	struct timeval tv = {0};
	tv.tv_sec = 2;
	tv.tv_usec = 0;
	int open = -1;
	struct addrinfo hints;
	struct addrinfo* serv_addr;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET; // ipv4
	hints.ai_socktype = SOCK_STREAM;

	if (getaddrinfo(ip, port, &hints, &serv_addr)) {
		perror("getaddrinfo error\n");
		return 1;
	}
	
	struct addrinfo* temp = serv_addr;
	int sockfd, status;
	while (temp->ai_next) {
		sockfd = socket(temp->ai_family, temp->ai_socktype,
				temp->ai_protocol);

		setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
		setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
		if (sockfd < 0) {
			printf("Port %s is NOT open. Sockfd < 0\n", port);
		} else {
			status = connect(sockfd, temp->ai_addr, temp->ai_addrlen);
			if (status < 0) {
				printf("Port %s is NOT open.\n", port);
			} else {
				printf("Port %s is open.\n", port);
				open = 0;
			}
			close(sockfd);
		}
		temp = temp->ai_next;
	}
	sockfd = socket(temp->ai_family, temp->ai_socktype,
			temp->ai_protocol);
		setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
		setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	if (sockfd < 0) {
		printf("Port %s is NOT open. Sockfd < 0 (2)\n", port);
	} else {
		status = connect(sockfd, temp->ai_addr, temp->ai_addrlen);
		if (status < 0) {
			printf("Port %s is NOT open.\n", port);
		} else {
			printf("Port %s is open.\n", port);
			open = 0;
		}
		close(sockfd);
	}
	freeaddrinfo(serv_addr);
	return open;
}

int main(int argc, char* argv[]) {
	srand(time(NULL));
	int rc;
	char port[6] = {0};
	sprintf(port, "%d", TELNET_PORT);
	if (argc < 2) {
		printf("Usage: %s <target_IPv4_subnet> <subnet_mask_len>\n", argv[0]);
	}
	char subnet_str[16] = {0};
	if (!strcpy(subnet_str, argv[1])) {
		perror("Malformed ip subnet!\n");
	}
	
	int prefix_len = atoi(argv[2]);

    uint32_t subnet = ntohl(inet_addr(subnet_str));
    uint32_t mask   = prefix_to_mask(prefix_len);

    printf("[*] Generating 5 random IPs in %s/%d\n\n", subnet_str, prefix_len);

    for (int i = 0; i < 5; i++) {
        uint32_t ip = random_ip_in_subnet(subnet, mask);

        struct in_addr in = { .s_addr = htonl(ip) };
		char* ip_str = inet_ntoa(in);
        printf("[%d] Generated IP: %s\n", i + 1, ip_str);
		
		rc = check_open(ip_str, port);
		if (rc == 0) {
			report(ip_str);
		}
    }

	return 0;
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

int report(char* ip) {
	struct addrinfo *report_addr;
	int sockfd;
	if (getaddrinfo(report_ip ,report_port, NULL, &report_addr) < 0) {
		perror("getaddrinfo error\n");
		return 1;
	}

	struct sockaddr_in* ipv4 = (struct sockaddr_in *)report_addr->ai_addr;
	struct sockaddr* sock = report_addr->ai_addr;

	if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket creation error\n");
	}

	if ( (connect(sockfd, sock, sizeof(*sock))) < 0) {
		perror("socket connection error\n");
	}

	size_t len = strlen(ip) + 3;
	char *report_info = malloc(len);
	if (!report_info) {
		perror("malloc error\n");
		return 1;
	}

	snprintf(report_info, len, "s:%s", ip);

	int rc;
	if ((rc = sendch(sockfd, report_info)) < 0) {
		printf("sendch error on report!\n");
		return 1;
	}

	printf("sent %s\n", report_info);

	freeaddrinfo(report_addr);
	free(report_info);
	close(sockfd);
	return 0;
}
