#!/bin/sh
# run_qemu.sh - Test NKS in QEMU

qemu-system-x86_64 \
    -m 256M \
    -drive file=output/nks.img,format=raw \
    -usb -device usb-kbd \
    -vga std \
    -soundhw hda \
    -net none \
    -display gtk
