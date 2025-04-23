CC = gcc

CFLAGS = -std=c99 -Wall -Wextra -pedantic -O2 -D_XOPEN_SOURCE=700

TARGETS = slosh

all: $(TARGETS)

slosh: slosh.c
	$(CC) $(CFLAGS) -o $@ $<

debug: slosh.c
	$(CC) $(CFLAGS) -g -o $@ $<


clean:
	rm -f $(TARGETS) debug

.PHONY: all clean
