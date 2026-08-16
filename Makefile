# NKS Makefile
# NyaaKitStation - RISC-V catboy console on FreeBSD

CC = cc
CFLAGS = -Wall -Wextra -O2 -std=c99 -I.
LDFLAGS = -lkvm -lutil -lpthread

TARGET = nks
SRCS = src/core/main.c \
       src/cpu/cpu.c \
       src/memory/mem.c \
       src/display/fb.c \
       src/audio/audio.c \
       src/input/kbd.c \
       src/panic/panic.c
OBJS = $(SRCS:.c=.o)

.PHONY: all clean install test

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

install: $(TARGET)
	cp $(TARGET) /usr/local/bin/

test: $(TARGET)
	./$(TARGET) test.rom

# FreeBSD-specific
build-freebsd:
	./scripts/build_freebsd.sh

build-rootfs:
	./scripts/build_rootfs.sh

mkimage:
	./scripts/mkimage.sh

qemu:
	./scripts/run_qemu.sh
