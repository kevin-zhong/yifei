#ifndef _YF_ATOMIC_H_
#define _YF_ATOMIC_H_


#include <base_struct/yf_core.h>
#include <ppc/yf_header.h>

#if defined HAVE_ASM_ATOMIC_H
#define HAVE_ATOMIC_H
#include <asm/atomic.h>
#elif defined HAVE_LINUX_ATOMIC_H
#define HAVE_ATOMIC_H
#include <linux/atomic.h>
#endif

/*
* atomic.h
*/
#if defined HAVE_ATOMIC_H
typedef  atomic_t  yf_atomic_int_t;
typedef  atomic_t  yf_atomic_uint_t;

#define YF_ATOMIC_T_LEN  YF_INT32_LEN

#define yf_atomic_cmp_swp(lock, old, set) \
        atomic_cmpxchg(lock, old, set)
#define yf_atomic_fetch_add(value, add)  \
        atomic_add_value(value, add)
#define yf_atomic_fetch_sub(value, sub)  \
        atomic_sub_value(value, sub)

/*
* GCC 4.1 builtin atomic operations
*/
#elif defined HAVE_GCC_BUILTIN_ATOMIC

#define YF_HAVE_ATOMIC_CMP_SWP  1

typedef long                             yf_atomic_int_t;
typedef unsigned long               yf_atomic_uint_t;

#if (YF_SIZEOF_PTR == 8)
#define YF_ATOMIC_T_LEN            YF_INT64_LEN
#else
#define YF_ATOMIC_T_LEN            YF_INT32_LEN
#endif

#define yf_atomic_cmp_swp(lock, old, set) \
        __sync_bool_compare_and_swap(lock, old, set)

#define yf_atomic_fetch_add(value, add)  \
        __sync_fetch_and_add(value, add)
#define yf_atomic_fetch_sub(value, sub)  \
        __sync_fetch_and_sub(value, sub)

#define yf_memory_barrier()        __sync_synchronize()
#if ( __i386__ || __i386 || __amd64__ || __amd64 )
#define yf_cpu_pause()             __asm__ ("pause")
#else
#define yf_cpu_pause()
#endif


/*
* atomic_ops
*/
#elif defined HAVE_ATOMIC_OPS_H 

#include <atomic_ops.h>

#define YF_HAVE_ATOMIC_CMP_SWP  1

typedef long                        yf_atomic_int_t;
typedef AO_t                        yf_atomic_uint_t;

#if (YF_SIZEOF_PTR == 8)
#define YF_ATOMIC_T_LEN            YF_INT64_LEN
#else
#define YF_ATOMIC_T_LEN            YF_INT32_LEN
#endif

#define yf_atomic_cmp_swp(lock, old, new) \
        AO_compare_and_swap(lock, old, new)
#define yf_atomic_fetch_add(value, add)     \
        AO_fetch_and_add(value, add)
#define yf_atomic_fetch_sub(value, sub) \
        AO_fetch_and_add(value, -sub)
#define yf_memory_barrier()        AO_nop()
#define yf_cpu_pause()


/*
* OSAtomic
*/
#elif defined HAVE_LIBKERN_OSATOMIC_H

#include <libkern/OSAtomic.h>

#define YF_HAVE_ATOMIC_CMP_SWP  1

#if (YF_SIZEOF_PTR == 8)
typedef yf_s64_t                     yf_atomic_int_t;
typedef yf_u64_t                    yf_atomic_uint_t;
#define YF_ATOMIC_T_LEN            YF_INT64_LEN

#define yf_atomic_cmp_swp(lock, old, new) \
        OSAtomicCompareAndSwap64Barrier(old, new, (yf_s64_t *) lock)

#define yf_atomic_fetch_add(value, add) \
        (OSAtomicAdd64(add, (yf_s64_t *) value) - add)
#define yf_atomic_fetch_sub(value, sub) \
        (OSAtomicAdd64(-sub, (yf_s64_t *) value) + sub)

#else

typedef yf_s32_t                     yf_atomic_int_t;
typedef yf_u32_t                     yf_atomic_uint_t;
#define YF_ATOMIC_T_LEN            YF_INT32_LEN

#define yf_atomic_cmp_swp(lock, old, new) \
        OSAtomicCompareAndSwap32Barrier(old, new, (yf_s32_t *) lock)

#define yf_atomic_fetch_add(value, add) \
        (OSAtomicAdd32(add, (yf_s32_t *) value) - add)
#define yf_atomic_fetch_sub(value, sub) \
        (OSAtomicAdd32(-sub, (yf_s32_t *) value) + sub)

#endif

#define yf_memory_barrier()        OSMemoryBarrier()
#define yf_cpu_pause()

#endif


#if !(YF_HAVE_ATOMIC_CMP_SWP)
#define YF_HAVE_ATOMIC_CMP_SWP  0
#else
typedef volatile yf_atomic_uint_t  yf_atomic_t;
#endif

#endif



