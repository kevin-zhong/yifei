#ifndef _YF_NODE_POOL_H_20120813_H
#define _YF_NODE_POOL_H_20120813_H
/*
* copyright@: kevin_zhong, mail:qq2000zhong@gmail.com
* time: 20120813-20:12:30
*/

#include <base_struct/yf_core.h>
#include <ppc/yf_header.h>

yf_u32_t yf_node_taken_size(yf_u32_t size);

struct yf_node_pool_s
{
        //init attr
        char* nodes_array;
        yf_u32_t  total_num;
        yf_u32_t  each_taken_size;

        //inner data...
        yf_slist_part_t  free_list;
        yf_u32_t  free_size;
        yf_u32_t  base_index;
        yf_id_seed_group_t  id_seed;
};

void  yf_init_node_pool(yf_node_pool_t* node_pool, yf_log_t* log);

//return NULL if no free...
void* yf_alloc_node_from_pool(yf_node_pool_t* node_pool, yf_log_t* log);

void yf_free_node_to_pool(yf_node_pool_t* node_pool, void* node, yf_log_t* log);

yf_u64_t  yf_get_id_by_node(yf_node_pool_t* node_pool, void* node, yf_log_t* log);
void* yf_get_node_by_id(yf_node_pool_t* node_pool, yf_u64_t id, yf_log_t* log);


/*
* high level node pool 
*/
typedef  yf_u64_t  yf_hnpool_t;

//node_size is the org size, no need transfered by yf_node_taken_size(size)
yf_hnpool_t* yf_hnpool_create(yf_u32_t node_size
                , yf_u32_t num_per_chunk, yf_u16_t max_chunk
                , yf_log_t* log);

void* yf_hnpool_alloc(yf_hnpool_t* hpool, yf_u64_t* id, yf_log_t* log);

//if node==NULL, then will get node by id
void  yf_hnpool_free(yf_hnpool_t* hpool, yf_u64_t id, void* node, yf_log_t* log);

void* yf_hnpool_id2node(yf_hnpool_t* hpool, yf_u64_t id, yf_log_t* log);


#endif
