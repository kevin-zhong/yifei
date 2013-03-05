#ifndef _YF_SLAB_POOL_H_20120907_H
#define _YF_SLAB_POOL_H_20120907_H
/*
* copyright@: kevin_zhong, mail:qq2000zhong@gmail.com
* time: 20120907-17:18:30
*/

#include <base_struct/yf_core.h>
#include <ppc/yf_header.h>

struct yf_slab_pool_s
{
        yf_pool_t  *base_pool;
        yf_slist_part_t  *free;
        yf_u16_t   *size;
        yf_u32_t   type:7;
};


//must end with last size=0, max=16k
yf_int_t  yf_slab_pool_init(struct yf_slab_pool_s* slab_pool, yf_log_t* log, ...);
void yf_slab_pool_destory(struct yf_slab_pool_s* slab_pool, yf_log_t* log);

void* yf_slab_pool_alloc(struct yf_slab_pool_s* slab_pool, yf_u16_t size, yf_log_t* log);
void yf_slab_pool_free(struct yf_slab_pool_s* slab_pool, void* data, yf_log_t* log);



#endif
