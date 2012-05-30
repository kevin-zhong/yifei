#include <gtest/gtest.h>
#include <list>

extern "C" {
#include <ppc/yf_header.h>
#include <base_struct/yf_core.h>
#include <mio_driver/yf_event.h>
}

sig_atomic_t g_reload_flag;
sig_atomic_t g_noaccept_flag;
sig_atomic_t g_terminate_flag;
sig_atomic_t g_shutdown_flag;
sig_atomic_t g_chagebin_flag;
sig_atomic_t g_alarm_flag;
sig_atomic_t g_child_flag;

yf_signal_t signals[] = {
        { yf_signal_value(YF_RECONFIGURE_SIGNAL),
          "SIG" yf_value(YF_RECONFIGURE_SIGNAL),
          "reload",
          &g_reload_flag },

        { yf_signal_value(YF_NOACCEPT_SIGNAL),
          "SIG" yf_value(YF_NOACCEPT_SIGNAL),
          "stop_accept",
          &g_noaccept_flag },

        { yf_signal_value(YF_TERMINATE_SIGNAL),
          "SIG" yf_value(YF_TERMINATE_SIGNAL),
          "stop",
          &g_terminate_flag },

        { yf_signal_value(YF_SHUTDOWN_SIGNAL),
          "SIG" yf_value(YF_SHUTDOWN_SIGNAL),
          "quit",
          &g_shutdown_flag },

        { yf_signal_value(YF_CHANGEBIN_SIGNAL),
          "SIG" yf_value(YF_CHANGEBIN_SIGNAL),
          "",
          &g_chagebin_flag },

        { SIGALRM,                               "SIGALRM",           "", &g_alarm_flag },

        { SIGCHLD,                               "SIGCHLD",           "", &g_child_flag },

        { SIGSYS,                                "SIGSYS, SIG_IGN",   "", NULL         },

        { SIGPIPE,                               "SIGPIPE, SIG_IGN",  "", NULL         },

        { 0,                                     NULL,                "", NULL         }
};


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

void  empty_child_proc(void *data, yf_log_t *log)
{
        erase_sigal_mask();
        
        yf_setproctitle("yf_proc_testor empty", log);
        
        for (int cnt = 0; true; ++cnt)
        {
                usleep(300000);
                yf_log_debug2(YF_LOG_DEBUG, log, 0, "sleep_%d _ %d", yf_process_slot, cnt);
                
                if (g_reload_flag)
                {
                        yf_log_debug0(YF_LOG_DEBUG, log, 0, "need reload");
                        g_reload_flag = 0;
                }
                else if (g_noaccept_flag)
                {
                        yf_log_debug0(YF_LOG_DEBUG, log, 0, "need stop accept");
                        g_noaccept_flag = 0;                        
                }
                else if (g_shutdown_flag)
                {
                        yf_log_debug0(YF_LOG_DEBUG, log, 0, "need shutdown");
                        g_shutdown_flag = 0;
                        exit(0);
                }                
        }
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
                               NULL, "test_channel_func", YF_PROCESS_NORESPAWN, proc_log);
        yf_pid_t pid = yf_spawn_process(test_channel_func,
                               NULL, "test_channel_func", YF_PROCESS_NORESPAWN, proc_log);

        yf_pid_t signal_pid = yf_spawn_process(empty_child_proc,
                               NULL, "empty_child_proc", YF_PROCESS_NORESPAWN, proc_log);
        ASSERT_TRUE(signal_pid != YF_INVALID_PID);        
        
        for (size_t i = 0; i < YF_ARRAY_SIZE(cnt); ++i)
        {
                test_ctx.try_times = cnt[i];
                test_ctx.is_add = flag[i];

                pid = yf_spawn_process(child_process,
                                       &test_ctx, "child_process", YF_PROCESS_NORESPAWN, proc_log);

                ASSERT_TRUE(pid != YF_INVALID_PID);
        } 

        int exit_cnt = 0;

        signal_mask();

        sigset_t set;
        sigemptyset(&set);
        
        while (exit_cnt < YF_ARRAY_SIZE(cnt))
        {
                sigsuspend(&set);
                if (g_child_flag)
                {
                        yf_process_get_status();
                        g_child_flag = 0;
                        exit_cnt++;
                        yf_log_debug1(YF_LOG_DEBUG, &_log, 0, "exit child process = %d", exit_cnt);
                }
        }

        printf("last cnt=%d\n", shm_st->cnt);
        ASSERT_EQ(shm_st->cnt, (cnt[0] + cnt[1] - cnt[2] - cnt[3]));

        //send signal to empty child proc
        yf_os_signal_process("reload", signal_pid, &_log);
        yf_os_signal_process("stop_accept", signal_pid, &_log);
        yf_os_signal_process("quit", signal_pid, &_log);

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
        //channel �����ԣ�����һ��socket��һ�˷��͸������proc�󣬾ͻ��γɣ���������ߺ�һ������
        //�ߵ����ӣ�ֻ�е�������Щproc�ر���˺������Ƕν����߲����յ�recv=0�ı�־(�����ӶϿ�)
        usleep(100000);

        yf_memzero(&channel, sizeof(channel));
        channel.command = YF_CMD_QUIT;
        ret = yf_write_channel(yf_processes[0].channel[0], &channel, &_log);
        ASSERT_EQ(ret, YF_OK);

        exit_cnt = 0;
        while (exit_cnt < 3)
        {
                sigsuspend(&set);
                if (g_child_flag)//bug here, may lost, cause g_child_flag may set many times .. TODO
                {
                        yf_process_get_status();//should return exited num
                        g_child_flag = 0;
                        exit_cnt++;
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

        //init singnals
        ret = yf_init_signals(&_log);

        testing::InitGoogleTest(&argc, (char **)argv);
        ret = RUN_ALL_TESTS();

        yf_destroy_pool(_mem_pool);

        return ret;
}
