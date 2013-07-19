#include <gtest/gtest.h>
#include <sched.h>

extern "C" {
#include <ppc/yf_header.h>
#include <base_struct/yf_core.h>
#include <mio_driver/yf_event.h>
#include <log_ext/yf_log_file.h>
}

yf_pool_t *_mem_pool = NULL;
yf_log_t* _log;
yf_evt_driver_t *_evt_driver = NULL;

yf_s64_t tv_ms[4096];
size_t tm_cnt = 0;
yf_s64_t tags[4096];
yf_tm_evt_t *tm_evts[4096];
yf_int_t _sleep_test_timeout = 0;

void on_poll(yf_evt_driver_t *evt_driver, void *, yf_log_t *log)
{
        static size_t erase_cnt = 0;

        if (erase_cnt >= tm_cnt / 3 || tm_cnt >= YF_ARRAY_SIZE(tv_ms) - 64)
                return;

        for (int i = 0; i < 3; ++i)
        {
                int index = ::random() % YF_ARRAY_SIZE(tv_ms);
                if (tv_ms[index])
                {
                        yf_tm_evt_t *tm_evt = tm_evts[index];
                        yf_log_debug2(YF_LOG_DEBUG, log, 0, "unregist tm evt, index=%d, ms=%d",
                                      index, tv_ms[index]);

                        yf_int_t ret = yf_unregister_tm_evt(tm_evt);
                        assert(ret == YF_OK);
                        ++tm_cnt;
                        tv_ms[index] = 0;
                        ++erase_cnt;
                }
        }
}

void on_timeout(struct yf_tm_evt_s *evt, yf_time_t *start)
{
        yf_s64_t *org_tms = (yf_s64_t *)evt->data;
        yf_s64_t real_tms = yf_time_diff_ms(&yf_now_times.clock_time, start);
        yf_s64_t  diff_tms = (real_tms - *org_tms);
        
        if (diff_tms >= 10)
        {
                yf_log_debug3(YF_LOG_DEBUG, evt->log, 0, "real=%L vs target=%L, big lacy diff=%L", 
                                real_tms, *org_tms, diff_tms);
        }
        else {
                yf_log_debug3(YF_LOG_DEBUG, evt->log, 0, "real=%L vs target=%L, diff=%L", 
                                real_tms, *org_tms, diff_tms);                
        }

        *org_tms = 0;

        if (tags[tm_cnt] && _sleep_test_timeout)
        {
                yf_log_debug2(YF_LOG_DEBUG, evt->log, 0, "cnt=%d, msleep=%d", tm_cnt, tags[tm_cnt]);
                yf_msleep(tags[tm_cnt]);
        }

        if (++tm_cnt >= YF_ARRAY_SIZE(tv_ms))
        {
                yf_evt_driver_stop(_evt_driver);
        }
}

yf_fd_event_t *_read_evt = NULL;

void  on_fd_evt(yf_fd_event_t* evt)
{
        yf_int_t  ret;
        if (evt->timeout)
        {
                yf_log_error(YF_LOG_DEBUG, evt->log, 0, 
                                "fd wait data timeout, fd=%d", evt->fd);
        }
        else {
                yf_channel_t  ch;
                ret = yf_read_channel(evt->fd, &ch, evt->log);
                if (ret == YF_ERROR)
                {
                        yf_log_error(YF_LOG_WARN, evt->log, 0, 
                                        "fd read failed, fd=%d, ret=%d", evt->fd, ret);
                }
                else if (ret == YF_AGAIN)
                {
                        yf_log_error(YF_LOG_WARN, evt->log, 0, 
                                        "fd read need again, fd=%d", evt->fd);
                        evt->ready = 0;
                }
                else {
                        if (ch.command == YF_CMD_DATA)
                        {
                                yf_log_debug1(YF_LOG_DEBUG, evt->log, 0, "recv cmd data=[%s]", ch.data);
                        }
                        else
                                assert(0);
                }
        }

        yf_time_t  time_out = {::random() % 120,  ::random() % 1000};
        ret = yf_register_fd_evt(_read_evt, &time_out);
        ASSERT_EQ(ret, YF_OK);
}

void  on_default_evt(yf_fd_event_t* evt)
{
}


