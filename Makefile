# NKS Makefile
CC = cc
CFLAGS = -Wall -Wextra -O2 -I. -D_POSIX_C_SOURCE=199309L

UNAME_S != uname -s 2>/dev/null || echo "Linux"
.if $(UNAME_S) == "FreeBSD"
LDFLAGS = -lkvm -lutil -lpthread -lm
.else
LDFLAGS = -lutil -lpthread -lm
.endif

TARGET = nks
SRCS = src/core/main.c \
       src/cpu/cpu.c \
       src/memory/mem.c \
       src/display/fb.c \
       src/audio/audio.c \
       src/input/kbd.c \
       src/panic/panic.c
OBJS = $(SRCS:.c=.o)

.PHONY: all clean install test image qemu create-output

all: $(TARGET) create-output
	@echo "✅ NKS binary built!"
	@ls -la $(TARGET) || true

create-output:
	mkdir -p output

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
	rm -rf output/

install: $(TARGET)
	cp $(TARGET) /usr/local/bin/

image: $(TARGET) create-output
	@echo "🐾 Building full NKS image..."
	@chmod +x scripts/*.sh
	./scripts/build_freebsd.sh || true
	./scripts/build_rootfs.sh || true
	./scripts/mkimage.sh || true
	@ls -la output/ || true

test: $(TARGET)
	./$(TARGET) test.rom

qemu: image
	./scripts/run_qemu.sh
