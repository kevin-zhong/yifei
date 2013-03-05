#include <ppc/yf_header.h>
#include <base_struct/yf_core.h>

#define _ID_SEED_SLICE_BIT 14
#define _ID_SEED_SLICE_SIZE (1<<14)

static yf_lock_t  _id_seed_lock = YF_LOCK_INITIALIZER;
static yf_u32_t   _id_seed_group_used = 0;
static yf_u32_t   _id_seed_start_factor = 2;

//changed on 2013/02/28 to support multi slice, for large area id alloc...

static void _id_seed_group_update(yf_id_seed_group_t* seed_group, yf_u32_t slice_multi)
{
        seed_group->seed = _id_seed_group_used;
        _id_seed_group_used += _ID_SEED_SLICE_SIZE * slice_multi;
        
        if (_id_seed_group_used >= (1<<31))
        {
                _id_seed_group_used = yf_align(
                                yf_now_times.wall_utime.tv_sec / _id_seed_start_factor, 
                                _ID_SEED_SLICE_SIZE);

                assert(_id_seed_group_used > 12345);
                _id_seed_start_factor = yf_min(_id_seed_start_factor<<1, 128);

#ifdef YF_DEBUG
                fprintf(stdout, "seed group roll from=%d, start factor=%d\n", 
                                _id_seed_group_used, _id_seed_start_factor);
#endif
        }
}


yf_int_t yf_id_seed_group_init(yf_id_seed_group_t* seed_group, yf_u32_t slice_size)
{
        if (slice_size > _ID_SEED_SLICE_SIZE * 255)
        {
                fprintf(stdout, "too larget slice size=%d, max=%d\n", 
                                slice_size, _ID_SEED_SLICE_SIZE * 255);
                return  YF_ERROR;
        }
        
        yf_lock(&_id_seed_lock);
        
        if (_id_seed_group_used == 0) 
        {
                _id_seed_group_used = yf_align(yf_now_times.wall_utime.tv_sec, 
                                _ID_SEED_SLICE_SIZE);
                assert(_id_seed_group_used > 12345);
        }
        
        if (slice_size == 0) {
                seed_group->slice_multi = 1;
        }
        else {
                seed_group->slice_multi = (yf_align(yf_max(slice_size, _ID_SEED_SLICE_SIZE), 
                                _ID_SEED_SLICE_SIZE) >> _ID_SEED_SLICE_BIT);

#ifdef YF_DEBUG
                fprintf(stdout, "seed group init slice size=%d, slice_multi=%d\n", 
                                slice_size, seed_group->slice_multi);
#endif                
        }
        
        _id_seed_group_update(seed_group, seed_group->slice_multi);
        seed_group->used_slice = 0;
        
        yf_unlock(&_id_seed_lock);
        return YF_OK;
}


inline yf_u32_t  yf_id_seed_alloc(yf_id_seed_group_t* seed_group)
{
        yf_u32_t seed_new = seed_group->seed;
        seed_group->seed += 1;
        
        if (yf_mod(seed_group->seed, _ID_SEED_SLICE_SIZE) == 0)
        {
                seed_group->used_slice += 1;
                
                if (seed_group->used_slice == seed_group->slice_multi)
                {
                        yf_lock(&_id_seed_lock);
                        _id_seed_group_update(seed_group, seed_group->slice_multi);
                        yf_unlock(&_id_seed_lock);
                        
                        seed_group->used_slice = 0;
                }
        }
        
        return  seed_new;
}


