#pragma once
#include <stdint.h>

#define TELNET_PORT 23

int check_open(char* ip, char* port);
static uint32_t random_ip_in_subnet(uint32_t subnet, uint32_t mask);
static uint32_t prefix_to_mask(int prefix_len);
