# Linux target run in initramfs

## 1. Build Main

```bash
bash scripts/build.sh
```
output: `build/simple-init` (ELF Executable)

## 2. Generate minimal initramfs

```bash
bash scripts/gen-minimal-initramfs.sh
```
output:  `/tmp/initramfs.img` (ASCII cpio archive compress with gzip)

if you want a big initramfs with commands, you can use `gen-initramfs.sh` instead `gen-minimal-initramfs.sh`

## 3. Generate testing image (just once)

```bash
bash scripts/gen-logfs.sh
bash scripts/gen-miniroot.sh
```

output: `/tmp/logfs.img` (FAT16 filesystem with MBR label) for save logs (LOGFS)

output: `/tmp/minidisk.img` (EXT4 filesystem image) for a test switchroot

## 4. Run QEMU

```bash
bash scripts/qemu.sh
```
NOTICE: you need a `prebuilts/vmlinuz` and `prebuilts/kernel.txz` for boot linux