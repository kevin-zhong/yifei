#include <gtest/gtest.h>
#include <list>

extern "C" {
#include <ppc/yf_header.h>
#include <base_struct/yf_core.h>
#include <mio_driver/yf_event.h>
}

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

void  signal_mask(yf_int_t unset = 0)
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

        sigprocmask(unset ? SIG_UNBLOCK : SIG_BLOCK, &set, NULL);
}

void  erase_sigal_mask()
{
        signal_mask(1);
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


yf_signal_t child_signals[] = {
        { yf_signal_value(YF_RECONFIGURE_SIGNAL),
          "SIG" yf_value(YF_RECONFIGURE_SIGNAL), "reload",
          NULL},
        { yf_signal_value(YF_NOACCEPT_SIGNAL),
          "SIG" yf_value(YF_NOACCEPT_SIGNAL),"stop_accept",
          NULL},
        { yf_signal_value(YF_TERMINATE_SIGNAL),
          "SIG" yf_value(YF_TERMINATE_SIGNAL), "stop",
          NULL},
        { yf_signal_value(YF_SHUTDOWN_SIGNAL),
          "SIG" yf_value(YF_SHUTDOWN_SIGNAL),"quit",
          NULL},
        { yf_signal_value(YF_CHANGEBIN_SIGNAL),
          "SIG" yf_value(YF_CHANGEBIN_SIGNAL),"",
          NULL},
        { SIGALRM,   "SIGALRM",           "", NULL},
        { SIGSYS,      "SIGSYS, SIG_IGN",   "", NULL},
        { SIGPIPE,     "SIGPIPE, SIG_IGN",  "", NULL},
        { 0,              NULL,                "", NULL}
};

void on_signal(yf_sig_event_t* sig_evt)
{
        printf("signo=%d\t", sig_evt->signo);
        uint32_t i1 = 0;
        for ( i1 = 0; i1 < YF_ARRAY_SIZE(child_signals); i1++ )
        {
                if (child_signals[i1].signo == sig_evt->signo)
                {
                        printf("singnal=(%s) happend", child_signals[i1].name);
                        break;
                }
        }
        printf("\n");
        
        if (sig_evt->signo == yf_signal_value(YF_SHUTDOWN_SIGNAL))
                exit(0);
}


void  empty_child_proc(void *data, yf_log_t *log)
{
        //必须解除 继承得到的 signal mask，否则，将收不到信号
        erase_sigal_mask();
        
        yf_setproctitle("yf_proc_testor empty", log);

        yf_evt_driver_init_t driver_init = {0, 128, 16, log, YF_DEFAULT_DRIVER_CB};
        yf_evt_driver_t * driver = yf_evt_driver_create(&driver_init);

        yf_sig_event_t  sig_evt = {0, data, log, NULL, on_signal};

        for (yf_signal_t* sig = child_signals; sig->signo; ++sig)
        {
                sig_evt.signo = sig->signo;
                ASSERT_EQ(YF_OK, yf_register_singal_evt(driver, &sig_evt, log));
        }
        yf_evt_driver_start(driver);
}


void  test_channel_func(void *data, yf_log_t *log)
{
        yf_channel_t  ch;
        yf_blocking(yf_channel);

        yf_setproctitle("yf_proc_testor channel", log);
        
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
                        
                        //dont use ASSERT_EQ in child proc, else if fail, will run parent's follow exes...
                        //ASSERT_EQ(ret, YF_OK);
                        assert(ret == YF_OK);
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
                        assert(ret == YF_OK);
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
        printf("child=%d exit, chnnel=[%d-%d]\n", proc->pid, 
                        proc->channel[0], 
                        proc->channel[1]);
        
        proc->pid = -1;
        yf_close_channel(proc->channel, &_log);
        ++exit_cnt;
}


TEST_F(ProcTestor, ShareMem)
{
        exit_cnt = 0;

        yf_set_sig_handler(SIGCHLD, on_child_process_exit, &_log);
        yf_set_sig_handler(SIGIO, SIG_IGN, &_log);
        signal_mask();
        
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

        printf("last cnt=%d, pid=%d\n", shm_st->cnt, getpid());
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

        //send file fd to child proc
        yf_memzero(&channel, sizeof(channel));
        channel.command = YF_CMD_SEND_FD;
        yf_fd_t fd_send = yf_open_file("dir/test_fd_send", YF_FILE_APPEND, 
                        YF_FILE_CREATE_OR_OPEN, 
                        YF_FILE_DEFAULT_ACCESS);
        channel.fd = fd_send;
        assert(fd_send >= 0);
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

        yf_close_file(fd_send);
        yf_lock_destory(&shm_st->lock);
        yf_log_close(proc_log);
        yf_shm_free(&shm);
}


/*
* test proc event(like popen)
*/
yf_evt_driver_t * g_proc_driver = NULL;

void on_exe_callback(yf_process_t* proc);

char* g_normal_shell = "normal_shell";
char* g_cat_shell = "cat_shell";
char* g_py_shell = "py_shell";
char* g_unexsit_exe = "unexsit_exe";
char* g_timeout_shell = "timeout_shell";
char* g_silent_shell = "silent_shell";
char* g_killed_shell = "killed_shell";

//1，命令字符串前后不能有空格，否则会出现 "permission denied(13) !!"
//2，如果有参数，参数肯定需要大于1，因为第一个参数实际上是执行文件名字
char* const normal_argv[] = {"cat", "./exe_ctx/un_xx_nu.txt", NULL};
char* const cat_argv[] = {"cat", "./exe_ctx/cat_target.txt", NULL};
char* const uname_argv[] = {"uname", "-a", NULL};
char* const sleep_argv[] = {"sleep", "10", NULL};

yf_exec_ctx_t exec_ctx[] = { 
        {normal_argv[0], g_normal_shell, normal_argv, NULL, 
                NULL, NULL, YF_PROC_POPEN_R}, 
        {cat_argv[0], g_cat_shell, cat_argv, NULL, 
                NULL, NULL, YF_PROC_POPEN_R}, 
        {"./exe_ctx/echo.py", g_py_shell, NULL, NULL, 
                NULL, NULL, YF_PROC_POPEN_R}, 
        {sleep_argv[0], g_timeout_shell, sleep_argv, NULL, 
                NULL, NULL, YF_PROC_POPEN_R}, 
        {uname_argv[0], g_silent_shell, uname_argv, NULL, 
                NULL, NULL, YF_PROC_DETACH}, 
        {sleep_argv[0], g_killed_shell, sleep_argv, NULL, 
                NULL, NULL, YF_PROC_DETACH},
        {"./exe_ctx/test_unexsit", g_unexsit_exe, NULL, NULL, 
                NULL, NULL, YF_PROC_POPEN_R}
};

void on_exe_callback(yf_processor_event_t* proc_evt)
{
        yf_exec_ctx_t* exe_ctx = &proc_evt->exec_ctx;
        
        if (exe_ctx->name == g_normal_shell)
        {
                assert(proc_evt->error == 0 && proc_evt->exit_code != 0);
        }
        else if (exe_ctx->name == g_cat_shell
                || exe_ctx->name == g_silent_shell
                || exe_ctx->name == g_py_shell)
        {
                assert(proc_evt->error == 0 && proc_evt->exit_code == 0);
                
                if (exe_ctx->type == YF_PROC_POPEN_R)
                {
                        printf("--------------------------output exe stdout %s !!\n", 
                                        exe_ctx->name);
                        char buf_tmp[102400];
                        for (yf_chain_t* cl = proc_evt->read_chain; cl; cl = cl->next)
                        {
                                printf("[bufsize=%d]", yf_buf_size(cl->buf));
                                yf_memcpy(buf_tmp, cl->buf->pos, yf_buf_size(cl->buf));
                                buf_tmp[yf_buf_size(cl->buf)] = 0;

                                printf(buf_tmp);
                                printf("[bufsize=%d done]", yf_buf_size(cl->buf));
                        }
                        printf("--------------------------\n\n");
                }
        }
        else if (exe_ctx->name == g_timeout_shell)
        {
                assert(proc_evt->timeout);
        }
        else if (exe_ctx->name == g_killed_shell)
        {
                assert(proc_evt->error);
        }
        else if (exe_ctx->name == g_unexsit_exe)
        {
                assert(proc_evt->error == 0 && proc_evt->exit_code == 2);
        }

        if (++exit_cnt == YF_ARRAY_SIZE(exec_ctx))
                yf_evt_driver_stop(g_proc_driver);
}


void on_exe_exit_signal(yf_sig_event_t* sig_evt)
{
        yf_process_get_status(sig_evt->log);
}


TEST_F(ProcTestor, ProcEvt)
{
        exit_cnt = 0;
        erase_sigal_mask();
        yf_set_sig_handler(SIGIO, SIG_IGN, &_log);

        yf_evt_driver_init_t driver_init = {0, 128, 16, &_log, YF_DEFAULT_DRIVER_CB};
        g_proc_driver = yf_evt_driver_create(&driver_init);

        yf_sig_event_t  sig_child_evt = {SIGCHLD, NULL, NULL, NULL, on_exe_exit_signal};
        ASSERT_EQ(YF_OK, yf_register_singal_evt(g_proc_driver, &sig_child_evt, &_log));

        yf_time_t  time_out = {6, 0};

        for (yf_int_t i = 0; i < YF_ARRAY_SIZE(exec_ctx); ++i)
        {
                yf_exec_ctx_t* tex_ctx = exec_ctx + i;
                
                yf_processor_event_t* proc_evt;
                yf_int_t ret = yf_alloc_proc_evt(g_proc_driver, &proc_evt, &_log);
                
                proc_evt->pool = _mem_pool;
                proc_evt->exec_ctx = *tex_ctx;
                proc_evt->ret_handler = on_exe_callback;
                
                ret = yf_register_proc_evt(proc_evt, &time_out);
                ASSERT_EQ(ret, YF_OK);

                if (tex_ctx->name == g_killed_shell)
                {
                        yf_process_t* proc = yf_get_proc_by_evt(proc_evt);
                        kill(proc->pid, yf_signal_value(YF_KILL_SIGNAL));
                }
        }

        yf_evt_driver_start(g_proc_driver);
        yf_evt_driver_destory(g_proc_driver);
}


#ifdef TEST_F_INIT
TEST_F_INIT(ProcTestor, ShareMem);
TEST_F_INIT(ProcTestor, ProcEvt);
#endif

int main(int argc, char **argv)
{
        srandom(time(NULL));
        yf_pagesize = getpagesize();
        printf("pagesize=%d\n", yf_pagesize);

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
        for (int i = 0; yf_os_environ[i]; ++i)
        {
                printf("os_env{ %s }\n", yf_os_environ[i]);
        }

        ret = yf_init_setproctitle(&_log);
        assert(ret == YF_OK);

        ret = yf_init_processs(&_log);
        assert(ret == YF_OK);

        ret = yf_init_threads(36, 1024 * 1024, 1, &_log);
        assert(ret == YF_OK);

        testing::InitGoogleTest(&argc, (char **)argv);
        ret = RUN_ALL_TESTS();

        yf_destroy_pool(_mem_pool);

        return ret;
}
