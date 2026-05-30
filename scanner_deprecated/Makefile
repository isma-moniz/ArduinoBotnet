CC     = gcc
CFLAGS = -Wall -Wextra -O2
TARGET = scanner
TEST   = scanner_test
SRCS   = main.c scanner.c
TSRCS  = main_test.c scanner.c

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

clean:
	rm -f $(TARGET) $(TEST)
