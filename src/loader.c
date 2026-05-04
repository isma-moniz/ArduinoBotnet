/**
 * This is a program for bruteforcing telnet credentials. I am roughly basing it on
 * https://github.com/petrocchi/brute_telnet.
 */
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>

struct telnet_config {
	char* username;
	char* password;
};

int main(int argc, char *argv[]) {
	char *username, *password;
	struct addrinfo *tel_addr;
	int verbose;
	
	if ((argc != 6) && (argc != 7)) {
		printf("\nUsage: %s <target ip> <port> <userfile> <passfile> <n thread> [options]\n\n"
		       "<userfile>\tUsername wordlist\n"
		       "<passdile>\tPassword wordlist\n"
		       "<n threads>\tNumber of parallel threads\n"
		       "\nOptions:\n\t-v\tVerbose mode\n\nExamples:\n"
		       "\t./loader 192.168.1.1 23 user.txt wordlist.txt 30\n"
		       "\t./loader 192.168.1.1 23 user.txt wordlist.txt 30 -v\n\n", argv[0]);
		return(1);
	}
	printf("[+] Target: %s:%s\n", argv[1], argv[2]);

	int n_threads = atoi(argv[5]);

	struct telnet_config tdata[n_threads];
	pthread_t thread_ids[n_threads];

	if( (argc != 6) || ( (argc == 7) && (!strcmp(argv[6], "-v"))) ) {
		verbose = 1;
	}

	if (getaddrinfo(argv[1], argv[2], NULL, &tel_addr) < 0) {
		perror("getaddrinfo error");
		return 1;
	}

	freeaddrinfo(tel_addr);
	return 0;
}
