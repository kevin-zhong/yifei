#include <gtest/gtest.h>
#include <list>

extern "C" {
#include <ppc/yf_header.h>
#include <base_struct/yf_core.h>
#include <mio_driver/yf_event.h>
}

#if 0
#undef  yf_lock_init
#undef  yf_lock_destory
#undef  yf_lock
#undef  yf_unlock
#define yf_lock_t  yf_mlock_t

#define  yf_lock_init(lock) yf_mlock_init(lock)
#define  yf_lock_destory(lock) yf_mlock_destory(lock)
#define  yf_lock(lock)  yf_mlock(lock)
#define  yf_unlock(lock)   yf_munlock(lock)

//经测试, fast lock 比 mutex lock 快大概1倍左右(即耗时大概在60%左右)
//且多次测试发现fast lock耗时非常稳定，而mutex lock耗时则不怎么稳定，上下起伏较大
//mutex[11546+8094+11984+8663]  fast_lock[6614+6983+6767+6534]
#endif

yf_pool_t  *_mem_pool;
yf_log_t*    _log;

class ThreadTest : public testing::Test
{
};

/*
* thread
*/
yf_thread_value_t thread_func1(void* arg)
{
        int cnt = int((long)arg);
        for (int i = 0; i < cnt; ++i)
        {
                usleep(5);
                yf_log_error(YF_LOG_DEBUG, _log, i, "thread run %d", cnt);
        }
}

TEST_F(ThreadTest, thread)
{
        yf_tid_t  tid, tid2, tid3;
        yf_create_thread(&tid, thread_func1, (void*)150, _log);
        yf_create_thread(&tid2, thread_func1, (void*)200, _log);
        yf_create_thread(&tid3, thread_func1, (void*)150, _log);

        void* ptr = NULL;
        yf_thread_join(tid, &ptr);
        yf_thread_join(tid2, &ptr);
        yf_thread_join(tid3, &ptr);
}


struct  TestMa
{
        yf_lock_t  lock;
        int  cnt;
};

struct  TestCtx
{
        TestMa *test_ma;
        int  try_times;
};
yf_thread_value_t  add(void* arg)
{
        TestCtx* ctx = (TestCtx*)arg;
        
        TestMa* test_ma = ctx->test_ma;
        int  try_times = ctx->try_times;
        
        for (int i = 0; i < try_times; ++i)
        {
                yf_lock(&test_ma->lock);
                test_ma->cnt++;
                yf_unlock(&test_ma->lock);
        }
}

yf_thread_value_t  sub(void* arg)
{
        TestCtx* ctx = (TestCtx*)arg;
        
        TestMa* test_ma = ctx->test_ma;
        int  try_times = ctx->try_times;
        
        for (int i = 0; i < try_times; ++i)
        {
                yf_lock(&test_ma->lock);
                test_ma->cnt--;
                yf_unlock(&test_ma->lock);
        }
}

TEST_F(ThreadTest, fast_lock)
{
        TestMa  test_ma;
        test_ma.cnt = 0;
        yf_lock_init(&test_ma.lock);

        int  cnt[4] = {6900000, 7500000, 6600000, 6215968};

        TestCtx  test_ctx[4];
        for (size_t i = 0; i < sizeof(test_ctx) / sizeof(test_ctx[0]); ++i)
        {
                test_ctx[i].test_ma = &test_ma;
                test_ctx[i].try_times = cnt[i];
        }
        
        yf_tid_t  tid[4];
        yf_create_thread(tid, add, test_ctx, _log);
        yf_create_thread(tid + 2, sub, test_ctx + 2, _log);
        yf_create_thread(tid + 1, add, test_ctx + 1, _log);
        yf_create_thread(tid + 3, sub, test_ctx + 3, _log);

        void* ptr = NULL;
        for (size_t i = 0; i < sizeof(tid) / sizeof(yf_tid_t); ++i)
                yf_thread_join(tid[i], &ptr);

        printf("last cnt=%d\n", test_ma.cnt);
        ASSERT_EQ(test_ma.cnt, (cnt[0] + cnt[1] - cnt[2] - cnt[3]));

        yf_lock_destory(&test_ma.lock);
}


struct  CondCtx
{
        yf_mutex_t  *_mutex;
        yf_cond_t    *_unempty_cond;
        yf_cond_t    *_unfull_cond;
        yf_int_t   _thread_size;
        yf_int_t   _consume_wait_size;
        std::list<int>  _content;
};

#define  FULL_SIZE  32

