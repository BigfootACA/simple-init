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
mkdir -p "${TESTROOT}"/usr/{bin,lib}
ln -s usr/lib "${TESTROOT}"/lib64
ln -s usr/lib "${TESTROOT}"/lib
add_binary build/simple-init "${TESTROOT}"/usr
add_binary /usr/bin/strace "${TESTROOT}"/usr
add_library /usr/lib/libnss_dns.so.2 "${TESTROOT}"/usr ||true
add_library /usr/lib/libnss_files.so.2 "${TESTROOT}"/usr ||true
ln -s usr/bin/simple-init "${TESTROOT}"/init
if [ -f /usr/bin/busybox ]
then	add_binary /usr/bin/busybox "${TESTROOT}"/usr
	init_busybox "${TESTROOT}"
fi
gen_initcpio "${TESTROOT}" "${INITRAMFS}"
popd >/dev/null
