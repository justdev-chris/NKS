#!/bin/sh
# build_freebsd.sh - Clone & build FreeBSD kernel for NKS

set -e

WORKDIR=$(pwd)
OUTPUT_DIR="$WORKDIR/output"
KERNEL_CONF="$WORKDIR/kernel/conf/NKS"

echo "🐾 Building FreeBSD kernel for NKS..."
echo "Workdir: $WORKDIR"

if [ "$(uname)" != "FreeBSD" ]; then
    echo "⚠️  Not on FreeBSD. Skipping kernel build."
    mkdir -p $OUTPUT_DIR/boot/kernel
    touch $OUTPUT_DIR/boot/kernel/kernel
    exit 0
fi

BUILD_DIR="/tmp/freebsd-build"
rm -rf $BUILD_DIR
mkdir -p $BUILD_DIR
cd $BUILD_DIR

echo "Cloning FreeBSD 14.3 source..."
git clone --depth 1 --branch releng/14.3 https://git.freebsd.org/src.git src

if [ ! -f "$KERNEL_CONF" ]; then
    echo "❌ Kernel config not found: $KERNEL_CONF"
    exit 1
fi

echo "Copying kernel config..."
cp $KERNEL_CONF src/sys/amd64/conf/

echo "Building kernel..."
cd src
make -j$(sysctl -n hw.ncpu) buildkernel KERNCONF=NKS

echo "Installing kernel..."
mkdir -p $OUTPUT_DIR/boot/kernel
make installkernel KERNCONF=NKS DESTDIR=$OUTPUT_DIR

echo "✅ FreeBSD kernel built successfully!"
ls -lh $OUTPUT_DIR/boot/kernel/kernel
