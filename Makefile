# NKS Makefile
CC = cc
CFLAGS = -Wall -Wextra -O2 -std=c99 -I. -D_POSIX_C_SOURCE=199309L

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),FreeBSD)
    LDFLAGS = -lkvm -lutil -lpthread
else
    LDFLAGS = -lutil -lpthread
endif

TARGET = nks
SRCS = src/core/main.c \
       src/cpu/cpu.c \
       src/memory/mem.c \
       src/display/fb.c \
       src/audio/audio.c \
       src/input/kbd.c \
       src/panic/panic.c
OBJS = $(SRCS:.c=.o)

.PHONY: all clean install test build-freebsd build-rootfs mkimage qemu

all: $(TARGET) create-output
	@echo "✅ NKS binary built!"
	@echo "Run 'make image' to build the full bootable image."
	@ls -la $(TARGET) || true

create-output:
	mkdir -p output

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
	rm -rf output/

install: $(TARGET)
	cp $(TARGET) /usr/local/bin/

# Full image build (FreeBSD + NKS)
image: $(TARGET) create-output
	@echo "🐾 Building full NKS image..."
	@chmod +x scripts/*.sh
	./scripts/build_freebsd.sh || true
	./scripts/build_rootfs.sh || true
	./scripts/mkimage.sh || true
	@echo "✅ Image built! Check output/"
	@ls -la output/ || true

test: $(TARGET)
	./$(TARGET) test.rom

qemu: image
	./scripts/run_qemu.sh
