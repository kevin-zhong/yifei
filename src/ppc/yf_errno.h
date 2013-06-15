#ifndef  _YF_ERRNO_H_
#define _YF_ERRNO_H_

#include <ppc/yf_header.h>

#define YF_EINTR         EINTR
#define YF_ECHILD        ECHILD
#define YF_ENOMEM        ENOMEM
#define YF_EACCES        EACCES
#define YF_EBUSY         EBUSY
#define YF_EEXIST        EEXIST
#define YF_EXDEV         EXDEV
#define YF_ENOTDIR       ENOTDIR
#define YF_EISDIR        EISDIR
#define YF_EINVAL        EINVAL
#define YF_ENOSPC        ENOSPC
#define YF_EPIPE         EPIPE
#define YF_EPERM         EPERM
#define YF_ENOENT        ENOENT
#define YF_ENOPATH       ENOENT
#define YF_ESRCH         ESRCH
#define YF_EHOSTDOWN     EHOSTDOWN
#define YF_EHOSTUNREACH  EHOSTUNREACH
#define YF_ENOSYS        ENOSYS
#define YF_ECANCELED     ECANCELED
#define YF_EINPROGRESS   EINPROGRESS
#define YF_EADDRINUSE    EADDRINUSE
#define YF_ECONNABORTED  ECONNABORTED
#define YF_ECONNRESET    ECONNRESET
#define YF_ENOTCONN      ENOTCONN
#define YF_ETIMEDOUT     ETIMEDOUT
#define YF_ECONNREFUSED  ECONNREFUSED
#define YF_ENAMETOOLONG  ENAMETOOLONG
#define YF_ENETDOWN      ENETDOWN
#define YF_ENETUNREACH   ENETUNREACH
#define YF_EBADF  EBADF
#define YF_ENFILE  ENFILE

#ifdef EWOULDBLOCK

#ifdef EAGAIN
#define YF_EAGAIN(err) (err==EWOULDBLOCK || err==EAGAIN)
#else
#define YF_EAGAIN(err) (err==EWOULDBLOCK)
#endif

#define YF_EAGAINNO EWOULDBLOCK

#else
#define YF_EAGAIN(err) (err==EAGAIN)
#define YF_EAGAINNO  EAGAIN
#endif

#define yf_errno                  errno
#define yf_set_errno(err)     errno = err
#define yf_socket_errno      yf_errno

const yf_str_t* yf_strerror(yf_err_t err);
yf_int_t yf_strerror_init(void);

#endif

