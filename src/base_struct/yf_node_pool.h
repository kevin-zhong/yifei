#ifndef _YF_NODE_POOL_H_20120813_H
#define _YF_NODE_POOL_H_20120813_H
/*
* author: kevin_zhong, mail:qq2000zhong@gmail.com
* time: 20120813-20:12:30
*/

#include <base_struct/yf_core.h>
#include <ppc/yf_header.h>

size_t yf_node_taken_size(size_t size);

struct yf_node_pool_s
{
        char* nodes_array;
        yf_slist_part_t  free_list;
        size_t  total_num;
        size_t  each_taken_size;
        size_t  free_size;
        yf_u32_t  id_seed;
};

void  yf_init_node_pool(yf_node_pool_t* node_pool, yf_log_t* log);

//return NULL if no free...
void* yf_alloc_node_from_pool(yf_node_pool_t* node_pool, yf_log_t* log);

void yf_free_node_to_pool(yf_node_pool_t* node_pool, void* node, yf_log_t* log);

yf_u64_t  yf_get_id_by_node(yf_node_pool_t* node_pool, void* node, yf_log_t* log);
void* yf_get_node_by_id(yf_node_pool_t* node_pool, yf_u64_t id, yf_log_t* log);


#endif
