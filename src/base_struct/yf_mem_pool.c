#include <ppc/yf_header.h>
#include <base_struct/yf_core.h>

#define YF_MAX_ALLOC_FROM_POOL  (yf_pagesize - 1)

static void *yf_palloc_block(yf_pool_t *pool, size_t size);
static void *yf_palloc_large(yf_pool_t *pool, size_t size);


yf_pool_t *
yf_create_pool(size_t size, yf_log_t *log)
{
        yf_pool_t *p;

        p = yf_memalign(YF_POOL_ALIGNMENT, size, log);
        if (p == NULL)
        {
                return NULL;
        }

        yf_memzero(p, sizeof(yf_pool_t));

        p->d.last = (char *)p + sizeof(yf_pool_t);
        p->d.end = (char *)p + size;
        p->d.next = NULL;
        p->d.failed = 0;

        size = size - sizeof(yf_pool_t);
        p->max = (size < YF_MAX_ALLOC_FROM_POOL) ? size : YF_MAX_ALLOC_FROM_POOL;

        p->current = p;
        p->chain = NULL;
        p->large = NULL;
        p->cleanup = NULL;
        p->log = log;

        return p;
}


void
yf_destroy_pool(yf_pool_t *pool)
{
        yf_pool_t *p, *n;
        yf_pool_large_t *l;
        yf_pool_cleanup_t *c;

        for (c = pool->cleanup; c; c = c->next)
        {
                if (c->handler)
                {
                        yf_log_debug1(YF_LOG_DEBUG, pool->log, 0,
                                      "run cleanup: %p", c);
                        c->handler(c->data);
                }
        }

        for (l = pool->large; l; l = l->next)
        {
                yf_log_debug1(YF_LOG_DEBUG, pool->log, 0, "free: %p", l->alloc);

                if (l->alloc)
                {
                        yf_free(l->alloc);
                }
        }

#ifdef YF_DEBUG

        /*
         * we could allocate the pool->log from this pool
         * so we can not use this log while the free()ing the pool
         */

        for (p = pool, n = pool->d.next; /* void */; p = n, n = n->d.next)
        {
                yf_log_debug2(YF_LOG_DEBUG, pool->log, 0,
                              "free: %p, unused: %uz", p, p->d.end - p->d.last);

                if (n == NULL)
                {
                        break;
                }
        }

#endif

        for (p = pool, n = pool->d.next; /* void */; p = n, n = n->d.next)
        {
                yf_free(p);

                if (n == NULL)
                {
                        break;
                }
        }
}


void
yf_reset_pool(yf_pool_t *pool)
{
        yf_pool_t *p;
        yf_pool_large_t *l;
        yf_pool_cleanup_t *c;

        for (l = pool->large; l; l = l->next)
        {
                if (l->alloc)
                {
                        yf_free(l->alloc);
                }
        }

        pool->large = NULL;

        for (c = pool->cleanup; c; c = c->next)
        {
                if (c->handler)
                {
                        yf_log_debug1(YF_LOG_DEBUG, pool->log, 0,
                                      "run cleanup: %p", c);
                        c->handler(c->data);
                }
        }        

        for (p = pool; p; p = p->d.next)
        {
                p->d.last = (char *)p + sizeof(yf_pool_t);
        }
}


void *
yf_palloc(yf_pool_t *pool, size_t size)
{
        char *m;
        yf_pool_t *p;

        if (size <= pool->max)
        {
                p = pool->current;

                do {
                        m = yf_align_ptr(p->d.last, YF_ALIGNMENT);

                        if ((size_t)(p->d.end - m) >= size)
                        {
                                p->d.last = m + size;

                                return m;
                        }

                        p = p->d.next;
                } while (p);

                return yf_palloc_block(pool, size);
        }

        return yf_palloc_large(pool, size);
}


