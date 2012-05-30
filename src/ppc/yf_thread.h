#ifndef _YF_THREAD_H
#define _YF_THREAD_H

#include <base_struct/yf_core.h>
#include <ppc/yf_header.h>

#define  yf_abort() exit(-1)

typedef void *yf_thread_value_t;

#define YF_MAX_THREADS      128

#include <pthread.h>

typedef pthread_t yf_tid_t;

#define yf_thread_self()            pthread_self()
#define yf_log_tid                  (yf_uint_t)yf_thread_self()

#if defined (YF_FREEBSD) && !defined (YF_LINUXTHREADS)
#define YF_TID_T_FMT                "%p"
#else
#define YF_TID_T_FMT                "%ud"
#endif


typedef pthread_key_t yf_tls_key_t;

#define yf_thread_key_create(key)   pthread_key_create(key, NULL)
#define yf_thread_key_create_n      "pthread_key_create()"
#define yf_thread_set_tls           pthread_setspecific
#define yf_thread_set_tls_n         "pthread_setspecific()"
#define yf_thread_get_tls           pthread_getspecific


#define YF_MUTEX_LIGHT     0

typedef pthread_mutex_t yf_mutex_t;
typedef pthread_cond_t yf_cond_t;


#define yf_thread_sigmask     pthread_sigmask
#define yf_thread_sigmask_n  "pthread_sigmask()"

#define yf_thread_join(t, p)  pthread_join(t, p)
#define yf_thread_detach(t)  pthread_detach(t)

#define yf_setthrtitle(n)



yf_int_t yf_mutex_trylock(yf_mutex_t *m, yf_log_t *log);
void yf_mutex_lock(yf_mutex_t *m, yf_log_t *log);
void yf_mutex_unlock(yf_mutex_t *m, yf_log_t *log);


#define yf_thread_volatile   volatile


typedef struct
{
        yf_tid_t   tid;
        yf_cond_t *cv;
        yf_uint_t  state;
} 
yf_thread_t;

#define YF_THREAD_FREE   1
#define YF_THREAD_BUSY   2
#define YF_THREAD_EXIT   3
#define YF_THREAD_DONE   4

yf_int_t yf_init_threads(int n, size_t size, yf_log_t *log);
yf_err_t yf_create_thread(yf_tid_t * tid,
                          yf_thread_value_t (*func)(void *arg), void *arg, yf_log_t * log);

yf_mutex_t *yf_mutex_init(yf_log_t *log);
void yf_mutex_destroy(yf_mutex_t *m, yf_log_t *log);


yf_cond_t *yf_cond_init(yf_log_t *log);
void yf_cond_destroy(yf_cond_t *cv, yf_log_t *log);
yf_int_t yf_cond_wait(yf_cond_t *cv, yf_mutex_t *m, yf_log_t *log);
yf_int_t yf_cond_signal(yf_cond_t *cv, yf_log_t *log);


extern yf_int_t yf_threads_n;
extern volatile yf_thread_t yf_threads[];


#endif /* _YF_THREAD_H_INCLUDED_ */
