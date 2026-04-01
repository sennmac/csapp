CC=clang
CFLAGS=-std=c11 -Wall -Wextra -O0 -g -pthread

.PHONY: all clean

all: concurrency/race_counter concurrency/prodcons_queue concurrency/deadlock_demo

concurrency/race_counter: concurrency/race_counter.c
	$(CC) $(CFLAGS) -o $@ $<

concurrency/prodcons_queue: concurrency/prodcons_queue.c
	$(CC) $(CFLAGS) -o $@ $<

concurrency/deadlock_demo: concurrency/deadlock_demo.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f concurrency/race_counter concurrency/prodcons_queue concurrency/deadlock_demo
