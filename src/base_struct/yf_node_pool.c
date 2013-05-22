#include <ppc/yf_header.h>
#include <base_struct/yf_core.h>


typedef  struct _np_node_head_s
{
        yf_u32_t  magic;
        yf_u32_t  id_pre:31;
        yf_u32_t  used:1;
}
_np_node_head_t;

#define _NP_NODE_HEAD_SIZE yf_align(sizeof(_np_node_head_t), YF_ALIGNMENT)

#define _np_nh_2_addr(node) ((char*)(node) + _NP_NODE_HEAD_SIZE)
#define _np_addr_2_nh(addr) ((_np_node_head_t*)((char*)(addr) - _NP_NODE_HEAD_SIZE))


yf_u32_t yf_node_taken_size(yf_u32_t size)
{
        return yf_align(_NP_NODE_HEAD_SIZE + size, YF_ALIGNMENT);
}


void  yf_init_node_pool(yf_node_pool_t* node_pool, yf_log_t* log)
{
        yf_int_t  i;
        _np_node_head_t* node_head;

        assert(node_pool->each_taken_size > _NP_NODE_HEAD_SIZE);

        yf_id_seed_group_init(&node_pool->id_seed, 0);
        
        yf_log_debug1(YF_LOG_DEBUG, log, 0, "node pool id_seed=%d", node_pool->id_seed);
        
        node_pool->free_size = node_pool->total_num;

        yf_init_slist_head(&node_pool->free_list);
        
        for (i = 0; i < node_pool->total_num; ++i)
        {
                node_head = (_np_node_head_t*)(node_pool->nodes_array 
                                + node_pool->each_taken_size * i);
                
                yf_memzero(node_head, _NP_NODE_HEAD_SIZE);
                
                yf_slist_push((yf_slist_part_t*)_np_nh_2_addr(node_head), &node_pool->free_list);
        }
}

//return NULL if no free...
void* yf_alloc_node_from_pool(yf_node_pool_t* node_pool, yf_log_t* log)
{
        if (unlikely(node_pool->free_size == 0))
        {
                yf_log_error(YF_LOG_WARN, log, 0, "node_pool used out, total_num=%d", 
                                node_pool->total_num);
                return NULL;
        }
        
        yf_slist_part_t * part = yf_slist_pop(&node_pool->free_list);
        assert(part);
        
        _np_node_head_t* node_head = _np_addr_2_nh(part);
        yf_set_magic(node_head->magic);
        node_head->used = 1;
        node_head->id_pre = yf_id_seed_alloc(&node_pool->id_seed);
        
        node_pool->free_size -= 1;

        yf_log_debug4(YF_LOG_DEBUG, log, 0, 
                        "total_num=%d, after alloc %d, free=%d, alloc_ptr=%p", 
                        node_pool->total_num, node_head->id_pre, node_pool->free_size, part);
        
        return  part;
}


void yf_free_node_to_pool(yf_node_pool_t* node_pool, void* node, yf_log_t* log)
{
        yf_slist_part_t * part = node;
        _np_node_head_t* node_head = _np_addr_2_nh(part);
        yf_u32_t id_pre = node_head->id_pre;

        assert(id_pre && node_head->used && yf_check_magic(node_head->magic));
        
        assert((char*)node_head >= node_pool->nodes_array 
                        && (char*)node_head < node_pool->nodes_array 
                                + node_pool->each_taken_size * node_pool->total_num);

        yf_slist_push(part, &node_pool->free_list);
        node_pool->free_size += 1;

        yf_memzero(node_head, _NP_NODE_HEAD_SIZE);
        
        yf_log_debug4(YF_LOG_DEBUG, log, 0, 
                        "total_num=%d, after free %d, free=%d, free_ptr=%p", 
                        node_pool->total_num, id_pre, node_pool->free_size, part);
}


yf_u64_t  yf_get_id_by_node(yf_node_pool_t* node_pool, void* node, yf_log_t* log)
{
        _np_node_head_t* node_head = _np_addr_2_nh(node);
        assert(node_head->used && yf_check_magic(node_head->magic));

        yf_u64_t offset = ((char*)node_head - node_pool->nodes_array)
                        / node_pool->each_taken_size;

        return  ((offset | (node_pool->pool_index<<24)) << 32) | node_head->id_pre;
}

void* yf_get_node_by_id(yf_node_pool_t* node_pool, yf_u64_t id, yf_log_t* log)
{
        _np_node_head_t* node_head = (_np_node_head_t*)(node_pool->nodes_array 
                        + ((id>>32) & ((1<<24)-1)) * node_pool->each_taken_size);

        if (!node_head->used)
                return NULL;
        assert(yf_check_magic(node_head->magic));

        if (node_head->id_pre != yf_u32tou64_getl(id))
        {
                yf_log_debug2(YF_LOG_DEBUG, log, 0, "id pre not equall, id=%d, buf=%d", 
                                yf_u32tou64_getl(id), node_head->id_pre);
                return NULL;
        }

        return _np_nh_2_addr(node_head);
}


/*
* high level node pool 
*/
typedef struct _yf_child_np_info_s
{
        yf_node_pool_t* pool;
}
_yf_child_np_info_t;


static int __cmp_pool_freesize(const void *p1, const void *p2)
{
        //from max to min
        return ((_yf_child_np_info_t*)p2)->pool->free_size 
                        - ((_yf_child_np_info_t*)p1)->pool->free_size;
}



