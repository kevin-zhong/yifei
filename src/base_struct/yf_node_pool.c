#include <ppc/yf_header.h>
#include <base_struct/yf_core.h>


typedef  struct yf_node_head_s
{
        yf_u32_t  magic;
        yf_u32_t  id_pre:31;
        yf_u32_t  used:1;
}
yf_node_head_t;

#define YF_NODE_HEAD_SIZE yf_align(sizeof(yf_node_head_t), YF_ALIGNMENT)

#define yf_nh_2_addr(node) ((char*)(node) + YF_NODE_HEAD_SIZE)
#define yf_addr_2_nh(addr) ((yf_node_head_t*)((char*)(addr) - YF_NODE_HEAD_SIZE))

#define yf_check_id_seed(seed)  if (seed >= (1<<31)) seed = 0x155578;


size_t yf_node_taken_size(size_t size)
{
        return yf_align(YF_NODE_HEAD_SIZE + size, YF_ALIGNMENT);
}


void  yf_init_node_pool(yf_node_pool_t* node_pool, yf_log_t* log)
{
        yf_int_t  i;
        yf_node_head_t* node_head;

        assert(node_pool->each_taken_size > YF_NODE_HEAD_SIZE);
        if (node_pool->id_seed == 0)
                node_pool->id_seed = 0x123456;
        
        yf_check_id_seed(node_pool->id_seed);
        
        node_pool->free_size = node_pool->total_num;

        yf_init_slist_head(&node_pool->free_list);
        
        for (i = 0; i < node_pool->total_num; ++i)
        {
                node_head = (yf_node_head_t*)(node_pool->nodes_array 
                                + node_pool->each_taken_size * i);
                
                yf_memzero(node_head, YF_NODE_HEAD_SIZE);
                
                yf_slist_push((yf_slist_part_t*)yf_nh_2_addr(node_head), &node_pool->free_list);
        }
}

//return NULL if no free...
void* yf_alloc_node_from_pool(yf_node_pool_t* node_pool, yf_log_t* log)
{
        if (node_pool->free_size == 0)
        {
                yf_log_error(YF_LOG_WARN, log, 0, "node_pool used out, total_num=%d", 
                                node_pool->total_num);
                return NULL;
        }
        
        yf_slist_part_t * part = yf_slist_pop(&node_pool->free_list);
        assert(part);
        
        yf_node_head_t* node_head = yf_addr_2_nh(part);
        yf_set_magic(node_head->magic);
        node_head->used = 1;
        node_head->id_pre = node_pool->id_seed++;
        
        yf_check_id_seed(node_pool->id_seed);
        
        node_pool->free_size -= 1;

        yf_log_debug2(YF_LOG_DEBUG, log, 0, "total_num=%d, after alloc %d, free=%d", 
                        node_pool->total_num, node_head->id_pre, node_pool->free_size);
        
        return  part;
}


void yf_free_node_to_pool(yf_node_pool_t* node_pool, void* node, yf_log_t* log)
{
        yf_slist_part_t * part = node;
        yf_node_head_t* node_head = yf_addr_2_nh(part);
        yf_u32_t id_pre = node_head->id_pre;

        assert(id_pre && node_head->used && yf_check_magic(node_head->magic));
        
        assert((char*)node_head >= node_pool->nodes_array 
                        && (char*)node_head < node_pool->nodes_array 
                                + node_pool->each_taken_size * node_pool->total_num);

        yf_slist_push(part, &node_pool->free_list);
        node_pool->free_size += 1;

        yf_memzero(node_head, YF_NODE_HEAD_SIZE);
        
        yf_log_debug2(YF_LOG_DEBUG, log, 0, "total_num=%d, after free %d, free=%d", 
                        node_pool->total_num, id_pre, node_pool->free_size);        
}


yf_u64_t  yf_get_id_by_node(yf_node_pool_t* node_pool, void* node, yf_log_t* log)
{
        yf_node_head_t* node_head = yf_addr_2_nh(node);
        assert(node_head->used && yf_check_magic(node_head->magic));

        yf_u64_t offset = ((char*)node_head - node_pool->nodes_array)
                        / node_pool->each_taken_size;

        return  (offset<<32) | node_head->id_pre;
}

void* yf_get_node_by_id(yf_node_pool_t* node_pool, yf_u64_t id, yf_log_t* log)
{
        yf_node_head_t* node_head = (yf_node_head_t*)(node_pool->nodes_array 
                        + (id>>32) * node_pool->each_taken_size);

        if (!node_head->used)
                return NULL;
        yf_check_magic(node_head->magic);

        if (node_head->id_pre != ((id<<32)>>32))
        {
                yf_log_debug2(YF_LOG_DEBUG, log, 0, "id pre not equall, id=%d, buf=%d", 
                                ((id<<32)>>32), node_head->id_pre);
                return NULL;
        }

        return yf_nh_2_addr(node_head);
}

