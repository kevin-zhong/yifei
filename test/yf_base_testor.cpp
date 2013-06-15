#include <gtest/gtest.h>
#include <list>

extern "C" {
#include <ppc/yf_header.h>
#include <base_struct/yf_core.h>
#include <mio_driver/yf_event.h>
}

yf_pool_t  *_mem_pool;
yf_log_t*    _log;

class BaseTest : public testing::Test
{
};

/*
* test base algo
*/
#define TEST_CMP(val, pval) (val - *pval)

TEST_F(BaseTest, BaseAlgo)
{
        int  vals[] = {1, 3, 5, 5, 7, 9, 15, 22};
        int  index;

        int  test_vals[] = {0, 1, 2, 3, 8, 9, 17, 22, 23};
        int  assert_indexs[] = {0, 0, 1, 1, 5, 5, 7, 7, 8};

        for (int i = 0; i < YF_ARRAY_SIZE(vals); ++i)
        {
                printf("test bsearch i=%d\n", i);
                yf_bsearch(test_vals[i], vals, YF_ARRAY_SIZE(vals), TEST_CMP, index);
                ASSERT_EQ(index, assert_indexs[i]);
        }

        yf_bsearch(5, vals, YF_ARRAY_SIZE(vals), TEST_CMP, index);
        ASSERT_TRUE(index == 2 || index == 3);
}

/*
* test bitop
*/
TEST_F(BaseTest, BitOp)
{
        ASSERT_EQ(yf_align(7, 4), 8);
        ASSERT_EQ(yf_align(9, 8), 16);

        ASSERT_EQ(yf_align_ptr(8, 8), (char*)8);
        ASSERT_EQ(yf_align_ptr(1, 8), (char*)8);

        ASSERT_EQ(yf_align_ptr(1, 8), (char*)8);

        yf_u64_t  bit_val = 0;
        yf_set_bit(bit_val, 8);
        ASSERT_EQ(bit_val, (yf_u64_t)256);

        yf_set_bit(bit_val, 35);
        ASSERT_EQ(bit_val, ((yf_u64_t)1<<35)+256);

        yf_reset_bit(bit_val, 8);
        ASSERT_EQ(bit_val, ((yf_u64_t)1<<35));

        yf_reset_bit(bit_val, 12);
        yf_reset_bit(bit_val, 36);
        ASSERT_EQ(bit_val, ((yf_u64_t)1<<35));

        yf_revert_bit(bit_val, 12);
        ASSERT_EQ(bit_val, ((yf_u64_t)1<<35) + (1<<12));

        yf_revert_bit(bit_val, 35);
        ASSERT_EQ(bit_val, ((yf_u64_t)1<<12));

        ASSERT_TRUE(yf_test_bit(bit_val, 35) == 0);
        ASSERT_TRUE(yf_test_bit(bit_val, 12) != 0);

        ASSERT_EQ(yf_mod(17, 4), 1);
        ASSERT_EQ(yf_mod(24, 16), 8);
        ASSERT_EQ(yf_mod(16, 8), 0);

        ASSERT_EQ(yf_cicur_add(5, 3, 8), 0);
        ASSERT_EQ(yf_cicur_add(7, 15, 8), 6);

        yf_u64_t test_val[] = {58842421LLU, 588425557421LLU, 27444222LLU, 
                        877LLU, 680000000000LLU, 7777777777777777LLU, 1222454548923LLU, 
                        5522111546453424233LLU, 555541122121212LLU, 
                        (yf_u64_t(1))<<50, (yf_u64_t(1))<<32, (yf_u64_t(1))<<62};

        yf_bit_set_t  bit_set;
        yf_set_bits cacu_bits, get_bits;
        
        for (int i = 0; i < YF_ARRAY_SIZE(test_val); ++i)
        {
                yf_memzero_st(cacu_bits);
                yf_memzero_st(get_bits);

                yf_int_t  index = 0, offset = 0;
                
                for (bit_val = test_val[i]; bit_val; bit_val >>= 1, ++index)
                {
                        if (!yf_test_bit(bit_val, 0))
                                continue;
                        cacu_bits[offset++] = index;
                }
                cacu_bits[offset] = YF_END_INDEX;
                printf("test_val=%ld have %d bits set\n", (long long)test_val[i], offset);

                bit_set.bit_64 = test_val[i];
                yf_get_set_bits(&bit_set, get_bits);

                ASSERT_EQ(memcmp(cacu_bits, get_bits, sizeof(yf_set_bits)), 0);
        }
}


