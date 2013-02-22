#ifndef _YF_ID_SEED_H_20130222_H
#define _YF_ID_SEED_H_20130222_H
/*
* author: kevin_zhong, mail:qq2000zhong@gmail.com
* time: 20130222-16:54:55
*/

#include <base_struct/yf_core.h>
#include <ppc/yf_header.h>

typedef yf_u32_t  yf_id_seed_group_t;

void yf_id_seed_group_init(yf_id_seed_group_t* seed_group);

yf_u32_t  yf_id_seed_alloc(yf_id_seed_group_t* seed_group);

#endif

