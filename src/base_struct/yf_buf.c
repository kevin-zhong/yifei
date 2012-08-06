#include <ppc/yf_header.h>
#include <base_struct/yf_core.h>

yf_buf_t *
yf_create_temp_buf(yf_pool_t *pool, size_t size)
{
        yf_buf_t *b;

        b = yf_calloc_buf(pool);
        if (b == NULL)
        {
                return NULL;
        }

        b->start = yf_palloc(pool, size);
        if (b->start == NULL)
        {
                return NULL;
        }

        b->pos = b->start;
        b->last = b->start;
        b->end = b->last + size;
        b->temporary = 1;

        return b;
}


yf_chain_t *
yf_alloc_chain_link(yf_pool_t *pool)
{
        yf_chain_t *cl;

        cl = pool->chain;

        if (cl)
        {
                pool->chain = cl->next;
                return cl;
        }

        cl = yf_palloc(pool, sizeof(yf_chain_t));
        if (cl == NULL)
        {
                return NULL;
        }

        return cl;
}


yf_chain_t *
yf_create_chain_of_bufs(yf_pool_t *pool, yf_bufs_t *bufs)
{
        char *p;
        yf_int_t i;
        yf_buf_t *b;
        yf_chain_t *chain, *cl, **ll;

        p = yf_palloc(pool, bufs->num * bufs->size);
        if (p == NULL)
        {
                return NULL;
        }

        ll = &chain;

        for (i = 0; i < bufs->num; i++)
        {
                b = yf_calloc_buf(pool);
                if (b == NULL)
                {
                        return NULL;
                }

                b->pos = p;
                b->last = p;
                b->temporary = 1;

                b->start = p;
                p += bufs->size;
                b->end = p;

                cl = yf_alloc_chain_link(pool);
                if (cl == NULL)
                {
                        return NULL;
                }

                cl->buf = b;
                *ll = cl;
                ll = &cl->next;
        }

        *ll = NULL;

        return chain;
}


yf_int_t
yf_chain_add_copy(yf_pool_t *pool, yf_chain_t **chain, yf_chain_t *in)
{
        yf_chain_t *cl, **ll;

        ll = chain;

        for (cl = *chain; cl; cl = cl->next)
        {
                ll = &cl->next;
        }

        while (in)
        {
                cl = yf_alloc_chain_link(pool);
                if (cl == NULL)
                {
                        return YF_ERROR;
                }

                cl->buf = in->buf;
                *ll = cl;
                ll = &cl->next;
                in = in->next;
        }

        *ll = NULL;

        return YF_OK;
}


yf_chain_t *
yf_chain_get_free_buf(yf_pool_t *p, yf_chain_t **free)
{
        yf_chain_t *cl;

        if (*free)
        {
                cl = *free;
                *free = cl->next;
                cl->next = NULL;
                return cl;
        }

        cl = yf_alloc_chain_link(p);
        if (cl == NULL)
        {
                return NULL;
        }

        cl->buf = yf_calloc_buf(p);
        if (cl->buf == NULL)
        {
                return NULL;
        }

        cl->next = NULL;

        return cl;
}


void
yf_chain_update_chains(yf_chain_t **free
        , yf_chain_t **busy
        , yf_chain_t **out)
{
        yf_chain_t *cl;

        yf_add_chain_to_tail(busy, *out);
        *out = NULL;

        while (*busy)
        {
                if (yf_buf_size((*busy)->buf) != 0)
                {
                        break;
                }

                (*busy)->buf->pos = (*busy)->buf->start;
                (*busy)->buf->last = (*busy)->buf->start;

                cl = *busy;
                *busy = cl->next;
                cl->next = *free;
                *free = cl;
        }
}


void yf_add_chain_to_tail(yf_chain_t **head, yf_chain_t *chain)
{
        yf_chain_t *cl;
        
        if (*head)
                *head = chain;
        else {
                for (cl = *head; cl->next; cl = cl->next)
                {
                }
                cl->next = chain;
        }
}

