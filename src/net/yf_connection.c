#include <ppc/yf_header.h>
#include <base_struct/yf_core.h>
#include <mio_driver/yf_event.h>
#include <net/yf_net.h>


yf_listening_t *
yf_create_listening(yf_net_t *net, yf_sock_addr_t *sockaddr, yf_sock_len_t socklen)
{
        size_t len;
        yf_listening_t *ls;
        yf_sock_addr_t *sa;
        yf_uchar_t text[MAX_SOCKADDR_STRLEN];

        ls = yf_array_push(&net->listening);
        if (ls == NULL)
        {
                return NULL;
        }
        
        //zero .... must

        yf_memzero(ls, sizeof(yf_listening_t));

        sa = yf_palloc(net->pool, socklen);
        if (sa == NULL)
        {
                return NULL;
        }

        yf_memcpy(sa, sockaddr, socklen);

        ls->sockaddr = sa;
        ls->socklen = socklen;

        len = yf_sock_ntop(sa, text, MAX_SOCKADDR_STRLEN, 1);
        ls->addr_text.len = len;

        switch (ls->sockaddr->sa_family)
        {
#ifdef  IPV6
        case AF_INET6:
                ls->addr_text_max_len = INET6_ADDRSTRLEN;
                break;
#endif

#ifdef  AF_UNIX
        case AF_UNIX:
                ls->addr_text_max_len = UNIX_ADDRSTRLEN;
                len++;
                break;
#endif
        case AF_INET:
                ls->addr_text_max_len = INET_ADDRSTRLEN;
                break;
        default:
                ls->addr_text_max_len = MAX_SOCKADDR_STRLEN;
                break;
        }

        ls->addr_text.data = yf_pnalloc(net->pool, len);
        if (ls->addr_text.data == NULL)
        {
                return NULL;
        }

        yf_memcpy(ls->addr_text.data, text, len);

        ls->fd = (yf_socket_t)-1;
        ls->type = SOCK_STREAM;

        ls->backlog = YF_LISTEN_BACKLOG;
        ls->rcvbuf = -1;
        ls->sndbuf = -1;

        return ls;
}


yf_int_t
yf_set_inherited_sockets(yf_net_t *net)
{
        size_t len;
        yf_uint_t i;
        yf_listening_t *ls;
        yf_sock_len_t olen;

        ls = net->listening.elts;
        for (i = 0; i < net->listening.nelts; i++)
        {
                ls[i].sockaddr = yf_palloc(net->pool, MAX_SOCKADDR_STRLEN);
                if (ls[i].sockaddr == NULL)
                {
                        return YF_ERROR;
                }

                ls[i].socklen = MAX_SOCKADDR_STRLEN;
                if (getsockname(ls[i].fd, ls[i].sockaddr, &ls[i].socklen) == -1)
                {
                        yf_log_error(YF_LOG_CRIT, net->log, yf_socket_errno,
                                     "getsockname() of the inherited "
                                     "socket #%d failed", ls[i].fd);
                        ls[i].ignore = 1;
                        continue;
                }

                switch (ls[i].sockaddr->sa_family)
                {
#ifdef  IPV6
                case AF_INET6:
                        ls[i].addr_text_max_len = INET6_ADDRSTRLEN;
                        len = INET6_ADDRSTRLEN + sizeof(":65535") - 1;
                        break;
#endif

#ifdef  AF_UNIX
                case AF_UNIX:
                        ls[i].addr_text_max_len = UNIX_ADDRSTRLEN;
                        len = UNIX_ADDRSTRLEN;
                        break;
#endif

                case AF_INET:
                        ls[i].addr_text_max_len = INET_ADDRSTRLEN;
                        len = INET_ADDRSTRLEN + sizeof(":65535") - 1;
                        break;

                default:
                        yf_log_error(YF_LOG_CRIT, net->log, yf_socket_errno,
                                     "the inherited socket #%d has "
                                     "an unsupported protocol family", ls[i].fd);
                        ls[i].ignore = 1;
                        continue;
                }

                ls[i].addr_text.data = yf_pnalloc(net->pool, len);
                if (ls[i].addr_text.data == NULL)
                {
                        return YF_ERROR;
                }

                len = yf_sock_ntop(ls[i].sockaddr, ls[i].addr_text.data, len, 1);
                if (len == 0)
                {
                        return YF_ERROR;
                }

                ls[i].addr_text.len = len;

                ls[i].backlog = YF_LISTEN_BACKLOG;

                olen = sizeof(int);

                if (getsockopt(ls[i].fd, SOL_SOCKET, SO_RCVBUF, (void *)&ls[i].rcvbuf,
                               &olen)
                    == -1)
                {
                        yf_log_error(YF_LOG_ALERT, net->log, yf_socket_errno,
                                     "getsockopt(SO_RCVBUF) %V failed, ignored",
                                     &ls[i].addr_text);

                        ls[i].rcvbuf = -1;
                }

                olen = sizeof(int);

                if (getsockopt(ls[i].fd, SOL_SOCKET, SO_SNDBUF, (void *)&ls[i].sndbuf,
                               &olen)
                    == -1)
                {
                        yf_log_error(YF_LOG_ALERT, net->log, yf_socket_errno,
                                     "getsockopt(SO_SNDBUF) %V failed, ignored",
                                     &ls[i].addr_text);

                        ls[i].sndbuf = -1;
                }
        }

        return YF_OK;
}


