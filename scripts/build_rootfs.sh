#!/bin/sh
# build_rootfs.sh - Create minimal rootfs for NKS

set -e

ROOTFS_DIR="/root/NKS/rootfs"
BASE_TXZ="/tmp/freebsd-base.txz"

echo "🐾 Building NKS rootfs..."

# Clean old rootfs
rm -rf $ROOTFS_DIR
mkdir -p $ROOTFS_DIR

# Fetch FreeBSD base tarball
if [ ! -f $BASE_TXZ ]; then
    fetch -o $BASE_TXZ https://download.freebsd.org/releases/amd64/14.0-RELEASE/base.txz
fi

# Extract base
tar -xzf $BASE_TXZ -C $ROOTFS_DIR

# Remove unnecessary files
cd $ROOTFS_DIR
rm -rf boot/ rescue/ var/ tmp/
rm -f bin/vi bin/ed bin/rmail

# Keep only essential libs
# (We strip more later)

# Create minimal dev nodes
mkdir -p $ROOTFS_DIR/dev
cd $ROOTFS_DIR/dev
mknod fb0 c 198 0
mknod uhid0 c 197 0
mknod dsp0 c 30 0
mknod kbd0 c 0 0
mknod null c 2 2
mknod zero c 2 12

# Create init symlink
ln -sf /usr/local/bin/nks $ROOTFS_DIR/bin/init

echo "✅ NKS rootfs built successfully!"
