# Makefile for RISC-V Linux Emulator
# "If it boots, it boots" - 2026

CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c99 -march=native
LDFLAGS = -lm

TARGET = riscv-emu
SOURCES = emu.c cpu.c loader.c serial.c
HEADERS = riscv.h
OBJECTS = $(SOURCES:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

distclean: clean
	rm -f *.bin *.elf vmlinux

run: $(TARGET)
	./$(TARGET)

# Download a pre-built Linux kernel for RISC-V
get-image:
	wget -O Image https://storage.googleapis.com/riscv-emu/Image
	wget -O opensbi.bin https://storage.googleapis.com/riscv-emu/opensbi.bin
	wget -O initramfs.cpio.gz https://storage.googleapis.com/riscv-emu/initramfs.cpio.gz

.PHONY: all clean distclean run get-image