/*
* test array
*/
TEST_F(BaseTest, Array)
{
        yf_array_t *array = yf_array_create(_mem_pool, 128, sizeof(yf_int_t));
        assert(array);
        
        for (int i = 0; i < 168; ++i)
        {
                yf_int_t* new_i = (yf_int_t*)yf_array_push(array);
                assert(new_i);
                *new_i = 168 - i;
        }

        ASSERT_EQ(array->nelts, (yf_uint_t)168);
        for (int i = 0; i < 168; ++i)
        {
                ASSERT_EQ(((yf_int_t*)array->elts)[i], 168-i);
        }
}

/*
* test list
*/
typedef struct
{
        yf_list_part_t  node;
        int  i;
} test_list_t;

TEST_F(BaseTest, List)
{
        yf_list_part_t  list1, list2;
        
        yf_init_list_head(&list1);
        yf_init_list_head(&list2);

        int i;

#define NODE_NUM 10

        test_list_t  node[NODE_NUM];
        for (i = 0; i < NODE_NUM; ++i)
        {
                node[i].i = i;
                yf_list_add_tail(&node[i].node, &list1);
        }

        i = 0;
        test_list_t *iter, *iter_mid;
        yf_list_for_each_entry(iter, &list1, node)
        {
                ASSERT_EQ(iter->i, i);
                ++i;
        }
        ASSERT_EQ(i, NODE_NUM);
        
        i = 0;
        yf_list_for_each_entry_r(iter, &list1, node)
        {
                ASSERT_EQ(iter->i, NODE_NUM-1 - i);
                ++i;
        }

        yf_list_part_t* node_iter, *node_iter_mid;
        yf_list_for_each_safe(node_iter, node_iter_mid, &list1)
        {
                yf_list_move_head(node_iter, &list2);
        }
        ASSERT_TRUE(yf_list_empty(&list1));
        
        i = 0;
        yf_list_for_each_entry(iter, &list2, node)
        {
                ASSERT_EQ(iter->i, NODE_NUM-1 - i);
                ++i;
        }

        printf("node num=%d\n", i);

        yf_list_for_each_safe(node_iter, node_iter_mid, &list2)
        {
                yf_list_move_tail(node_iter, &list1);
        }
        ASSERT_TRUE(yf_list_empty(&list2));
        
        i = 0;
        yf_list_for_each_entry(iter, &list1, node)
        {
                ASSERT_EQ(iter->i, NODE_NUM-1 - i);
                ++i;
        }

        //test pop
        yf_init_list_head(&list1);
        for (i = 0; i < NODE_NUM; ++i)
        {
                node[i].i = i;
                yf_list_add_tail(&node[i].node, &list1);
        }        
        i = 0;
        while ((node_iter = yf_list_pop_head(&list1)) != NULL)
        {
                iter = container_of(node_iter, test_list_t, node);
                ASSERT_EQ(iter->i, i);
                ++i;
        }

        for (i = 0; i < NODE_NUM; ++i)
        {
                node[i].i = i;
                yf_list_add_head(&node[i].node, &list1);
        }
        i = 0;
        while ((node_iter = yf_list_pop_tail(&list1)) != NULL)
        {
                iter = container_of(node_iter, test_list_t, node);
                ASSERT_EQ(iter->i, i);
                ++i;
        }        
}

/*
* test rbtree
*/
typedef struct
{
        yf_rbtree_node_t  link;
        int  test_val;
} 
test_rbtree_t;

