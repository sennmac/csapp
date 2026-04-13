CC=clang
CFLAGS=-std=c11 -Wall -Wextra -O0 -g
TARGET=proxy/proxy

.PHONY: all tiny clean

all: $(TARGET)

$(TARGET): proxy/proxy.c
	$(CC) $(CFLAGS) -o $@ $< -lpthread

tiny:
	$(MAKE) -C ../../labs/proxylab/proxylab-handout/tiny

clean:
	rm -f $(TARGET)
