[ -z "${TMPDIR}" ]&&export TMPDIR="/tmp"
export LOGFS="${TMPDIR}/logfs.img"
export INITRAMFS="${TMPDIR}/initramfs.img"
export MINIDISK="${TMPDIR}/minidisk.img"
export MINIROOT="${TMPDIR}/mini-root"
export ORIGROOT="${TMPDIR}/orig-root"
export TESTROOT="${TMPDIR}/test-root"
export LOGFS_SIZE=16M
export MINISIZE=256M
export QEMU_MEM=512M
export MININAME=root
export COMPRESS=gzip
export CMDLINE="console=ttyS0 panic=5 root=LABEL=${MININAME}"
export KERNEL="prebuilts/vmlinuz"
export MODS_ARCHIVE="prebuilts/kernel.txz"
export ARCH="x86_64"
export QEMU_BIN="qemu-system-${ARCH}"
export FONTSIZE=(24 48)
export FONTPATH=fonts/wqy-mono-microhei.ttf
export LIBRARIES=(
	libnss_files.so.2
	libnss_dns.so.2
)
export DATAS=(
	usr/share/file
	usr/share/misc
	usr/share/magic
	usr/share/hwdata
	usr/share/terminfo
	usr/lib/locale
	etc/locale.conf
	etc/hostname
)
export PROGRAMS=(
	strace busybox gdbserver gdb bash htop lsof gzip zstd tar
	lz4 sed parted fdisk mke2fs e2fsck mkfs.fat fsck.fat grep
	xz awk rm ls clear ip ifconfig route ping ln rz sz du arp
	tr tee df dd cpio ldd gpm reset nano shred chown chmod cp
	cat chroot cut base32 base64 basename date comm env false
	true expand expr install head id pwd md5sum mktemp printf
	mknod mkfifo seq vi sleep mkdir nohup sha256sum sha512sum
	sha1sum shuf stty sum sort stat split sync uniq tail test
	touch timeout uname wc blkid whoami who yes lsblk cal col
	cfdisk agetty dmesg eject fallocate findfs getopt hexdump
	switch_root hwclock kill logger losetup lscpu swapon mkfs
	umount mount nsenter mkswap uuidgen rfkill script swapoff
	setsid mountpoint vim diff fsck find which whereis uptime
	partprobe xargs watch less more kmod modprobe lsmod rmmod
	insmod depmod modinfo lsusb lspci dhcpcd ldconfig curl ss
	dmidecode efibootmgr btrfs file getfacl setfacl brctl dtc
	ss fdtoverlay fdtdump fdtget fdtput tree hostname ed chvt
)