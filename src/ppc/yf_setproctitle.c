#include <ppc/yf_header.h>
#include <base_struct/yf_core.h>

#if (YF_SETPROCTITLE_USES_ENV)

static char *yf_os_argv_last;

yf_int_t
yf_init_setproctitle(yf_log_t *log)
{
        char *p;
        size_t size;
        yf_uint_t i;

        size = 0;

        for (i = 0; environ[i]; i++)
        {
                size += yf_strlen(environ[i]) + 1;
        }

        p = yf_alloc(size);
        if (p == NULL)
        {
                return YF_ERROR;
        }

        yf_os_argv_last = yf_os_argv[0];

        for (i = 0; yf_os_argv[i]; i++)
        {
                if (yf_os_argv_last == yf_os_argv[i])
                {
                        yf_os_argv_last = yf_os_argv[i] + yf_strlen(yf_os_argv[i]) + 1;
                }
        }

        for (i = 0; environ[i]; i++)
        {
                if (yf_os_argv_last == environ[i])
                {
                        size = yf_strlen(environ[i]) + 1;
                        yf_os_argv_last = environ[i] + size;

                        yf_cpystrn(p, (char *)environ[i], size);
                        environ[i] = (char *)p;
                        p += size;
                }
        }

        yf_os_argv_last--;

        return YF_OK;
}


void
yf_setproctitle(char *title, yf_log_t *log)
{
        char *p;

#if defined (YF_SOLARIS)

        yf_int_t i;
        size_t size;

#endif

        yf_os_argv[1] = NULL;

        p = yf_cpystrn((char *)yf_os_argv[0], (char *)"yifei: ",
                       yf_os_argv_last - yf_os_argv[0]);

        p = yf_cpystrn(p, (char *)title, yf_os_argv_last - (char *)p);

#if defined (YF_SOLARIS)

        size = 0;

        for (i = 0; i < yf_argc; i++)
        {
                size += yf_strlen(yf_argv[i]) + 1;
        }

        if (size > (size_t)((char *)p - yf_os_argv[0]))
        {
                p = yf_cpystrn(p, (char *)" (", yf_os_argv_last - (char *)p);

                for (i = 0; i < yf_argc; i++)
                {
                        p = yf_cpystrn(p, (char *)yf_argv[i],
                                       yf_os_argv_last - (char *)p);
                        p = yf_cpystrn(p, (char *)" ", yf_os_argv_last - (char *)p);
                }

                if (*(p - 1) == ' ')
                {
                        *(p - 1) = ')';
                }
        }

#endif

        if (yf_os_argv_last - (char *)p)
        {
                yf_memset(p, YF_SETPROCTITLE_PAD, yf_os_argv_last - (char *)p);
        }

        yf_log_debug1(YF_LOG_DEBUG, log, 0,
                      "setproctitle: \"%s\"", yf_os_argv[0]);
}

#endif /* YF_SETPROCTITLE_USES_ENV */
