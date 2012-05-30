#ifndef _YF_PALLOC_H_INCLUDED_
#define _YF_PALLOC_H_INCLUDED_

#include <base_struct/yf_core.h>
#include <ppc/yf_header.h>

#define YF_DEFAULT_POOL_SIZE    (16 * 1024)

#define YF_POOL_ALIGNMENT       16


typedef void (*yf_pool_cleanup_pt)(void *data);

struct yf_pool_cleanup_s
{
        yf_pool_cleanup_pt handler;
        void *             data;
        struct yf_pool_cleanup_s *next;
};

typedef struct yf_pool_cleanup_s yf_pool_cleanup_t;


struct yf_pool_large_s
{
        struct yf_pool_large_s *next;
        void *           alloc;
};
typedef struct yf_pool_large_s yf_pool_large_t;


typedef struct
{
        char *   last;
        char *   end;
        yf_pool_t *next;
        yf_uint_t  failed;
} 
yf_pool_data_t;


struct yf_pool_s
{
        yf_pool_data_t     d;
        size_t             max;
        yf_pool_t *        current;
        yf_chain_t *       chain;
        yf_pool_large_t *  large;
        yf_pool_cleanup_t *cleanup;
        yf_log_t *         log;
};

#define YF_MIN_POOL_SIZE                                                     \
        yf_align((sizeof(yf_pool_t) + 2 * sizeof(yf_pool_large_t)),            \
                 YF_POOL_ALIGNMENT)

yf_pool_t *yf_create_pool(size_t size, yf_log_t *log);
void yf_destroy_pool(yf_pool_t *pool);
void yf_reset_pool(yf_pool_t *pool);

void *yf_palloc(yf_pool_t *pool, size_t size);
void *yf_pnalloc(yf_pool_t *pool, size_t size);
void *yf_pcalloc(yf_pool_t *pool, size_t size);
void *yf_pmemalign(yf_pool_t *pool, size_t size, size_t alignment);
yf_int_t yf_pfree(yf_pool_t *pool, void *p);


yf_pool_cleanup_t *yf_pool_cleanup_add(yf_pool_t *p, size_t size);


#endif

