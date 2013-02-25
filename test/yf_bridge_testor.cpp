#include <gtest/gtest.h>
#include <list>

extern "C" {
#include <ppc/yf_header.h>
#include <base_struct/yf_core.h>
#include <mio_driver/yf_event.h>
#include <bridge/yf_bridge.h>
#include <log_ext/yf_log_file.h>

//just for test..., dont include this...
#include <bridge/bridge_in/yf_bridge_in.h>
#include <bridge/bridge_in/yf_bridge_task.h>
}


yf_pool_t *_mem_pool;
yf_log_t* _log;
yf_evt_driver_t* _evt_driver;

class BridgeTestor : public testing::Test
{
};


struct  TaskTest
{
        yf_int_t  req;
        yf_u64_t  resp;
        yf_int_t  buf_len;
        char  buf[0];
};


TEST_F(BridgeTestor, TaskQueue)
{
        yf_log_file_init_ctx_t log_file_init = {1024*128, 1024*1024*64, 8, NULL};
        yf_log_t* log_tmp = yf_log_open(YF_LOG_DEBUG, 8192, &log_file_init);
        
        char  tq_buf[102400];
        task_info_t  task_info;
        char  task_buf[10240];

        /*
        * note, must clear, else, repeat test will fail...
        */
        yf_memzero(tq_buf, sizeof(tq_buf));
        yf_task_queue_t* tq = yf_init_task_queue(tq_buf, sizeof(tq_buf), log_tmp);

        std::list<std::pair<int, int> > task_datas;
        std::list<task_info_t> task_infos;
        
        for (int i = 0; i < 16; ++i)
        {
                yf_uint_t  try_cnt = random() % 128;
                
                for (int j = 0; j < try_cnt; ++j)
                {
                        TaskTest* task_test = (TaskTest*)task_buf;
                        task_test->req = yf_mod(random(), (1<<30));
                        task_test->buf_len = yf_mod(random(), 4096);
                        if (task_test->buf_len < 8)
                        {
                                task_test->buf_len = 0;
                                yf_log_debug(YF_LOG_DEBUG, log_tmp, 0, "set buflen=0");
                        }

                        task_info.id = random();
                        if (yf_task_push(tq, &task_info, (char*)task_test, 
                                        sizeof(TaskTest) + task_test->buf_len, log_tmp) != YF_OK)
                        {
                                break;
                        }
                        task_infos.push_back(task_info);
                        task_datas.push_back(std::pair<int, int>(task_test->req, task_test->buf_len));

                        yf_log_debug(YF_LOG_DEBUG, log_tmp, 0, 
                                        "send task req=%d", task_test->req);
                }

                try_cnt = random() % 128;
                
                for (int j = 0; j < try_cnt; ++j)
                {
                        size_t  task_size = sizeof(task_buf);
                        yf_int_t  ret = yf_task_pop(tq, &task_info, task_buf, &task_size, log_tmp);
                        
                        if (ret == YF_OK)
                        {
                                TaskTest* task_test = (TaskTest*)task_buf;
                                std::pair<int, int>& pair_cmp = task_datas.front();

                                yf_log_debug(YF_LOG_DEBUG, log_tmp, 0, 
                                                "recv task req=%d", task_test->req);

                                ASSERT_EQ(task_size, sizeof(TaskTest) + task_test->buf_len);
                                ASSERT_EQ(pair_cmp.first, task_test->req);
                                ASSERT_EQ(pair_cmp.second, task_test->buf_len);

                                task_datas.pop_front();
                                
                                ASSERT_EQ(task_infos.front().id, task_info.id);
                                task_infos.pop_front();
                        }
                        else {
                                ASSERT_TRUE(ret == YF_AGAIN);
                                ASSERT_TRUE(task_datas.empty());
                                break;
                        }
                }
        }

        yf_log_close(log_tmp);
}


#define MAX_SEND_BATCH 128
yf_uint_t  g_task_cnt = 0;
yf_uint_t  g_send_batch = 0;
yf_bridge_t* g_bridge;

void _send_task(yf_bridge_t* bridge
                , void* task, size_t len, yf_u64_t id, yf_log_t* log)
{
        TaskTest* task_test = (TaskTest*)task;
        task_test->resp = task_test->req * 2;

        assert(task_test->buf_len + sizeof(TaskTest) == len);

        //if tq size short, maybe fail...
        yf_uint_t  cnt = 0;
        while (yf_send_task_res(bridge, task, len, id, 0, log) != YF_OK)
        {
                yf_log_debug(YF_LOG_DEBUG, log, 0, "send task res fail, try cnt=%d...", ++cnt);
                yf_msleep(100);
        }
}

