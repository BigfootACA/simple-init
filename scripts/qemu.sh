#!/bin/bash
set -e
pushd "$(dirname "$0")/.." >/dev/null
source scripts/functions.sh.inc
source scripts/environments.sh.inc
exec "${QEMU_BIN}" \
	-m "${QEMU_MEM}" \
	-enable-kvm \
	-display sdl \
	-kernel "${KERNEL}" \
	-append "${CMDLINE} $(stty size|awk '{print "LINES="$1" COLUMNS="$2}') $*" \
	-initrd "${INITRAMFS}" \
	-chardev stdio,id=char0 \
	-serial chardev:char0 \
	-drive file="${LOGFS}",format=raw,if=none,id=sda \
	-drive file="${MINIDISK}",format=raw,if=none,id=sdb \
	-device virtio-blk-pci,drive=sda \
	-device virtio-blk-pci,drive=sdb \
	-device qxl,xres=540,yres=960 \
	-vga none \
	-rtc base=localtime
