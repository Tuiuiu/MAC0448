CC = gcc
CFLAGS = -Wall -ansi -pedantic
LDLIBS = -lpthread -lm

ircserver: cJSON.o ircserver.o users_list.o utils.o

cJSON.o users_list.o utils.o ircserver.u: cJSON.h users_list.h

.PHONY clean:
	rm *.o ircserver
