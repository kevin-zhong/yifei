#ifndef _YF_TPL_HASH_H_20130316_H
#define _YF_TPL_HASH_H_20130316_H
/*
* copyright@: kevin_zhong, mail:qq2000zhong@gmail.com
* time: 20130316-19:24:15
*/

#include <base_struct/yf_core.h>
#include <ppc/yf_header.h>

/*
* so cheap hashtable, in fact, no need to use this hashtable
* this hashtable just have statics + relarge buckets, just so so...
*/

typedef yf_slist_part_t yf_tpl_hash_link_t;


typedef yf_uint_t (*yf_tpl_hash_pt)(yf_tpl_hash_link_t* linker);
typedef yf_int_t   (*yf_tpl_cmp_pt)(yf_tpl_hash_link_t* linker, void* key, size_t len);

typedef struct
{
        yf_tpl_hash_link_t **buckets;
        yf_uint_t       bucket_size;
        yf_uint_t       elt_size;

        yf_tpl_hash_pt  hash;
        yf_tpl_cmp_pt  cmp;

        //statics
        yf_u8_t  max_bucket_elt;
        yf_u64_t  find_cnt;
        yf_u64_t  find_cmp;
} 
yf_tpl_hash_t;

yf_int_t  yf_tpl_hash_init(yf_tpl_hash_t* hash, yf_uint_t suggest_buckets
                , yf_tpl_hash_pt  hfunc, yf_tpl_cmp_pt  cmp);

void  yf_tpl_hash_insert(yf_tpl_hash_t* hash, yf_tpl_hash_link_t* nlinker
                , yf_uint_t hashkey);

yf_tpl_hash_link_t*  yf_tpl_hash_find(yf_tpl_hash_t* hash, void* key, size_t len
                , yf_uint_t hashkey, yf_tpl_hash_link_t** pre);

#define yf_tpl_hash_delete(hash, pre, target) do { \
                yf_slist_delete(pre, target); \
                hash->elt_size--; \
        } while (0)


#endif

