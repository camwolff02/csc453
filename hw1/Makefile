CC = gcc

CFLAGS = -std=c99 -Wall -Wextra -pedantic -O2

TARGETS = memory-consumer

all: $(TARGETS)

memory-consumer: memory-consumer.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGETS)

.PHONY: all clean
