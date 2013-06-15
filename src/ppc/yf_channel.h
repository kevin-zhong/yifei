#ifndef _YF_CHANNEL_H
#define _YF_CHANNEL_H

#include <base_struct/yf_core.h>
#include <ppc/yf_header.h>

#define YF_CMD_SEND_FD         1
#define YF_CMD_CONT_SVR      2
#define YF_CMD_SUSPEND_SVR  3
#define YF_CMD_DATA              4

#define YF_CMD_OPEN_CHANNEL   5
#define YF_CMD_CLOSE_CHANNEL  6
#define YF_CMD_QUIT           7
#define YF_CMD_TERMINATE      8


struct  yf_channel_s
{
        yf_uchar_t command;
        char   data[47];
        yf_int_t  slot;
        yf_pid_t  pid;
        yf_fd_t   fd;
};

yf_int_t  yf_open_channel(yf_socket_t* fds, yf_int_t unblock1, yf_int_t unblock2
                , yf_int_t close_on_exe, yf_log_t *log);

yf_int_t yf_write_channel(yf_socket_t s, yf_channel_t *ch, yf_log_t *log);
yf_int_t yf_read_channel(yf_socket_t s, yf_channel_t *ch, yf_log_t *log);

void yf_close_channel(yf_fd_t *fd, yf_log_t *log);


#endif