yf_int_t cmp_func(void *left, void *right, yf_int_t cmp_by_node)
{
        if (cmp_by_node)
                return  container_of((yf_rbtree_node_t*)left, test_rbtree_t, link)->test_val 
                         < container_of((yf_rbtree_node_t*)right, test_rbtree_t, link)->test_val;
        else
                return  (yf_uint_ptr_t)left < (yf_uint_ptr_t)right;
}

TEST_F(BaseTest, Rbtree)
{
        yf_rbtree_t  tree;
        yf_rbtree_node_t nil;
        yf_rbtree_init(&tree, &nil, cmp_func);

        yf_rbtree_t *ptree = &tree;
        yf_rbtree_node_t *iter;
        test_rbtree_t *val_iter;
        int i = 0;

#define NODE_NUM_RB 10
        test_rbtree_t  nodes[NODE_NUM_RB], node_test;
        
        for (i = 0; i < NODE_NUM_RB; ++i)
        {
                nodes[i].test_val = i;
                ASSERT_EQ(yf_rbtree_insert_uniq(ptree, &nodes[i].link), YF_OK);
        }

        node_test.test_val = 1;
        ASSERT_EQ(yf_rbtree_insert_uniq(ptree, &node_test.link), YF_ERROR);

        i = 0;
        yf_rbtree_for_each(iter, ptree)
        {
                val_iter = container_of(iter, test_rbtree_t, link);
                printf("%d\n", val_iter->test_val);
                ASSERT_EQ(i, val_iter->test_val);
                ++i;
        }
        
        ASSERT_EQ(i, NODE_NUM_RB);

        i = 0;
        yf_rbtree_for_each_r(iter, ptree)
        {
                val_iter = container_of(iter, test_rbtree_t, link);
                ASSERT_EQ(NODE_NUM_RB-1 - i, val_iter->test_val);
                ++i;
        }

        //find
        yf_rbtree_node_t *find_node;
        for (i = 5; i < NODE_NUM_RB; ++i)
        {
                yf_rbtree_find(ptree, test_rbtree_t, link, test_val, i, find_node);
                ASSERT_TRUE(find_node != NULL);
                val_iter = container_of(find_node, test_rbtree_t, link);
                ASSERT_EQ(val_iter->test_val, i);
        }

        yf_rbtree_find(ptree, test_rbtree_t, link, test_val, 101, find_node);
        ASSERT_TRUE(find_node==NULL);

        //delete
        yf_rbtree_delete(ptree, &nodes[5].link);
        i = 0;
        yf_rbtree_for_each(iter, ptree)
        {
                val_iter = container_of(iter, test_rbtree_t, link);
                ++i;
        }
        
        ASSERT_EQ(i, NODE_NUM_RB-1);

#define RANDOM_NUM 2048
        test_rbtree_t  nodes_srand[RANDOM_NUM];
        for (i = 0; i < RANDOM_NUM; ++i)
        {
                nodes_srand[i].test_val = random() % RANDOM_NUM;
                yf_rbtree_insert_uniq(ptree, &nodes_srand[i].link);
        }

        for (i = 0; i < RANDOM_NUM; ++i)
        {
                int rand_val = random() % RANDOM_NUM;
                yf_rbtree_find(ptree, test_rbtree_t, link, test_val, rand_val, find_node);
                if (find_node)
                {
                        printf("find %d\n", rand_val);
                        yf_rbtree_delete(ptree, find_node);
                }
        }

        yf_rbtree_for_each(iter, ptree)
        {
                val_iter = container_of(iter, test_rbtree_t, link);
                printf("rest val=%d\n", val_iter->test_val);
        }        
}

