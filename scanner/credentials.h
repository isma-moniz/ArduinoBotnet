#ifndef CREDENTIALS_H
#define CREDENTIALS_H

#include "scanner.h"

static const Credential DEFAULT_CREDS[] = {
    {"root", "root"},
    {"root",    "admin"},
    {"root",    ""},
    {"root",    "vizxv"},
    {"root",    "xc3511"},
    {"admin",   "admin"},
    {"admin",   "password"},
    {"admin",   ""},
    {"support", "support"},
    {"user",    "user"},
    {"guest",   "guest"},
};

#define NUM_DEFAULT_CREDS (sizeof(DEFAULT_CREDS) / sizeof(DEFAULT_CREDS[0]))

#endif