yf_int_t
yf_open_listening_sockets(yf_net_t *net, yf_int_t test_config)
{
        int reuseaddr;
        yf_uint_t i, tries, failed;
        yf_err_t err;
        yf_log_t *log;
        yf_socket_t s;
        yf_listening_t *ls;

        reuseaddr = 1;

        log = net->log;
        
        for (tries = 5; tries; tries--)
        {
                failed = 0;

                /* for each listening socket */

                ls = net->listening.elts;
                for (i = 0; i < net->listening.nelts; i++)
                {
                        if (ls[i].ignore)
                        {
                                continue;
                        }

                        if (ls[i].fd != -1)
                        {
                                continue;
                        }

                        if (ls[i].inherited)
                        {
                                /* TODO: close on exit */
                                /* TODO: nonblocking */
                                /* TODO: deferred accept */
                                continue;
                        }

                        s = yf_socket(ls[i].sockaddr->sa_family, ls[i].type, 0);

                        if (s == -1)
                        {
                                yf_log_error(YF_LOG_EMERG, log, yf_socket_errno,
                                             yf_socket_n " %V failed", &ls[i].addr_text);
                                return YF_ERROR;
                        }

                        if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR,
                                       (const void *)&reuseaddr, sizeof(int))
                            == -1)
                        {
                                yf_log_error(YF_LOG_EMERG, log, yf_socket_errno,
                                             "setsockopt(SO_REUSEADDR) %V failed",
                                             &ls[i].addr_text);

                                if (yf_close_socket(s) == -1)
                                {
                                        yf_log_error(YF_LOG_EMERG, log, yf_socket_errno,
                                                     yf_close_socket_n " %V failed",
                                                     &ls[i].addr_text);
                                }

                                return YF_ERROR;
                        }
                        
                        /* TODO: close on exit */

                        if (yf_nonblocking(s) == -1)
                        {
                                yf_log_error(YF_LOG_EMERG, log, yf_socket_errno,
                                             yf_nonblocking_n " %V failed",
                                             &ls[i].addr_text);

                                if (yf_close_socket(s) == -1)
                                {
                                        yf_log_error(YF_LOG_EMERG, log, yf_socket_errno,
                                                     yf_close_socket_n " %V failed",
                                                     &ls[i].addr_text);
                                }

                                return YF_ERROR;
                        }

                        yf_log_debug2(YF_LOG_DEBUG, log, 0,
                                      "bind() %V #%d ", &ls[i].addr_text, s);

                        if (bind(s, ls[i].sockaddr, ls[i].socklen) == -1)
                        {
                                err = yf_socket_errno;

                                if (err == YF_EADDRINUSE && test_config)
                                {
                                        continue;
                                }

                                yf_log_error(YF_LOG_EMERG, log, err,
                                             "bind() to %V failed", &ls[i].addr_text);

                                if (yf_close_socket(s) == -1)
                                {
                                        yf_log_error(YF_LOG_EMERG, log, yf_socket_errno,
                                                     yf_close_socket_n " %V failed",
                                                     &ls[i].addr_text);
                                }

                                if (err != YF_EADDRINUSE)
                                {
                                        return YF_ERROR;
                                }

                                failed = 1;

                                continue;
                        }

#ifdef  AF_UNIX
                        if (ls[i].sockaddr->sa_family == AF_LOCAL)
                        {
                                mode_t mode;
                                yf_uchar_t *name;

                                name = ls[i].addr_text.data + sizeof("unix:") - 1;
                                mode = (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

                                if (chmod((char *)name, mode) == -1)
                                {
                                        yf_log_error(YF_LOG_EMERG, net->log, yf_errno,
                                                     "chmod() \"%s\" failed", name);
                                }

                                if (test_config)
                                {
                                        if (yf_delete_file(name) == -1)
                                        {
                                                yf_log_error(YF_LOG_EMERG, net->log, yf_errno,
                                                             yf_delete_file_n " %s failed", name);
                                        }
                                }
                        }
#endif

                        if (listen(s, ls[i].backlog) == -1)
                        {
                                yf_log_error(YF_LOG_EMERG, log, yf_socket_errno,
                                             "listen() to %V, backlog %d failed",
                                             &ls[i].addr_text, ls[i].backlog);

                                if (yf_close_socket(s) == -1)
                                {
                                        yf_log_error(YF_LOG_EMERG, log, yf_socket_errno,
                                                     yf_close_socket_n " %V failed",
                                                     &ls[i].addr_text);
                                }

                                return YF_ERROR;
                        }

                        ls[i].listen = 1;

                        ls[i].fd = s;
                }

                if (!failed)
                {
                        break;
                }

                /* TODO: delay configurable */

                yf_log_error(YF_LOG_NOTICE, log, 0,
                             "try again to bind() after 500ms");

                yf_msleep(500);
        }

        if (failed)
        {
                yf_log_error(YF_LOG_EMERG, log, 0, "still could not bind()");
                return YF_ERROR;
        }

        return YF_OK;
}


