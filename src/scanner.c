#include "scanner.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <stdlib.h>
#include <time.h>


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
			// TODO: report
		}
    }

	return 0;
}
