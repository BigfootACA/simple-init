/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2022 John Sanpe <sanpeqf@gmail.com>
 */

#include <stdlib.h>
#include <limits.h>

static bool bitmap_full(const unsigned long *bitmap, unsigned int bits)
{
    unsigned long mask;
    uintptr_t offset;

    for (offset = 0; bits >= __LONG_WIDTH__; bits -= __LONG_WIDTH__) {
        if (bitmap[offset++] != ULONG_MAX)
            return false;
    }

    if (bits) {
        mask = ULONG_MAX << bits;
        if ((bitmap[offset] | mask) != ULONG_MAX)
            return false;
    }

    return true;
}

static bool bitmap_test_set(unsigned long *bitmap, unsigned int shift)
{
    uintptr_t offset;
    bool retval;

    offset = shift / __LONG_WIDTH__;
    retval = (bitmap[offset] >> (shift % __LONG_WIDTH__)) & 0x1;
    bitmap[offset] |= 1Ul << (shift % __LONG_WIDTH__);

    return retval;
}

static unsigned long *bitmap_zalloc(unsigned int bits)
{
    unsigned long *bitmap;
    size_t size;

    size = (bits + __LONG_WIDTH__ - 1) / __LONG_WIDTH__;
    bitmap = malloc(size * sizeof(*bitmap));
    if (!bitmap)
        return NULL;

    memset(bitmap, 0, size * sizeof(*bitmap));
    return bitmap;
}


static void bitmap_free(unsigned long *bitmap)
{
    free(bitmap);
}
