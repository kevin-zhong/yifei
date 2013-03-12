#include <ppc/yf_header.h>
#include <base_struct/yf_core.h>

#ifdef PAGESIZE
yf_uint_t yf_pagesize = PAGESIZE;
#else
yf_uint_t yf_pagesize = 4096;
#endif
yf_uint_t yf_cacheline_size;


#if (HAVE_POSIX_MEMALIGN)

void *
yf_memalign(size_t alignment, size_t size, yf_log_t *log)
{
        void *p;
        int err;

        err = posix_memalign(&p, alignment, size);

        if (unlikely(err))
        {
                yf_log_error(YF_LOG_CRIT, log, yf_errno, "posix_memalign failed, size=%d", size);
                p = NULL;
        }

        return p;
}

#elif (HAVE_MEMALIGN)

void *
yf_memalign(size_t alignment, size_t size, yf_log_t *log)
{
        void *p;

        p = memalign(alignment, size);
        if (unlikely(p == NULL))
        {
                yf_log_error(YF_LOG_CRIT, log, yf_errno, "memalign failed, size=%d", size);
        }

        return p;
}

#endif
