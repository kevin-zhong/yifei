#ifndef _YF_SETPROCTITLE_H
#define _YF_SETPROCTITLE_H

#include <base_struct/yf_core.h>
#include <ppc/yf_header.h>

#if (HAVE_SETPROCTITLE)

/* FreeBSD, NetBSD, OpenBSD */

#define yf_init_setproctitle(log)
#define yf_setproctitle(title, log)    setproctitle("%s", title)


#else

#if !defined YF_SETPROCTITLE_USES_ENV


#if defined YF_SOLARIS

#define YF_SETPROCTITLE_USES_ENV  1
#define YF_SETPROCTITLE_PAD       ' '

yf_int_t yf_init_setproctitle(yf_log_t *log);
void yf_setproctitle(char *title, yf_log_t *log);

#elif defined YF_LINUX || defined YF_DARWIN

#define YF_SETPROCTITLE_USES_ENV  1
#define YF_SETPROCTITLE_PAD       '\0'

yf_int_t yf_init_setproctitle(yf_log_t *log);
void yf_setproctitle(char *title, yf_log_t *log);

#else

#define yf_init_setproctitle(log)
#define yf_setproctitle(title, log)

#endif /* OSes */

#endif /* YF_SETPROCTITLE_USES_ENV */

#endif /* YF_HAVE_SETPROCTITLE */


#endif /* _YF_SETPROCTITLE_H */
