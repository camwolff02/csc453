CC = gcc

CFLAGS = -std=c99 -Wall -Wextra -pedantic -O2 -D_XOPEN_SOURCE=700

TARGETS = thread

all: $(TARGETS)

thread: thread_lab_skeleton.c
	$(CC) $(CFLAGS) -o $@ $<

debug: thread_lab_skeleton.c
	$(CC) $(CFLAGS) -g -o $@ $<


clean:
	rm -f $(TARGETS)

.PHONY: all clean
