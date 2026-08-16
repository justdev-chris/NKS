#!/bin/sh
# build_freebsd.sh - Clone & build FreeBSD kernel for NKS

set -e

FREEBSD_REPO="https://git.freebsd.org/src.git"
FREEBSD_BRANCH="releng/14.0"
BUILD_DIR="/tmp/freebsd-build"

echo "🐾 Building FreeBSD kernel for NKS..."

# Clean old build
rm -rf $BUILD_DIR
mkdir -p $BUILD_DIR
cd $BUILD_DIR

# Clone FreeBSD source
git clone --depth 1 --branch $FREEBSD_BRANCH $FREEBSD_REPO src

# Copy our kernel config
cp /root/NKS/kernel/conf/NKS src/sys/amd64/conf/

# Build kernel
cd src
make -j$(sysctl -n hw.ncpu) buildkernel KERNCONF=NKS

# Install kernel to output
mkdir -p /root/NKS/output/boot/kernel
make installkernel KERNCONF=NKS DESTDIR=/root/NKS/output

echo "✅ FreeBSD kernel built successfully!"
