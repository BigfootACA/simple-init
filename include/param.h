#ifndef _PARAM_H
#define _PARAM_H
#include"keyval.h"

// src/lib/param.c: read string from fd and convert to keyval array
extern keyval**read_params(int fd);

// src/lib/param.c: append new to ptr
extern keyval**append_params(keyval**ptr,keyval**new);
#endif
