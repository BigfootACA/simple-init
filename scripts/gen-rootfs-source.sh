#!/bin/bash
WORKSPACE="$(realpath "$(dirname $0)"/..)"
BUILD="/tmp"
ROOT="${WORKSPACE}/root"
[ -n "${1}" ]&&WORKSPACE="${1}"
[ -n "${2}" ]&&BUILD="${2}"
[ -n "${3}" ]&&ROOT="${3}"
set -e
"${HOSTCC:-gcc}" \
	-Wall -Wextra -Werror -g \
	-I"${WORKSPACE}/include" \
	"${WORKSPACE}/src/host/rootfs.c" \
	-o "${BUILD}/assets"
"${BUILD}/assets" \
	"${ROOT}" \
	"${BUILD}" \
	assets_rootfs
if [ -z "${NOBUILD}" ]
then	pushd "${BUILD}" >/dev/null
	"${CC:-${CROSS_COMPILE}gcc}" \
		${CFLAGS} \
		-r -Wl,-b,binary \
		-o rootfs_data.o \
		rootfs.bin
	popd >/dev/null
fi
