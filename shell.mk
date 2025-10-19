CC = gcc
CFLAGS = -Wall -Wextra -std=c99
TARGET = minish

all: $(TARGET)

$(TARGET): shell.c
	$(CC) $(CFLAGS) shell.c -o $(TARGET)

clean:
	rm -f $(TARGET)
