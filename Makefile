# Project settings
debug ?= 0
SRC_DIR := ./src
BIN_DIR := ./bin

# Executables
LOADER_EXE := loader_amd64
SCANNER_EXE := scanner_amd64

# Source files
LOADER_SRCS := loader.c
SCANNER_SRCS := scanner.c

# Compiler settings
CFLAGS := -Wextra -Wall --pedantic
LFLAGS_LOADER := -pthread

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

.PHONY: clean
clean:
	@rm -rf $(BIN_DIR)

dir:
	@mkdir -p $(BIN_DIR) $(SRC_DIR)