void
yf_configure_listening_sockets(yf_net_t *net)
{
        yf_uint_t i;
        yf_listening_t *ls;

        ls = net->listening.elts;
        for (i = 0; i < net->listening.nelts; i++)
        {
                ls[i].log = *ls[i].logp;

                if (ls[i].rcvbuf != -1)
                {
                        if (setsockopt(ls[i].fd, SOL_SOCKET, SO_RCVBUF,
                                       (const void *)&ls[i].rcvbuf, sizeof(int))
                            == -1)
                        {
                                yf_log_error(YF_LOG_ALERT, net->log, yf_socket_errno,
                                             "setsockopt(SO_RCVBUF, %d) %V failed, ignored",
                                             ls[i].rcvbuf, &ls[i].addr_text);
                        }
                }

                if (ls[i].sndbuf != -1)
                {
                        if (setsockopt(ls[i].fd, SOL_SOCKET, SO_SNDBUF,
                                       (const void *)&ls[i].sndbuf, sizeof(int))
                            == -1)
                        {
                                yf_log_error(YF_LOG_ALERT, net->log, yf_socket_errno,
                                             "setsockopt(SO_SNDBUF, %d) %V failed, ignored",
                                             ls[i].sndbuf, &ls[i].addr_text);
                        }
                }

                if (ls[i].listen)
                {
                        /* change backlog via listen() */

                        if (listen(ls[i].fd, ls[i].backlog) == -1)
                        {
                                yf_log_error(YF_LOG_ALERT, net->log, yf_socket_errno,
                                             "listen() to %V, backlog %d failed, ignored",
                                             &ls[i].addr_text, ls[i].backlog);
                        }
                }
        }

        return;
}


