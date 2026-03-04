CC = gcc
CFLAGS = -Wall -Wextra -g -std=c11 -Iinclude -D_XOPEN_SOURCE=700

SRCS = $(wildcard src/*.c) $(wildcard src/commands/*.c)

OBJS = $(patsubst src/%.c, build/%.o, $(SRCS))

TARGET = eit

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@

build/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf build $(TARGET)

.PHONY: all clean
