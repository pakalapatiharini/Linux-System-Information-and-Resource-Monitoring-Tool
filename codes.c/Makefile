CC = gcc
CFLAGS = -Wall -Wextra -std=c11
PTHREAD = -pthread

all: server client

server: server.c monitor.c logger.c monitor.h logger.h
	$(CC) $(CFLAGS) $(PTHREAD) server.c monitor.c logger.c -o server

client: client.c
	$(CC) $(CFLAGS) client.c -o client

clean:
	rm -f server client
