CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -pedantic -O2 -D_XOPEN_SOURCE=700
LDFLAGS = -pthread -lm
TARGETS = thread

all: $(TARGETS)

thread: thread_lab_skeleton.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

debug: thread_lab_skeleton.c
	$(CC) $(CFLAGS) -g -o $@ $< $(LDFLAGS)

clean:
	rm -f $(TARGETS)

tar:
	tar -zcvf submission.tgz thread_lab_skeleton.c README Makefile

.PHONY: all clean
