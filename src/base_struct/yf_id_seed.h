#ifndef _YF_ID_SEED_H_20130222_H
#define _YF_ID_SEED_H_20130222_H
/*
* copyright@: kevin_zhong, mail:qq2000zhong@gmail.com
* time: 20130222-20:54:55
*/

#include <base_struct/yf_core.h>
#include <ppc/yf_header.h>

typedef struct yf_id_seed_group_s
{
        yf_u32_t  seed;
        yf_u32_t  slice_multi:8;//max 255 multi
        yf_u32_t  used_slice:8;
}
yf_id_seed_group_t;


//if slice_size==0, use default slice size
yf_int_t yf_id_seed_group_init(yf_id_seed_group_t* seed_group, yf_u32_t slice_size);

yf_u32_t  yf_id_seed_alloc(yf_id_seed_group_t* seed_group);

#endif

