/** @file
*
*  Copyright (c) 2011-2014, ARM Limited. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#ifndef _LIBFDT_ENV_H
#define _LIBFDT_ENV_H
#include <stdint.h>
#include <string.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
typedef UINT16 fdt16_t;
typedef UINT32 fdt32_t;
typedef UINT64 fdt64_t;
static inline uint16_t fdt16_to_cpu(fdt16_t x){return SwapBytes16(x);}
#define cpu_to_fdt16(x) fdt16_to_cpu(x)
static inline uint32_t fdt32_to_cpu(fdt32_t x){return SwapBytes32(x);}
#define cpu_to_fdt32(x) fdt32_to_cpu(x)
static inline uint64_t fdt64_to_cpu(fdt64_t x){return SwapBytes64(x);}
#define cpu_to_fdt64(x) fdt64_to_cpu(x)

#endif /* _LIBFDT_ENV_H */
