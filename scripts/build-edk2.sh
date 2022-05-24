#!/bin/bash
set -e
pushd "$(dirname "$0")/.." >/dev/null
source scripts/functions.sh.inc
source scripts/environments.sh.inc
export SIMPLE_INIT="${PWD}"
for i in "${EDK2}" ../edk2 edk2
do [ -d "${i}" ]&&EDK2="$(realpath "${i}")"
done
if ! [ -d "${EDK2}" ]
then	echo "edk2 not found" >&2
	exit 1
fi
export TOOLCHAIN=GCC5
export TARGET=RELEASE
export PACKAGES_PATH="${EDK2}":"${SIMPLE_INIT}"
export EDK_TOOLS_PATH="${EDK2}"/BaseTools
export WORKSPACE="${SIMPLE_INIT}/build-edk2"
export BUILD="${SIMPLE_INIT}/build"
_ARG1="${1}"
shift
source "${EDK2}"/edksetup.sh
mkdir -p "${WORKSPACE}"
mkdir -p "${BUILD}"
rm -rf "${SIMPLE_INIT}"/root/usr/share/locale
for i in "${SIMPLE_INIT}/po/"*.po
do	[ -f "${i}" ]||continue
	_name="$(basename "$i" .po)"
	_path="${SIMPLE_INIT}/root/usr/share/locale/${_name}/LC_MESSAGES"
	mkdir -p "${_path}"
	msgfmt -o "${_path}/simple-init.mo" "${i}"
done
make -C "${EDK2}"/BaseTools
case "$_ARG1" in
	ia32|IA32)
		export CROSS_COMPILE=i686-linux-gnu-
		export GCC5_IA32_PREFIX="${CROSS_COMPILE}"
		export GCC5_BIN="${CROSS_COMPILE}"
		bash scripts/gen-rootfs-source.sh "${SIMPLE_INIT}" "${BUILD}" "${SIMPLE_INIT}"/root
		build -p SimpleInit.dsc -t "${TOOLCHAIN}" -a IA32 -b "${TARGET}"
	;;
	x64|X64)
		export CROSS_COMPILE=x86_64-linux-gnu-
		export GCC5_X64_PREFIX="${CROSS_COMPILE}"
		export GCC5_BIN="${CROSS_COMPILE}"
		bash scripts/gen-rootfs-source.sh "${SIMPLE_INIT}" "${BUILD}" "${SIMPLE_INIT}"/root
		build -p SimpleInit.dsc -t "${TOOLCHAIN}" -a X64 -b "${TARGET}"
	;;
	arm|ARM)
		export CROSS_COMPILE=arm-none-eabi-
		export GCC5_ARM_PREFIX="${CROSS_COMPILE}"
		export GCC5_BIN="${CROSS_COMPILE}"
		bash scripts/gen-rootfs-source.sh "${SIMPLE_INIT}" "${BUILD}" "${SIMPLE_INIT}"/root
		build -p SimpleInit.dsc -t "${TOOLCHAIN}" -a ARM -b "${TARGET}"
	;;
	aarch64|AARCH64)
		export CROSS_COMPILE=aarch64-linux-gnu-
		export GCC5_AARCH64_PREFIX="${CROSS_COMPILE}"
		export GCC5_BIN="${CROSS_COMPILE}"
		bash scripts/gen-rootfs-source.sh "${SIMPLE_INIT}" "${BUILD}" "${SIMPLE_INIT}"/root
		build -p SimpleInit.dsc -t "${TOOLCHAIN}" -a AARCH64 -b "${TARGET}"
	;;
	all)
		bash "${0}" ia32
		bash "${0}" x64
		bash "${0}" arm
		bash "${0}" aarch64
	;;
	img)
		bash "${0}" all
		rm -f "${WORKSPACE}"/esp.img
		mkfs.vfat -C -F32 "${WORKSPACE}"/esp.img 65536
		mkdir -p "${WORKSPACE}"/esp
		umount "${WORKSPACE}"/esp &>/dev/null||true
		mount "${WORKSPACE}"/esp.img "${WORKSPACE}"/esp
		for i in "${WORKSPACE}/Build/SimpleInit/${TARGET}_${TOOLCHAIN}"/*
		do	[ -d "${i}/main" ]||continue
			ARCH="$(awk -F/ '{print $7}' <<<"$i")"
			cp "$i/SimpleInitMain.efi" "${WORKSPACE}/esp/SimpleInitMain-$ARCH.efi"
			cp "$i/KernelFdtDxe.efi" "${WORKSPACE}/esp/KernelFdtDxe-$ARCH.efi"
		done
		umount "${WORKSPACE}"/esp
		rmdir "${WORKSPACE}"/esp
	;;
esac
popd >/dev/null
