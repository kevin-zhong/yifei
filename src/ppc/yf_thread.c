#include <ppc/yf_header.h>
#include <base_struct/yf_core.h>

static yf_uint_t nthreads;
static yf_uint_t max_threads;
static yf_int_t thread_sig_mask = 0;
static pthread_attr_t thr_attr;

yf_tid_t  yf_main_thread_id = 0;

typedef struct
{
        yf_thread_exe_pt  func;
        void* arg;
}
_yf_thread_ctx_t;

static yf_thread_value_t _yf_thread_exe_wrapper(void *arg)
{
        _yf_thread_ctx_t ctx = *((_yf_thread_ctx_t*)arg);
        yf_free(arg);
        
        if (thread_sig_mask)
        {
                sigset_t mask;
                sigfillset(&mask);
                
                //if block this signos, proc will exit unnormly...
                sigdelset(&mask, SIGSEGV);
                sigdelset(&mask, SIGFPE);
                sigdelset(&mask, SIGBUS);
                
                yf_int_t rc = pthread_sigmask(SIG_BLOCK, &mask, NULL);
                assert(rc == 0);
        }
        
        return ctx.func(ctx.arg);
}


yf_err_t
yf_create_thread(yf_tid_t *tid, yf_thread_exe_pt func, void *arg, yf_log_t *log)
{
        int err;

        if (nthreads >= max_threads)
        {
                yf_log_error(YF_LOG_CRIT, log, 0,
                             "no more than %ui threads can be created", max_threads);
                return YF_ERROR;
        }

        _yf_thread_ctx_t* ctx = (_yf_thread_ctx_t*)yf_alloc(sizeof(_yf_thread_ctx_t));
        ctx->func = func;
        ctx->arg = arg;
        err = pthread_create(tid, &thr_attr, _yf_thread_exe_wrapper, ctx);

        if (err != 0)
        {
                yf_log_error(YF_LOG_ALERT, log, err, "pthread_create() failed");
                return err;
        }

        yf_log_debug1(YF_LOG_DEBUG, log, 0,
                      "thread is created: " YF_TID_T_FMT, *tid);

        nthreads++;

        return err;
}


yf_int_t
yf_init_threads(int n, size_t size, yf_int_t sig_maskall, yf_log_t *log)
{
        int err;

        max_threads = n;
        yf_main_thread_id = yf_thread_self();
        thread_sig_mask = sig_maskall;

        err = pthread_attr_init(&thr_attr);

        if (err != 0)
        {
                yf_log_error(YF_LOG_ALERT, log, err,
                             "pthread_attr_init() failed");
                return YF_ERROR;
        }

        err = pthread_attr_setstacksize(&thr_attr, size);

        if (err != 0)
        {
                yf_log_error(YF_LOG_ALERT, log, err,
                             "pthread_attr_setstacksize() failed");
                return YF_ERROR;
        }

        return YF_OK;
}


yf_mutex_t *
yf_mutex_init(yf_log_t *log)
{
        int err;
        yf_mutex_t *m;

        m = yf_alloc(sizeof(yf_mutex_t));
        if (m == NULL)
        {
                return NULL;
        }

        err = pthread_mutex_init(m, NULL);

        if (err != 0)
        {
                yf_log_error(YF_LOG_ALERT, log, err,
                             "pthread_mutex_init() failed");
                return NULL;
        }

        return m;
}


void
yf_mutex_destroy(yf_mutex_t *m, yf_log_t *log)
{
        int err;

        err = pthread_mutex_destroy(m);

        if (err != 0)
        {
                yf_log_error(YF_LOG_ALERT, log, err,
                             "pthread_mutex_destroy(%p) failed", m);
        }

        yf_free(m);
}


void
yf_mutex_lock(yf_mutex_t *m, yf_log_t *log)
{
        int err;

        yf_log_debug1(YF_LOG_DEBUG, log, 0, "lock mutex %p", m);

        err = pthread_mutex_lock(m);

        if (err != 0)
        {
                yf_log_error(YF_LOG_ALERT, log, err,
                             "pthread_mutex_lock(%p) failed", m);
                yf_abort();
        }

        yf_log_debug1(YF_LOG_DEBUG, log, 0, "mutex %p is locked", m);

        return;
}


yf_int_t
yf_mutex_trylock(yf_mutex_t *m, yf_log_t *log)
{
        int err;

        yf_log_debug1(YF_LOG_DEBUG, log, 0, "try lock mutex %p", m);

        err = pthread_mutex_trylock(m);

        if (err == YF_EBUSY)
        {
                return YF_AGAIN;
        }

        if (err != 0)
        {
                yf_log_error(YF_LOG_ALERT, log, err,
                             "pthread_mutex_trylock(%p) failed", m);
                yf_abort();
        }

        yf_log_debug1(YF_LOG_DEBUG, log, 0, "mutex %p is locked", m);

        return YF_OK;
}


void
yf_mutex_unlock(yf_mutex_t *m, yf_log_t *log)
{
        int err;
        
        yf_log_debug1(YF_LOG_DEBUG, log, 0, "unlock mutex %p", m);

        err = pthread_mutex_unlock(m);

        if (err != 0)
        {
                yf_log_error(YF_LOG_ALERT, log, err,
                             "pthread_mutex_unlock(%p) failed", m);
                yf_abort();
        }

        yf_log_debug1(YF_LOG_DEBUG, log, 0, "mutex %p is unlocked", m);

        return;
}


yf_cond_t *
yf_cond_init(yf_log_t *log)
{
        int err;
        yf_cond_t *cv;

        cv = yf_alloc(sizeof(yf_cond_t));
        if (cv == NULL)
        {
                return NULL;
        }

        err = pthread_cond_init(cv, NULL);

        if (err != 0)
        {
                yf_log_error(YF_LOG_ALERT, log, err,
                             "pthread_cond_init() failed");
                return NULL;
        }

        return cv;
}


void
yf_cond_destroy(yf_cond_t *cv, yf_log_t *log)
{
        int err;

        err = pthread_cond_destroy(cv);

        if (err != 0)
        {
                yf_log_error(YF_LOG_ALERT, log, err,
                             "pthread_cond_destroy(%p) failed", cv);
        }

        yf_free(cv);
}


yf_int_t
yf_cond_wait(yf_cond_t *cv, yf_mutex_t *m, yf_log_t *log)
{
        int err;

        yf_log_debug1(YF_LOG_DEBUG, log, 0, "cv %p wait", cv);

        err = pthread_cond_wait(cv, m);

        if (err != 0)
        {
                yf_log_error(YF_LOG_ALERT, log, err,
                             "pthread_cond_wait(%p) failed", cv);
                return YF_ERROR;
        }

        yf_log_debug1(YF_LOG_DEBUG, log, 0, "cv %p is waked up", cv);

        yf_log_debug1(YF_LOG_DEBUG, log, 0, "mutex %p is locked", m);

        return YF_OK;
}


yf_int_t
yf_cond_signal(yf_cond_t *cv, yf_log_t *log)
{
        int err;

        yf_log_debug1(YF_LOG_DEBUG, log, 0, "cv %p to signal", cv);

        err = pthread_cond_signal(cv);

        if (err != 0)
        {
                yf_log_error(YF_LOG_ALERT, log, err,
                             "pthread_cond_signal(%p) failed", cv);
                return YF_ERROR;
        }

        yf_log_debug1(YF_LOG_DEBUG, log, 0, "cv %p is signaled", cv);

        return YF_OK;
}
