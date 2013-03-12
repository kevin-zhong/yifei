#include <ppc/yf_header.h>
#include <base_struct/yf_core.h>

#ifdef  YF_SYS_CALL_REPTR

yf_usleep_ret_t (*yf_usleep)(unsigned int us) = usleep;
unsigned int (*yf_sleep)(unsigned int s) = sleep;

int (*yf_close)(int fd) = close;
int (*yf_shutdown)(int fd, int howto) = shutdown;

ssize_t (*yf_read)(int fd, void *buf, size_t count) = read;
ssize_t (*yf_write)(int fd, const void *buf, size_t count) = write;

ssize_t (*yf_recvfrom)(int s, void *buf, size_t len, int flags, 
                struct sockaddr *from, socklen_t *fromlen) = recvfrom;
ssize_t (*yf_sendto)(int s, const void *buf, size_t len, int flags, 
                const struct sockaddr *to, socklen_t tolen) = sendto;

ssize_t (*yf_recvmsg)(int s, struct msghdr *msg, int flags) = recvmsg;
ssize_t (*yf_sendmsg)(int s, const struct msghdr *msg, int flags) = sendmsg;

ssize_t (*yf_readv)(int fd, const struct iovec *vector, int count) = readv;
ssize_t (*yf_writev)(int fd, const struct iovec *vector, int count) = writev;

int (*yf_socket)(int domain, int type, int protocol) = socket;
int (*yf_connect)(int sockfd, const struct sockaddr *serv_addr, socklen_t addrlen) = connect;
int (*yf_accept)(int sockfd, struct sockaddr *addr, socklen_t *addrlen) = accept;

int (*yf_fcntl)(int fd, int cmd, ...) = fcntl;

int (*yf_getsockopt)(int sockfd, int level, int optname,
              void *optval, socklen_t *optlen) = getsockopt;
int (*yf_setsockopt)(int s, int level, int optname, 
                const void *optval, socklen_t optlen) = setsockopt;

struct hostent* (*yf_gethostbyname)(const char *name) = gethostbyname;
int (*yf_getaddrinfo)(const char *node, const char *service, 
               const struct addrinfo *hints, 
               struct addrinfo **res) = getaddrinfo;

#ifdef  HAVE_SYS_SELECT_H
int (*yf_select)(int nfds, fd_set *readfds, fd_set *writefds,
                fd_set *exceptfds, struct timeval *timeout) = select;
#endif

#ifdef HAVE_GETHOSTBYNAME2
struct hostent* (*yf_gethostbyname2)(const char *name, int af) = gethostbyname2;
#endif

#endif

