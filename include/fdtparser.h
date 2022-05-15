/*
 *
 * Copyright (C) 2022 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef _FDTPARSER_H
#define _FDTPARSER_H
#include<stdint.h>
#include"keyval.h"

// fdt type
typedef void* fdt;

// fdt magic
extern const uint32_t fdt_magic;

// src/lib/fdtparser.c: check and convert a pointer to fdt
extern fdt get_fdt_from_pointer(void*fdt);

// src/lib/fdtparser.c: get address cells from fdt (/#address-cells)
extern int32_t fdt_get_address_cells(fdt*fdt);

// src/lib/fdtparser.c: get size cells from fdt (/#size-cells)
extern int32_t fdt_get_size_cells(fdt*fdt);

// src/lib/fdtparser.c: get a reg from fdt
extern bool fdt_get_reg(fdt*fdt,int off,int index,uint64_t*base,uint64_t*size);

// src/lib/fdtparser.c: get a memory from fdt (/memory reg, pass from bootloader)
extern bool fdt_get_memory(fdt*fdt,int index,uint64_t*base,uint64_t*size);

// src/lib/fdtparser.c: get kernel cmdline from fdt (/chosen bootargs, pass from bootloader)
extern int fdt_get_cmdline(fdt*fdt,char**cmdline,int*length);

// src/lib/fdtparser.c: get kernel cmdline from fdt and convert to keyval
extern keyval**fdt_get_cmdline_items(fdt*fdt,size_t*length);

// src/lib/fdtparser.c: get kernel initrd from fdt
extern bool fdt_get_initrd(fdt*fdt,void**initrd,size_t*size);

#endif
