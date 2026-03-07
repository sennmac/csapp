CC=clang
CFLAGS=-std=c11 -Wall -Wextra -O0 -g

.PHONY: all test clean

all: test

test: demos/bit_print demos/shift_behavior demos/overflow_check demos/float_bits
	@echo "[OK] built demos"

# --- demos ---
demos/bit_print: demos/bit_print.c
	$(CC) $(CFLAGS) -o $@ $<

demos/shift_behavior: demos/shift_behavior.c
	$(CC) $(CFLAGS) -o $@ $<

demos/overflow_check: demos/overflow_check.c
	$(CC) $(CFLAGS) -o $@ $<

demos/float_bits: demos/float_bits.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f demos/bit_print demos/shift_behavior demos/overflow_check demos/float_bits
