#ifndef SIMPLE_INIT_STDBOOL_H
#define SIMPLE_INIT_STDBOOL_H
#include "compatible.h"
#ifndef __cplusplus
#ifdef ENABLE_UEFI
typedef BOOLEAN bool;
#define true TRUE
#define false FALSE
#else
typedef _Bool bool;
#if defined __STDC_VERSION__&&__STDC_VERSION__>201710L
#define true    ((_Bool)+1u)
#define false   ((_Bool)+0u)
#else
#define true 1
#define false 0
#endif
#endif
#endif
#endif
