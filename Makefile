# Project settings
debug ?= 0
SRC_DIR := ./src
BIN_DIR := ./bin

# Executables (FIXED: All use immediate assignment)
LOADER_EXE  := loader_amd64
SCANNER_EXE := scanner_amd64
BRUTE_EXE   := brute

# Source files
LOADER_SRCS  := loader.c
SCANNER_SRCS := scanner.c
BRUTE_SRCS   := brute.c

# Compiler settings
CFLAGS := -Wextra -Wall
LFLAGS_LOADER := -pthread

ifeq ($(debug), 1)
    CFLAGS := $(CFLAGS) -g -O0
else
    CFLAGS := $(CFLAGS) -O2
endif

# Default Target (NEW: Typing 'make' now compiles everything)
.PHONY: all
all: loader scanner brute

# Targets
loader: dir $(SRC_DIR)/$(LOADER_SRCS)
	musl-gcc -static $(CFLAGS) $(LFLAGS_LOADER) -o $(BIN_DIR)/$(LOADER_EXE) $(SRC_DIR)/$(LOADER_SRCS)

scanner: dir $(SRC_DIR)/$(SCANNER_SRCS)
	musl-gcc -static $(CFLAGS) -o $(BIN_DIR)/$(SCANNER_EXE) $(SRC_DIR)/$(SCANNER_SRCS)

brute: dir $(SRC_DIR)/$(BRUTE_SRCS)
	gcc $(CFLAGS) -o $(BIN_DIR)/$(BRUTE_EXE) $(SRC_DIR)/$(BRUTE_SRCS) -lcurl

.PHONY: clean
clean:
	@rm -rf $(BIN_DIR)

dir:
	@mkdir -p $(BIN_DIR) $(SRC_DIR)