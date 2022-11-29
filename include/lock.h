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
typedef pthread_rwlock_t rwlock_t;

#define MUTEX_INIT(lock) pthread_mutex_init(&(lock),NULL)
#define MUTEX_LOCK(lock) pthread_mutex_lock(&(lock))
#define MUTEX_UNLOCK(lock) pthread_mutex_unlock(&(lock))
#define MUTEX_TRYLOCK(lock) pthread_mutex_trylock(&(lock))
#define MUTEX_DESTROY(lock) pthread_mutex_destroy(&(lock))

#define RWLOCK_INIT(lock) pthread_rwlock_init(&(lock),NULL)
#define RWLOCK_RDLOCK(lock) pthread_rwlock_rdlock(&(lock))
#define RWLOCK_WRLOCK(lock) pthread_rwlock_wrlock(&(lock))
#define RWLOCK_UNLOCK(lock) pthread_rwlock_unlock(&(lock))
#define RWLOCK_TRY_RDLOCK(lock) pthread_rwlock_tryrdlock(&(lock))
#define RWLOCK_TRY_WRLOCK(lock) pthread_rwlock_trywrlock(&(lock))
#define RWLOCK_DESTROY(lock) pthread_rwlock_destroy(&(lock))
#else
typedef char mutex_t;
typedef char rwlock_t;

static inline __attribute__((used)) int dummy_mutex_init(mutex_t*lock){(void)lock;return 0;}
static inline __attribute__((used)) int dummy_mutex_lock(mutex_t*lock){(void)lock;return 0;}
static inline __attribute__((used)) int dummy_mutex_unlock(mutex_t*lock){(void)lock;return 0;}
static inline __attribute__((used)) int dummy_mutex_trylock(mutex_t*lock){(void)lock;return 0;}
static inline __attribute__((used)) int dummy_mutex_destroy(mutex_t*lock){(void)lock;return 0;}
#define MUTEX_INIT(lock) dummy_mutex_init(&(lock))
#define MUTEX_LOCK(lock) dummy_mutex_lock(&(lock))
#define MUTEX_UNLOCK(lock) dummy_mutex_unlock(&(lock))
#define MUTEX_TRYLOCK(lock) dummy_mutex_trylock(&(lock))
#define MUTEX_DESTROY(lock) dummy_mutex_destroy(&(lock))

static inline __attribute__((used)) int dummy_rwlock_init(rwlock_t*lock){(void)lock;return 0;}
static inline __attribute__((used)) int dummy_rwlock_rdlock(rwlock_t*lock){(void)lock;return 0;}
static inline __attribute__((used)) int dummy_rwlock_wrlock(rwlock_t*lock){(void)lock;return 0;}
static inline __attribute__((used)) int dummy_rwlock_unlock(rwlock_t*lock){(void)lock;return 0;}
static inline __attribute__((used)) int dummy_rwlock_tryrdlock(rwlock_t*lock){(void)lock;return 0;}
static inline __attribute__((used)) int dummy_rwlock_trywrlock(rwlock_t*lock){(void)lock;return 0;}
static inline __attribute__((used)) int dummy_rwlock_destroy(rwlock_t*lock){(void)lock;return 0;}
#define RWLOCK_INIT(lock) dummy_rwlock_init(&(lock))
#define RWLOCK_RDLOCK(lock) dummy_rwlock_rdlock(&(lock))
#define RWLOCK_WRLOCK(lock) dummy_rwlock_wrlock(&(lock))
#define RWLOCK_UNLOCK(lock) dummy_rwlock_unlock(&(lock))
#define RWLOCK_TRY_RDLOCK(lock) dummy_rwlock_tryrdlock(&(lock))
#define RWLOCK_TRY_WRLOCK(lock) dummy_rwlock_trywrlock(&(lock))
#define RWLOCK_DESTROY(lock) dummy_rwlock_destroy(&(lock))
#endif
#endif
