#include <gtest/gtest.h>
#include <list>

extern "C" {
#include <ppc/yf_header.h>
#include <base_struct/yf_core.h>
#include <mio_driver/yf_event.h>
#include <bridge/yf_bridge.h>

//just for test..., dont include this...
#include <bridge/bridge_in/yf_bridge_in.h>
#include <bridge/bridge_in/yf_bridge_task.h>
}


yf_pid_t  yf_pid;
yf_pool_t *_mem_pool;
yf_log_t _log;
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

#define MAX_SEND_BATCH 32
yf_u64_t  g_task_cnt = 0;
yf_int_t  g_send_batch = 0;

void on_task(yf_bridge_t* bridge
                , void* task, size_t len, yf_u64_t id, yf_log_t* log)
{
        TaskTest* task_test = (TaskTest*)task;
        task_test->resp = task_test->req * 2;

        ASSERT_TRUE(yf_send_task_res(bridge, task, len, id, 0, log) == 0);
}

void on_task_res(yf_bridge_t* bridge
                , void* task_res, size_t len, yf_u64_t id
                , yf_int_t status, void* data, yf_log_t* log)
{
        uint32_t i1 = 0;
        if (status == 0)
        {
                TaskTest* task_test = (TaskTest*)task_res;
                ASSERT_EQ(task_test->req * 2, task_test->resp);
                ASSERT_EQ(task_test->buf_len + sizeof(TaskTest), len);
        }

        yf_log_debug(YF_LOG_DEBUG, log, 0, "now g_task_cnt=%d", g_task_cnt);
        
        if (--g_task_cnt == 0 && g_send_batch >= MAX_SEND_BATCH)
        {
                for (i1 = 0; i1 < yf_last_process; i1++)
                {
                        kill(yf_processes[i1].pid, yf_signal_value(YF_KILL_SIGNAL));
                }
                yf_sleep(2);
                yf_evt_driver_stop(_evt_driver);
        }
}

void on_timeout_handler(struct yf_tm_evt_s* evt, yf_time_t* start)
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
                        send_buf, yf_mod(random(), 1024), &_log);
                
                if (task_id == yf_u64_t(-1))
                {
                        yf_log_debug(YF_LOG_WARN, &_log, 0, "send task failed, stop...");
                        break;
                }
                else
                        ++g_task_cnt;
        }

        if (++g_send_batch < MAX_SEND_BATCH)
        {
                yf_time_t time_out = {0, yf_mod(random(), 1024)};
                yf_register_tm_evt(evt, &time_out);
        }
        yf_log_debug(YF_LOG_DEBUG, &_log, 0, "now send batch=%d g_task_cnt=%d", 
                        g_send_batch, g_task_cnt);
}


void  child_proc(void *data, yf_log_t *log)
{
        yf_evt_driver_init_t driver_init = {0, 128, 16, 1, log, YF_DEFAULT_DRIVER_CB};
        yf_evt_driver_t* child_evt_driver = yf_evt_driver_create(&driver_init);
        ASSERT_TRUE(child_evt_driver != NULL);        
        
        yf_bridge_t* bridge = (yf_bridge_t*)data;

        ASSERT_EQ(YF_OK, yf_attach_bridge(bridge, 
                        child_evt_driver, on_task, &_log));

        yf_evt_driver_start(child_evt_driver);
}


