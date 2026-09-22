#!/bin/sh
# build_rootfs.sh - Create minimal rootfs for NKS

set -e

WORKDIR=$(pwd)
ROOTFS_DIR="$WORKDIR/rootfs"
OUTPUT_DIR="$WORKDIR/output"

echo "🐾 Building NKS rootfs..."

# Check if we're on FreeBSD
if [ "$(uname)" != "FreeBSD" ]; then
    echo "⚠️  Not on FreeBSD. Skipping rootfs build."
    mkdir -p "$OUTPUT_DIR"
    touch "$OUTPUT_DIR/rootfs.dummy"
    echo "✅ Rootfs build skipped."
    exit 0
fi

# Real FreeBSD rootfs build
rm -rf "$ROOTFS_DIR"
mkdir -p "$ROOTFS_DIR"

BASE_TXZ="/tmp/freebsd-base.txz"

echo "Fetching FreeBSD 14.3 base.txz..."

# FreeBSD 14.3 is an older release, so try the official release
# location first, then the FreeBSD archive.
BASE_URLS="
https://download.freebsd.org/releases/amd64/14.3-RELEASE/base.txz
https://archive.freebsd.org/old-releases/amd64/14.3-RELEASE/base.txz
"

rm -f "$BASE_TXZ"

downloaded=false

for URL in $BASE_URLS; do
    echo "Trying: $URL"

    if fetch -o "$BASE_TXZ" "$URL"; then
        if [ -s "$BASE_TXZ" ]; then
            downloaded=true
            echo "✅ Downloaded base.txz"
            break
        fi
    fi

    echo "⚠️ Failed: $URL"
done

if [ "$downloaded" != "true" ]; then
    echo "❌ Failed to download FreeBSD 14.3 base.txz"
    exit 1
fi

echo "Extracting base.txz..."
tar -xzf "$BASE_TXZ" -C "$ROOTFS_DIR"

# Remove unnecessary files
cd "$ROOTFS_DIR"
rm -rf boot/ rescue/ var/ tmp/ 2>/dev/null || true
rm -f bin/vi bin/ed bin/rmail 2>/dev/null || true

# Create minimal dev nodes
mkdir -p "$ROOTFS_DIR/dev"
cd "$ROOTFS_DIR/dev"
mknod fb0 c 198 0 2>/dev/null || true
mknod uhid0 c 197 0 2>/dev/null || true
mknod dsp0 c 30 0 2>/dev/null || true
mknod kbd0 c 0 0 2>/dev/null || true
mknod null c 2 2 2>/dev/null || true
mknod zero c 2 12 2>/dev/null || true

# Create init symlink
ln -sf /usr/local/bin/nks "$ROOTFS_DIR/bin/init"

echo "✅ NKS rootfs built successfully!"