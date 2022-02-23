/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<stddef.h>
#include"internal.h"

extern compressor compressor_gzip;
compressor*compressors[]={
	#ifdef ENABLE_ZLIB
	&compressor_gzip,
	#endif
	NULL
};
