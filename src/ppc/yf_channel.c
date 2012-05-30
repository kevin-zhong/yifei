#include <ppc/yf_header.h>
#include <base_struct/yf_core.h>

yf_int_t
yf_write_channel(yf_socket_t s, yf_channel_t *ch, yf_log_t *log)
{
        ssize_t n;
        yf_err_t err;
        struct iovec iov[1];
        struct msghdr msg;

#if defined (HAVE_MSGHDR_MSG_CONTROL)

        union {
                struct cmsghdr cm;
                char           space[CMSG_SPACE(sizeof(int))];
        } cmsg;

        if (ch->fd == -1)
        {
                msg.msg_control = NULL;
                msg.msg_controllen = 0;
        }
        else {
                msg.msg_control = (caddr_t)&cmsg;
                msg.msg_controllen = sizeof(cmsg);

                cmsg.cm.cmsg_len = CMSG_LEN(sizeof(int));
                cmsg.cm.cmsg_level = SOL_SOCKET;
                cmsg.cm.cmsg_type = SCM_RIGHTS;
                
                yf_memcpy(CMSG_DATA(&cmsg.cm), &ch->fd, sizeof(int));
        }

        msg.msg_flags = 0;

#else

        if (ch->fd == -1)
        {
                msg.msg_accrights = NULL;
                msg.msg_accrightslen = 0;
        }
        else {
                msg.msg_accrights = (caddr_t)&ch->fd;
                msg.msg_accrightslen = sizeof(int);
        }

#endif

        iov[0].iov_base = (char *)ch;
        iov[0].iov_len = sizeof(yf_channel_t);

        msg.msg_name = NULL;
        msg.msg_namelen = 0;
        msg.msg_iov = iov;
        msg.msg_iovlen = 1;

        n = sendmsg(s, &msg, 0);

        if (n == -1)
        {
                err = yf_errno;
                if (YF_EAGAIN(err))
                {
                        return YF_AGAIN;
                }

                yf_log_error(YF_LOG_ALERT, log, err, "sendmsg() failed");
                return YF_ERROR;
        }
        return YF_OK;
}


yf_int_t
yf_read_channel(yf_socket_t s, yf_channel_t *ch, yf_log_t *log)
{
        ssize_t n;
        yf_err_t err;
        struct iovec iov[1];
        struct msghdr msg;

#if defined (HAVE_MSGHDR_MSG_CONTROL)
        union {
                struct cmsghdr cm;
                char           space[CMSG_SPACE(sizeof(int))];
        } cmsg;
#else
        int fd;
#endif

        iov[0].iov_base = (char *)ch;
        iov[0].iov_len = sizeof(yf_channel_t);

        msg.msg_name = NULL;
        msg.msg_namelen = 0;
        msg.msg_iov = iov;
        msg.msg_iovlen = 1;

#if defined (HAVE_MSGHDR_MSG_CONTROL)
        msg.msg_control = (caddr_t)&cmsg;
        msg.msg_controllen = sizeof(cmsg);
#else
        msg.msg_accrights = (caddr_t)&fd;
        msg.msg_accrightslen = sizeof(int);
#endif

        n = recvmsg(s, &msg, 0);

        if (n == -1)
        {
                err = yf_errno;
                if (YF_EAGAIN(err))
                {
                        return YF_AGAIN;
                }

                yf_log_error(YF_LOG_ALERT, log, err, "recvmsg() failed");
                return YF_ERROR;
        }

        if (n == 0)
        {
                yf_log_debug0(YF_LOG_DEBUG, log, 0, "recvmsg() returned zero");
                return YF_ERROR;
        }

        if ((size_t)n < sizeof(yf_channel_t))
        {
                yf_log_error(YF_LOG_ALERT, log, 0,
                             "recvmsg() returned not enough data: %uz", n);
                return YF_ERROR;
        }

#if defined (HAVE_MSGHDR_MSG_CONTROL)

        if (ch->command == YF_CMD_SEND_FD)
        {
                if (cmsg.cm.cmsg_len < (yf_sock_len_t)CMSG_LEN(sizeof(int)))
                {
                        yf_log_error(YF_LOG_ALERT, log, 0,
                                     "recvmsg() returned too small ancillary data");
                        return YF_ERROR;
                }

                if (cmsg.cm.cmsg_level != SOL_SOCKET || cmsg.cm.cmsg_type != SCM_RIGHTS)
                {
                        yf_log_error(YF_LOG_ALERT, log, 0,
                                     "recvmsg() returned invalid ancillary data "
                                     "level %d or type %d",
                                     cmsg.cm.cmsg_level, cmsg.cm.cmsg_type);
                        return YF_ERROR;
                }

                /* ch->fd = *(int *) CMSG_DATA(&cmsg.cm); */

                yf_memcpy(&ch->fd, CMSG_DATA(&cmsg.cm), sizeof(int));
        }

        if (msg.msg_flags & (MSG_TRUNC | MSG_CTRUNC))
        {
                yf_log_error(YF_LOG_ALERT, log, 0,
                             "recvmsg() truncated data");
        }

#else

        if (ch->command == YF_CMD_SEND_FD)
        {
                if (msg.msg_accrightslen != sizeof(int))
                {
                        yf_log_error(YF_LOG_ALERT, log, 0,
                                     "recvmsg() returned no ancillary data");
                        return YF_ERROR;
                }

                ch->fd = fd;
        }

#endif

        return n;
}


void
yf_close_channel(yf_fd_t *fd, yf_log_t *log)
{
        if (close(fd[0]) == -1)
        {
                yf_log_error(YF_LOG_ALERT, log, yf_errno, "close() channel failed");
        }

        if (close(fd[1]) == -1)
        {
                yf_log_error(YF_LOG_ALERT, log, yf_errno, "close() channel failed");
        }
}
