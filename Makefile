CC = gcc
CFLAGS = -Wall -ansi -pedantic
LDLIBS = -lpthread

ircserver: ircserver.c

.PHONY clean:
	rm *.o ircserver
