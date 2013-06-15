#include <gtest/gtest.h>
#include <list>

extern "C" {
#include <ppc/yf_header.h>
#include <base_struct/yf_core.h>
#include <mio_driver/yf_event.h>
}


yf_pool_t *_mem_pool;
yf_log_t* _log;

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
        yf_close_channel(proc->channel, _log);
        ++exit_cnt;
}

struct _SpawnCtx
{
        const char* path;
        yf_u64_t data;
};

const char* _path = NULL;
yf_fd_t  _fd = 0;
void _on_exit()
{
        printf("on exit, pid=%d\n", yf_pid);
        yf_close_file(_fd);
        yf_delete_file(_path);
}

void _set_atexit(void *data, yf_log_t *log)
{
        yf_atexit_handler_add(_on_exit, log);
        
        _path = (const char*)((_SpawnCtx*)data)->path;
        _fd = yf_open_file(_path, YF_FILE_RDWR, 
                        YF_FILE_CREATE_OR_OPEN, 
                        YF_FILE_DEFAULT_ACCESS);
        assert(_fd >= 0);

        char buf[128] = {0};
        yf_snprintf(buf, sizeof(buf), "%d", getpid());
        yf_write(_fd, buf, strlen(buf));
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

void  child_thread_do(void *data, yf_log_t *log)
{
        _set_atexit(data, log);
        yf_tid_t tid = 0;
        yf_create_thread(&tid, (yf_thread_exe_pt)((_SpawnCtx*)data)->data, NULL, log);
        pthread_join(tid, NULL);
}

void  child_exit_kill(void *data, yf_log_t *log)
{
        _set_atexit(data, log);
        kill(getpid(), SIGQUIT);
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

yf_thread_value_t _thread_abort(void *arg)
{
        printf("_thread_abort\n");
        yf_sleep(5);
        printf("_thread_abort\n");
        fflush(stdout);
        abort();
}

yf_thread_value_t _thread_sig_bus(void *arg)
{
        yf_u32_t i1 = 0;
        ftruncate(_fd, 8192);
        yf_u64_t* addr = (yf_u64_t*)mmap(NULL, 8192, PROT_WRITE|PROT_READ, 
                        MAP_SHARED, _fd, 0);
        
        if (addr == MAP_FAILED) {
                printf("map failed, err=%d, str=%s\n", errno, strerror(errno));
                assert(0);
        }
        yf_u64_t  tmp = 0;
        for ( i1 = 0; i1 < 1024 ; i1++ )
        {
                tmp += addr[i1];
                yf_msleep(20);
                if ((i1 & 31) == 0)
                        printf("cacu to %d\n", i1);
        }
        
        printf("_thread_sig_bus end now, last cacu=%lld\n", tmp);
}


TEST_F(SigTestor, AtExit)
{
        yf_set_sig_handler(SIGCHLD, on_child_process_exit, _log);
        sigset_t set;

        sigemptyset(&set);
        sigaddset(&set, SIGCHLD);
        sigprocmask(SIG_BLOCK, &set, NULL);

        _SpawnCtx pathdatas[] = {
                                {"dir/1", (yf_u64_t)_thread_float}, {"dir/2", (yf_u64_t)_thread_seg}, 
                                {"dir/3", 0}, {"dir/4", 0}, 
                                {"dir/5", SIGPIPE}, {"dir/6", SIGSEGV}, 
                                {"dir/7", (yf_u64_t)_thread_abort},
                                {"dir/8", (yf_u64_t)_thread_sig_bus}
                                };
        
        yf_spawn_proc_pt  child_procs[] = {
                        child_thread_do, child_thread_do, 
                        child_exit_kill, child_sleep, 
                        child_thread_kill, child_thread_kill,
                        child_thread_do, child_thread_do
                        };
        
        yf_pid_t pids[YF_ARRAY_SIZE(pathdatas)] = {0};

        for (int i = 0; i < YF_ARRAY_SIZE(pathdatas); ++i)
        {
                pids[i] = yf_spawn_process(child_procs[i],
                               (void*)(pathdatas+i), "child_thread_do", YF_PROC_CHILD, 
                               on_child_exit_cb, _log);
        }
        
        sigemptyset(&set);
        while (exit_cnt < YF_ARRAY_SIZE(pathdatas))
        {
                sigsuspend(&set);
                if (g_child_flag)
                {
                        yf_process_get_status(_log);
                        g_child_flag = 0;
                        
                        yf_log_debug1(YF_LOG_DEBUG, _log, 0, "exit child process = %d", exit_cnt);
                        switch (exit_cnt)
                        {
                                case 3:
                                        kill(pids[3], SIGBUS);
                                        break;
                                case YF_ARRAY_SIZE(pathdatas) - 1:
                                        ASSERT_EQ(truncate(pathdatas[7].path, 1024), 0);
                                        printf("dir8 mmap file truncated...\n");
                                        break;
                        }
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

        _log = yf_log_open(YF_LOG_DEBUG, 8192, NULL);
        _mem_pool = yf_create_pool(1024000, _log);

        yf_init_bit_indexs();

        yf_init_time(_log);
        yf_update_time(NULL, NULL, _log);

        yf_int_t ret = yf_strerror_init();
        assert(ret == YF_OK);

        ret = yf_save_argv(_log, argc, argv);
        assert(ret == YF_OK);
        for (int i = 0; yf_os_environ[i]; ++i)
        {
                printf("os_env{ %s }\n", yf_os_environ[i]);
        }

        ret = yf_init_setproctitle(_log);
        assert(ret == YF_OK);

        ret = yf_init_processs(_log);
        assert(ret == YF_OK);

        ret = yf_init_threads(36, 1024 * 1024, 1, _log);
        assert(ret == YF_OK);

        testing::InitGoogleTest(&argc, (char **)argv);
        ret = RUN_ALL_TESTS();

        yf_destroy_pool(_mem_pool);

        return ret;
}



