CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -pedantic -O2 -D_XOPEN_SOURCE=700
LDFLAGS = -pthread -lm
TARGETS = scheduler

all: $(TARGETS)

thread: scheduler_skeleton.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

debug: scheduler_skeleton.c
	$(CC) $(CFLAGS) -g -o $@ $< $(LDFLAGS)

clean:
	rm -f $(TARGETS) debug submission.tgz

tar:
	tar -zcvf submission.tgz scheduler_skeleton.c README Makefile

.PHONY: all clean
