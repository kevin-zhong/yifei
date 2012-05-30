#ifndef _YF_ARRAY_H_INCLUDED_
#define _YF_ARRAY_H_INCLUDED_


#include <base_struct/yf_core.h>
#include <ppc/yf_header.h>

struct yf_array_s
{
        void *     elts;
        yf_uint_t  nelts;
        size_t     size;
        yf_uint_t  nalloc;
        yf_pool_t *pool;
};


yf_array_t *yf_array_create(yf_pool_t *p, yf_uint_t n, size_t size);
void yf_array_destroy(yf_array_t *a);
void *yf_array_push(yf_array_t *a);
void *yf_array_push_n(yf_array_t *a, yf_uint_t n);


static inline yf_int_t
yf_array_init(yf_array_t *array, yf_pool_t *pool, yf_uint_t n, size_t size)
{
        array->nelts = 0;
        array->size = size;
        array->nalloc = n;
        array->pool = pool;

        array->elts = yf_palloc(pool, n * size);
        if (array->elts == NULL)
        {
                return YF_ERROR;
        }

        return YF_OK;
}


#endif
