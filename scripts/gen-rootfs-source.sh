#!/bin/bash
WORKSPACE="$(realpath "$(dirname $0)"/..)"
BUILD="/tmp"
ROOT="${WORKSPACE}/root"
[ -n "${1}" ]&&WORKSPACE="${1}"
[ -n "${2}" ]&&BUILD="${2}"
[ -n "${3}" ]&&ROOT="${3}"
"${CC:-${CROSS_COMPILE}gcc}" \
	-Wall -Wextra -Werror \
	-I"${WORKSPACE}/include" \
	${CFLAGS} ${LDFLAGS} \
	"${WORKSPACE}/src/host/rootfs.c" \
	-o "${BUILD}/assets"||exit 1
"${BUILD}/assets" \
	"${ROOT}" \
	"${BUILD}/rootfs.c" \
	assets_rootfs
