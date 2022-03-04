/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<boot.h>
#if \
	defined(__amd64__)||defined(__amd64)||defined(amd64)||\
	defined(__x86_64__)||defined(__x86_64)||defined(x86_64)||\
	defined(__AMD64__)||defined(__AMD64)||defined(AMD64)||\
	defined(__X86_64__)||defined(__X86_64)||defined(X86_64)||\
	defined(__x64__)||defined(__x64)||defined(x64)||\
	defined(__X64__)||defined(__X64)||defined(X64)
const enum cpu_type current_cpu=CPU_X64;
#elif \
	defined(__x86_32__)||defined(__x86_32)||defined(x86_32)||\
	defined(__X86_32__)||defined(__X86_32)||defined(X86_32)
	defined(__x86__)||defined(__x86)||defined(x86)||\
	defined(__X86__)||defined(__X86)||defined(X86)||\
	defined(__i386__)||defined(__i386)||defined(i386)||\
	defined(__I386__)||defined(__I386)||defined(I386)||\
	defined(__i486__)||defined(__i486)||defined(i486)||\
	defined(__I486__)||defined(__I486)||defined(I486)||\
	defined(__i586__)||defined(__i586)||defined(i586)||\
	defined(__I586__)||defined(__I586)||defined(I586)||\
	defined(__i686__)||defined(__i686)||defined(i686)||\
	defined(__I686__)||defined(__I686)||defined(I686)
const enum cpu_type current_cpu=CPU_IA32;
#elif \
	defined(__aarch64__)||defined(__aarch64)||defined(aarch64)||\
	defined(__AARCH64__)||defined(__AARCH64)||defined(AARCH64)||\
	defined(__AARCH64EL__)||defined(__AARCH64EL)||defined(AARCH64EL)||\
	defined(__armv8__)||defined(__armv8)||defined(armv8)||\
	defined(__armv9__)||defined(__armv9)||defined(armv9)||\
	defined(__armv8a__)||defined(__armv8a)||defined(armv8a)||\
	defined(__ARMV8__)||defined(__ARMV8)||defined(ARMV8)||\
	defined(__ARMV9__)||defined(__ARMV9)||defined(ARMV9)||\
	defined(__ARMV8a__)||defined(__ARMV8a)||defined(ARMV8a)||\
	__ARM_ARCH>=8
const enum cpu_type current_cpu=CPU_AARCH64;
#elif \
	defined(__aarch32__)||defined(__aarch32)||defined(aarch32)||\
	defined(__AARCH32__)||defined(__AARCH32)||defined(AARCH32)||\
	defined(__AARCH32EL__)||defined(__AARCH32EL)||defined(AARCH32EL)||\
	defined(__arm__)||defined(__arm)||defined(arm)||\
	defined(__ARMEL__)||defined(__ARMEL)||defined(ARMEL)||\
	defined(__ARM__)||defined(__ARM)||defined(ARM)||\
	defined(__armv4__)||defined(__armv4)||defined(armv4)||\
	defined(__armv5__)||defined(__armv5)||defined(armv5)||\
	defined(__armv6__)||defined(__armv6)||defined(armv6)||\
	defined(__armv7__)||defined(__armv7)||defined(armv7)||\
	defined(__armhf__)||defined(__armhf)||defined(armhf)||\
	defined(__ARMV4__)||defined(__ARMV4)||defined(ARMV4)||\
	defined(__ARMV5__)||defined(__ARMV5)||defined(ARMV5)||\
	defined(__ARMV6__)||defined(__ARMV6)||defined(ARMV6)||\
	defined(__ARMV7__)||defined(__ARMV7)||defined(ARMV7)||\
	defined(__ARMHF__)||defined(__ARMHF)||defined(ARMHF)||\
	__ARM_ARCH<=7
const enum cpu_type current_cpu=CPU_ARM;
#endif
