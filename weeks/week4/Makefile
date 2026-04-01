CC=clang
COMMON_CFLAGS=-std=c11 -Wall -Wextra -g -fno-omit-frame-pointer
UNSAFE_CFLAGS=$(COMMON_CFLAGS) -O0 -fno-stack-protector -Wno-array-bounds
PROTECTED_CFLAGS=$(COMMON_CFLAGS) -O0 -fstack-protector-strong -DPROTECTED_BUILD=1

.PHONY: all crash protected ret2func disasm clean

all: overflow_demo overflow_demo_protected ret2func_demo

overflow_demo: overflow_demo.c
	$(CC) $(UNSAFE_CFLAGS) -o $@ $<

overflow_demo_protected: overflow_demo.c
	$(CC) $(PROTECTED_CFLAGS) -o $@ $<

ret2func_demo: ret2func_demo.c
	$(CC) $(UNSAFE_CFLAGS) -o $@ $<

crash: overflow_demo
	./overflow_demo

protected: overflow_demo_protected
	./overflow_demo_protected

ret2func: ret2func_demo
	./ret2func_demo

disasm: ret2func_demo
	otool -tvV ret2func_demo > ret2func_demo.disasm
	@echo "wrote ret2func_demo.disasm"

clean:
	rm -f overflow_demo overflow_demo_protected ret2func_demo ret2func_demo.disasm
	rm -rf overflow_demo.dSYM overflow_demo_protected.dSYM ret2func_demo.dSYM
