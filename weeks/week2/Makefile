CC=clang
CFLAGS=-std=c11 -Wall -Wextra -O2 -g

.PHONY: all run clean

all: bench cache_sim transpose_baseline transpose_blocking

bench: bench.c
	$(CC) $(CFLAGS) -o $@ $<

cache_sim: cache_sim.c
	$(CC) $(CFLAGS) -o $@ $<

transpose_baseline: transpose_baseline.c
	$(CC) $(CFLAGS) -o $@ $<

transpose_blocking: transpose_blocking.c
	$(CC) $(CFLAGS) -o $@ $<

run: day1 day2 day3 day4

day1: bench
	./bench 1024 5 16

day2: cache_sim
	./cache_sim -s 2 -E 1 -b 3 -t traces/sample.trace

day3: transpose_baseline
	./transpose_baseline 1024 5

day4: transpose_blocking
	./transpose_blocking 1024 5 16

clean:
	rm -f bench cache_sim transpose_baseline transpose_blocking
