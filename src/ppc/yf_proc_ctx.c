#include <ppc/yf_header.h>
#include <base_struct/yf_core.h>

int yf_argc;
char **yf_argv;
char **yf_os_argv;
char **yf_os_environ;

yf_int_t
yf_save_argv(yf_log_t* log, int argc, char *const *argv)
{
#if (YF_FREEBSD)
        yf_os_argv = (char **)argv;
        yf_argc = argc;
        yf_argv = (char **)argv;

#else
        size_t len;
        yf_int_t i;

        yf_os_argv = (char **)argv;
        yf_argc = argc;

        yf_argv = yf_alloc((argc + 1) * sizeof(char *));
        if (yf_argv == NULL)
        {
                return YF_ERROR;
        }

        for (i = 0; i < argc; i++)
        {
                len = yf_strlen(argv[i]) + 1;

                yf_argv[i] = yf_alloc(len);
                if (yf_argv[i] == NULL)
                {
                        return YF_ERROR;
                }

                (void)yf_cpystrn(yf_argv[i], argv[i], len);
        }

        yf_argv[i] = NULL;

#endif

        yf_os_environ = environ;

        return YF_OK;
}


