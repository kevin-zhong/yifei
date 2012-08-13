#ifndef  _YF_LOCK_H
#define _YF_LOCK_H

#include <base_struct/yf_core.h>
#include <ppc/yf_header.h>

#if (YF_THREADS) && defined  HAVE_PTHREAD_H

#include <pthread.h>

typedef  pthread_mutex_t  yf_mlock_t;
#define  yf_mlock_init(lock)  pthread_mutex_init(lock, NULL)
#define  yf_mlock_destory(lock)  pthread_mutex_destroy(lock)

#define  yf_mlock(lock)      pthread_mutex_lock(lock)
#define  yf_mtrylock(lock)  pthread_mutex_trylock(lock)
#define  yf_munlock(lock)   pthread_mutex_unlock(lock)
#endif

#if YF_HAVE_ATOMIC_CMP_SWP

typedef  yf_atomic_t  yf_lock_t;

#define  yf_lock_init(lock) (*(lock) = 0)
#define  yf_lock_destory(lock)

void _yf_lock_in(yf_lock_t* lock, yf_atomic_t v);

#define yf_lock_v(lock, v) _yf_lock_in(lock, v)
#define yf_lock(lock) _yf_lock_in(lock, 1)

#define yf_trylock_v(lock, v) (*(lock) == 0 && yf_atomic_cmp_swp(lock, 0, v))
#define yf_trylock(lock)  yf_trylock_v(lock, 1)

#define yf_unlock(lock)    (*(lock) = 0)

#elif (YF_THREADS) && defined  HAVE_PTHREAD_H

typedef  yf_mlock_t  yf_lock_t;

#define  yf_lock_init(lock) yf_mlock_init(lock)
#define  yf_lock_destory(lock) yf_mlock_destory(lock)

#define  yf_lock(lock)      yf_mlock(lock)
#define  yf_trylock(lock)  yf_mtrylock(lock)
#define  yf_unlock(lock)   yf_munlock(lock)

#else
#error lock not implement
#endif

#endif