void on_task_fchild(yf_bridge_t* bridge
                , void* task, size_t len, yf_u64_t id, yf_log_t* log)
{
        static int child_task_cnt = 0;
        _send_task(bridge, task, len, id, log);
        if ((++child_task_cnt & 63) == 0)
        {
                yf_log_debug(YF_LOG_DEBUG, log, 0, "parent deal task num=%d", child_task_cnt);
        }
}

void on_task_fparent(yf_bridge_t* bridge
                , void* task, size_t len, yf_u64_t id, yf_log_t* log)
{
        _send_task(bridge, task, len, id, log);
        
        if (random() % 4 == 0) 
        {
                //printf("recv task_%d\n", id);
                yf_msleep(yf_mod(random(), 512));
        }
}

char cmp_buf[10240];

void on_task_res_fparent(yf_bridge_t* bridge
                , void* task_res, size_t len, yf_u64_t id
                , yf_int_t status, void* data, yf_log_t* log)
{
        uint32_t i1 = 0;
        if (status == 0)
        {
                TaskTest* task_test = (TaskTest*)task_res;
                assert(task_test->req * 2 == task_test->resp);
                assert(task_test->buf_len + sizeof(TaskTest) == len);
                char* cbuf = (char*)task_test + sizeof(TaskTest);
                assert(memcmp(cmp_buf, cbuf, task_test->buf_len) == 0);
        }
        else {
                yf_log_debug(YF_LOG_DEBUG, log, 0, "task_%l err status=%d", 
                                id, status);
        }
}


void on_task_res_fchild(yf_bridge_t* bridge
                , void* task_res, size_t len, yf_u64_t id
                , yf_int_t status, void* data, yf_log_t* log)
{
        on_task_res_fparent(bridge, task_res, len, id, status, data, log);
        --g_task_cnt;
        yf_log_debug(YF_LOG_DEBUG, log, 0, "now g_task_cnt=%d", g_task_cnt);

        //cant call this in this function and also, yf_evt_driver_destroy()...
        //yf_bridge_destory(g_bridge, log);        
}

void _on_timeout_handler(struct yf_tm_evt_s* evt, yf_uint_t* task_cnt, yf_uint_t* send_batch)
{
        yf_bridge_t* bridge = (yf_bridge_t*)evt->data;

        char send_buf[10240];
        yf_uint_t  send_cnt = random() % 128 * 5;

        for (int i = 0; i < send_cnt; ++i)
        {
                TaskTest* task_test = (TaskTest*)send_buf;
                task_test->req = yf_mod(random(), (1<<30));
                task_test->buf_len = yf_mod(random(), 4096);

                char* cbuf = send_buf + sizeof(TaskTest);
                yf_memset(cbuf, '8', task_test->buf_len);

                yf_u64_t task_id = yf_send_task(bridge, send_buf, 
                        sizeof(TaskTest) + task_test->buf_len, 0, 
                        send_buf, yf_mod(random(), 1024), evt->log);
                
                if (task_id == yf_u64_t(-1))
                {
                        yf_log_debug(YF_LOG_WARN, evt->log, 0, "send task failed, stop...");
                        break;
                }
                else
                        ++*task_cnt;
        }

        //printf("after send %d sleep now!!!\n", send_cnt);
        //yf_sleep(36);

        if (++*send_batch < MAX_SEND_BATCH)
        {
                yf_time_t time_out = {0, yf_mod(random(), 1024)};
                yf_register_tm_evt(evt, &time_out);
        }
        yf_log_debug(YF_LOG_DEBUG, evt->log, 0, "now send batch=%d g_task_cnt=%d", 
                        *send_batch, *task_cnt);
}


void on_parent_timeout_handler(struct yf_tm_evt_s* evt, yf_time_t* start)
{
        _on_timeout_handler(evt, &g_task_cnt, &g_send_batch);
        
        yf_log_debug(YF_LOG_DEBUG, evt->log, 0, "parent send child task cnt=%d, batch=%d", 
                        g_task_cnt, g_send_batch);
}


void  on_child_timeout_handler(struct yf_tm_evt_s* evt, yf_time_t* start)
{
        yf_uint_t  cnt1, cnt2;
        _on_timeout_handler(evt, &cnt1, &cnt2);
        yf_log_debug(YF_LOG_DEBUG, evt->log, 0, "child send parent task cnt=%d", cnt1);
}


yf_uint_t  g_poll_cnt = 0;

