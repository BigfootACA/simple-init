# Linux目标运行在initramfs

## 1. 编译主程序

```bash
bash scripts/build.sh -DENABLE_SDL2=ON
```

输出: `build/simple-init` (ELF可执行文件)

## 2. 生成最小initramfs

```bash
bash scripts/gen-minimal-initramfs.sh
```

输出:  `/tmp/initramfs.img` (ASCII cpio归档使用gzip压缩)

如果你需要一个附带了常用命令的大initramfs，则使用`gen-initramfs.sh`代替`gen-minimal-initramfs.sh`

## 3. 生成测试镜像(只需要在第一次执行)

```bash
bash scripts/gen-logfs.sh
bash scripts/gen-miniroot.sh
```

输出: `/tmp/logfs.img` (MBR分区表以及FAT16 文件系统)用于储存日志(LOGFS)

输出: `/tmp/minidisk.img` (EXT4文件系统镜像)用于测试switchroot

## 4. 运行QEMU

```bash
bash scripts/qemu.sh
```

注意: 你需要`prebuilts/vmlinuz`和`prebuilts/kernel.txz`用来启动Linux
