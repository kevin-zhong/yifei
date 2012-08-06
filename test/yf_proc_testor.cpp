#include <gtest/gtest.h>
#include <list>

extern "C" {
#include <ppc/yf_header.h>
#include <base_struct/yf_core.h>
#include <mio_driver/yf_event.h>
}


yf_pid_t yf_pid;
yf_pool_t *_mem_pool;
yf_log_t _log;

class ProcTestor : public testing::Test
{
};

struct  ShmSt
{
        yf_lock_t lock;
        int       cnt;
};

struct  TestCtx
{
        ShmSt *test_ma;
        int    try_times;
        bool   is_add;
};

void  erase_sigal_mask()
{
        sigset_t set;
        sigemptyset(&set);
        sigprocmask(SIG_BLOCK, &set, NULL);
}

void  signal_mask()
{
        sigset_t set;

        sigemptyset(&set);
        sigaddset(&set, SIGCHLD);
        sigaddset(&set, SIGALRM);
        sigaddset(&set, SIGIO);
        sigaddset(&set, SIGINT);
        sigaddset(&set, yf_signal_value(YF_RECONFIGURE_SIGNAL));
        sigaddset(&set, yf_signal_value(YF_NOACCEPT_SIGNAL));
        sigaddset(&set, yf_signal_value(YF_TERMINATE_SIGNAL));
        sigaddset(&set, yf_signal_value(YF_SHUTDOWN_SIGNAL));
        sigaddset(&set, yf_signal_value(YF_CHANGEBIN_SIGNAL));

        sigprocmask(SIG_BLOCK, &set, NULL);        
}

void  child_process(void *data, yf_log_t *log)
{
        printf("child process_%d, pid=%d\n", yf_process_slot, getpid());
        erase_sigal_mask();
        
        char  title[512];
        sprintf(title, "yf_proc_testor_child_%d", yf_process_slot);
        yf_setproctitle(title, log);
        
        TestCtx *test_ctxt = (TestCtx *)data;

        ShmSt *test_ma = test_ctxt->test_ma;
        int try_times = test_ctxt->try_times;

        for (int i = 0; i < try_times; ++i)
        {
                if (i % 1000 == 0)
                        yf_log_debug2(YF_LOG_DEBUG, log, 0, "process_%d _ %d", yf_process_slot, i);

                yf_lock(&test_ma->lock);

                if (test_ctxt->is_add)
                        test_ma->cnt++;
                else
                        test_ma->cnt--;

                yf_unlock(&test_ma->lock);
        }

        printf("child process_%d end, pid=%d\n", yf_process_slot, getpid());
        exit(0);
}


void on_signal(yf_int_t signo);

yf_signal_t child_signals[] = {
        { yf_signal_value(YF_RECONFIGURE_SIGNAL),
          "SIG" yf_value(YF_RECONFIGURE_SIGNAL), "reload",
          on_signal},
        { yf_signal_value(YF_NOACCEPT_SIGNAL),
          "SIG" yf_value(YF_NOACCEPT_SIGNAL),"stop_accept",
          on_signal},
        { yf_signal_value(YF_TERMINATE_SIGNAL),
          "SIG" yf_value(YF_TERMINATE_SIGNAL), "stop",
          on_signal},
        { yf_signal_value(YF_SHUTDOWN_SIGNAL),
          "SIG" yf_value(YF_SHUTDOWN_SIGNAL),"quit",
          on_signal},
        { yf_signal_value(YF_CHANGEBIN_SIGNAL),
          "SIG" yf_value(YF_CHANGEBIN_SIGNAL),"",
          on_signal},
        { SIGALRM,   "SIGALRM",           "", on_signal},
        { SIGSYS,      "SIGSYS, SIG_IGN",   "", NULL},
        { SIGPIPE,     "SIGPIPE, SIG_IGN",  "", NULL},
        { 0,              NULL,                "", NULL}
};

void on_signal(yf_int_t signo)
{
        printf("signo=%d\t", signo);
        uint32_t i1 = 0;
        for ( i1 = 0; i1 < YF_ARRAY_SIZE(child_signals); i1++ )
        {
                if (child_signals[i1].signo == signo)
                {
                        printf("singnal=(%s) happend", child_signals[i1].name);
                        break;
                }
        }
        printf("\n");
        
        if (signo == yf_signal_value(YF_SHUTDOWN_SIGNAL))
                exit(0);
}