void on_poll_evt_driver(yf_evt_driver_t* evt_driver, void* data, yf_log_t* log)
{
        if (g_task_cnt == 0 && g_send_batch >= MAX_SEND_BATCH)
        {
                if (g_poll_cnt == 0)
                {
                        yf_log_debug(YF_LOG_DEBUG, log, 0, "will exit, first destory bridge...");
                        yf_bridge_destory(g_bridge, log);
                        //wait for child proc exit...
                        yf_sleep(3);
                }
                else if (g_poll_cnt == 1)
                        yf_evt_driver_stop(_evt_driver);
                
                ++g_poll_cnt;
        }
}

void aflter_stop_evt_driver(yf_evt_driver_t* evt_driver, void* data, yf_log_t* log)
{
        yf_evt_driver_destory(_evt_driver);
}

void on_child_poll_evt_driver(yf_evt_driver_t* evt_driver, void* data, yf_log_t* log)
{
        if (random() % 32 == 0)
        {
                yf_log_debug(YF_LOG_DEBUG, log, 0, "will exit now !!");
                
                yf_evt_driver_destory(evt_driver);
                yf_sleep(1);
                exit(random() % 5);
        }
}


#define  TEST_CHILD_NUM 4
yf_bridge_t* g_child2parent_bridges[TEST_CHILD_NUM] = {NULL};


void  evt_driven_child(yf_bridge_t* bridge, yf_int_t is_proc, yf_log_t *log)
{
        yf_evt_driver_init_t driver_init = {0, 128, 16, log, YF_DEFAULT_DRIVER_CB};
        driver_init.stop_cb = aflter_stop_evt_driver;
        if (is_proc)
                driver_init.poll_cb = on_child_poll_evt_driver;
        
        yf_evt_driver_t* child_evt_driver = yf_evt_driver_create(&driver_init);
        assert(child_evt_driver != NULL);

        yf_int_t ret = yf_attach_bridge(bridge, child_evt_driver, on_task_fparent, log);
        assert(ret == YF_OK);

        if (!is_proc)
        {
                yf_bridge_t* bridge_parent = g_child2parent_bridges[yf_bridge_child_no(bridge, log)];
                if (bridge_parent)
                {
                        ret = yf_attach_res_bridge(bridge_parent, child_evt_driver, 
                                        on_task_res_fparent, log);
                        assert(ret == YF_OK);

                        yf_tm_evt_t* tm_evt;
                        yf_alloc_tm_evt(child_evt_driver, &tm_evt, log);
                        tm_evt->data = bridge_parent;
                        tm_evt->timeout_handler = on_child_timeout_handler;
                        yf_time_t time_out = {1, 150};
                        ret = yf_register_tm_evt(tm_evt, &time_out);
                        assert(ret == YF_OK);
                }
        }

        yf_evt_driver_start(child_evt_driver);
}

void on_exe_exit_signal(yf_sig_event_t* sig_evt)
{
        yf_process_get_status(sig_evt->log);
}

void  bridge_parent_init(yf_bridge_cxt_t* bridge_ctx, yf_int_t attach_task_bridge)
{
        yf_bridge_t* bridge = yf_bridge_create(bridge_ctx, _log);
        ASSERT_TRUE(bridge != NULL);
        g_bridge = bridge;

        yf_evt_driver_init_t driver_init = {0, 128, 16, _log, YF_DEFAULT_DRIVER_CB};
        driver_init.poll_cb = on_poll_evt_driver;
        driver_init.stop_cb = aflter_stop_evt_driver;
        
        _evt_driver = yf_evt_driver_create(&driver_init);
        ASSERT_TRUE(_evt_driver != NULL);

        yf_log_file_flush_drive(_evt_driver, 5000, NULL);

        yf_sig_event_t  sig_child_evt = {SIGCHLD, NULL, NULL, NULL, on_exe_exit_signal};
        ASSERT_EQ(YF_OK, yf_register_singal_evt(_evt_driver, &sig_child_evt, _log));
        
        ASSERT_EQ(YF_OK, yf_attach_res_bridge(bridge, 
                        _evt_driver, on_task_res_fchild, _log));

        if (attach_task_bridge)
        {
                for (int i = 0; i < TEST_CHILD_NUM; ++i)
                {
                        ASSERT_EQ(YF_OK, yf_attach_bridge(g_child2parent_bridges[i], 
                                        _evt_driver, on_task_fchild, _log));
                }
        }

        yf_tm_evt_t* tm_evt;
        yf_alloc_tm_evt(_evt_driver, &tm_evt, _log);
        tm_evt->data = bridge;
        tm_evt->timeout_handler = on_parent_timeout_handler;
        yf_time_t time_out = {1, 200};
        yf_register_tm_evt(tm_evt, &time_out);

        yf_evt_driver_start(_evt_driver);        
}


/*
* bridge proc
*/
void  child_proc(void *data, yf_log_t *log)
{
        yf_bridge_t* bridge = (yf_bridge_t*)data;
        evt_driven_child(bridge, 1, log);
}


