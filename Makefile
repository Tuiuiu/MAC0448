BIN := ircserver

CFLAGS = -ansi -Wall -pedantic
LDFLAGS = -lpthread -lm

CC = gcc

SRC := $(filter-out $(BIN).c,$(wildcard *.c))
OBJ := $(SRC:.c=.o)

$(BIN): $(OBJ) $(BIN).o
	$(CC) $^ -o $@ $(LDFLAGS)

$(BIN).o: %.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ): %.o: %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f *.o $(BIN)
