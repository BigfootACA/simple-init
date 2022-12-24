/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2022 John Sanpe <sanpeqf@gmail.com>
 */

static unsigned long pjwhash(const char *str)
{
    unsigned long value, hash = 0;

    while (*str) {
        hash <<= 4;
        hash += *str++;

        value = hash & (0xfUL << (PJWHASH_BITS - 4));
        if (value) {
            hash ^= value >> (PJWHASH_BITS - 8);
            hash ^= value;
        }
    }

    return hash;
}