void  start_tm_test(void *data, yf_log_t *log)
{
        yf_int_t ret;
        yf_fd_event_t *write_evt;
        
        //child process
        if (data)
        {
                yf_int_t ret = yf_alloc_fd_evt(_evt_driver, yf_channel, 
                                &_read_evt, &write_evt, log);
                ASSERT_EQ(ret, YF_OK);

                _read_evt->fd_evt_handler = on_fd_evt;

                yf_time_t  time_out = {::random() % 120,  ::random() % 1000};
                ret = yf_register_fd_evt(_read_evt, &time_out);
                ASSERT_EQ(ret, YF_OK);
        }
        else {
                yf_fd_t  tmp_channel[2];
                socketpair(AF_LOCAL, SOCK_STREAM, 0, tmp_channel);
                yf_int_t ret = yf_alloc_fd_evt(_evt_driver, tmp_channel[0], 
                                &_read_evt, &write_evt, log);
                ASSERT_EQ(ret, YF_OK);
                _read_evt->fd_evt_handler = on_default_evt;
                ret = yf_register_fd_evt(_read_evt, NULL);
                ASSERT_EQ(ret, YF_OK);
        }
        
        //init tm driver
        tm_cnt = 0;
        yf_memzero(tags, sizeof(tags));
        int random_num = ::random() % 19 + 7;
        for (int i = 0; i < random_num; ++i)
        {
                tags[::random() % YF_ARRAY_SIZE(tags)] = ::random() % 2899;
        }

        yf_tm_evt_t *tm_evt = NULL;
        yf_time_t utime;
        for (size_t i = 0; i < YF_ARRAY_SIZE(tv_ms); ++i)
        {
                ret = yf_alloc_tm_evt(_evt_driver, &tm_evt, _log);
                ASSERT_EQ(ret, YF_OK);

                tm_evt->data = (void *)(tv_ms + i);
                tm_evt->timeout_handler = on_timeout;

                tv_ms[i] = ::random() % 392144;
                yf_ms_2_time(tv_ms[i], &utime);
                ret = yf_register_tm_evt(tm_evt, &utime);
                ASSERT_EQ(ret, YF_OK);

                tm_evts[i] = tm_evt;
        }

        yf_evt_driver_start(_evt_driver);

        if (_read_evt)
                yf_free_fd_evt(_read_evt, write_evt);

        exit(0);
}


class DriverTestor : public testing::Test
{
public:
        virtual ~DriverTestor()
        {
        }
        
        virtual void SetUp()
        {
                srandom(time(NULL));
                yf_pid = getpid();

                yf_cpuinfo();

                yf_init_threads(36, 1024 * 1024, 1, NULL);

                //test if proc delay by file log...
                yf_log_file_init(NULL);
                yf_log_file_init_ctx_t log_file_init = {1024*128, 1024*1024*64, 8, 
                                NULL, "%t [%f:%l]-[%v]-[%r]"};

                _log = yf_log_open(YF_LOG_DEBUG, 8192, (void*)&log_file_init);
                assert(_log);

                //_log = yf_log_open(YF_LOG_DEBUG, 8192, NULL);
                _mem_pool = yf_create_pool(1024000, _log);

                yf_init_time(_log);
                yf_update_time(NULL, NULL, _log);

                yf_int_t ret = yf_strerror_init();
                assert(ret == YF_OK);
                ret = yf_init_setproctitle(_log);
                assert(ret == YF_OK);
                ret = yf_init_processs(_log);
                assert(ret == YF_OK);

                //init evt driver
                yf_evt_driver_init_t evt_driver_init;
                yf_memzero_st(evt_driver_init);
                evt_driver_init.log = _log;
                evt_driver_init.nfds = 1024;
                evt_driver_init.nstimers = 4296;

                //evt_driver_init.poll_type = YF_POLL_BY_DETECT;
                evt_driver_init.poll_type = ::random() % (YF_POLL_BY_EPOLL + 1);
                evt_driver_init.poll_cb = on_poll;
                
                _evt_driver = yf_evt_driver_create(&evt_driver_init);
                assert(_evt_driver);                
        }
        
        virtual void TearDown()
        {
                yf_destroy_pool(_mem_pool);
        }
};


TEST_F(DriverTestor, Timer)
{
        start_tm_test(NULL, _log);
}

TEST_F(DriverTestor, TmFd)
{
        yf_pid_t pid = yf_spawn_process(start_tm_test, (void*)-1, "tm_test_child", 
                        YF_PROC_CHILD, NULL, _log);

        for (int i = 0; true; ++i)
        {
                yf_msleep(::random() % 1500 + 100);
                
                int  status;
                yf_pid_t wait_pid = waitpid(-1, &status, WNOHANG);
                if (wait_pid == pid)
                {
                        yf_log_debug0(YF_LOG_DEBUG, _log, 0, "child process exit");
                        break;
                }

                if (i % 20 == 0)
                {
                        yf_channel_t  channel = {0};
                        channel.command = YF_CMD_DATA;
                        
                        yf_sprintf(channel.data, "cmd_data_%L", ::random());
                        yf_log_debug1(YF_LOG_DEBUG, _log, 0, "send cmd data=%s", channel.data);

                        //send twice
                        yf_write_channel(yf_processes[0].channel[0], &channel, _log);
                        yf_write_channel(yf_processes[0].channel[0], &channel, _log);
                }
        }
}



#ifdef TEST_F_INIT
TEST_F_INIT(DriverTestor, Timer);
TEST_F_INIT(DriverTestor, TmFd);
#endif

int main(int argc, char **argv)
{
        printf("max_pri is:%d,min_pri:%d\n ",sched_get_priority_max(2),sched_get_priority_min(2));
        struct sched_param para;
        
        int maxpri = sched_get_priority_max(SCHED_RR);
        para.sched_priority = maxpri;
        int ret = 0;//sched_setscheduler(getpid(), SCHED_FIFO, &para);

        //printf("ret=%d, Parent pid is %d policy is %d, prio: %d\n",ret,
        //        getpid(),sched_getscheduler(getpid()),para.sched_priority);	
	
        const char* sleep_test_timeout = getenv("sleep_test_timeout");
        if (sleep_test_timeout)
                _sleep_test_timeout = atoi(sleep_test_timeout);

        ret = yf_save_argv(_log, argc, argv);
        assert(ret == YF_OK);

        yf_init_bit_indexs();
        
        testing::InitGoogleTest(&argc, (char **)argv);
        return RUN_ALL_TESTS();
}
