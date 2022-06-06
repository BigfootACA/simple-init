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

#ifdef ENABLE_READLINE
#define _FEATURE_READLINE "READLINE "
#else
#define _FEATURE_READLINE
#endif

#ifdef ENABLE_FREETYPE2
#define _FEATURE_FREETYPE2 "FREETYPE2 "
#else
#define _FEATURE_FREETYPE2
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

#ifdef ENABLE_LODEPNG
#define _FEATURE_LODEPNG "LODEPNG "
#else
#define _FEATURE_LODEPNG
#endif

#ifdef ENABLE_LIBJPEG
#define _FEATURE_LIBJPEG "LIBJPEG "
#else
#define _FEATURE_LIBJPEG
#endif

#ifdef ENABLE_NANOSVG
#define _FEATURE_NANOSVG "NANOSVG "
#else
#define _FEATURE_NANOSVG
#endif

#ifdef ENABLE_JSONC
#define _FEATURE_JSONC "JSONC "
#else
#define _FEATURE_JSONC
#endif

#ifdef ENABLE_MXML
#define _FEATURE_MXML "MXML "
#else
#define _FEATURE_MXML
#endif

#ifdef ENABLE_VNCSERVER
#define _FEATURE_VNCSERVER "VNCSERVER "
#else
#define _FEATURE_VNCSERVER
#endif

#ifdef ENABLE_HIVEX
#define _FEATURE_HIVEX "HIVEX "
#else
#define _FEATURE_HIVEX
#endif

#ifdef ENABLE_STB
#define _FEATURE_STB "STB "
#else
#define _FEATURE_STB
#endif

#ifdef ENABLE_LUA
#define _FEATURE_LUA "LUA "
#else
#define _FEATURE_LUA
#endif

#define FEATURES \
	_FEATURE_ASAN \
	_FEATURE_READLINE \
	_FEATURE_FREETYPE2 \
	_FEATURE_GUI \
	_FEATURE_DRM \
	_FEATURE_GTK \
	_FEATURE_SDL2 \
	_FEATURE_LODEPNG \
	_FEATURE_LIBJPEG \
	_FEATURE_NANOSVG \
	_FEATURE_JSONC \
	_FEATURE_MXML \
	_FEATURE_VNCSERVER \
	_FEATURE_HIVEX \
	_FEATURE_STB \
	_FEATURE_LUA

#define VERSION_INFO \
	PRODUCT "\n"\
	"Author: " AUTHOR "\n"\
	"Features: " FEATURES

#endif