void  empty_child_proc(void *data, yf_log_t *log)
{
        yf_setproctitle("yf_proc_testor empty", log);

        yf_evt_driver_init_t driver_init = {0, 128, 16, 1, log, YF_DEFAULT_DRIVER_CB};
        yf_evt_driver_t * driver = yf_evt_driver_create(&driver_init);

        yf_register_singal_evts(driver, child_signals, log);
        yf_evt_driver_start(driver);
}


void  test_channel_func(void *data, yf_log_t *log)
{
        yf_channel_t  ch;
        yf_blocking(yf_channel);
        
        while(true)
        {
                yf_int_t  ret = yf_read_channel(yf_channel, &ch, log);
                if (ret == YF_ERROR)
                {
                        continue;
                }
                yf_log_debug1(YF_LOG_DEBUG, log, 0, "recv cmd=%d", ch.command);

                if (ch.command == YF_CMD_DATA)
                {
                        yf_log_debug1(YF_LOG_DEBUG, log, 0, "recv cmd data=[%s]", ch.data);
                }
                else if (ch.command == YF_CMD_QUIT)
                {
                        yf_log_debug3(YF_LOG_DEBUG, log, 0, "recv quit cmd, need broadcast t, "
                                        "slot=%d, fd=%d, other=%d"
                                        ,(yf_process_slot+1)
                                        , yf_processes[yf_process_slot+1].channel[0]
                                        , yf_processes[yf_process_slot+1].channel[1]);

                        yf_channel_t  channel = {0};
                        channel.command = YF_CMD_TERMINATE;
                        
                        ret = yf_write_channel(yf_processes[yf_process_slot+1].channel[0], 
                                        &channel, &_log);
                        ASSERT_EQ(ret, YF_OK);
                        exit(0);
                }
                else if (ch.command == YF_CMD_TERMINATE)
                {
                        yf_log_debug0(YF_LOG_DEBUG, log, 0, "recv terminate cmd");
                        yf_msleep(10);
                        exit(0);
                }
                else if (ch.command == YF_CMD_SEND_FD)
                {
                        char buf[256];
                        int cnt = sprintf(buf, "get file fd=%d, pid=%d\n", ch.fd, yf_pid);
                        yf_write_fd(ch.fd, buf, cnt);

                        yf_channel_t  channel = {0};
                        channel.command = YF_CMD_DATA;
                        sprintf(channel.data, "2233455566788888875");
                        ret = yf_write_channel(yf_processes[yf_process_slot-1].channel[0], &channel, &_log);
                        ASSERT_EQ(ret, YF_OK);
                }
                else
                        yf_update_channel(&ch, log);
        }
}


sig_atomic_t g_child_flag = 0;

void on_child_process_exit(yf_int_t signo)
{
        g_child_flag = 1;
}

int exit_cnt = 0;

void on_child_exit_cb(struct  yf_process_s* proc)
{
        ++exit_cnt;
}


