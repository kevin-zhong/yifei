#include <ppc/yf_header.h>
#include <base_struct/yf_core.h>

#if YF_HAVE_ATOMIC_CMP_SWP

void yf_lock(yf_lock_t *lock)
{
        yf_uint_t i, n;

        for (;;)
        {
                if (yf_trylock(lock))
                {
                        return;
                }

                if (YF_NCPU > 1)
                {
                        for (n = 1; n < 16; n <<= 1)
                        {
                                for (i = 0; i < n; i++)
                                {
                                        yf_cpu_pause();
                                }

                                if (yf_trylock(lock))
                                {
                                        return;
                                }
                        }
                }

                yf_sched_yield();
        }
}

#else


#endif