TEST_F(BridgeTestor, EvtProc)
{
        yf_bridge_cxt_t bridge_ctx = {YF_BRIDGE_INS_PROC, 
                        YF_BRIDGE_INS_PROC,
                        YF_BRIDGE_EVT_DRIVED,
                        YF_BRIDGE_EVT_DRIVED,
                        YF_TASK_DISTPATCH_IDLE,
                        (void*)child_proc, 
                        1, 10240, 128, 1024 * 1024
                };
        
        yf_bridge_t* bridge = yf_bridge_create(&bridge_ctx, &_log);
        ASSERT_TRUE(bridge != NULL);

        yf_evt_driver_init_t driver_init = {0, 128, 16, 1, &_log, YF_DEFAULT_DRIVER_CB};
        _evt_driver = yf_evt_driver_create(&driver_init);
        ASSERT_TRUE(_evt_driver != NULL);
        
        ASSERT_EQ(YF_OK, yf_attach_res_bridge(bridge, 
                        _evt_driver, on_task_res, &_log));

        yf_tm_evt_t* tm_evt;
        yf_alloc_tm_evt(_evt_driver, &tm_evt, &_log);
        tm_evt->data = bridge;
        tm_evt->timeout_handler = on_timeout_handler;
        yf_time_t time_out = {1, 200};
        yf_register_tm_evt(tm_evt, &time_out);

        yf_evt_driver_start(_evt_driver);
}


TEST_F(BridgeTestor, TaskQueue)
{
        yf_log_t  log_tmp;
        log_tmp.max_log_size = 8192;
        yf_log_open(NULL, &log_tmp);
        
        char  tq_buf[102400];
        task_info_t  task_info;
        char  task_buf[10240];

        /*
        * note, must clear, else, repeate test will fail...
        */
        yf_memzero(tq_buf, sizeof(tq_buf));
        yf_task_queue_t* tq = yf_init_task_queue(tq_buf, sizeof(tq_buf), &log_tmp);

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
                                yf_log_debug(YF_LOG_DEBUG, &log_tmp, 0, "set buflen=0");
                        }

                        task_info.id = random();
                        if (yf_task_push(tq, &task_info, (char*)task_test, 
                                        sizeof(TaskTest) + task_test->buf_len, &log_tmp) != YF_OK)
                        {
                                break;
                        }
                        task_infos.push_back(task_info);
                        task_datas.push_back(std::pair<int, int>(task_test->req, task_test->buf_len));

                        yf_log_debug(YF_LOG_DEBUG, &log_tmp, 0, 
                                        "send task req=%d", task_test->req);
                }

                try_cnt = random() % 128;
                
                for (int j = 0; j < try_cnt; ++j)
                {
                        size_t  task_size = sizeof(task_buf);
                        yf_int_t  ret = yf_task_pop(tq, &task_info, task_buf, &task_size, &log_tmp);
                        
                        if (ret == YF_OK)
                        {
                                TaskTest* task_test = (TaskTest*)task_buf;
                                std::pair<int, int>& pair_cmp = task_datas.front();

                                yf_log_debug(YF_LOG_DEBUG, &log_tmp, 0, 
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

        yf_log_close(&log_tmp);
        printf("%d\n", log_tmp.file);
}


#ifdef TEST_F_INIT
TEST_F_INIT(BridgeTestor, EvtProc);
TEST_F_INIT(BridgeTestor, TaskQueue);
#endif

int main(int argc, char **argv)
{
        srandom(time(NULL));
        yf_pid = getpid();
        yf_pagesize = getpagesize();

        yf_cpuinfo();

        _log.max_log_size = 8192;
        yf_log_open("dir/bridge.log", &_log);
        _mem_pool = yf_create_pool(102400, &_log);

        yf_init_bit_indexs();

        yf_init_time(&_log);
        yf_update_time(NULL, NULL, &_log);

        yf_int_t ret = yf_strerror_init();
        assert(ret == YF_OK);

        ret = yf_save_argv(&_log, argc, argv);
        assert(ret == YF_OK);

        ret = yf_init_setproctitle(&_log);
        assert(ret == YF_OK);

        ret = yf_init_processs(&_log);
        assert(ret == YF_OK);

        ret = yf_init_threads(36, 1024 * 1024, &_log);
        assert(ret == YF_OK);

        yf_set_sig_handler(SIGIO, SIG_IGN, &_log);

        testing::InitGoogleTest(&argc, (char **)argv);
        ret = RUN_ALL_TESTS();

        yf_destroy_pool(_mem_pool);

        return ret;
}


