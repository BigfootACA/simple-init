#!/bin/bash
set -e
pushd "$(dirname "$0")/.." >/dev/null
source scripts/functions.sh.inc
source scripts/environments.sh.inc
[ -f "${LOGFS}" ]&&rm -f "${LOGFS}"
fallocate -l "${LOGFS_SIZE}" "${LOGFS}"
parted "${LOGFS}" mklabel gpt
parted "${LOGFS}" mkpart logfs fat16 2048s "${LOGFS_SIZE}"
_BLK="$(losetup --partscan --find --show "${LOGFS}")"
[ -b "${_BLK}" ]||exit 1
mkfs.vfat -n LOGFS "${_BLK}p1"
losetup -d "${_BLK}"
popd >/dev/null
