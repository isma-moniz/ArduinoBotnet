# Project settings
debug ?= 0
SRC_DIR := ./src
BIN_DIR := ./bin

# Executables
LOADER_EXE := loader_amd64
SCANNER_EXE := scanner_amd64
BRUTE_EXE := brute

# Source files
LOADER_SRCS := loader.c
SCANNER_SRCS := scanner.c
BRUTE_SRCS := brute.c

# Compiler settings
CFLAGS := -Wextra -Wall
LFLAGS_LOADER := -pthread
LFLAGS_BRUTE := -lcurl

ifeq ($(debug), 1)
	CFLAGS := $(CFLAGS) -g -O0
else
	CFLAGS := $(CFLAGS) -O2
endif

# Targets
loader: dir $(SRC_DIR)/loader.c
	musl-gcc -static $(CFLAGS) $(LFLAGS_LOADER) -o $(BIN_DIR)/$(LOADER_EXE) $(foreach file,$(LOADER_SRCS),$(SRC_DIR)/$(file))

scanner: dir $(SRC_DIR)/scanner.c
	musl-gcc -static $(CFLAGS) -o $(BIN_DIR)/$(SCANNER_EXE) $(foreach file,$(SCANNER_SRCS),$(SRC_DIR)/$(file))

brute: dir $(BRUTE_OBJS)
	gcc $(CFLAGS) $(LFLAGS_BRUTE) -o $(BIN_DIR)/$(BRUTE_EXE) $(foreach file,$(BRUTE_OBJS),$(BUILD_DIR)/$(file))

.PHONY: clean
clean:
	@rm -rf $(BIN_DIR)

dir:
	@mkdir -p $(BIN_DIR) $(SRC_DIR)
