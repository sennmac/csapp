CC=clang
CFLAGS=-std=c11 -Wall -Wextra -O0 -g
TARGET=allocator/allocator_demo

.PHONY: all day2 day3 day4 clean

all: $(TARGET)

$(TARGET): allocator/allocator.c allocator/allocator_demo.c allocator/allocator.h
	$(CC) $(CFLAGS) -o $@ allocator/allocator.c allocator/allocator_demo.c

day2: $(TARGET)
	./$(TARGET) allocator/traces/day2_bump.trace 256

day3: $(TARGET)
	./$(TARGET) allocator/traces/day3_freelist.trace 256

day4: $(TARGET)
	./$(TARGET) allocator/traces/day4_coalesce.trace 192

clean:
	rm -f $(TARGET)
