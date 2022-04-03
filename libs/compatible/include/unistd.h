#ifndef	_UNISTD_H
#define _UNISTD_H
#define __NEED_size_t
#define __NEED_ssize_t
#include "compatible.h"
#include <bits/alltypes.h>
int usleep(unsigned);
int close(int);
ssize_t read(int, void *, size_t);
ssize_t write(int, const void *, size_t);
#endif
