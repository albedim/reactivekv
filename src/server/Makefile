CC = gcc
CFLAGS = -Wall -pthread -g
SRCS = kvdb_server.c kvdb_handler.c kvdb_store.c
HEADERS = kvdb.h
OBJS = $(SRCS:.c=.o)
TARGET = server

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
