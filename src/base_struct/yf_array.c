#include <ppc/yf_header.h>
#include <base_struct/yf_core.h>

yf_array_t *
yf_array_create(yf_pool_t *p, yf_uint_t n, size_t size)
{
        yf_array_t *a;

        a = yf_palloc(p, sizeof(yf_array_t));
        if (a == NULL)
        {
                return NULL;
        }

        a->elts = yf_palloc(p, n * size);
        if (a->elts == NULL)
        {
                return NULL;
        }

        a->nelts = 0;
        a->size = size;
        a->nalloc = n;
        a->pool = p;

        return a;
}


void
yf_array_destroy(yf_array_t *a)
{
        yf_pool_t *p;

        p = a->pool;

        if ((char *)a->elts + a->size * a->nalloc == p->d.last)
        {
                p->d.last -= a->size * a->nalloc;
        }

        if ((char *)a + sizeof(yf_array_t) == p->d.last)
        {
                p->d.last = (char *)a;
        }
}


void *
yf_array_push(yf_array_t *a)
{
        void *elt, *new;
        size_t size;
        yf_pool_t *p;

        if (a->nelts == a->nalloc)
        {
                /* the array is full */
                size = a->size * a->nalloc;

                p = a->pool;

                if ((char *)a->elts + size == p->d.last
                    && p->d.last + a->size <= p->d.end)
                {
                        /*
                         * the array allocation is the last in the pool
                         * and there is space for new allocation
                         */
                        p->d.last += a->size;
                        a->nalloc++;
                }
                else {
                        /* allocate a new array */
                        new = yf_palloc(p, 2 * size);
                        if (new == NULL)
                        {
                                return NULL;
                        }

                        yf_memcpy(new, a->elts, size);
                        a->elts = new;
                        a->nalloc *= 2;
                }
        }

        elt = (char *)a->elts + a->size * a->nelts;
        a->nelts++;

        return elt;
}


void *
yf_array_push_n(yf_array_t *a, yf_uint_t n)
{
        void *elt, *new;
        size_t size;
        yf_uint_t nalloc;
        yf_pool_t *p;

        size = n * a->size;

        if (a->nelts + n > a->nalloc)
        {
                /* the array is full */
                p = a->pool;

                if ((char *)a->elts + a->size * a->nalloc == p->d.last
                    && p->d.last + size <= p->d.end)
                {
                        /*
                         * the array allocation is the last in the pool
                         * and there is space for new allocation
                         */
                        p->d.last += size;
                        a->nalloc += n;
                }
                else {
                        /* allocate a new array */

                        nalloc = 2 * ((n >= a->nalloc) ? n : a->nalloc);

                        new = yf_palloc(p, nalloc * a->size);
                        if (new == NULL)
                        {
                                return NULL;
                        }

                        yf_memcpy(new, a->elts, a->nelts * a->size);
                        a->elts = new;
                        a->nalloc = nalloc;
                }
        }

        elt = (char *)a->elts + a->size * a->nelts;
        a->nelts += n;

        return elt;
}
