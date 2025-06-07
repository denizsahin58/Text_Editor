CC = gcc
CFLAGS = -Wall -Wextra
LDFLAGS = -lncurses

TARGET = cse232editor
SRC = cse232editor.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(TARGET)

.PHONY: all clean 