/*
* string + log
*/
TEST_F(BaseTest, StringLog)
{
        char buf[] = "AfdafdasDF";

        char copy_buf[sizeof(buf)];
        yf_strtoupper_copy(copy_buf, buf, sizeof(buf));
        ASSERT_EQ(std::string(copy_buf), std::string("AFDAFDASDF"));
        
        yf_strtolower(buf, sizeof(buf) - 1);
        ASSERT_EQ(std::string(buf), std::string("afdafdasdf"));

        yf_str_t  str_tmp = yf_str("[fdsaff]");

        char  out_buf[256];
        yf_sprintf(out_buf, "%p-%V-%c-%s%N", copy_buf, &str_tmp, buf[0], copy_buf);
        printf("%s", out_buf);
        
        yf_log_error(YF_LOG_ERR, _log, YF_ENOSPC, "%p-%V-%c-%s", 
                        copy_buf, &str_tmp, buf[0], copy_buf);

        yf_log_t* file_log = yf_log_open(YF_LOG_DEBUG, 8192, (void*)"dir/test.log");

        for (int k = 0; k < 512; ++k)
        {
                for (int i = 0; i < 68; ++i)
                {
                        yf_log_error(YF_LOG_ERR, file_log, i, "%p-%V-%c-%s", 
                                        copy_buf, &str_tmp, buf[0], copy_buf);
                }
        }
}


void  test_hash()
{
        yf_hash_t  hash_test;
        
        yf_hash_init_t  hash_init;
        hash_init.hash = &hash_test;
        hash_init.name = "test_hash";
        hash_init.bucket_size = yf_align(64, yf_cacheline_size);

        printf("bucket size=%d\n", hash_init.bucket_size);
        hash_init.max_size = 512;
        hash_init.pool = _mem_pool;
        hash_init.temp_pool = NULL;
        hash_init.key = yf_hash_key_lc;

        const char* keys[] = {"aaa", "Abb", "ffcc", "fdsadf", "hgrfhdfdh", "fhgfdh"};
        
        size_t  key_size = YF_ARRAY_SIZE(keys);
        
        yf_hash_key_t  hash_kyes[key_size];
        for (size_t i = 0; i < key_size; ++i)
        {
                hash_kyes[i].key.data = (char*)keys[i];
                hash_kyes[i].key.len = strlen(keys[i]);
                hash_kyes[i].key_hash = yf_hash_key_lc(hash_kyes[i].key.data, hash_kyes[i].key.len);
                hash_kyes[i].value = (void*)keys[key_size - 1 - i];
                printf("addr=%l, key_hash=%d\n", (long)hash_kyes[i].value, hash_kyes[i].key_hash);
        }

        yf_int_t ret = yf_hash_init(&hash_init, hash_kyes, key_size);
        ASSERT_EQ(ret, YF_OK);

        for (size_t i = 0; i < key_size; ++i)
        {
                yf_uint_t key = yf_hash_key_lc((char*)keys[i], strlen(keys[i]));
                void* find_ret = yf_hash_find(&hash_test, key, (char*)keys[i], strlen(keys[i]));
                
                printf("find ret addr=%l vs org=%l, key hash=%d\n", 
                                find_ret, ((void*)keys[key_size - 1 - i]), 
                                key);
                ASSERT_EQ(find_ret, ((void*)keys[key_size - 1 - i]));

                find_ret = yf_hash_find(&hash_test, key, (char*)keys[i], strlen(keys[i])-1);
                ASSERT_TRUE(find_ret == NULL);
        }        
}

/*
* hash
*/
TEST_F(BaseTest, Hash)
{
        test_hash();
}


