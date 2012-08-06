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


typedef struct yf_output_chain_ctx_s yf_output_chain_ctx_t;

typedef yf_int_t (*yf_output_chain_filter_pt)(void *ctx, yf_chain_t *in);


struct yf_output_chain_ctx_s
{
        yf_buf_t *                buf;
        yf_chain_t *              in;
        yf_chain_t *              free;
        yf_chain_t *              busy;
        
        yf_pool_t *               pool;
        yf_int_t                    allocated;
        yf_bufs_t                  bufs;

        yf_output_chain_filter_pt output_filter;
        void *                    filter_ctx;
};


#define YF_CHAIN_ERROR     (yf_chain_t *)YF_ERROR

#define yf_buf_size(b) (size_t)(b->last - b->pos)


yf_buf_t *yf_create_temp_buf(yf_pool_t *pool, size_t size);
yf_chain_t *yf_create_chain_of_bufs(yf_pool_t *pool, yf_bufs_t *bufs);


#define yf_alloc_buf(pool)  yf_palloc(pool, sizeof(yf_buf_t))
#define yf_calloc_buf(pool) yf_pcalloc(pool, sizeof(yf_buf_t))

yf_chain_t *yf_alloc_chain_link(yf_pool_t *pool);

#define yf_free_chain(pool, cl) do {\
        cl->next = pool->chain; \
        pool->chain = cl; \
} while (0)

yf_int_t yf_chain_add_copy(yf_pool_t *pool, yf_chain_t **chain, yf_chain_t *in);
yf_chain_t *yf_chain_get_free_buf(yf_pool_t *p, yf_chain_t **free);
void yf_chain_update_chains(yf_chain_t **free, yf_chain_t **busy, yf_chain_t **out);

void yf_add_chain_to_tail(yf_chain_t **head, yf_chain_t *chain);


yf_int_t yf_output_chain(yf_output_chain_ctx_t *ctx, yf_chain_t *in);
yf_int_t yf_chain_writer(void *ctx, yf_chain_t *in);

#endif