void
yf_close_listening_sockets(yf_net_t *net, yf_int_t owner)
{
        yf_uint_t i;
        yf_listening_t *ls;
        yf_connection_t *c;

        ls = net->listening.elts;
        for (i = 0; i < net->listening.nelts; i++)
        {
                c = ls[i].connection;

                if (c)
                {
                        yf_free_fd_evt(c->read, c->write);

                        yf_free_connection(net, c);

                        c->fd = (yf_socket_t)-1;
                }

                yf_log_debug2(YF_LOG_DEBUG, net->log, 0,
                              "close listening %V #%d ", &ls[i].addr_text, ls[i].fd);

                if (yf_close_socket(ls[i].fd) == -1)
                {
                        yf_log_error(YF_LOG_EMERG, net->log, yf_socket_errno,
                                     yf_close_socket_n " %V failed", &ls[i].addr_text);
                }

#ifdef  AF_UNIX

                if (ls[i].sockaddr->sa_family == AF_UNIX && owner)
                {
                        yf_uchar_t *name = ls[i].addr_text.data + sizeof("unix:") - 1;

                        if (yf_delete_file(name) == -1)
                        {
                                yf_log_error(YF_LOG_EMERG, net->log, yf_socket_errno,
                                             yf_delete_file_n " %s failed", name);
                        }
                }

#endif

                ls[i].fd = (yf_socket_t)-1;
        }
}


yf_connection_t *
yf_get_connection(yf_net_t *net, yf_socket_t s, yf_log_t *log)
{
        yf_uint_t instance;
        yf_fd_event_t *rev, *wev;
        yf_connection_t *c;

        /* disable warning: Win32 SOCKET is u_int while UNIX socket is int */

        if (net->pconn && (yf_uint_t)s >= net->conn_capcity)
        {
                yf_log_error(YF_LOG_ALERT, log, 0,
                             "the new socket has number %d, "
                             "but only %ui files are available",
                             s, net->conn_capcity);
                return NULL;
        }

        if (net->pconn[s].used)
        {
                yf_log_error(YF_LOG_ERR, log, 0, 
                                "conn used, not free, may leak, socket=%d", s);
                return  NULL;
        }
        c = net->pconn + s;

        yf_memzero(c, sizeof(yf_connection_t));

        if (yf_alloc_fd_evt(net->evt_driver, s, &c->read, &c->write, log) != YF_OK)
        {
                yf_log_error(YF_LOG_ERR, log, 0, 
                                "allc fd evt failed, so alloc conn failed, socket=%d", s);
                return  NULL;                
        }

        rev = c->read;
        wev = c->write;

        c->used = 1;
        net->used_conn++;
        
        c->fd = s;
        c->log = log;

        rev->data = c;
        wev->data = c;

        yf_log_debug(YF_LOG_DEBUG, log, 0, "alloc conn, socket=%i", s);

        return c;
}


void
yf_free_connection(yf_net_t *net, yf_connection_t *c)
{
        yf_socket_t s = c->fd;

        if (s == -1)
        {
                yf_log_error(YF_LOG_ALERT, c->log, 0, "connection already closed");
                return;
        }
        
        if (!net->pconn[s].used)
        {
                yf_log_error(YF_LOG_ERR, (c->log ? c->log : net->log), 0, 
                                "conn used, not free, may leak, socket=%d", s);
                return;
        }

        net->pconn[s].used = 0;
        net->used_conn--;

        yf_log_debug(YF_LOG_DEBUG, c->log, 0, "free conn, socket=%i", s);
}