/*
* node pool
*/
TEST_F(BaseTest, NodePool)
{
        yf_node_pool_t  node_pool;
        yf_memzero_st(node_pool);
        
        node_pool.each_taken_size = yf_node_taken_size(sizeof(yf_u64_t));
        node_pool.total_num = 1024;

        node_pool.nodes_array = (char*)yf_alloc(
                        node_pool.each_taken_size * node_pool.total_num);

        yf_init_node_pool(&node_pool, _log);

        typedef std::list<void*> ListV;
        ListV allocted_list;
        
        void* new_node = NULL, *cmp_node = NULL;
        
        for (int i = 0; i < 1024; ++i)
        {
                new_node = yf_alloc_node_from_pool(&node_pool, _log);
                ASSERT_TRUE(new_node != NULL);
                
                yf_u64_t  id = yf_get_id_by_node(&node_pool, new_node, _log);
                cmp_node = yf_get_node_by_id(&node_pool, id, _log);
                
                ASSERT_EQ(new_node, cmp_node);
                
                allocted_list.push_back(new_node);
        }

        new_node = yf_alloc_node_from_pool(&node_pool, _log);
        ASSERT_TRUE(new_node == NULL);

        for (int i = 0; i < 16; ++i)
        {
                int delete_num = std::min((long)allocted_list.size(), random() % 1024);
                
                for (int j = 0; j < delete_num; ++j)
                {
                        ListV::iterator iter = allocted_list.begin();
                        std::advance(iter, random() % allocted_list.size());

                        cmp_node = *iter;
                        yf_u64_t  id = yf_get_id_by_node(&node_pool, cmp_node, _log);
                        yf_free_node_to_pool(&node_pool, cmp_node, _log);
                        cmp_node = yf_get_node_by_id(&node_pool, id, _log);
                        ASSERT_TRUE(cmp_node == NULL);
                        
                        allocted_list.erase(iter);
                }

                int add_num = random() % 1024;
                
                for (int j = 0; j < add_num; ++j)
                {
                        new_node = yf_alloc_node_from_pool(&node_pool, _log);
                        
                        if (allocted_list.size() >= 1024)
                        {
                                ASSERT_TRUE(new_node == NULL);
                                yf_log_debug(YF_LOG_DEBUG, _log, 0, "used out, break now\n");
                                break;
                        }
                        ASSERT_TRUE(new_node != NULL);
                        //yf_log_debug(YF_LOG_DEBUG, _log, 0, "size=%d\n", allocted_list.size());
                        
                        yf_u64_t  id = yf_get_id_by_node(&node_pool, new_node, _log);
                        cmp_node = yf_get_node_by_id(&node_pool, id, _log);

                        ASSERT_EQ(new_node, cmp_node);
                        
                        allocted_list.push_back(new_node);
                }
        }

        if (!allocted_list.empty())
        {
                cmp_node = *allocted_list.begin();
                yf_u64_t  id = yf_get_id_by_node(&node_pool, cmp_node, _log);
                yf_free_node_to_pool(&node_pool, cmp_node, _log);
                
                cmp_node = yf_get_node_by_id(&node_pool, id, _log);
                ASSERT_TRUE(cmp_node == NULL);

                new_node = yf_alloc_node_from_pool(&node_pool, _log);
                cmp_node = yf_get_node_by_id(&node_pool, id, _log);
                ASSERT_TRUE(cmp_node == NULL);
        }

        yf_free(node_pool.nodes_array);
}


TEST_F(BaseTest, HNodePool)
{
        yf_hnpool_t* hnp = yf_hnpool_create(sizeof(yf_u64_t), 512, 10, _log);
        assert(hnp);

        typedef std::list<yf_u64_t> ListV;
        ListV allocted_list;
        yf_u64_t  id;
        long  alloc_cnt = 0;
        
        void* new_node = NULL, *cmp_node = NULL;
        
        for (int i = 0; i < 4096; ++i)
        {
                if ((i & 255) == 0)
                        printf("test rool_%d\n", i);
                int add_num = random() % 1024;
                
                for (int j = 0; j < add_num; ++j)
                {
                        new_node = yf_hnpool_alloc(hnp, &id, _log);
                        if (alloc_cnt == 5120)
                        {
                                ASSERT_TRUE(new_node == NULL);
                                yf_log_debug(YF_LOG_DEBUG, _log, 0, "hnp used out...");
                                break;
                        }
                        else {
                                ASSERT_TRUE(new_node != NULL);
                                cmp_node = yf_hnpool_id2node(hnp, id, _log);
                                
                                ASSERT_EQ(new_node, cmp_node);
                                
                                allocted_list.push_back(id);
                                ++alloc_cnt;
                        }
                }

                int delete_num = std::min(alloc_cnt, random() % 1024);
                
                for (int j = 0; j < delete_num; ++j)
                {
                        ListV::iterator iter = allocted_list.begin();
                        std::advance(iter, random() % alloc_cnt);

                        cmp_node = yf_hnpool_id2node(hnp, *iter, _log);
                        ASSERT_TRUE(cmp_node != NULL);

                        yf_hnpool_free(hnp, *iter, cmp_node, _log);
                        cmp_node = yf_hnpool_id2node(hnp, *iter, _log);
                        ASSERT_TRUE(NULL == cmp_node);
                        
                        allocted_list.erase(iter);
                        --alloc_cnt;
                }
        }        
}


