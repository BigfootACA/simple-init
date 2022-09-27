/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef FILEOPEN_H
#define FILEOPEN_H
#include"filesystem.h"

// src/gui/interface/filemgr/fileopen.c: show a dialog to choose file open method
extern void fileopen_open(const char*path);

// src/gui/interface/filemgr/fileopen.c: show a dialog to choose file open method with url
extern void fileopen_open_url(url*uri);

// src/gui/interface/filemgr/fileopen.c: show a dialog to choose file open method with fsh
extern void fileopen_open_fsh(fsh*f);
#endif
