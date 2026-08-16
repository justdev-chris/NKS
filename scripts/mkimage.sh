#!/bin/sh
# mkimage.sh - Create bootable NKS image

set -e

WORKDIR=$(pwd)
OUTPUT_DIR="$WORKDIR/output"
ROOTFS_DIR="$WORKDIR/rootfs"

echo "🐾 Creating NKS boot image..."

mkdir -p $OUTPUT_DIR

# Check if we're on FreeBSD
if [ "$(uname)" != "FreeBSD" ]; then
    echo "⚠️  Not on FreeBSD. Creating placeholder image for GitHub Actions."
    dd if=/dev/zero of=$OUTPUT_DIR/nks.img bs=1M count=64 2>/dev/null
    dd if=/dev/zero of=$OUTPUT_DIR/nks.iso bs=1M count=64 2>/dev/null
    echo "NKS placeholder image (build on FreeBSD for real image)" > $OUTPUT_DIR/README.txt
    ls -la $OUTPUT_DIR/
    echo "✅ Placeholder images created."
    exit 0
fi

# ============================================================
# REAL FREEBSD IMAGE BUILD (only runs on actual FreeBSD)
# ============================================================

KERNEL_DIR="$OUTPUT_DIR/boot"
IMG_FILE="$OUTPUT_DIR/nks.img"
ISO_FILE="$OUTPUT_DIR/nks.iso"

# Create image file (2GB for real FreeBSD)
dd if=/dev/zero of=$IMG_FILE bs=1M count=2048

# Partition
gpart create -s GPT $IMG_FILE
gpart add -t efi -s 100M $IMG_FILE
gpart add -t freebsd-ufs $IMG_FILE

# Format
mdconfig -a -f $IMG_FILE -u 0
newfs_msdos /dev/md0p1
newfs /dev/md0p2

# Mount and copy files
mount /dev/md0p2 /mnt
mkdir -p /mnt/boot
cp -r $KERNEL_DIR/* /mnt/boot/
cp -r $ROOTFS_DIR/* /mnt/

# Copy boot configs
cp $WORKDIR/boot/loader.conf /mnt/boot/
cp $WORKDIR/boot/rc.conf /mnt/etc/
cp $WORKDIR/boot/fstab /mnt/etc/

# Install bootloader
gpart bootcode -b /boot/pmbr -p /boot/gptboot -i 1 $IMG_FILE

umount /mnt
mdconfig -d -u 0

echo "✅ Real FreeBSD image built!"
