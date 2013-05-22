#ifndef _YF_SYS_CALL_H_20130226_H
#define _YF_SYS_CALL_H_20130226_H
/*
* copyright@: kevin_zhong, mail:qq2000zhong@gmail.com
* time: 20130226-19:42:25
*/

#include <base_struct/yf_core.h>
#include <ppc/yf_header.h>

#ifdef YF_FREEBSD
#define yf_usleep_ret_t void
#else
#define yf_usleep_ret_t int
#endif

#ifndef  YF_SYS_CALL_REPTR

#define yf_usleep  usleep
#define yf_sleep    sleep

#define yf_close  close
#define yf_shutdown  shutdown

#define yf_read   read
#define yf_write  write

#define yf_recvfrom recvfrom
#define yf_sendto     sendto

#define yf_sendmsg  sendmsg
#define yf_recvmsg  recvmsg

#define yf_readv   readv
#define yf_writev  writev

#define yf_socket    socket
#define yf_connect  connect
#define yf_accept    accept

#define yf_fcntl       fcntl
#define yf_setsockopt        setsockopt

#define yf_gethostbyname  gethostbyname
#define yf_getaddrinfo       getaddrinfo

#define yf_getsockopt  getsockopt

#ifdef  HAVE_SYS_SELECT_H
#define yf_select  select
#endif

/* Older resolvers do not have gethostbyname2() */
#ifdef HAVE_GETHOSTBYNAME2
#define yf_gethostbyname2  gethostbyname2
#endif

#ifdef HAVE_GETHOSTBYNAME_R
#define yf_gethostbyname_r gethostbyname_r
#endif

#ifdef HAVE_GETHOSTBYNAME2_R
#define yf_gethostbyname2_r gethostbyname2_r
#endif

#else

extern yf_usleep_ret_t (*yf_usleep)(unsigned int us);
extern unsigned int (*yf_sleep)(unsigned int s);

extern int (*yf_close)(int fd);
extern int (*yf_shutdown)(int fd, int howto);

extern ssize_t (*yf_read)(int fd, void *buf, size_t count);
extern ssize_t (*yf_write)(int fd, const void *buf, size_t count);

extern ssize_t (*yf_recvfrom)(int s, void *buf, size_t len, int flags,
                struct sockaddr *from, socklen_t *fromlen);
extern ssize_t (*yf_sendto)(int s, const void *buf, size_t len, int flags, 
                const struct sockaddr *to, socklen_t tolen);

extern ssize_t (*yf_recvmsg)(int s, struct msghdr *msg, int flags);
extern ssize_t (*yf_sendmsg)(int s, const struct msghdr *msg, int flags);

extern ssize_t (*yf_readv)(int fd, const struct iovec *vector, int count);
extern ssize_t (*yf_writev)(int fd, const struct iovec *vector, int count);

extern int (*yf_socket)(int domain, int type, int protocol);
extern int (*yf_connect)(int sockfd, const struct sockaddr *serv_addr, socklen_t addrlen);
extern int (*yf_accept)(int sockfd, struct sockaddr *addr, socklen_t *addrlen);

extern int (*yf_fcntl)(int fd, int cmd, ...);

extern int (*yf_getsockopt)(int sockfd, int level, int optname,
              void *optval, socklen_t *optlen);
extern int (*yf_setsockopt)(int s, int level, int optname, 
                const void *optval, socklen_t optlen);

extern struct hostent* (*yf_gethostbyname)(const char *name);
extern int (*yf_getaddrinfo)(const char *node, const char *service, 
               const struct addrinfo *hints, 
               struct addrinfo **res);

#ifdef  HAVE_SYS_SELECT_H
#include <sys/select.h>
extern int (*yf_select)(int nfds, fd_set *readfds, fd_set *writefds,
                fd_set *exceptfds, struct timeval *timeout);
#endif

#ifdef  HAVE_POLL_H
#include <poll.h>
extern int (*yf_poll)(struct pollfd *fds, nfds_t nfds, int timeout);
#endif

#ifdef HAVE_GETHOSTBYNAME2
extern struct hostent* (*yf_gethostbyname2)(const char *name, int af);
#endif

#ifdef HAVE_GETHOSTBYNAME_R
extern int (*yf_gethostbyname_r)(const char *name,
       struct hostent *ret, char *buf, size_t buflen,
       struct hostent **result, int *h_errnop);
#endif

#ifdef HAVE_GETHOSTBYNAME2_R
extern int (*yf_gethostbyname2_r)(const char *name, int af,
       struct hostent *ret, char *buf, size_t buflen,
       struct hostent **result, int *h_errnop);
#endif

#endif

#define yf_recv(a, b, c, d)   yf_recvfrom(a, b, c, d, 0, 0)
#define yf_send(a, b, c, d)   yf_sendto(a, b, c, d, 0, 0)
#define yf_msleep(ms)        yf_usleep(ms * 1000)

#endif

