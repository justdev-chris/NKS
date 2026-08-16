#!/bin/sh
# build_freebsd.sh - Clone & build FreeBSD kernel for NKS

set -e

# Use current directory instead of hardcoded paths
WORKDIR=$(pwd)
OUTPUT_DIR="$WORKDIR/output"
KERNEL_CONF="$WORKDIR/kernel/conf/NKS"

echo "🐾 Building FreeBSD kernel for NKS..."
echo "Workdir: $WORKDIR"

# Check if we're on FreeBSD
if [ "$(uname)" != "FreeBSD" ]; then
    echo "⚠️  Not on FreeBSD. Skipping kernel build (GitHub Actions)."
    mkdir -p $OUTPUT_DIR
    touch $OUTPUT_DIR/kernel.dummy
    echo "✅ FreeBSD kernel build skipped (placeholder created)."
    exit 0
fi

# FreeBSD build code here (if we're actually on FreeBSD)
BUILD_DIR="/tmp/freebsd-build"
rm -rf $BUILD_DIR
mkdir -p $BUILD_DIR
cd $BUILD_DIR

git clone --depth 1 --branch releng/14.0 https://git.freebsd.org/src.git src

# Copy our kernel config
cp $KERNEL_CONF src/sys/amd64/conf/

cd src
make -j$(sysctl -n hw.ncpu) buildkernel KERNCONF=NKS

mkdir -p $OUTPUT_DIR/boot/kernel
make installkernel KERNCONF=NKS DESTDIR=$OUTPUT_DIR

echo "✅ FreeBSD kernel built successfully!"
