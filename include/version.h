/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef INIT_VERSION_H
#define INIT_VERSION_H

#define NAME "simple-init"
#define AUTHOR "BigfootACA"
#define VERSION "0.0.1"
#define PRODUCT NAME " " VERSION

#ifdef ENABLE_ASAN
#define _FEATURE_ASAN "ASAN "
#else
#define _FEATURE_ASAN
#endif

#ifdef ENABLE_BLKID
#define _FEATURE_BLKID "BLKID "
#else
#define _FEATURE_BLKID
#endif

#ifdef ENABLE_KMOD
#define _FEATURE_KMOD "KMOD "
#else
#define _FEATURE_KMOD
#endif

#ifdef ENABLE_INITSHELL
#define _FEATURE_INITSHELL "INITSHELL "
#else
#define _FEATURE_INITSHELL
#endif

#ifdef ENABLE_FREETYPE2
#define _FEATURE_FREETYPE2 "FREETYPE2 "
#else
#define _FEATURE_FREETYPE2
#endif

#ifdef ENABLE_FDISK
#define _FEATURE_FDISK "FDISK "
#else
#define _FEATURE_FDISK
#endif

#ifdef ENABLE_GUI
#define _FEATURE_GUI "GUI "
#else
#define _FEATURE_GUI
#endif

#ifdef ENABLE_DRM
#define _FEATURE_DRM "DRM "
#else
#define _FEATURE_DRM
#endif

#ifdef ENABLE_GTK
#define _FEATURE_GTK "GTK "
#else
#define _FEATURE_GTK
#endif

#ifdef ENABLE_SDL2
#define _FEATURE_SDL2 "SDL2 "
#else
#define _FEATURE_SDL2
#endif

#define _FEATURE_MOUNT "MOUNT"

#define FEATURES \
	_FEATURE_ASAN \
	_FEATURE_BLKID \
	_FEATURE_KMOD \
	_FEATURE_INITSHELL \
	_FEATURE_FREETYPE2 \
	_FEATURE_FDISK \
	_FEATURE_GUI \
	_FEATURE_DRM \
	_FEATURE_GTK \
	_FEATURE_SDL2 \
	_FEATURE_MOUNT

#define VERSION_INFO \
	PRODUCT "\n"\
	"Author: " AUTHOR "\n"\
	"Features: " FEATURES

#endif
