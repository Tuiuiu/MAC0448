CC = gcc
CFLAGS = -Wall -ansi -pedantic
LDLIBS = -lpthread

ircserver: ircserver.o users_list.o

users_list.o ircserver.u: users_list.h

.PHONY clean:
	rm *.o ircserver