void *
yf_pnalloc(yf_pool_t *pool, size_t size)
{
        char *m;
        yf_pool_t *p;

        if (size <= pool->max)
        {
                p = pool->current;

                do {
                        m = p->d.last;

                        if ((size_t)(p->d.end - m) >= size)
                        {
                                p->d.last = m + size;

                                return m;
                        }

                        p = p->d.next;
                } while (p);

                return yf_palloc_block(pool, size);
        }

        return yf_palloc_large(pool, size);
}


static void *
yf_palloc_block(yf_pool_t *pool, size_t size)
{
        char *m;
        size_t psize;
        yf_pool_t *p, *new, *current;

        psize = (size_t)(pool->d.end - (char *)pool);

        m = yf_memalign(YF_POOL_ALIGNMENT, psize, pool->log);
        if (m == NULL)
        {
                return NULL;
        }

        new = (yf_pool_t *)m;

        new->d.end = m + psize;
        new->d.next = NULL;
        new->d.failed = 0;

        m += sizeof(yf_pool_data_t);
        m = yf_align_ptr(m, YF_ALIGNMENT);
        new->d.last = m + size;

        current = pool->current;

        for (p = current; p->d.next; p = p->d.next)
        {
                if (p->d.failed++ > 4)
                {
                        current = p->d.next;
                }
        }

        p->d.next = new;

        pool->current = current ? current : new;

        return m;
}


static void *
yf_palloc_large(yf_pool_t *pool, size_t size)
{
        void *p;
        yf_uint_t n;
        yf_pool_large_t *large;

        p = yf_alloc(size);
        if (p == NULL)
        {
                return NULL;
        }

        n = 0;

        for (large = pool->large; large; large = large->next)
        {
                if (large->alloc == NULL)
                {
                        large->alloc = p;
                        return p;
                }

                if (n++ > 3)
                {
                        break;
                }
        }

        large = yf_palloc(pool, sizeof(yf_pool_large_t));
        if (large == NULL)
        {
                yf_free(p);
                return NULL;
        }

        large->alloc = p;
        large->next = pool->large;
        pool->large = large;

        return p;
}


void *
yf_pmemalign(yf_pool_t *pool, size_t size, size_t alignment)
{
        void *p;
        yf_pool_large_t *large;

        p = yf_memalign(alignment, size, pool->log);
        if (p == NULL)
        {
                return NULL;
        }

        large = yf_palloc(pool, sizeof(yf_pool_large_t));
        if (large == NULL)
        {
                yf_free(p);
                return NULL;
        }

        large->alloc = p;
        large->next = pool->large;
        pool->large = large;

        return p;
}


yf_int_t
yf_pfree(yf_pool_t *pool, void *p)
{
        yf_pool_large_t *l;

        for (l = pool->large; l; l = l->next)
        {
                if (p == l->alloc)
                {
                        yf_log_debug1(YF_LOG_DEBUG, pool->log, 0,
                                      "free: %p", l->alloc);
                        yf_free(l->alloc);
                        l->alloc = NULL;

                        return YF_OK;
                }
        }

        return YF_DECLINED;
}


void *
yf_pcalloc(yf_pool_t *pool, size_t size)
{
        void *p;

        p = yf_palloc(pool, size);
        if (p)
        {
                yf_memzero(p, size);
        }

        return p;
}


yf_pool_cleanup_t *
yf_pool_cleanup_add(yf_pool_t *p, size_t size)
{
        yf_pool_cleanup_t *c;

        c = yf_palloc(p, sizeof(yf_pool_cleanup_t));
        if (c == NULL)
        {
                return NULL;
        }

        if (size)
        {
                c->data = yf_palloc(p, size);
                if (c->data == NULL)
                {
                        return NULL;
                }
        }
        else {
                c->data = NULL;
        }

        c->handler = NULL;
        c->next = p->cleanup;

        p->cleanup = c;

        yf_log_debug1(YF_LOG_DEBUG, p->log, 0, "add cleanup: %p", c);

        return c;
}


