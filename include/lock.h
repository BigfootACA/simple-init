/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef _LOCK_H
#define _LOCK_H
#ifndef ENABLE_UEFI
#include<pthread.h>
typedef pthread_mutex_t mutex_t;
#define MUTEX_INIT(lock) pthread_mutex_init(&(lock),NULL)
#define MUTEX_LOCK(lock) pthread_mutex_lock(&(lock))
#define MUTEX_UNLOCK(lock) pthread_mutex_unlock(&(lock))
#define MUTEX_TRYLOCK(lock) pthread_mutex_trylock(&(lock))
#define MUTEX_DESTROY(lock) pthread_mutex_destroy(&(lock))
#else
typedef char mutex_t;
static inline __attribute__((used)) int dumb_lock_init(mutex_t*lock){(void)lock;return 0;}
static inline __attribute__((used)) int dumb_lock_lock(mutex_t*lock){(void)lock;return 0;}
static inline __attribute__((used)) int dumb_lock_unlock(mutex_t*lock){(void)lock;return 0;}
static inline __attribute__((used)) int dumb_lock_trylock(mutex_t*lock){(void)lock;return 0;}
static inline __attribute__((used)) int dumb_lock_destroy(mutex_t*lock){(void)lock;return 0;}
#define MUTEX_INIT(lock) dumb_lock_init(&(lock))
#define MUTEX_LOCK(lock) dumb_lock_lock(&(lock))
#define MUTEX_UNLOCK(lock) dumb_lock_unlock(&(lock))
#define MUTEX_TRYLOCK(lock) dumb_lock_trylock(&(lock))
#define MUTEX_DESTROY(lock) dumb_lock_destroy(&(lock))
#endif
#endif