yf_thread_value_t  produce(void* arg)
{
        CondCtx* cond_ctx = (CondCtx*)arg;

#define  SEND_CONTENT(_c) \
                yf_mlock(cond_ctx->_mutex);\
                if (cond_ctx->_content.size() >= FULL_SIZE) { \
                        yf_log_debug0(YF_LOG_DEBUG, _log, 0, "queue full, wait");\
                        yf_cond_wait(cond_ctx->_unfull_cond, cond_ctx->_mutex, _log); \
                } \
                cond_ctx->_content.push_back(_c); \
                yf_log_debug1(YF_LOG_DEBUG, _log, 0, "produce[ %d ]", _c); \
                if (cond_ctx->_consume_wait_size) {\
                        yf_log_debug0(YF_LOG_DEBUG, _log, 0, "have consume wait, signal"); \
                        yf_cond_signal(cond_ctx->_unempty_cond, _log); \
                } \
                yf_munlock(cond_ctx->_mutex);

        for (int i = 0; i < 1024; ++i)
        {
                int  random_val = ::random() % 10240000 + 1;
                SEND_CONTENT(random_val);
                usleep(2);
                if (i % 256 == 0)
                        usleep(300000);
        }
        for (int i = 0; i < cond_ctx->_thread_size; ++i)
        {
                SEND_CONTENT(0);
                usleep(2);
        }
}

yf_thread_value_t  consume(void* arg)
{
        CondCtx* cond_ctx = (CondCtx*)arg;
        
        while (true)
        {
                yf_mlock(cond_ctx->_mutex);
                while (cond_ctx->_content.empty())//note, this must circ, else will core..
                {
                        yf_log_debug0(YF_LOG_DEBUG, _log, 0, "queue empty, wait");
                        cond_ctx->_consume_wait_size++;
                        yf_cond_wait(cond_ctx->_unempty_cond, cond_ctx->_mutex, _log);
                        cond_ctx->_consume_wait_size--;
                }
                int  content = cond_ctx->_content.front();
                cond_ctx->_content.pop_front();
                yf_log_debug1(YF_LOG_DEBUG, _log, 0, "consume[ %d ]", content);

                if (cond_ctx->_content.size() == FULL_SIZE - 1)
                {
                        yf_log_debug0(YF_LOG_DEBUG, _log, 0, "queue not full, signal");
                        yf_cond_signal(cond_ctx->_unfull_cond, _log);
                }
                yf_munlock(cond_ctx->_mutex);

                if (content == 0)
                {
                        yf_log_debug0(YF_LOG_DEBUG, _log, 0, "end thread now");
                        break;
                }

                usleep(10000);
        }
}

TEST_F(ThreadTest, cond)
{
        yf_tid_t  tid[4];
        
        CondCtx  cond_ctx;
        cond_ctx._mutex = yf_mutex_init(_log);
        cond_ctx._unempty_cond = yf_cond_init(_log);
        cond_ctx._unfull_cond = yf_cond_init(_log);
        cond_ctx._thread_size = (int)(sizeof(tid) / sizeof(yf_tid_t)) - 1;
        cond_ctx._consume_wait_size = 0;
        
        yf_create_thread(tid, produce, &cond_ctx, _log);
        
        for (size_t i = 1; i < sizeof(tid) / sizeof(yf_tid_t); ++i)
        {
                printf("create thread i=%d\n", i);
                yf_create_thread(tid + i, consume, &cond_ctx, _log);
        }

        void* ptr = NULL;
        for (size_t i = 0; i < sizeof(tid) / sizeof(yf_tid_t); ++i)
                yf_thread_join(tid[i], &ptr);

        yf_mutex_destroy(cond_ctx._mutex, _log);
        yf_cond_destroy(cond_ctx._unempty_cond, _log);
        yf_cond_destroy(cond_ctx._unfull_cond, _log);
}


#ifdef TEST_F_INIT
TEST_F_INIT(ThreadTest, thread);
TEST_F_INIT(ThreadTest, fast_lock);
TEST_F_INIT(ThreadTest, cond);
#endif

int main(int argc, char **argv)
{
        srandom(time(NULL));
        
        yf_cpuinfo();

        yf_init_bit_indexs();

        _log = yf_log_open(YF_LOG_DEBUG, 8192, NULL);
        _mem_pool = yf_create_pool(1024000, _log);

        yf_init_time(_log);
        yf_update_time(NULL, NULL, _log);

        yf_int_t  ret = yf_strerror_init();
        assert(ret == YF_OK);

        ret = yf_init_threads(36, 1024 * 1024, 1, _log);
        assert(ret == YF_OK);
        
        testing::InitGoogleTest(&argc, (char**)argv);
        ret = RUN_ALL_TESTS();

        yf_destroy_pool(_mem_pool);

        return ret;
}


