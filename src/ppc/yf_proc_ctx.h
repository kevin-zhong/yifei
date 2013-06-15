#ifndef _YF_PROC_CTX_H
#define _YF_PROC_CTX_H

#include <base_struct/yf_core.h>
#include <ppc/yf_header.h>

extern int yf_argc;
extern char **yf_argv;
extern char **yf_os_argv;
extern char **yf_os_environ;

yf_int_t
yf_save_argv(yf_log_t* log, int argc, char *const *argv);

#endif

