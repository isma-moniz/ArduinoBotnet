# Project settings
debug ?= 0
SRC_DIR := ./src
BUILD_DIR := ./build
BIN_DIR := ./bin

# Executables
LOADER_EXE := loader_amd64

# Object files
LOADER_OBJS := loader.o

# Compiler settings
CFLAGS := -Wextra -Wall --pedantic
LFLAGS_LOADER := -pthread

ifeq ($(debug), 1)
	CFLAGS := $(CFLAGS) -g -O0
else
	CFLAGS := $(CFLAGS) -O2
endif

# Targets
loader: dir $(LOADER_OBJS)
	gcc $(CFLAGS) $(LFLAGS_LOADER) -o $(BIN_DIR)/$(LOADER_EXE) $(foreach file,$(LOADER_OBJS),$(BUILD_DIR)/$(file))

%.o: dir $(SRC_DIR)/%.c
	gcc $(CFLAGS) -o $(BUILD_DIR)/$*.o -c $(SRC_DIR)/$*.c

.PHONY: clean
clean:
	@rm -rf $(BUILD_DIR) $(BIN_DIR)

dir:
	@mkdir -p $(BUILD_DIR) $(BIN_DIR) $(SRC_DIR)
