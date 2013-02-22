#include <ppc/yf_header.h>
#include <base_struct/yf_core.h>

#define _ID_SEED_SLICE_SIZE 16384

static yf_lock_t  _id_seed_lock = YF_LOCK_INITIALIZER;
static yf_u32_t   _id_seed_group_used = 0;
static yf_u32_t   _id_seed_start_factor = 2;


static void _id_seed_group_update(yf_id_seed_group_t* seed_group)
{
        *seed_group = _id_seed_group_used;
        _id_seed_group_used += _ID_SEED_SLICE_SIZE;
        
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


void yf_id_seed_group_init(yf_id_seed_group_t* seed_group)
{
        yf_lock(&_id_seed_lock);
        
        if (_id_seed_group_used == 0) 
        {
                _id_seed_group_used = yf_align(yf_now_times.wall_utime.tv_sec, 
                                _ID_SEED_SLICE_SIZE);
                assert(_id_seed_group_used > 12345);
        }

        _id_seed_group_update(seed_group);
        
        yf_unlock(&_id_seed_lock);
}


inline yf_u32_t  yf_id_seed_alloc(yf_id_seed_group_t* seed_group)
{
        yf_u32_t seed_new = *seed_group;
        *seed_group += 1;
        
        if (yf_mod(*seed_group, _ID_SEED_SLICE_SIZE) == 0)
        {
                yf_lock(&_id_seed_lock);
                _id_seed_group_update(seed_group);
                yf_unlock(&_id_seed_lock);
        }
        
        return  seed_new;
}


