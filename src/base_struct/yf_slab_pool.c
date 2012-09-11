#include <ppc/yf_header.h>
#include <base_struct/yf_core.h>

typedef  struct yf_slab_trick_s
{
        yf_u32_t  magic;
        yf_u32_t  used:1;
        yf_u32_t  size:14;
        yf_u32_t  emagic:17;
}
yf_slab_trick_t;

#define YF_SLAB_TRICK_SIZE yf_align_mem(sizeof(yf_slab_trick_t))

#define yf_slab_set_trick(slab) (slab)->magic = YF_MAGIC_VAL; \
                (slab)->emagic = (((yf_uint_ptr_t)slab)&0x1ffff);
#define yf_slab_check_trick(slab) ((slab)->magic == YF_MAGIC_VAL && \
                (slab)->emagic == (((yf_uint_ptr_t)slab)&0x1ffff))


yf_int_t  yf_slab_pool_init(struct yf_slab_pool_s* slab_pool, yf_log_t* log, ...)
{
        yf_u16_t  slab_cnts = 0, input_size, i, j, swap_mid;
        yf_u16_t  slab_size[64];
        
        va_list args;
        va_start(args, log);
        while (1)
        {
                input_size = va_arg(args, size_t);
                if (input_size == 0)
                        break;

                input_size = yf_align_mem(input_size);
                CHECK_RV(input_size >= (1<<14), YF_ERROR);
                
                slab_size[slab_cnts++] = input_size;
        }
        va_end(args);

        if (slab_cnts == 0)
                return  YF_ERROR;

        //sort + uniq
        for (i = 0; i < slab_cnts; ++i)
        {
                for (j = i+1; j < slab_cnts; ++j)
                {
                        if (slab_size[j] < slab_size[i])
                        {
                                yf_swap(slab_size[j], slab_size[i], swap_mid);
                        }
                        else if (slab_size[j] == slab_size[i])
                        {
                                yf_log_debug1(YF_LOG_DEBUG, log, 0, "duplicated size=%d", slab_size[j]);
                                --slab_cnts;
                                
                                if (j == slab_cnts)
                                        break;
                                else {
                                        //swap with last elem
                                        yf_swap(slab_size[j], slab_size[slab_cnts], swap_mid);
                                        --j;//try again at the same place
                                }
                        }
                }
        }

        yf_log_debug1(YF_LOG_DEBUG, log, 0, "last type=%d", slab_cnts);

        slab_pool->type = slab_cnts;

        slab_pool->base_pool = yf_create_pool(yf_pagesize * 2, log);
        CHECK_RV(slab_pool->base_pool == NULL, YF_ERROR);

        slab_pool->size = yf_palloc(slab_pool->base_pool, sizeof(yf_u16_t) * slab_cnts);
        assert(slab_pool->size);
        yf_memcpy(slab_pool->size, slab_size, sizeof(yf_u16_t) * slab_cnts);
        
        slab_pool->free = yf_palloc(slab_pool->base_pool, sizeof(yf_slist_part_t) * slab_cnts);
        assert(slab_pool->free);

        for (i = 0; i < slab_cnts; ++i)
                yf_init_slist_head(slab_pool->free + i);
        
        return  YF_OK;
}


void yf_slab_pool_destory(struct yf_slab_pool_s* slab_pool, yf_log_t* log)
{
        yf_destroy_pool(slab_pool->base_pool);
}


#define  yf_cmp_size(size, psize) ((int)size - (int)*(psize))

void* yf_slab_pool_alloc(struct yf_slab_pool_s* slab_pool, yf_u16_t size, yf_log_t* log)
{
        yf_int_t  begin = -1;
        yf_slist_part_t* alloc_part;
        yf_slab_trick_t* slab_trick;
        
        yf_u16_t  alloc_size = yf_align_mem(size);

        yf_bsearch(alloc_size, slab_pool->size, slab_pool->type, yf_cmp_size, begin);

        if (begin == slab_pool->type)
        {
                yf_log_error(YF_LOG_ERR, log, 0, "alloc size=%d failed, max=%d", 
                                size, slab_pool->size[slab_pool->type - 1]);
                return  NULL;
        }

        alloc_size = slab_pool->size[begin];
        yf_log_debug2(YF_LOG_DEBUG, log, 0, "require size=%d, real alloc size=%d", 
                        size, alloc_size);

        alloc_part = yf_slist_pop(slab_pool->free + begin);
        if (alloc_part)
        {
                slab_trick = yf_mem_off(alloc_part, -YF_SLAB_TRICK_SIZE);
                
                assert(yf_slab_check_trick(slab_trick));
                assert(slab_trick->size == alloc_size && slab_trick->used == 0);
                slab_trick->used = 1;
                
                return  alloc_part;
        }

        slab_trick = yf_palloc(slab_pool->base_pool, YF_SLAB_TRICK_SIZE + alloc_size);
        if (slab_trick == NULL)
                return NULL;

        yf_slab_set_trick(slab_trick);
        slab_trick->size = alloc_size;
        slab_trick->used = 1;

        return  yf_mem_off(slab_trick, YF_SLAB_TRICK_SIZE);
}


void yf_slab_pool_free(struct yf_slab_pool_s* slab_pool, void* data, yf_log_t* log)
{
        yf_int_t  index;

        yf_slist_part_t* free_part = data;
        yf_slab_trick_t* slab_trick = yf_mem_off(free_part, -YF_SLAB_TRICK_SIZE);

        assert(yf_slab_check_trick(slab_trick));
        assert(slab_trick->used);

        yf_bsearch(slab_trick->size, slab_pool->size, slab_pool->type, yf_cmp_size, index);
        assert(index != slab_pool->type);

        yf_slist_push(free_part, slab_pool->free + index);
        slab_trick->used = 0;
}