void
yf_close_connection(yf_net_t *net, yf_connection_t *c)
{
        yf_err_t err;
        yf_uint_t log_error, level;
        yf_socket_t fd;

        if (c->fd == -1)
        {
                yf_log_error(YF_LOG_ALERT, c->log, 0, "connection already closed");
                return;
        }

        if (!yf_free_fd_evt(c->read, c->write))
                return;

        log_error = c->log_error;

        yf_free_connection(net, c);

        fd = c->fd;
        c->fd = (yf_socket_t)-1;

        if (yf_close_socket(fd) == -1)
        {
                err = yf_socket_errno;

                if (err == YF_ECONNRESET || err == YF_ENOTCONN)
                {
                        switch (log_error)
                        {
                        case YF_ERROR_INFO:
                                level = YF_LOG_INFO;
                                break;

                        case YF_ERROR_ERR:
                                level = YF_LOG_ERR;
                                break;

                        default:
                                level = YF_LOG_CRIT;
                        }
                }
                else {
                        level = YF_LOG_CRIT;
                }

                /* we use yf_cycle->log because c->log was in c->pool */

                yf_log_error(level, net->log, err,
                             yf_close_socket_n " %d failed", fd);
        }
}


yf_int_t
yf_connection_local_sockaddr(yf_connection_t *c, yf_str_t *s, yf_uint_t port)
{
        yf_sock_len_t len;
        yf_uint_t addr;
        yf_uchar_t sa[MAX_SOCKADDR_STRLEN];
        struct sockaddr_in *sin;

#ifdef  IPV6
        yf_uint_t i;
        struct sockaddr_in6 *sin6;
#endif

        switch (c->local_sockaddr->sa_family)
        {
#ifdef  IPV6
        case AF_INET6:
                sin6 = (struct sockaddr_in6 *)c->local_sockaddr;

                for (addr = 0, i = 0; addr == 0 && i < 16; i++)
                {
                        addr |= sin6->sin6_addr.s6_addr[i];
                }

                break;
#endif

        default: /* AF_INET */
                sin = (struct sockaddr_in *)c->local_sockaddr;
                addr = sin->sin_addr.s_addr;
                break;
        }

        if (addr == 0)
        {
                len = MAX_SOCKADDR_STRLEN;

                if (getsockname(c->fd, (yf_sock_addr_t *)&sa, &len) == -1)
                {
                        yf_connection_error(c, yf_socket_errno, "getsockname() failed");
                        return YF_ERROR;
                }

                c->local_sockaddr = yf_palloc(c->pool, len);
                if (c->local_sockaddr == NULL)
                {
                        return YF_ERROR;
                }

                yf_memcpy(c->local_sockaddr, &sa, len);
        }

        if (s == NULL)
        {
                return YF_OK;
        }

        s->len = yf_sock_ntop(c->local_sockaddr, s->data, s->len, port);

        return YF_OK;
}


yf_int_t
yf_connection_error(yf_connection_t *c, yf_err_t err, char *text)
{
        yf_uint_t level;

        /* Winsock may return YF_ECONNABORTED instead of YF_ECONNRESET */

        if ((err == YF_ECONNRESET
#if defined (YF_WIN32)
             || err == YF_ECONNABORTED
#endif
             ) && c->log_error == YF_ERROR_IGNORE_ECONNRESET)
        {
                return 0;
        }

#if defined (YF_SOLARIS)
        if (err == YF_EINVAL && c->log_error == YF_ERROR_IGNORE_EINVAL)
        {
                return 0;
        }
#endif

        if (err == 0
            || err == YF_ECONNRESET
#if defined (YF_WIN32)
            || err == YF_ECONNABORTED
#else
            || err == YF_EPIPE
#endif
            || err == YF_ENOTCONN
            || err == YF_ETIMEDOUT
            || err == YF_ECONNREFUSED
            || err == YF_ENETDOWN
            || err == YF_ENETUNREACH
            || err == YF_EHOSTDOWN
            || err == YF_EHOSTUNREACH)
        {
                switch (c->log_error)
                {
                case YF_ERROR_IGNORE_EINVAL:
                case YF_ERROR_IGNORE_ECONNRESET:
                case YF_ERROR_INFO:
                        level = YF_LOG_INFO;
                        break;

                default:
                        level = YF_LOG_ERR;
                }
        }
        else {
                level = YF_LOG_ALERT;
        }
        
        // good so beautifull ....

        yf_log_error(level, c->log, err, text);

        return YF_ERROR;
}