/*
* slab pool
*/
TEST_F(BaseTest, SlabPool)
{
        yf_slab_pool_t  slab_pool;
        yf_slab_pool_init(&slab_pool, _log, 4, 8, 15, 20, 32, 64);

        void* data = yf_slab_pool_alloc(&slab_pool, 65, _log);
        ASSERT_TRUE(data == NULL);

        typedef std::list<void*> allocted_ct;
        allocted_ct allocted;
        int total_allocted = 0;

        for (int i = 0; i < 100000; ++i)
        {
                int try_cnts = random() % 16;
                
                if (random() % 2 == 0)
                {
                        for (int j = 0; j < try_cnts; ++j)
                        {
                                if (total_allocted == 0)
                                        break;
                                allocted_ct::iterator iter = allocted.begin();
                                std::advance(iter, random() % total_allocted);
                                yf_slab_pool_free(&slab_pool, *iter, _log);
                                allocted.erase(iter);
                                --total_allocted;
                        }
                }
                else {
                        for (int j = 0; j < try_cnts; ++j)
                        {
                                void* data = yf_slab_pool_alloc(&slab_pool, random() % 64, _log);
                                assert(data);
                                allocted.push_back(data);
                                ++total_allocted;
                        }
                }
        }
}


/*
* slab pool
*/
TEST_F(BaseTest, IdSeed)
{
        yf_id_seed_group_t  seed_group;
        yf_id_seed_group_init(&seed_group, 0);
        for (yf_u64_t i = 0; i < (((yf_u64_t)1)<<32); ++i)
        {
                yf_u32_t alloc_id = yf_id_seed_alloc(&seed_group);
                if (yf_mod(alloc_id, 8388608) == 0)
                        fprintf(stdout, "alloc id=%d\n", alloc_id);
        }
}