TEST_F(ProcTestor, ShareMem)
{
        yf_shm_t shm;

        shm.size = 4096;
        yf_str_set(&shm.name, "test_shm");
        shm.log = &_log;

        yf_int_t ret = yf_shm_alloc(&shm);
        ASSERT_EQ(ret, YF_OK);

        int cnt[4] = { 6900000, 7500000, 6600000, 6215968 };
        bool flag[4] = { true, true, false, false };

        ShmSt *shm_st = (ShmSt *)shm.addr;
        shm_st->cnt = 0;
        yf_lock_init(&shm_st->lock);

        TestCtx test_ctx;
        test_ctx.test_ma = shm_st;

        yf_log_t *proc_log = (yf_log_t *)yf_align_ptr(shm.addr + sizeof(ShmSt), YF_ALIGNMENT);
        proc_log->max_log_size = 1024 * 1024 * 1024;
        ASSERT_EQ(yf_log_open("dir/proc.log", proc_log), proc_log);

        yf_pid_t active_pid = yf_spawn_process(test_channel_func,
                               NULL, "test_channel_func", YF_PROC_CHILD, 
                               on_child_exit_cb, proc_log);
        
        yf_pid_t pid = yf_spawn_process(test_channel_func,
                               NULL, "test_channel_func", YF_PROC_CHILD, 
                               on_child_exit_cb, proc_log);

        yf_pid_t signal_pid = yf_spawn_process(empty_child_proc,
                               NULL, "empty_child_proc", YF_PROC_CHILD, 
                               on_child_exit_cb, proc_log);
        ASSERT_TRUE(signal_pid != YF_INVALID_PID);
        
        for (size_t i = 0; i < YF_ARRAY_SIZE(cnt); ++i)
        {
                test_ctx.try_times = cnt[i];
                test_ctx.is_add = flag[i];

                pid = yf_spawn_process(child_process,
                                       &test_ctx, "child_process", YF_PROC_CHILD, 
                                       on_child_exit_cb, proc_log);

                ASSERT_TRUE(pid != YF_INVALID_PID);
        } 

        yf_set_sig_handler(SIGCHLD, on_child_process_exit, &_log);
        signal_mask();
        sigset_t set;
        sigemptyset(&set);
        
        while (exit_cnt < YF_ARRAY_SIZE(cnt))
        {
                sigsuspend(&set);
                if (g_child_flag)
                {
                        yf_process_get_status(&_log);
                        g_child_flag = 0;
                        yf_log_debug1(YF_LOG_DEBUG, &_log, 0, "exit child process = %d", exit_cnt);
                }
        }

        printf("last cnt=%d\n", shm_st->cnt);
        ASSERT_EQ(shm_st->cnt, (cnt[0] + cnt[1] - cnt[2] - cnt[3]));

        //send signal to empty child proc
        yf_os_signal_process(child_signals, "reload", signal_pid, &_log);
        yf_os_signal_process(child_signals, "stop_accept", signal_pid, &_log);
        yf_sleep(1);//must
        yf_os_signal_process(child_signals, "quit", signal_pid, &_log);

        //send cmd by channel to child proc, and old child -> new child
        yf_channel_t  channel = {0};
        
        channel.command = YF_CMD_DATA;
        sprintf(channel.data, "gfdgfds564fdaefafd");
        ret = yf_write_channel(yf_processes[0].channel[0], &channel, &_log);
        ASSERT_EQ(ret, YF_OK);

        yf_memzero(&channel, sizeof(channel));
        channel.command = YF_CMD_SEND_FD;
        channel.fd = yf_open_file("dir/test_fd_send", YF_FILE_APPEND, 
                        YF_FILE_CREATE_OR_OPEN, 
                        YF_FILE_DEFAULT_ACCESS);
        assert(channel.fd >= 0);
        ret = yf_write_channel(yf_processes[1].channel[0], &channel, &_log);
        ASSERT_EQ(ret, YF_OK);

        //close test 1's fds
        //close(yf_processes[1].channel[0]);
        //close(yf_processes[1].channel[1]);
        //channel 的特性，当把一对socket中一端发送给另外的proc后，就会形成，多个发送者和一个接受
        //者的连接；只有当所有这些proc关闭这端后，另外那段接受者才能收到recv=0的标志(即连接断开)
        usleep(100000);

        yf_memzero(&channel, sizeof(channel));
        channel.command = YF_CMD_QUIT;
        ret = yf_write_channel(yf_processes[0].channel[0], &channel, &_log);
        ASSERT_EQ(ret, YF_OK);

        exit_cnt = 0;
        while (exit_cnt < 3)
        {
                sigsuspend(&set);
                if (g_child_flag)
                {
                        yf_process_get_status(&_log);
                        g_child_flag = 0;
                        yf_log_debug1(YF_LOG_DEBUG, &_log, 0, "exit child process = %d", exit_cnt);
                }
        }

        yf_lock_destory(&shm_st->lock);
        yf_log_close(proc_log);
        yf_shm_free(&shm);
}


#ifdef TEST_F_INIT
TEST_F_INIT(ProcTestor, ShareMem);
#endif

int main(int argc, char **argv)
{
        srandom(time(NULL));
        yf_pid = getpid();

        yf_cpuinfo();

        _log.max_log_size = 8192;
        yf_log_open(NULL, &_log);
        _mem_pool = yf_create_pool(1024000, &_log);

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

        testing::InitGoogleTest(&argc, (char **)argv);
        ret = RUN_ALL_TESTS();

        yf_destroy_pool(_mem_pool);

        return ret;
}
