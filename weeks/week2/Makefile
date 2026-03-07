CC=clang
CFLAGS=-std=c11 -Wall -Wextra -O2 -g

.PHONY: all run clean

all: bench

bench: bench.c
	$(CC) $(CFLAGS) -o $@ $<

run: bench
	./bench

clean:
	rm -f bench
