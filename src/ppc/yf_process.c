#include <ppc/yf_header.h>
#include <base_struct/yf_core.h>

static void yf_execute_proc(void *data, yf_log_t *log);
static void yf_pass_open_channel(yf_log_t *log);
static void yf_close_parent_channels(yf_log_t *log);

yf_int_t yf_process_slot;
yf_socket_t yf_channel;
yf_int_t yf_last_process;
yf_process_t yf_processes[YF_MAX_PROCESSES];

yf_int_t
yf_init_processs(yf_log_t *log)
{
        size_t i = 0;

        yf_process_slot = 0;
        yf_channel = 0;
        yf_last_process = 0;

        yf_memzero(yf_processes, sizeof(yf_processes));

        for (i = 0; i < YF_ARRAY_SIZE(yf_processes); ++i)
        {
                yf_processes[i].channel[0] = -1;
                yf_processes[i].channel[1] = -1;
                yf_processes[i].pid = -1;
        }
        return 0;
}


yf_pid_t
yf_spawn_process(yf_spawn_proc_pt proc
                 , void *data, char *name, yf_int_t respawn
                 , yf_proc_exit_pt  exit_cb
                 , yf_log_t *log)
{
        yf_pid_t pid;
        yf_int_t s;

        if (respawn >= 0)
        {
                s = respawn;
        }
        else {
                for (s = 0; s < yf_last_process; s++)
                {
                        if (yf_processes[s].pid == -1)
                        {
                                break;
                        }
                }

                if (s == YF_MAX_PROCESSES)
                {
                        yf_log_error(YF_LOG_ALERT, log, 0,
                                     "no more than %d processes can be spawned",
                                     YF_MAX_PROCESSES);
                        return YF_INVALID_PID;
                }
        }

        if (respawn != YF_PROC_DETACH)
        {
                if (yf_open_channel(yf_processes[s].channel, 1, respawn == YF_PROC_CHILD, 
                                1, log) != YF_OK)
                {
                        yf_log_error(YF_LOG_ALERT, log, 0, 
                                     "yf_open_channel() failed while spawning \"%s\"", name);
                        return YF_INVALID_PID;
                }
                yf_channel = yf_processes[s].channel[1];
        }
        else {
                yf_processes[s].channel[0] = -1;
                yf_processes[s].channel[1] = -1;
        }

        yf_process_slot = s;

        pid = fork();

        switch (pid)
        {
        case -1:
                yf_log_error(YF_LOG_ALERT, log, yf_errno,
                             "fork() failed while spawning \"%s\"", name);
                yf_close_channel(yf_processes[s].channel, log);
                return YF_INVALID_PID;

        case 0:
                if (yf_proc_readable(respawn))
                {
                        dup2(yf_processes[s].channel[1], STDOUT_FILENO);
                }
                if (yf_proc_writable(respawn))
                {
                        dup2(yf_processes[s].channel[1], STDIN_FILENO);
                }
                yf_pid = yf_getpid();
                yf_main_thread_id = yf_thread_self();
                yf_close_parent_channels(log);
                proc(data, log);
                break;

        default:
                break;
        }

        yf_log_error(YF_LOG_NOTICE, log, 0, "start %s %P, slot=%d", name, pid, s);

        yf_processes[s].pid = pid;
        yf_processes[s].exited = 0;

        if (respawn >= 0)
        {
                yf_pass_open_channel(log);
                return pid;
        }

        yf_processes[s].type = respawn;
        yf_processes[s].proc = proc;
        yf_processes[s].exit_cb = exit_cb;
        yf_processes[s].data = data;
        yf_processes[s].name = name;
        yf_processes[s].exiting = 0;

        if (s == yf_last_process)
        {
                yf_last_process++;
        }

        yf_pass_open_channel(log);

        return pid;
}


yf_pid_t
yf_execute(yf_exec_ctx_t *ctx, yf_log_t *log)
{
        return yf_spawn_process(yf_execute_proc, ctx, ctx->name,
                                ctx->type, ctx->exit_cb, log);
}


static void
yf_execute_proc(void *data, yf_log_t *log)
{
        yf_exec_ctx_t *ctx = data;

        yf_int_t  ret = 0;
        if (ctx->envp)
                ret = execve(ctx->path, ctx->argv, ctx->envp);
        else
                ret = execvp(ctx->path, ctx->argv);
        if (ret == -1)
        {
                yf_log_error(YF_LOG_ALERT, log, yf_errno,
                             "execve() failed while executing %s \"%s\"",
                             ctx->name, ctx->path);
        }

        //note, if execve failed, then exit(2)
        exit(2);
}

yf_int_t yf_pid_slot(yf_pid_t  pid)
{
        yf_int_t i1 = 0;
        for ( i1 = 0; i1 < yf_last_process; i1++ )
        {
                if (pid == yf_processes[i1].pid)
                        return i1;
        }
        return  -1;
}

