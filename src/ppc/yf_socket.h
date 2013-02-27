#ifndef  _YF_SOCKET_TYPE_H
#define _YF_SOCKET_TYPE_H

#include <base_struct/yf_core.h>
#include <ppc/yf_header.h>

#ifndef INADDR_NONE
/* $$.Ic INADDR_NONE$$ */
#define INADDR_NONE 0xffffffff  /* should have been in <netinet/in.h> */
#endif

#ifndef SHUT_RD         /* these three POSIX names are new */
#define SHUT_RD     0   /* shutdown for reading */
#define SHUT_WR     1   /* shutdown for writing */
#define SHUT_RDWR   2   /* shutdown for reading and writing */
/* $$.Ic SHUT_RD$$ */
/* $$.Ic SHUT_WR$$ */
/* $$.Ic SHUT_RDWR$$ */
#endif

/* *INDENT-OFF* */
#ifndef INET_ADDRSTRLEN
/* $$.Ic INET_ADDRSTRLEN$$ */
#define	INET_ADDRSTRLEN		16	/* "ddd.ddd.ddd.ddd\0"
								    1234567890123456 */
#endif

/* Define following even if IPv6 not supported, so we can always allocate
   an adequately sized buffer without #ifdefs in the code. */
#ifndef INET6_ADDRSTRLEN
/* $$.Ic INET6_ADDRSTRLEN$$ */
#define	INET6_ADDRSTRLEN	46	/* max size of IPv6 address string:
				   "xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx" or
				   "xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:ddd.ddd.ddd.ddd\0"
				    1234567890123456789012345678901234567890123456 */
#endif
/* *INDENT-ON* */

#ifdef  AF_UNIX
#define  UNIX_ADDRSTRLEN  sizeof(struct sockaddr_un)
#else
#define  UNIX_ADDRSTRLEN 512
#endif


/* The structure returned by recvfrom_flags() */
struct unp_in_pktinfo
{
        struct in_addr ipi_addr;    /* dst IPv4 address */
        int            ipi_ifindex; /* received interface index */
};
/* $$.It unp_in_pktinfo$$ */
/* $$.Ib ipi_addr$$ */
/* $$.Ib ipi_ifindex$$ */

/* POSIX requires the SUN_LEN() macro, but not all implementations DefinE
 * it (yet).  Note that this 4.4BSD macro works regardless whether there is
 * a length field or not. */
#ifndef SUN_LEN
/* $$.Im SUN_LEN$$ */
#define SUN_LEN(su) \
        (sizeof(*(su)) - sizeof((su)->sun_path) + strlen((su)->sun_path))
#endif

/* POSIX renames "Unix domain" as "local IPC."
 * Not all systems DefinE AF_LOCAL and PF_LOCAL (yet). */
#ifndef AF_LOCAL
#define AF_LOCAL    AF_UNIX
#endif
#ifndef PF_LOCAL
#define PF_LOCAL    PF_UNIX
#endif

/* POSIX requires that an #include of <poll.h> DefinE INFTIM, but many
 * systems still DefinE it in <sys/stropts.h>.  We don't want to include
 * all the STREAMS stuff if it's not needed, so we just DefinE INFTIM here.
 * This is the standard value, but there's no guarantee it is -1. */
#ifndef INFTIM
#define INFTIM          (-1)    /* infinite poll timeout */
/* $$.Ic INFTIM$$ */
#ifdef  HAVE_POLL_H
#define INFTIM_UNPH             /* tell unpxti.h we defined it */
#endif
#endif

/* Following could be derived from SOMAXCONN in <sys/socket.h>, but many
 * kernels still #define it as 5, while actually supporting many more */
#define YF_LISTEN_BACKLOG     1024    /* 2nd argument to listen() */

/* Following shortens all the typecasts of pointer arguments: */
#define yf_sock_addr_t  struct sockaddr
#define yf_sock_len_t     socklen_t

#ifndef HAVE_STRUCT_SOCKADDR_STORAGE
/*
 * RFC 3493: protocol-independent placeholder for socket addresses
 */
#define __SS_MAXSIZE    256
#define __SS_ALIGNSIZE  (sizeof(yf_s64_t))
#ifdef HAVE_SOCKADDR_SA_LEN
#define __SS_PAD1SIZE   (__SS_ALIGNSIZE - sizeof(unsigned char) - sizeof(sa_family_t))
#else
#define __SS_PAD1SIZE   (__SS_ALIGNSIZE - sizeof(sa_family_t))
#endif
#define __SS_PAD2SIZE   (__SS_MAXSIZE - 2 *__SS_ALIGNSIZE)

typedef struct
{
#ifdef HAVE_SOCKADDR_SA_LEN
        unsigned char ss_len;
#endif
        sa_family_t   ss_family;
        char          __ss_pad1[__SS_PAD1SIZE];
        yf_s64_t    __ss_align;
        char          __ss_pad2[__SS_PAD2SIZE];
} 
yf_sockaddr_storage_t;
#endif


#define  MAX_SOCKADDR_STRLEN  UNIX_ADDRSTRLEN

ssize_t yf_sock_ntop(const yf_sock_addr_t *sa, char *text, size_t len, yf_int_t port);

yf_int_t  yf_sock_get_port(const yf_sock_addr_t *sa);
void yf_sock_set_port(yf_sock_addr_t *sa, yf_int_t port);

#define yf_inet_aton(assic_addr)  inet_addr(assic_addr)

yf_int_t yf_sock_set_addr(yf_sock_addr_t *sa, const char *addr);

yf_sock_len_t  yf_sock_len(yf_sock_addr_t *sa);



typedef int  yf_socket_t;

#define yf_socket_n        "socket()"


#define yf_nonblocking(s)  yf_fcntl(s, F_SETFL, fcntl(s, F_GETFL) | O_NONBLOCK)
#define yf_nonblocking_n   "yf_fcntl(O_NONBLOCK)"

#define yf_blocking(s)     yf_fcntl(s, F_SETFL, fcntl(s, F_GETFL) & ~O_NONBLOCK)
#define yf_blocking_n      "yf_fcntl(!O_NONBLOCK)"


int  __yf_tcp_nocork(yf_socket_t s, int tcp_nocork);

#define yf_tcp_nocork(s)  __yf_tcp_nocork(s, 1)
#define yf_tcp_cork(s)  __yf_tcp_nocork(s, 0)

#define yf_tcp_nocork_n   "yf_setsockopt(TCP_CORK)"
#define yf_tcp_cork_n     "yf_setsockopt(!TCP_CORK)"


#define yf_shutdown_socket    yf_shutdown
#define yf_shutdown_socket_n  "shutdown()"

#define yf_close_socket    yf_close
#define yf_close_socket_n  "yf_close() socket"

#endif