TEST_F(BridgeTestor, EvtProc)
{
        yf_bridge_cxt_t bridge_ctx = {YF_BRIDGE_INS_PROC, 
                        YF_BRIDGE_INS_PROC,
                        YF_BRIDGE_EVT_DRIVED,
                        YF_BRIDGE_EVT_DRIVED,
                        YF_TASK_DISTPATCH_IDLE,
                        (void*)child_proc, 
                        TEST_CHILD_NUM, 10240, 128, 1024 * 1024
                };
        bridge_parent_init(&bridge_ctx, 0);
}


/*
* bridge blocked thread
*/
yf_thread_value_t thread_exe(void *arg)
{
        yf_bridge_t* bridge = (yf_bridge_t*)arg;

        yf_int_t ret = yf_attach_bridge(bridge, NULL, on_task_fparent, _log);
        assert(ret == YF_OK);
        
        while (1)
        {
                yf_poll_task(bridge, _log);
        }
        return NULL;
}


TEST_F(BridgeTestor, BlockedThread)
{
        yf_bridge_cxt_t bridge_ctx = {YF_BRIDGE_INS_PROC, 
                        YF_BRIDGE_INS_THREAD,
                        YF_BRIDGE_EVT_DRIVED,
                        YF_BRIDGE_BLOCKED,
                        YF_TASK_DISTPATCH_IDLE,
                        (void*)thread_exe, 
                        TEST_CHILD_NUM, 10240, 128, 1024 * 1024
                };
        bridge_parent_init(&bridge_ctx, 0);
}


/*
* bridge evt driven thread
*/
yf_thread_value_t evt_thread_exe(void *arg)//like child_proc...
{
        yf_bridge_t* bridge = (yf_bridge_t*)arg;
        evt_driven_child(bridge, 0, _log);
        return NULL;
}


TEST_F(BridgeTestor, EvtThread)
{
        //init child to parent bridge
        yf_bridge_cxt_t parent_bridge_ctx = {YF_BRIDGE_INS_THREAD, 
                        YF_BRIDGE_INS_THREAD,
                        YF_BRIDGE_EVT_DRIVED,
                        YF_BRIDGE_EVT_DRIVED,
                        YF_TASK_DISTPATCH_IDLE,
                        NULL, 1,
                        10240, 128, 1024 * 1024
                };
        
        for (int i = 0; i < TEST_CHILD_NUM; ++i)
        {
                g_child2parent_bridges[i] = yf_bridge_create(&parent_bridge_ctx, _log);
        }
        
        yf_bridge_cxt_t bridge_ctx = {YF_BRIDGE_INS_PROC, 
                        YF_BRIDGE_INS_THREAD,
                        YF_BRIDGE_EVT_DRIVED,
                        YF_BRIDGE_EVT_DRIVED,
                        YF_TASK_DISTPATCH_IDLE,
                        (void*)evt_thread_exe, 
                        TEST_CHILD_NUM, 10240, 128, 1024 * 1024
                };
        bridge_parent_init(&bridge_ctx, 1);
}



#ifdef TEST_F_INIT
TEST_F_INIT(BridgeTestor, EvtProc);
TEST_F_INIT(BridgeTestor, BlockedThread);
TEST_F_INIT(BridgeTestor, EvtThread);
TEST_F_INIT(BridgeTestor, TaskQueue);
#endif

int main(int argc, char **argv)
{
        yf_memset(cmp_buf, '8', sizeof(cmp_buf));
        
        srandom(time(NULL));
        yf_pagesize = getpagesize();

        yf_cpuinfo();

        yf_int_t ret = 0;

        //first need init threads before init log file
        ret = yf_init_threads(36, 1024 * 1024, 1, NULL);
        assert(ret == YF_OK);
        
        yf_log_file_init(NULL);
        yf_log_file_init_ctx_t log_file_init = {1024*128, 1024*1024*64, 8, "dir/bridge.log"};

        _log = yf_log_open(YF_LOG_DEBUG, 8192, (void*)&log_file_init);
        _mem_pool = yf_create_pool(102400, _log);

        yf_init_bit_indexs();

        yf_init_time(_log);
        yf_update_time(NULL, NULL, _log);

        ret = yf_strerror_init();
        assert(ret == YF_OK);

        ret = yf_save_argv(_log, argc, argv);
        assert(ret == YF_OK);

        ret = yf_init_setproctitle(_log);
        assert(ret == YF_OK);

        ret = yf_init_processs(_log);
        assert(ret == YF_OK);

        testing::InitGoogleTest(&argc, (char **)argv);
        ret = RUN_ALL_TESTS();

        yf_destroy_pool(_mem_pool);

        return ret;
}


