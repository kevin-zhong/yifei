#ifndef  _YF_TYPES_H
#define _YF_TYPES_H

#include "../yf_config.h"
#include <assert.h>

//    YF_SYSTEM=`uname -s 2>/dev/null`
//    YF_RELEASE=`uname -r 2>/dev/null`
//    YF_MACHINE=`uname -m 2>/dev/null`

#include    <ctype.h>
#include    <sys/types.h>   /* basic system data types */
#include    <sys/socket.h>  /* basic socket definitions */
#if TIME_WITH_SYS_TIME
#include    <sys/time.h>    /* timeval{} for select() */
#include    <time.h>        /* timespec{} for pselect() */
#else
#if HAVE_SYS_TIME_H
#include    <sys/time.h>    /* includes <time.h> unsafely */
#else
#include    <time.h>        /* old system? */
#endif
#endif
#include    <netinet/in.h>  /* sockaddr_in{} and other Internet defns */
#include    <arpa/inet.h>   /* inet(3) functions */
#include    <errno.h>
#include    <fcntl.h>       /* for nonblocking */
#include    <signal.h>
#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>
#include    <stdarg.h> /*for va_args..*/
#include    <sys/stat.h>    /* for S_xxx file mode constants */
#include    <sys/uio.h>     /* for iovec{} and readv/writev */
#include    <unistd.h>
#include    <sys/wait.h>
#include    <sys/un.h>      /* for Unix domain sockets */

#include    <dirent.h>

#ifdef  HAVE_MMAP
#include    <sys/mman.h>
#endif

#ifdef  HAVE_STRINGS_H
#include    <strings.h>
#endif

#ifdef  HAVE_SYS_SYSCTL_H
#ifdef  HAVE_SYS_PARAM_H
#include    <sys/param.h>   /* OpenBSD prereq for sysctl.h */
#endif
#include    <sys/sysctl.h>
#endif

#ifdef  HAVE_SYS_EVENT_H
#include    <sys/event.h>   /* for kqueue */
#endif

#ifdef HAVE_NETINET_TCP_H  
#include   <netinet/tcp.h>  /* TCP_NODELAY, TCP_CORK */
#endif

#ifdef HAVE_LIBUTIL_H
#include   <libutil.h> //for setproctitle
#endif

#ifdef HAVE_MALLOC_H 
#include <malloc.h>             /* memalign() */
#endif

#ifdef HAVE_LIMITS_H 
#include <limits.h>             /* IOV_MAX */
#endif

#ifdef HAVE_STDDEF_H 
#include <stddef.h>   /* offsetof() */
#endif

/* Three headers are normally needed for socket/file ioctl's:
 * <sys/ioctl.h>, <sys/filio.h>, and <sys/sockio.h>.
 */
#ifdef  HAVE_SYS_IOCTL_H
#include    <sys/ioctl.h>
#endif
#ifdef  HAVE_SYS_FILIO_H
#include    <sys/filio.h>
#endif
#ifdef  HAVE_SYS_SOCKIO_H
#include    <sys/sockio.h>
#endif

#ifdef HAVE_NET_IF_DL_H
#include    <net/if_dl.h>
#endif

typedef struct yf_file_s        yf_file_t;
typedef struct yf_channel_s  yf_channel_t;

#include "yf_int.h"
#include "yf_alloc.h"
#include "yf_atomic.h"
#include "yf_errno.h"
#include "yf_fast_lock.h"
#include "yf_socket.h"
#include "yf_file.h"
#include "yf_shmem.h"
#include "yf_process.h"
#include "yf_thread.h"
#include "yf_time.h"
#include "yf_channel.h"
#include "yf_signal.h"
#include "yf_proc_ctx.h"

extern char **environ;


#define FILE_MODE   (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
/* default file access permissions for new files */
#define DIR_MODE    (FILE_MODE | S_IXUSR | S_IXGRP | S_IXOTH)
/* default permissions for new directories */

typedef void Sigfunc (int);     /* for signal handlers */

#ifndef  YF_NCPU
#define  YF_NCPU  2
#endif

void  yf_cpuinfo(void);


#if (__GNUC__ && SMP_CACHE_BYTES)
#define ____cacheline_aligned __attribute__((__aligned__(SMP_CACHE_BYTES)))
#else
#define ____cacheline_aligned
#endif

#endif
