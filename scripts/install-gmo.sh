#!/bin/bash
WORKSPACE="$(realpath "$(dirname $0)"/..)"
BUILD="/tmp"
ROOT="${WORKSPACE}/root"
[ -n "${1}" ]&&WORKSPACE="${1}"
[ -n "${2}" ]&&BUILD="${2}"
[ -n "${3}" ]&&ROOT="${3}"
for i in "${BUILD}"/*.gmo
do	if ! [ -f "${i}" ]
	then	echo "${i} not found">&2
		exit 1
	fi
	LOCALE="$(basename "$i" .gmo)"
	TARGET="${ROOT}/usr/share/locale/${LOCALE}/LC_MESSAGES/simple-init.mo"
	install -Dm644 "${i}" "${TARGET}"
done