#include <ppc/yf_header.h>
#include <base_struct/yf_core.h>

static const unsigned long _yf_tpl_hash_num_primes[] = { 
        53,       97,      193,         389,     769,     1543,     3079,     
        6151,     12289,     24593,     49157,     98317,     196613,  393241, 786433,
        1572869,  3145739, 6291469, 12582917, 25165843, 
        50331653, 100663319, 201326611 
        };

yf_int_t  yf_tpl_hash_init(yf_tpl_hash_t* hash, yf_uint_t suggest_buckets, 
                yf_tpl_hash_pt  hfunc, yf_tpl_cmp_pt  cmp)
{
        yf_u32_t i1 = 0;
        yf_memzero(hash, sizeof(yf_tpl_hash_t));
        hash->hash = hfunc;
        hash->cmp = cmp;

        for ( i1 = 0; i1 < YF_ARRAY_SIZE(_yf_tpl_hash_num_primes); i1++ )
        {
                if (_yf_tpl_hash_num_primes[i1] >= suggest_buckets)
                {
                        suggest_buckets = _yf_tpl_hash_num_primes[i1];
                        break;
                }
        }

        hash->bucket_size = suggest_buckets;
        hash->buckets = yf_alloc(suggest_buckets * sizeof(yf_tpl_hash_link_t*));
        CHECK_RV(hash->buckets == NULL, -1);

        for ( i1 = 0; i1 < suggest_buckets ; i1++ )
        {
                yf_init_slist_head(hash->buckets[i1]);
        }
        return 0;
}


void  yf_tpl_hash_insert(yf_tpl_hash_t* hash, yf_tpl_hash_link_t* nlinker
                , yf_uint_t hashkey)
{
        yf_u32_t i1 = 0;
        if (unlikely(hash->elt_size >= hash->buckets))
        {
                yf_uint_t  nbuckets = hash->bucket_size;
                yf_tpl_hash_link_t** new_buckets;
                yf_tpl_hash_link_t* linker;
                
                for ( i1 = 0; i1 < YF_ARRAY_SIZE(_yf_tpl_hash_num_primes); i1++ )
                {
                        if (_yf_tpl_hash_num_primes[i1] >= nbuckets)
                        {
                                nbuckets = _yf_tpl_hash_num_primes[i1];
                                break;
                        }
                }
                if (unlikely(nbuckets > hash->bucket_size))
                        goto insert;
                
                new_buckets = yf_alloc(nbuckets * sizeof(yf_tpl_hash_link_t*));
                if (unlikely(new_buckets == NULL))
                        goto insert;

                for ( i1 = 0; i1 < nbuckets ; i1++ )
                {
                        yf_init_slist_head(new_buckets[i1]);
                }

                for ( i1 = 0; i1 < hash->bucket_size ; i1++ )
                {
                        while ((linker = yf_slist_pop(hash->buckets[i1])) != NULL)
                        {
                                yf_slist_push(linker, new_buckets[hash->hash(linker) % nbuckets]);
                        }
                }

                hash->buckets = new_buckets;
        }

insert:        
        hash->elt_size++;
        yf_slist_push(nlinker, hash->buckets[hashkey % hash->bucket_size]);
}



yf_tpl_hash_link_t*  yf_tpl_hash_find(yf_tpl_hash_t* hash, void* key, size_t len
                , yf_uint_t hashkey, yf_tpl_hash_link_t** pre)
{
        yf_tpl_hash_link_t* linker, *header = hash->buckets[hashkey % hash->bucket_size];
        yf_tpl_hash_link_t* tpre = header;
        hash->find_cnt++;
        
        yf_slist_for_each(linker, header)
        {
                hash->find_cmp++;
                if (hash->cmp(linker, key, len) == 0)
                {
                        if (pre)
                                *pre = tpre;
                        return linker;
                }
                tpre = linker;
        }
        return NULL;
}

