#ifndef _YF_BUF_H
#define _YF_BUF_H

#include <base_struct/yf_core.h>
#include <ppc/yf_header.h>

typedef struct yf_buf_s yf_buf_t;

struct yf_buf_s
{
        char *   pos;
        char *   last;
        
        char *   start;
        char *   end;

        /* the buf's content could be changed */
        unsigned temporary : 1;
        unsigned recycled : 1;
        unsigned last_buf : 1;
        unsigned last_in_chain : 1;
};

struct yf_chain_s
{
        yf_buf_t *  buf;
        yf_chain_t *next;
};

typedef struct
{
        yf_int_t num;
        size_t   size;
} 
yf_bufs_t;


#define YF_CHAIN_ERROR     (yf_chain_t *)YF_ERROR

#define yf_buf_size(b) (size_t)(b->last - b->pos)

//fill buf with mem
yf_buf_t *yf_alloc_buf_mem(yf_pool_t *pool, yf_buf_t * buf, size_t size);

//ret new buf with mem buf
yf_buf_t *yf_create_temp_buf(yf_pool_t *pool, size_t size);

//ret chains+buf+mem
yf_chain_t *yf_create_chain_of_bufs(yf_pool_t *pool, yf_bufs_t *bufs);


#define yf_alloc_buf(pool)  yf_palloc(pool, sizeof(yf_buf_t))
#define yf_calloc_buf(pool) yf_pcalloc(pool, sizeof(yf_buf_t))

/*
* ret chain maybe buf=NULL, or with buf which is freed to pool after use
* so must check the value of buf, if buf=NULL, you can call yf_create_temp_buf(p,..)
* add on 2013/03/21 21:10
*/
yf_chain_t *yf_alloc_chain_link(yf_pool_t *pool);

#define yf_free_chain(pool, cl) do {\
        cl->next = pool->chain; \
        pool->chain = cl; \
} while (0)

/*
* two chains take the same bufs
*/
yf_int_t yf_chain_add_copy(yf_pool_t *pool, yf_chain_t **chain, yf_chain_t *in);

/*
* ret chain have buf != NULL, but maybe start=end=NULL(without mem buf)
*/
yf_chain_t *yf_chain_get_free_buf(yf_pool_t *p, yf_chain_t **free);

void yf_chain_update_chains(yf_chain_t **free, yf_chain_t **busy, yf_chain_t **out);

void yf_add_chain_to_tail(yf_chain_t **head, yf_chain_t *chain);

#endif