yf_int_t yf_daemon(yf_log_t *log)
{
        int fd;

        switch (fork())
        {
        case -1:
                yf_log_error(YF_LOG_EMERG, log, yf_errno, "fork() failed");
                return YF_ERROR;

        case 0:
                break;

        default:
                exit(0);
        }

        yf_pid = yf_getpid();

        if (setsid() == -1)
        {
                yf_log_error(YF_LOG_EMERG, log, yf_errno, "setsid() failed");
                return YF_ERROR;
        }

        umask(0);

        fd = open("/dev/null", O_RDWR);
        if (fd == -1)
        {
                yf_log_error(YF_LOG_EMERG, log, yf_errno,
                             "open(\"/dev/null\") failed");
                return YF_ERROR;
        }

        if (dup2(fd, STDIN_FILENO) == -1)
        {
                yf_log_error(YF_LOG_EMERG, log, yf_errno, "dup2(STDIN) failed");
                return YF_ERROR;
        }

        if (dup2(fd, STDOUT_FILENO) == -1)
        {
                yf_log_error(YF_LOG_EMERG, log, yf_errno, "dup2(STDOUT) failed");
                return YF_ERROR;
        }

        if (fd > STDERR_FILENO)
        {
                if (close(fd) == -1)
                {
                        yf_log_error(YF_LOG_EMERG, log, yf_errno, "close() failed");
                        return YF_ERROR;
                }
        }

        return YF_OK;
}


static void
yf_close_parent_channels(yf_log_t *log)
{
        yf_int_t n;

        for (n = 0; n < yf_last_process; n++)
        {
                if (yf_processes[n].pid == -1)
                {
                        continue;
                }

                if (n == yf_process_slot)
                {
                        continue;
                }

                if (yf_processes[n].channel[1] == -1)
                {
                        continue;
                }

                if (close(yf_processes[n].channel[1]) == -1)
                {
                        yf_log_error(YF_LOG_ALERT, log, yf_errno,
                                     "close() channel failed");
                }
                yf_processes[n].channel[1] = -1;
        }

        /*if (close(yf_processes[yf_process_slot].channel[0]) == -1)
        {
                yf_log_error(YF_LOG_ALERT, log, yf_errno,
                             "close() channel failed");
        }*/
}


static void
yf_pass_open_channel(yf_log_t *log)
{
        uint32_t i1 = 0;
        yf_channel_t ch;

        ch.command = YF_CMD_OPEN_CHANNEL;
        yf_memzero(ch.data, sizeof(ch.data));
        ch.slot = yf_process_slot;
        ch.pid = yf_processes[yf_process_slot].pid;
        ch.fd = yf_processes[yf_process_slot].channel[0];
        yf_int_t i;

        for (i = 0; i < yf_last_process; i++)
        {
                if (i == yf_process_slot
                    || yf_processes[i].pid == -1
                    || yf_processes[i].channel[0] == -1
                    || yf_processes[i].type != YF_PROC_CHILD)
                {
                        continue;
                }

                yf_log_debug6(YF_LOG_DEBUG, log, 0,
                              "pass channel s:%d pid:%P fd:%d to s:%i pid:%P fd:%d",
                              ch.slot, ch.pid, ch.fd,
                              i, yf_processes[i].pid,
                              yf_processes[i].channel[0]);

                yf_write_channel(yf_processes[i].channel[0],
                                 &ch, log);
        }
}


void yf_update_channel(yf_channel_t *channel, yf_log_t *log)
{
        switch (channel->command)
        {
        case YF_CMD_OPEN_CHANNEL:
                yf_processes[channel->slot].pid = channel->pid;
                yf_processes[channel->slot].channel[0] = channel->fd;
                
                if (channel->slot >= yf_last_process)
                {
                        yf_last_process = channel->slot + 1;
                }

                yf_log_debug4(YF_LOG_DEBUG, log, 0,
                              "get channel s:%i pid:%P fd:%d process_size=%d",
                              channel->slot, channel->pid, channel->fd, 
                              yf_last_process);
                break;

        case YF_CMD_CLOSE_CHANNEL:

                yf_log_debug4(YF_LOG_DEBUG, log, 0,
                              "close channel s:%i pid:%P our:%P fd:%d",
                              channel->slot, channel->pid, yf_processes[channel->slot].pid,
                              yf_processes[channel->slot].channel[0]);

                if (close(yf_processes[channel->slot].channel[0]) == -1)
                {
                        yf_log_error(YF_LOG_ALERT, log, yf_errno,
                                     "close() channel failed");
                }

                yf_processes[channel->slot].channel[0] = -1;
                break;
        }
}