/*
* circular buf
*/
TEST_F(BaseTest, CircularBuf)
{
        yf_circular_buf_t  cic_buf, cic_buf_pre;
        yf_int_t  ret = yf_circular_buf_init(&cic_buf, 8192, _log);
        ASSERT_EQ(ret, YF_OK);

#define _TEST_MAX_BUF 8192*136
        char  buf_cmp[_TEST_MAX_BUF];

        yf_s32_t  head = 0, tail = 0, cursor = 0, cursor_seek, random_val;
        yf_s32_t  ftellv, fsizev;
        char* bres;
        
        yf_u16_t  vlength, cmplength, effective_len;
        char vbuf[32768];

#define _test_cmp_cs \
                ftellv = yf_cb_ftell(&cic_buf); \
                fsizev = yf_cb_fsize(&cic_buf); \
                assert(ftellv == cursor - head); \
                assert(fsizev == tail - head);

#define _test_seek_cursor \
                cursor_seek = (yf_s32_t)yf_mod(random(), 8192*64) - 8192*64; \
                switch (yf_mod(random(), 3)) \
                { \
                        case 0: \
                                cursor_seek = yf_min(tail-cursor, yf_max(cursor_seek, head-cursor)); \
                                cursor += cursor_seek; \
                                ret = yf_cb_fseek(&cic_buf, cursor_seek, YF_SEEK_CUR); \
                                break; \
                        case 1: \
                                cursor_seek = yf_min(tail-head, yf_max(cursor_seek, 0)); \
                                cursor = head + cursor_seek; \
                                ret = yf_cb_fseek(&cic_buf, cursor_seek, YF_SEEK_SET); \
                                break; \
                        case 2: \
                                cursor_seek = yf_max(head-tail, yf_min(0, cursor_seek)); \
                                cursor = tail + cursor_seek; \
                                ret = yf_cb_fseek(&cic_buf, cursor_seek, YF_SEEK_END); \
                                break; \
                } \
                assert(ret == YF_OK); \
                _test_cmp_cs;

#define _test_head_set \
                if (tail - head > 8192*80 || yf_mod(random(), 16) == 0) \
                { \
                        random_val = yf_mod(random(), 8192*16); \
                        yf_s32_t _head_offset = yf_mod(random(), 2) ? 0 : yf_min(random_val, tail - head); \
                        ret == yf_cb_fhead_set(&cic_buf, _head_offset); \
                        ASSERT_EQ(ret, YF_OK); \
                        if (_head_offset == 0) \
                                head = cursor; \
                        else { \
                                head += _head_offset; \
                                cursor = yf_max(head, cursor);\
                        } \
                        printf("head seted, now bsize=%d, offset=%d\n", tail - head, _head_offset); \
                        _test_cmp_cs; \
                }

#define _test_cb_infod "cb info={hi=%d,ho=%d; ci=%d,co=%d; ti=%d,to=%d}"
#define _test_cb_info cic_buf.head_index, cic_buf.head_offset, \
                                                cic_buf.cursor_index, cic_buf.cursor_offset, \
                                                cic_buf.tail_index, cic_buf.tail_offset \

        int choice, ichoice, i1;
        char cfill;
        for (int i = 0; i < 1024*1024; ++i)
        {
                choice = yf_mod(random(), 4);
                switch (choice)
                {
                        case 0:
                        case 1:
                                //write
                                if (choice == 0)
                                {
                                        _test_seek_cursor;
                                }
                                else {
                                        cursor = tail;
                                        yf_cb_fseek(&cic_buf, 0, YF_SEEK_END);
                                        _test_cmp_cs;
                                }

                                //if == 0, some wrong...
                                vlength = yf_mod(random(), 32768) + 1;
                                cfill = (char)random();
                                yf_memset(buf_cmp+cursor, cfill, vlength);

                                printf("before: write len=%d, now head=%d, cursor=%d, tail=%d, "
                                        _test_cb_infod"\n", vlength, head, cursor, tail, _test_cb_info);

                                if (yf_mod(random(), 2))
                                {
                                        cmplength = yf_cb_fwrite(&cic_buf, buf_cmp+cursor, vlength);
                                        ASSERT_EQ(cmplength, vlength);
                                }
                                else {
                                        char** wbufs = NULL, **tmp_wbuf;
                                        yf_s32_t woffset = 0;
                                        yf_s32_t wlen = yf_align(vlength, cic_buf.buf_size) + yf_mod(random(), 4096);

                                        //just for gdb core
                                        yf_memcpy(&cic_buf_pre, &cic_buf, offsetof(yf_circular_buf_t, buf));
                                        
                                        wlen = yf_cb_space_write_alloc(&cic_buf, wlen, &wbufs, &woffset);
                                        tmp_wbuf = wbufs;
                                        assert(wlen >= vlength);
                                        printf("after space alloc, "_test_cb_infod"\n", _test_cb_info);

                                        yf_s32_t  rest_len = vlength;
                                        yf_s32_t  writed = yf_min(rest_len, cic_buf.buf_size-woffset);
                                        yf_memset(wbufs[0]+woffset, cfill, writed);
                                        rest_len -= writed;
                                        ++wbufs;
                                        for (; rest_len > 0; rest_len-=cic_buf.buf_size, ++wbufs)
                                        {
                                                yf_memset(*wbufs, cfill, yf_min(rest_len, cic_buf.buf_size));
                                        }

                                        yf_cb_space_write_bytes(&cic_buf, vlength);
                                }
                                
                                cursor += vlength;
                                if (cursor > tail)
                                {
                                        tail = cursor;
                                        if (tail + 32768 >= _TEST_MAX_BUF)
                                        {
                                                printf("reach cmp buf's end, restart again\n");
                                                yf_memmove(buf_cmp, buf_cmp+head, tail-head);
                                                tail -= head;
                                                cursor -= head;
                                                head = 0;
                                        }
                                }

                                printf("after: write len=%d, now head=%d, cursor=%d, tail=%d, "
                                        _test_cb_infod"\n", vlength, head, cursor, tail, _test_cb_info);                                

                                _test_cmp_cs;
                                yf_circular_buf_shrink(&cic_buf, _log);
                                _test_head_set;
                                break;
                        case 2:
                                //read
                                _test_seek_cursor;
                                for (i1=0; i1 < 4; ++i1)
                                {
                                        ichoice = yf_mod(random(), 2);
                                        vlength = yf_mod(random(), 16384);
                                        effective_len = yf_min(vlength, tail-cursor);

                                        printf("read len=%d, effective_len=%d, peek=%d, "
                                                "now head=%d, cursor=%d, tail=%d\n", 
                                                        vlength, effective_len, ichoice, 
                                                        head, cursor, tail);
                                        
                                        cmplength = yf_cb_fread(&cic_buf, effective_len, ichoice, &bres);
                                        ASSERT_EQ(cmplength, effective_len);
                                        ret = yf_memcmp(bres, buf_cmp+cursor, effective_len);
                                        assert(ret == 0);
                                        ASSERT_EQ(ret, 0);

                                        if (!ichoice)
                                                cursor += effective_len;
                                        _test_cmp_cs;
                                }

                                yf_circular_buf_shrink(&cic_buf, _log);

                                _test_head_set;
                                break;
                        case 3:
                                //truncate
                                random_val = yf_mod(random(), 8192*128);
                                vlength = yf_min(random_val, tail-head);

                                printf("truncate to len=%d\n", vlength);
                                
                                tail = head + vlength;
                                if (cursor > tail)
                                        cursor = tail;

                                yf_cb_ftruncate(&cic_buf, vlength);
                                _test_cmp_cs;
                                break;
                        case 4:
                                break;
                        case 5:
                        case 6:
                        case 7:
                                break;
                }
        }

        yf_circular_buf_destory(&cic_buf);
}


