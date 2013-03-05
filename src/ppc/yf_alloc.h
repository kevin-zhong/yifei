#ifndef  _YF_PPC_ALLOC_H
#define _YF_PPC_ALLOC_H

#include <base_struct/yf_core.h>
#include <ppc/yf_header.h>

extern yf_uint_t yf_pagesize;
extern yf_uint_t yf_cacheline_size;




#ifdef  _MAX_ALIGNMENT
#define  YF_ALIGNMENT  _MAX_ALIGNMENT
#else
#define  YF_ALIGNMENT  SIZEOF_LONG
#endif

#define  yf_align_mem(a) yf_align(a, YF_ALIGNMENT)

#define  yf_mem_off(a, o) ((void*)(((char*)a) + (o)))

#define  yf_alloc(len)  ({void* pr = malloc(len); if (pr) yf_memzero(pr, len); pr;})
#define  yf_realloc  realloc
#define  yf_free(a)  { if (a) { free(a); a = NULL; }}

#if (HAVE_POSIX_MEMALIGN || HAVE_MEMALIGN)
void *yf_memalign(size_t alignment, size_t size, yf_log_t *log);
#else
#define yf_memalign(alignment, size, yf_log_t *log)  yf_alloc(size)
#endif

#endif
