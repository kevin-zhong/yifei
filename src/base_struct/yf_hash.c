#include <ppc/yf_header.h>
#include <base_struct/yf_core.h>

void *
yf_hash_find(yf_hash_t *hash, yf_uint_t key, char *name, size_t len)
{
        yf_uint_t i;
        yf_hash_elt_t *elt;

        elt = hash->buckets[key % hash->size];

        if (elt == NULL)
        {
                return NULL;
        }

        while (elt->value)
        {
                if (len != (size_t)elt->len)
                {
                        goto next;
                }

                for (i = 0; i < len; i++)
                {
                        if (yf_tolower(name[i]) != elt->name[i])
                        {
                                goto next;
                        }
                }

                return elt->value;

next:
                elt = (yf_hash_elt_t *)yf_align_ptr(&elt->name[0] + elt->len, YF_ALIGNMENT);
                continue;
        }

        return NULL;
}


#define YF_HASH_ELT_SIZE(name)                                               \
        (sizeof(void *) + yf_align((name)->key.len + 1, YF_ALIGNMENT))

yf_int_t
yf_hash_init(yf_hash_init_t *hinit, yf_hash_key_t *names, yf_uint_t nelts)
{
        char *elts;
        size_t len;
        yf_u16_t *test;
        yf_uint_t i, n, key, size, start, bucket_size;
        yf_hash_elt_t *elt, **buckets;

        for (n = 0; n < nelts; n++)
        {
                if (hinit->bucket_size < YF_HASH_ELT_SIZE(&names[n]) + sizeof(void *))
                {
                        yf_log_error(YF_LOG_EMERG, hinit->pool->log, 0,
                                     "could not build the %s, you should "
                                     "increase %s_bucket_size: %i",
                                     hinit->name, hinit->name, hinit->bucket_size);
                        return YF_ERROR;
                }
        }

        test = yf_palloc(hinit->pool, hinit->max_size * sizeof(yf_u16_t));
        if (test == NULL)
        {
                return YF_ERROR;
        }

        bucket_size = hinit->bucket_size - sizeof(void *);

        start = nelts / (bucket_size / (2 * sizeof(void *)));
        start = start ? start : 1;

        if (hinit->max_size > 10000 && hinit->max_size / nelts < 100)
        {
                start = hinit->max_size - 1000;
        }

        for (size = start; size < hinit->max_size; size++)
        {
                yf_memzero(test, size * sizeof(yf_u16_t));

                for (n = 0; n < nelts; n++)
                {
                        if (names[n].key.data == NULL)
                        {
                                continue;
                        }

                        key = names[n].key_hash % size;
                        test[key] = (yf_u16_t)(test[key] + YF_HASH_ELT_SIZE(&names[n]));
#if 0
                        yf_log_error(YF_LOG_ALERT, hinit->pool->log, 0,
                                     "%ui: %ui %ui \"%V\"",
                                     size, key, test[key], &names[n].key);
#endif
                        if (test[key] > (yf_u16_t)bucket_size)
                        {
                                goto next;
                        }
                }

                goto found;
next:
                continue;
        }

        yf_log_error(YF_LOG_EMERG, hinit->pool->log, 0,
                     "could not build the %s, you should increase "
                     "either %s_max_size: %i or %s_bucket_size: %i",
                     hinit->name, hinit->name, hinit->max_size,
                     hinit->name, hinit->bucket_size);

        yf_pfree(hinit->pool, test);

        return YF_ERROR;

found:
        for (i = 0; i < size; i++)
        {
                test[i] = sizeof(void *);
        }

        for (n = 0; n < nelts; n++)
        {
                if (names[n].key.data == NULL)
                {
                        continue;
                }

                key = names[n].key_hash % size;
                test[key] = (yf_u16_t)(test[key] + YF_HASH_ELT_SIZE(&names[n]));
        }

        len = 0;

        for (i = 0; i < size; i++)
        {
                if (test[i] == sizeof(void *))
                {
                        continue;
                }

                test[i] = (yf_u16_t)(yf_align(test[i], yf_cacheline_size));

                len += test[i];
        }

        if (hinit->hash == NULL)
        {
                yf_pfree(hinit->pool, test);;
                return YF_ERROR;
        }
        else {
                buckets = yf_pcalloc(hinit->pool, size * sizeof(yf_hash_elt_t *));
                if (buckets == NULL)
                {
                        yf_pfree(hinit->pool, test);;
                        return YF_ERROR;
                }
        }

        elts = yf_palloc(hinit->pool, len + yf_cacheline_size);
        if (elts == NULL)
        {
                yf_pfree(hinit->pool, test);
                return YF_ERROR;
        }

        elts = yf_align_ptr(elts, YF_ALIGNMENT);

        for (i = 0; i < size; i++)
        {
                if (test[i] == sizeof(void *))
                {
                        continue;
                }

                buckets[i] = (yf_hash_elt_t *)elts;
                elts += test[i];
        }

        for (i = 0; i < size; i++)
        {
                test[i] = 0;
        }

        for (n = 0; n < nelts; n++)
        {
                if (names[n].key.data == NULL)
                {
                        continue;
                }

                key = names[n].key_hash % size;
                elt = (yf_hash_elt_t *)((char *)buckets[key] + test[key]);

                elt->value = names[n].value;
                elt->len = (yf_u16_t)names[n].key.len;

                yf_strtolower_copy(elt->name, names[n].key.data, names[n].key.len);

                test[key] = (yf_u16_t)(test[key] + YF_HASH_ELT_SIZE(&names[n]));
        }

        for (i = 0; i < size; i++)
        {
                if (buckets[i] == NULL)
                {
                        continue;
                }

                elt = (yf_hash_elt_t *)((char *)buckets[i] + test[i]);

                elt->value = NULL;
        }

        yf_pfree(hinit->pool, test);;

        hinit->hash->buckets = buckets;
        hinit->hash->size = size;

        return YF_OK;
}


