CC=clang
CFLAGS=-std=c11 -Wall -Wextra -O0 -g -fno-omit-frame-pointer
TARGET=mini_bomb
DISASM=$(TARGET).disasm

.PHONY: all run run-solution disasm clean

all: $(TARGET)

$(TARGET): mini_bomb.c
	$(CC) $(CFLAGS) -o $@ $<

run: $(TARGET)
	./$(TARGET)

run-solution: $(TARGET)
	printf "frames\n1 2 5 10 19 34\n4 562\n10 37\n" | ./$(TARGET)

disasm: $(TARGET)
	otool -tvV $(TARGET) > $(DISASM)
	@echo "wrote $(DISASM)"

clean:
	rm -f $(TARGET) $(DISASM)
