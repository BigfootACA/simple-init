#!/bin/bash
set -e
pushd "$(dirname "$0")/.." >/dev/null
source scripts/functions.sh.inc
source scripts/environments.sh.inc
rm -rf "${MINIROOT}"
mkdir "${MINIROOT}"
init_rootfs "${MINIROOT}"
mkdir "${MINIROOT}"/etc/init.d
for i in "${PROGRAMS[@]}"
do add_cmd "${i}" "${MINIROOT}"/usr||true
done
for i in "${LIBRARIES[@]}"
do add_lib "${i}" "${MINIROOT}"/usr||true
done
for i in "${DATAS[@]}"
do add_data "${i}" "${MINIROOT}"||true
done
cat>"${MINIROOT}/etc/init.d/rcS"<<-EOF
#!/bin/sh
mountpoint -q /dev||mount -t devtmpfs devtmpfs /dev
mountpoint -q /proc||mount -t proc proc /proc
mountpoint -q /sys||mount -t sysfs sysfs /sys
mount -o remount,rw /
EOF
chmod +x "${MINIROOT}/etc/init.d/rcS"
init_busybox "${MINIROOT}"
if [ -f "${MINIDISK}" ]
then rm -f "${MINIDISK}"
fi
fallocate -l "${MINISIZE}" "${MINIDISK}"
mke2fs \
	-F \
	-t ext2 \
	-d "${MINIROOT}" \
	-L root \
	"${MINIDISK}"
popd >/dev/null
