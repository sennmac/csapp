CC=clang
CFLAGS=-std=c11 -Wall -Wextra -O0 -g
TARGET=minish/minish

.PHONY: all run day1 day2 day3 shlab-tools clean

all: $(TARGET)

$(TARGET): minish/minish.c
	$(CC) $(CFLAGS) -o $@ $<

run: $(TARGET)
	./$(TARGET)

day1: $(TARGET)
	./$(TARGET) < minish/traces/day1_basic.txt

day2: $(TARGET)
	./$(TARGET) < minish/traces/day2_path_and_cd.txt

day3: $(TARGET)
	rm -f /tmp/week5_redirect.txt
	./$(TARGET) < minish/traces/day3_redirect.txt

shlab-tools:
	$(MAKE) -C ../../labs/shlab/shlab-handout myspin mysplit mystop myint

clean:
	rm -f $(TARGET)
