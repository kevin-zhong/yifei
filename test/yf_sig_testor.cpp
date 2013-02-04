#include <gtest/gtest.h>
#include <list>

extern "C" {
#include <ppc/yf_header.h>
#include <base_struct/yf_core.h>
#include <mio_driver/yf_event.h>
}


yf_pool_t *_mem_pool;
yf_log_t _log;

class SigTestor : public testing::Test
{
};

sig_atomic_t g_child_flag = 0;
int exit_cnt = 0;

void on_child_process_exit(yf_int_t signo)
{
        g_child_flag = 1;
}

void on_child_exit_cb(struct  yf_process_s* proc)
{
        printf("child=%d exit, chnnel=[%d-%d]\n", proc->pid, 
                        proc->channel[0], 
                        proc->channel[1]);
        
        proc->pid = -1;
        yf_close_channel(proc->channel, &_log);
        ++exit_cnt;
}

struct _SpawnCtx
{
        const char* path;
        int data;
};

const char* _path = NULL;
void _on_exit()
{
        printf("on exit, pid=%d\n", yf_pid);
        yf_delete_file(_path);
}

void _set_atexit(void *data, yf_log_t *log)
{
        yf_atexit_handler_add(_on_exit, log);
        
        _path = (const char*)((_SpawnCtx*)data)->path;
        yf_fd_t fd = yf_open_file(_path, YF_FILE_APPEND, 
                        YF_FILE_CREATE_OR_OPEN, 
                        YF_FILE_DEFAULT_ACCESS);

        char buf[128] = {0};
        yf_snprintf(buf, sizeof(buf), "%d", getpid());
        yf_write_fd(fd, buf, strlen(buf));
        yf_close_file(fd);
}

yf_thread_value_t _thread_float(void *arg)
{
        yf_sleep(2);
        int i = 1;
        --i;
        printf("res=%d\n", 10 / i);
        return  NULL;
}
yf_thread_value_t _thread_seg(void *arg)
{
        yf_sleep(3);
        yf_int_t  *ptr_aa = NULL;
        *(ptr_aa+100) = 1000;
        return  NULL;
}

void  child_exit_float(void *data, yf_log_t *log)
{
        _set_atexit(data, log);
        yf_tid_t tid = 0;
        yf_create_thread(&tid, _thread_float, NULL, log);
        yf_sleep(5);
}
void  child_exit_kill(void *data, yf_log_t *log)
{
        _set_atexit(data, log);
        kill(getpid(), SIGQUIT);
}
void  child_exit_thread(void *data, yf_log_t *log)
{
        _set_atexit(data, log);
        yf_tid_t tid = 0;
        yf_create_thread(&tid, _thread_seg, NULL, log);
        pthread_join(tid, NULL);
}
void child_sleep(void *data, yf_log_t *log)
{
        _set_atexit(data, log);
        yf_sleep(10000);
}

yf_thread_value_t _thread_sleep(void *arg)
{
        yf_sleep(10);
}
void child_thread_kill(void *data, yf_log_t *log)
{
        _set_atexit(data, log);
        yf_tid_t tid = 0;
        yf_create_thread(&tid, _thread_sleep, NULL, log);
        yf_sleep(3);
        pthread_kill(tid, ((_SpawnCtx*)data)->data);
        yf_sleep(3);
}

TEST_F(SigTestor, AtExit)
{
        yf_set_sig_handler(SIGCHLD, on_child_process_exit, &_log);
        sigset_t set;

        sigemptyset(&set);
        sigaddset(&set, SIGCHLD);
        sigprocmask(SIG_BLOCK, &set, NULL);

        _SpawnCtx pathdatas[] = {{"dir/1", 0}, {"dir/2", 0}, {"dir/3", 0}, {"dir/4", 0}, 
                        {"dir/5", SIGPIPE}, {"dir/6", SIGSEGV}};
        yf_spawn_proc_pt  child_procs[] = {child_exit_float, child_exit_kill, 
                        child_exit_thread, child_sleep, child_thread_kill};
        
        yf_pid_t pids[YF_ARRAY_SIZE(pathdatas)] = {0};

        for (int i = 0; i < YF_ARRAY_SIZE(pathdatas); ++i)
        {
                pids[i] = yf_spawn_process(child_procs[i],
                               (void*)(pathdatas+i), "child_exit_float", YF_PROC_CHILD, 
                               on_child_exit_cb, &_log);
        }
        
        sigemptyset(&set);
        while (exit_cnt < YF_ARRAY_SIZE(pathdatas))
        {
                sigsuspend(&set);
                if (g_child_flag)
                {
                        yf_process_get_status(&_log);
                        g_child_flag = 0;
                        yf_log_debug1(YF_LOG_DEBUG, &_log, 0, "exit child process = %d", exit_cnt);
                        if (exit_cnt == 3)
                                kill(pids[3], SIGBUS);
                }
        }

        for (int i = 0; i < YF_ARRAY_SIZE(pathdatas); ++i)
        {
                ASSERT_EQ(-1, access(pathdatas[i].path, F_OK));
        }
}


#ifdef TEST_F_INIT
TEST_F_INIT(SigTestor, AtExit);
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