typedef struct yf_hnpool_in_s
{
        yf_u32_t  num_per_chunk;
        yf_u8_t    max_chunk;
        yf_u8_t    used_chunk;
        yf_u8_t    alloc_chunk;
        yf_u8_t    padding;
        yf_u32_t  node_taken_size;
        
        yf_node_pool_t*  node_pools;
        _yf_child_np_info_t*  node_pool_infos;
}
yf_hnpool_in_t;


static void _yf_hnpool_init_child_pool(yf_hnpool_in_t* hnpool
                , char* mem, yf_log_t* log)
{
        yf_node_pool_t * node_pool = hnpool->node_pools + hnpool->used_chunk;
        
        node_pool->nodes_array = mem;
        node_pool->total_num = hnpool->num_per_chunk;
        node_pool->each_taken_size = hnpool->node_taken_size;
        node_pool->pool_index = hnpool->used_chunk;

        hnpool->node_pool_infos[hnpool->used_chunk].pool = node_pool;

        yf_init_node_pool(node_pool, log);

        hnpool->used_chunk += 1;
}


yf_hnpool_t* yf_hnpool_create(yf_u32_t node_size
                , yf_u32_t num_per_chunk, yf_u8_t max_chunk
                , yf_log_t* log)
{
        CHECK_RV(num_per_chunk == 0, NULL);
        
        if (((yf_u64_t)num_per_chunk) * max_chunk >= (1<<31))
        {
                yf_log_error(YF_LOG_ERR, log, 0, 
                                "too many nodes, num_per_chunk=%d, max_chunk=%d", 
                                num_per_chunk, max_chunk);
                return NULL;
        }
        
        yf_u32_t  hnpool_size = yf_align_mem(sizeof(yf_hnpool_in_t));
        yf_u32_t  node_pool_size = sizeof(yf_node_pool_t) * max_chunk;
        yf_u32_t  np_info_size = sizeof(_yf_child_np_info_t) * max_chunk;

        node_size = yf_node_taken_size(node_size);

        yf_hnpool_in_t* hnpool = yf_alloc(hnpool_size
                        + node_pool_size + np_info_size
                        + node_size * num_per_chunk);
        CHECK_RV(hnpool == NULL, NULL);

        hnpool->num_per_chunk = num_per_chunk;
        hnpool->max_chunk = max_chunk;
        hnpool->node_taken_size = node_size;

        hnpool->node_pools = yf_mem_off(hnpool, hnpool_size);
        hnpool->node_pool_infos = yf_mem_off(hnpool->node_pools, node_pool_size);

        _yf_hnpool_init_child_pool(hnpool, 
                        yf_mem_off(hnpool->node_pool_infos, np_info_size), log);
        return (yf_hnpool_t*)hnpool;
}


void* yf_hnpool_alloc(yf_hnpool_t* hpool, yf_u64_t* id, yf_log_t* log)
{
        yf_hnpool_in_t* hp = (yf_hnpool_in_t*)hpool;
        yf_node_pool_t* pool = NULL;
        void* node = NULL;
        yf_int_t  sorted = 0;

        while (1) 
        {
                pool = hp->node_pool_infos[hp->alloc_chunk].pool;
                if (pool->free_size)
                {
                        node = yf_alloc_node_from_pool(pool, log);
                        assert(node);
                        *id = yf_get_id_by_node(pool, node, log);
                        return  node;
                }
                
                if (hp->alloc_chunk + 1 == hp->used_chunk)
                {
                        if (!sorted)
                        {
                                yf_sort(hp->node_pool_infos, hp->used_chunk, 
                                                sizeof(_yf_child_np_info_t), __cmp_pool_freesize);
                                sorted = 1;

                                yf_log_debug(YF_LOG_DEBUG, log, 0, "hpool sort free chunk chunks=%d", 
                                                hp->used_chunk);
                                
                                if (hp->node_pool_infos[0].pool->free_size)
                                {
                                        hp->alloc_chunk = 0;
                                        continue;
                                }
                                
                                yf_log_debug(YF_LOG_DEBUG, log, 0, "after sort, still no free node");
                        }
                        
                        if (hp->used_chunk < hp->max_chunk) 
                        {
                                char* mem = yf_alloc(hp->node_taken_size * hp->num_per_chunk);
                                CHECK_RV(mem == NULL, NULL);
                                _yf_hnpool_init_child_pool(hp, mem, log);

                                yf_log_debug(YF_LOG_DEBUG, log, 0, "hpool alloc one more chunk_%d", 
                                                hp->used_chunk);
                                
                                ++hp->alloc_chunk;
                        }
                        else {
                                yf_log_error(YF_LOG_WARN, log, 0, "hpool all node used");
                                return NULL;
                        }
                }
                else {
                        ++hp->alloc_chunk;
                }
        }
}


void  yf_hnpool_free(yf_hnpool_t* hpool, yf_u64_t id, void* node, yf_log_t* log)
{
        yf_hnpool_in_t* hp = (yf_hnpool_in_t*)hpool;
        
        if (node == NULL)
        {
                node = yf_hnpool_id2node(hpool, id, log);
                if (node == NULL)
                        return;
        }
        
        yf_u32_t chunk = (id >> 56);
        yf_free_node_to_pool(hp->node_pools + chunk, node, log);
}

void* yf_hnpool_id2node(yf_hnpool_t* hpool, yf_u64_t id, yf_log_t* log)
{
        yf_hnpool_in_t* hp = (yf_hnpool_in_t*)hpool;
        yf_u32_t chunk = (id >> 56);

        return yf_get_node_by_id(hp->node_pools + chunk, id, log);
}


