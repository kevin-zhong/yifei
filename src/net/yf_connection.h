#ifndef _YF_CONNECTION_H_INCLUDED_
#define _YF_CONNECTION_H_INCLUDED_

#include <base_struct/yf_core.h>
#include <ppc/yf_header.h>
#include <net/yf_net.h>

struct yf_listening_s
{
        yf_socket_t              fd;

        yf_sock_addr_t *        sockaddr;
        yf_sock_len_t                socklen; /* size of sockaddr */
        size_t                   addr_text_max_len;
        yf_str_t                 addr_text;

        int                      type;

        int                      backlog;
        int                      rcvbuf;
        int                      sndbuf;

        /* handler of accepted connection */
        void (*handler)(yf_connection_t *c);

        yf_log_t                 log;
        yf_log_t *               logp;

        size_t                   pool_size;
        yf_connection_t *        connection;

        unsigned                 open : 1;
        unsigned                 inherited : 1; /* inherited from previous process */
        unsigned                 ignore : 1;
        unsigned                 listen : 1;
};


typedef enum {
        YF_ERROR_ALERT = 0,
        YF_ERROR_ERR,
        YF_ERROR_INFO,
        YF_ERROR_IGNORE_ECONNRESET,
        YF_ERROR_IGNORE_EINVAL
} yf_connection_log_error_e;


typedef enum {
        YF_TCP_NODELAY_UNSET = 0,
        YF_TCP_NODELAY_SET,
        YF_TCP_NODELAY_DISABLED
} yf_connection_tcp_nodelay_e;


typedef enum {
        YF_TCP_NOPUSH_UNSET = 0,
        YF_TCP_NOPUSH_SET,
        YF_TCP_NOPUSH_DISABLED
} yf_connection_tcp_nopush_e;


struct yf_connection_s
{
        void *           data;
        yf_fd_event_t       *read;
        yf_fd_event_t       *write;

        yf_socket_t      fd;

        yf_listening_t * listening;

        off_t            sent;

        yf_log_t *       log;

        yf_pool_t *      pool;

        yf_sock_addr_t *sockaddr;
        yf_sock_len_t        socklen;
        yf_str_t         addr_text;

        yf_sock_addr_t *local_sockaddr;

        yf_buf_t *       buffer;

        yf_uint_t        requests;

        unsigned         log_error : 3;  /* yf_connection_log_error_e */

        unsigned         single_connection : 1;
        unsigned         unexpected_eof : 1;
        unsigned         timedout : 1;
        unsigned         error : 1;
        unsigned         destroyed : 1;

        unsigned         idle : 1;
        unsigned         reusable : 1;
        unsigned         close : 1;
        unsigned         used : 1;

        unsigned         sndlowat : 1;
        unsigned         tcp_nodelay : 2; /* yf_connection_tcp_nodelay_e */
        unsigned         tcp_nopush : 2;  /* yf_connection_tcp_nopush_e */
};


yf_listening_t *yf_create_listening(yf_net_t *cf, yf_sock_addr_t *sockaddr, yf_sock_len_t socklen);
yf_int_t yf_set_inherited_sockets(yf_net_t *net);
yf_int_t yf_open_listening_sockets(yf_net_t *net, yf_int_t test_config);
void yf_configure_listening_sockets(yf_net_t *net);
void yf_close_listening_sockets(yf_net_t *net, yf_int_t owner);


yf_connection_t *yf_get_connection(yf_net_t *net
                , yf_socket_t s, yf_log_t *log);
void yf_free_connection(yf_net_t *net, yf_connection_t *c);

void yf_close_connection(yf_net_t *net, yf_connection_t *c);

yf_int_t yf_connection_local_sockaddr(yf_connection_t *c, yf_str_t *s, yf_uint_t port);
yf_int_t yf_connection_error(yf_connection_t *c, yf_err_t err, char *text);

#endif /* _YF_CONNECTION_H_INCLUDED_ */
