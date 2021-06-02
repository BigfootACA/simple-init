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

#define _FEATURE_MOUNT "MOUNT"

#define FEATURES \
	_FEATURE_ASAN \
	_FEATURE_BLKID \
	_FEATURE_KMOD \
	_FEATURE_INITSHELL \
	_FEATURE_MOUNT

#define VERSION_INFO \
	PRODUCT "\n"\
	"Author: " AUTHOR "\n"\
	"Features: " FEATURES

#endif
