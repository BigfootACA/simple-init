#!/bin/bash
set -e
pushd "$(dirname "$0")/.." >/dev/null
source scripts/functions.sh.inc
source scripts/environments.sh.inc
[ -n "${1}" ]&&export INITRAMFS="${1}"
if ! [ -x build/simple-init ]
then	echo "simple-init binary not found, please compile first"
	exit 1
fi
if ! [ -d "${ORIGROOT}" ]
then	MODS="${ORIGROOT}"/usr/lib/modules
	mkdir -p "${MODS}"
	[ -f "${MODS_ARCHIVE}" ]&&tar -C "${MODS}" -xf "${MODS_ARCHIVE}"
	for i in "${MODS}"/*
	do [ -d "${i}/kernel" ]&&depmod -b "${ORIGROOT}/usr" "$(basename "$i")"
	done
fi
rm -rf "${TESTROOT}"
rm -f "${INITRAMFS}"
cp -r "${ORIGROOT}" "${TESTROOT}"
init_rootfs "${TESTROOT}"
for i in "${PROGRAMS[@]}"
do add_cmd "${i}" "${TESTROOT}"/usr||true
done
for i in "${LIBRARIES[@]}"
do add_lib "${i}" "${TESTROOT}"/usr||true
done
for i in "${DATAS[@]}"
do add_data "${i}" "${TESTROOT}"||true
done
add_binary build/simple-init "${TESTROOT}"/usr
cat<<EOF>>"${TESTROOT}"/init
#!/usr/bin/bash
mkdir -p /proc /sys
mount -t proc proc /proc
mount -t sysfs sysfs /sys
exec /usr/bin/simple-init init
EOF
chmod +x "${TESTROOT}"/init
init_busybox "${TESTROOT}"
gen_initcpio "${TESTROOT}" "${INITRAMFS}"
popd >/dev/null
