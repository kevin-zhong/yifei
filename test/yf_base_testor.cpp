#include <gtest/gtest.h>
#include <list>

extern "C" {
#include <ppc/yf_header.h>
#include <base_struct/yf_core.h>
#include <mio_driver/yf_event.h>
}

yf_pid_t  yf_pid;
yf_pool_t  *_mem_pool;
yf_log_t    _log;

class BaseTest : public testing::Test
{
};

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
                printf("test_val=%ld have %d bits set\n", test_val[i], offset);

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
        
        yf_log_error(YF_LOG_ERR, &_log, YF_ENOSPC, "%p-%V-%c-%s", 
                        copy_buf, &str_tmp, buf[0], copy_buf);

        yf_log_t  file_log;
        file_log.max_log_size = 4096;
        file_log.recur_log_num = 5;
        file_log.switch_log_size = 102400;

        yf_log_open("dir/test.log", &file_log);

        for (int k = 0; k < 512; ++k)
        {
                for (int i = 0; i < 68; ++i)
                {
                        yf_log_error(YF_LOG_ERR, &file_log, i, "%p-%V-%c-%s", 
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
                printf("addr=%d, key_hash=%d\n", hash_kyes[i].value, hash_kyes[i].key_hash);
        }

        yf_int_t ret = yf_hash_init(&hash_init, hash_kyes, key_size);
        ASSERT_EQ(ret, YF_OK);

        for (size_t i = 0; i < key_size; ++i)
        {
                yf_uint_t key = yf_hash_key_lc((char*)keys[i], strlen(keys[i]));
                void* find_ret = yf_hash_find(&hash_test, key, (char*)keys[i], strlen(keys[i]));
                
                printf("find ret addr=%d vs org=%d, key hash=%d\n", 
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
        
        node_pool.each_taken_size = yf_node_taken_size(sizeof(yf_u64_t));
        node_pool.total_num = 1024;

        node_pool.nodes_array = (char*)yf_alloc(
                        node_pool.each_taken_size * node_pool.total_num);

        yf_init_node_pool(&node_pool, &_log);

        typedef std::list<void*> ListV;
        ListV allocted_list;
        
        void* new_node = NULL, *cmp_node = NULL;
        
        for (int i = 0; i < 1024; ++i)
        {
                new_node = yf_alloc_node_from_pool(&node_pool, &_log);
                ASSERT_TRUE(new_node != NULL);
                
                yf_u64_t  id = yf_get_id_by_node(&node_pool, new_node, &_log);
                cmp_node = yf_get_node_by_id(&node_pool, id, &_log);
                
                ASSERT_EQ(new_node, cmp_node);
                
                allocted_list.push_back(new_node);
        }

        new_node = yf_alloc_node_from_pool(&node_pool, &_log);
        ASSERT_TRUE(new_node == NULL);

        for (int i = 0; i < 16; ++i)
        {
                int delete_num = std::min((long)allocted_list.size(), random() % 1024);
                
                for (int j = 0; j < delete_num; ++j)
                {
                        ListV::iterator iter = allocted_list.begin();
                        std::advance(iter, random() % allocted_list.size());

                        cmp_node = *iter;
                        yf_u64_t  id = yf_get_id_by_node(&node_pool, cmp_node, &_log);
                        yf_free_node_to_pool(&node_pool, cmp_node, &_log);
                        cmp_node = yf_get_node_by_id(&node_pool, id, &_log);
                        ASSERT_TRUE(cmp_node == NULL);
                        
                        allocted_list.erase(iter);
                }

                int add_num = random() % 1024;
                
                for (int j = 0; j < add_num; ++j)
                {
                        new_node = yf_alloc_node_from_pool(&node_pool, &_log);
                        
                        if (allocted_list.size() >= 1024)
                        {
                                ASSERT_TRUE(new_node == NULL);
                                yf_log_debug(YF_LOG_DEBUG, &_log, 0, "used out, break now\n");
                                break;
                        }
                        ASSERT_TRUE(new_node != NULL);
                        //yf_log_debug(YF_LOG_DEBUG, &_log, 0, "size=%d\n", allocted_list.size());
                        
                        yf_u64_t  id = yf_get_id_by_node(&node_pool, new_node, &_log);
                        cmp_node = yf_get_node_by_id(&node_pool, id, &_log);

                        ASSERT_EQ(new_node, cmp_node);
                        
                        allocted_list.push_back(new_node);
                }
        }

        if (!allocted_list.empty())
        {
                cmp_node = *allocted_list.begin();
                yf_u64_t  id = yf_get_id_by_node(&node_pool, cmp_node, &_log);
                yf_free_node_to_pool(&node_pool, cmp_node, &_log);
                
                cmp_node = yf_get_node_by_id(&node_pool, id, &_log);
                ASSERT_TRUE(cmp_node == NULL);

                new_node = yf_alloc_node_from_pool(&node_pool, &_log);
                cmp_node = yf_get_node_by_id(&node_pool, id, &_log);
                ASSERT_TRUE(cmp_node == NULL);
        }

        yf_free(node_pool.nodes_array);
}


#ifdef TEST_F_INIT
TEST_F_INIT(BaseTest, BitOp);
TEST_F_INIT(BaseTest, Array);
TEST_F_INIT(BaseTest, List);
TEST_F_INIT(BaseTest, Rbtree);
TEST_F_INIT(BaseTest, StringLog);
TEST_F_INIT(BaseTest, Hash);
TEST_F_INIT(BaseTest, NodePool);
#endif

int main(int argc, char **argv)
{
        srandom(time(NULL));
        yf_pid = getpid();

        yf_init_bit_indexs();

        yf_cpuinfo();

        _log.max_log_size = 8192;
        yf_log_open(NULL, &_log);
        _mem_pool = yf_create_pool(1024000, &_log);

        yf_init_time(&_log);
        yf_update_time(NULL, NULL, &_log);

        yf_int_t  ret = yf_strerror_init();
        assert(ret == YF_OK);
        
        testing::InitGoogleTest(&argc, (char**)argv);
        ret = RUN_ALL_TESTS();

        yf_destroy_pool(_mem_pool);

        return ret;
}