#ifdef TEST_F_INIT
TEST_F_INIT(BaseTest, BaseAlgo);
TEST_F_INIT(BaseTest, BitOp);
TEST_F_INIT(BaseTest, Array);
TEST_F_INIT(BaseTest, List);
TEST_F_INIT(BaseTest, Rbtree);
TEST_F_INIT(BaseTest, StringLog);
TEST_F_INIT(BaseTest, Hash);
TEST_F_INIT(BaseTest, NodePool);
TEST_F_INIT(BaseTest, HNodePool);
TEST_F_INIT(BaseTest, SlabPool);
TEST_F_INIT(BaseTest, IdSeed);
TEST_F_INIT(BaseTest, CircularBuf);
#endif

int main(int argc, char **argv)
{
        srandom(time(NULL));

        yf_init_bit_indexs();

        yf_cpuinfo();
     
        _log = yf_log_open(YF_LOG_DEBUG, 8192, NULL);
        _mem_pool = yf_create_pool(1024000, _log);

        yf_init_time(_log);
        yf_update_time(NULL, NULL, _log);

        yf_int_t  ret = yf_strerror_init();
        assert(ret == YF_OK);
        
        testing::InitGoogleTest(&argc, (char**)argv);
        ret = RUN_ALL_TESTS();

        yf_destroy_pool(_mem_pool);

        return ret;
}

