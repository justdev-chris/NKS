#!/bin/sh
# mkimage.sh - Create bootable NKS image

set -e

OUTPUT_DIR="/root/NKS/output"
ROOTFS_DIR="/root/NKS/rootfs"
KERNEL_DIR="$OUTPUT_DIR/boot"
IMG_FILE="$OUTPUT_DIR/nks.img"
ISO_FILE="$OUTPUT_DIR/nks.iso"
IMG_SIZE="512M"

echo "🐾 Creating NKS boot image..."

# Create image file
dd if=/dev/zero of=$IMG_FILE bs=1M count=512

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
cp /root/NKS/boot/loader.conf /mnt/boot/
cp /root/NKS/boot/rc.conf /mnt/etc/
cp /root/NKS/boot/fstab /mnt/etc/

# Install bootloader
gpart bootcode -b /boot/pmbr -p /boot/gptboot -i 1 $IMG_FILE

umount /mnt
mdconfig -d -u 0

# Create ISO (UEFI bootable)
mkdir -p /tmp/iso
cp -r $KERNEL_DIR/* /tmp/iso/boot/
cp -r $ROOTFS_DIR/* /tmp/iso/
mkisofs -U -R -b boot/cdboot -no-emul-boot -o $ISO_FILE /tmp/iso

echo "✅ NKS images built:"
echo "   USB: $IMG_FILE"
echo "   ISO: $ISO_FILE"
