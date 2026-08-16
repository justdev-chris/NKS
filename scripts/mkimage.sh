#!/bin/sh
# mkimage.sh - Create bootable NKS image and ISO

set -e

WORKDIR=$(pwd)
OUTPUT_DIR="$WORKDIR/output"
ROOTFS_DIR="$WORKDIR/rootfs"

echo "🐾 Creating NKS boot image..."

mkdir -p $OUTPUT_DIR

# Check if we're on FreeBSD
if [ "$(uname)" != "FreeBSD" ]; then
    echo "⚠️  Not on FreeBSD. Creating placeholder images."
    dd if=/dev/zero of=$OUTPUT_DIR/nks.img bs=1M count=64 2>/dev/null
    dd if=/dev/zero of=$OUTPUT_DIR/nks.iso bs=1M count=64 2>/dev/null
    echo "NKS placeholder image (build on FreeBSD for real image)" > $OUTPUT_DIR/README.txt
    echo "✅ Placeholder images created."
    exit 0
fi

# ============================================================
# REAL FREEBSD IMAGE BUILD
# ============================================================

KERNEL_DIR="$OUTPUT_DIR/boot"
IMG_FILE="$OUTPUT_DIR/nks.img"
ISO_FILE="$OUTPUT_DIR/nks.iso"

# Check if kernel exists
if [ ! -f "$KERNEL_DIR/kernel" ]; then
    echo "❌ Kernel not found! Run build_freebsd.sh first."
    exit 1
fi

# Check if rootfs exists
if [ ! -f "$ROOTFS_DIR/bin/init" ]; then
    echo "❌ Rootfs not found! Run build_rootfs.sh first."
    exit 1
fi

echo "✅ Kernel and rootfs found"

# ============================================================
# Build HARD DISK IMAGE (.img)
# ============================================================

echo "Creating UFS rootfs image..."
ROOTFS_IMG="$OUTPUT_DIR/rootfs.ufs"
rm -f $ROOTFS_IMG

# Create UFS filesystem image
dd if=/dev/zero of=$ROOTFS_IMG bs=1M count=500 status=progress
newfs -U -m 0 $ROOTFS_IMG

# Mount and populate
MDDEV=$(mdconfig -a -f $ROOTFS_IMG)
mount /dev/$MDDEV /mnt

echo "Copying rootfs..."
cp -r $ROOTFS_DIR/* /mnt/

# Copy kernel
mkdir -p /mnt/boot
cp -r $KERNEL_DIR/* /mnt/boot/

# Copy boot configs
mkdir -p /mnt/boot
cp $WORKDIR/boot/loader.conf /mnt/boot/
mkdir -p /mnt/etc
cp $WORKDIR/boot/rc.conf /mnt/etc/
cp $WORKDIR/boot/fstab /mnt/etc/

# Create dev nodes
mkdir -p /mnt/dev
cd /mnt/dev
mknod fb0 c 198 0 2>/dev/null || true
mknod uhid0 c 197 0 2>/dev/null || true
mknod dsp0 c 30 0 2>/dev/null || true
mknod null c 2 2 2>/dev/null || true
mknod zero c 2 12 2>/dev/null || true

echo "Unmounting..."
umount /mnt
mdconfig -d -u $MDDEV

# Create disk image with partition table
echo "Creating .img with mkimg..."
mkimg -s gpt \
    -p efi:=/boot/boot1.efifat \
    -p freebsd-ufs:$ROOTFS_IMG \
    -o $IMG_FILE

# ============================================================
# Build CD-ROM IMAGE (.iso)
# ============================================================

echo "Creating CD-ROM image..."
ISO_DIR="$OUTPUT_DIR/iso_stage"
rm -rf $ISO_DIR
mkdir -p $ISO_DIR/boot

# Copy kernel
cp -r $KERNEL_DIR/* $ISO_DIR/boot/

# Copy rootfs
cp -r $ROOTFS_DIR/* $ISO_DIR/

# Copy boot configs
cp $WORKDIR/boot/loader.conf $ISO_DIR/boot/
mkdir -p $ISO_DIR/etc
cp $WORKDIR/boot/rc.conf $ISO_DIR/etc/
cp $WORKDIR/boot/fstab $ISO_DIR/etc/

# Create bootable ISO using makefs + mkisofs
# FreeBSD uses 'mkisofs' from cdrtools or 'xorriso'
if command -v mkisofs >/dev/null 2>&1; then
    mkisofs -R -b boot/cdboot -no-emul-boot -o $ISO_FILE $ISO_DIR
elif command -v xorriso >/dev/null 2>&1; then
    xorriso -as mkisofs -R -b boot/cdboot -no-emul-boot -o $ISO_FILE $ISO_DIR
else
    echo "⚠️  No ISO tool found. Installing cdrtools..."
    pkg install -y cdrtools
    mkisofs -R -b boot/cdboot -no-emul-boot -o $ISO_FILE $ISO_DIR
fi

# Cleanup
rm -rf $ISO_DIR

echo "✅ Real FreeBSD images built successfully!"
ls -lh $IMG_FILE $ISO_FILE
file $IMG_FILE $ISO_FILE