yf_uint_t
yf_hash_key(char *data, size_t len)
{
        yf_uint_t i, key;

        key = 0;

        for (i = 0; i < len; i++)
        {
                key = yf_hash(key, data[i]);
        }

        return key;
}


yf_uint_t
yf_hash_key_lc(char *data, size_t len)
{
        yf_uint_t i, key;

        key = 0;

        for (i = 0; i < len; i++)
        {
                key = yf_hash(key, yf_tolower(data[i]));
        }

        return key;
}


yf_uint_t
yf_hash_strlow(char *dst, char *src, size_t n)
{
        yf_uint_t key;
        key = 0;

        while (n--)
        {
                *dst = yf_tolower(*src);
                key = yf_hash(key, *dst);
                dst++;
                src++;
        }

        return key;
}


yf_int_t
yf_hash_keys_array_init(yf_hash_keys_arrays_t *ha, yf_uint_t type)
{
        yf_uint_t asize;

        if (type == YF_HASH_SMALL)
        {
                asize = 4;
                ha->hsize = 107;
        }
        else {
                asize = YF_HASH_LARGE_ASIZE;
                ha->hsize = YF_HASH_LARGE_HSIZE;
        }

        if (yf_array_init(&ha->keys, ha->temp_pool, asize, sizeof(yf_hash_key_t))
            != YF_OK)
        {
                return YF_ERROR;
        }

        ha->keys_hash = yf_pcalloc(ha->temp_pool, sizeof(yf_array_t) * ha->hsize);
        if (ha->keys_hash == NULL)
        {
                return YF_ERROR;
        }

        return YF_OK;
}


yf_int_t
yf_hash_add_key(yf_hash_keys_arrays_t *ha, yf_str_t *key, void *value, yf_uint_t flags)
{
        size_t len;
        char *p;
        yf_str_t *name;
        yf_uint_t i, k, n, skip, last;
        yf_array_t *keys, *hwc;
        yf_hash_key_t *hk;

        last = key->len;

        /* exact hash */

        k = 0;

        for (i = 0; i < last; i++)
        {
                if (!(flags & YF_HASH_READONLY_KEY))
                {
                        key->data[i] = yf_tolower(key->data[i]);
                }
                k = yf_hash(k, key->data[i]);
        }

        k %= ha->hsize;

        /* check conflicts in exact hash */

        name = ha->keys_hash[k].elts;

        if (name)
        {
                for (i = 0; i < ha->keys_hash[k].nelts; i++)
                {
                        if (last != name[i].len)
                        {
                                continue;
                        }

                        if (yf_strncmp(key->data, name[i].data, last) == 0)
                        {
                                return YF_BUSY;
                        }
                }
        }
        else {
                if (yf_array_init(&ha->keys_hash[k], ha->temp_pool, 4,
                                  sizeof(yf_str_t))
                    != YF_OK)
                {
                        return YF_ERROR;
                }
        }

        name = yf_array_push(&ha->keys_hash[k]);
        if (name == NULL)
        {
                return YF_ERROR;
        }

        *name = *key;

        hk = yf_array_push(&ha->keys);
        if (hk == NULL)
        {
                return YF_ERROR;
        }

        hk->key = *key;
        hk->key_hash = yf_hash_key(key->data, last);
        hk->value = value;

        return YF_OK;
}
