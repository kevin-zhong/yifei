#ifndef _YF_HASH_H
#define _YF_HASH_H

#include <base_struct/yf_core.h>
#include <ppc/yf_header.h>


typedef struct
{
        void *  value;
        yf_u8_t len;
        char    name[1];
} 
yf_hash_elt_t;


typedef struct
{
        yf_hash_elt_t **buckets;
        yf_uint_t       size;
} 
yf_hash_t;


typedef struct
{
        yf_str_t  key;
        yf_uint_t key_hash;
        void *    value;
} 
yf_hash_key_t;

/*
* str hash, after inited, cant modify any more, used for config or something
* ignore case when cmp key, key len < 256
*/
typedef yf_uint_t (*yf_hash_key_pt)(char *data, size_t len);


typedef struct
{
        yf_hash_t *    hash;
        yf_hash_key_pt key;

        yf_uint_t      max_size;
        yf_uint_t      bucket_size;

        const char *   name;
        yf_pool_t *    pool;
        yf_pool_t *    temp_pool;
} 
yf_hash_init_t;


#define YF_HASH_SMALL            1
#define YF_HASH_LARGE            2

#define YF_HASH_LARGE_ASIZE      16384
#define YF_HASH_LARGE_HSIZE      10007

#define YF_HASH_READONLY_KEY     2

typedef struct
{
        yf_uint_t   hsize;

        yf_pool_t * pool;
        yf_pool_t * temp_pool;

        yf_array_t  keys;
        yf_array_t *keys_hash;
} 
yf_hash_keys_arrays_t;


void *yf_hash_find(yf_hash_t *hash, yf_uint_t key, char *name, size_t len);

yf_int_t yf_hash_init(yf_hash_init_t *hinit, yf_hash_key_t *names, yf_uint_t nelts);

#define yf_hash(key, c)   ((yf_uint_t)key * 31 + c)
yf_uint_t yf_hash_key(char *data, size_t len);
yf_uint_t yf_hash_key_lc(char *data, size_t len);
yf_uint_t yf_hash_strlow(char *dst, char *src, size_t n);


yf_int_t yf_hash_keys_array_init(yf_hash_keys_arrays_t *ha, yf_uint_t type);
yf_int_t yf_hash_add_key(yf_hash_keys_arrays_t *ha, yf_str_t *key, void *value, yf_uint_t flags);

//--str hash end --

#endif
