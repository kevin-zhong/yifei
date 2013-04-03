#ifndef  _YF_STRING_H_
#define _YF_STRING_H_

#include "base_struct/yf_core.h"
#include <ppc/yf_header.h>

#define yf_islower(c)   (c >= 'a' && c <= 'z')
#define yf_isupper(c)   (c >= 'A' && c <= 'Z')
#define yf_tolower(c)      tolower(c)
#define yf_toupper(c)      toupper(c)

static void inline yf_strtolower(char *src, size_t n)
{
        while (n)
        {
                *src = yf_tolower(*src);
                *src++;
                n--;
        }
}
static void inline yf_strtoupper(char *src, size_t n)
{
        while (n)
        {
                *src = yf_toupper(*src);
                *src++;
                n--;
        }
}


static void inline yf_strtolower_copy(char *dst, char *src, size_t n)
{
        while (n)
        {
                *dst++ = yf_tolower(*src++);
                n--;
        }
}
static void inline yf_strtoupper_copy(char *dst, char *src, size_t n)
{
        while (n)
        {
                *dst++ = yf_toupper(*src++);
                n--;
        }
}

#define yf_strcpy(dst, src)  strcpy(dst, src)
#define yf_strncpy(dst, src, n) strncpy(dst, src, n)

char * yf_cpystrn(char *dst, char *src, size_t n);

#define yf_strncmp(s1, s2, n)  strncmp((const char *)s1, (const char *)s2, n)
#define yf_strcmp(s1, s2)  strcmp((const char *)s1, (const char *)s2)

#define yf_strstr(s1, s2)  strstr((const char *)s1, (const char *)s2)
#define yf_strlen(s)       strlen((const char *)s)

#define yf_strchr(s1, c)   strchr((const char *)s1, (int)c)
#define yf_strrchr(s1, c)   strrchr((const char *)s1, (int)c)


#ifndef  HAVE_STRCASECMP
yf_int_t yf_strcasecmp(char *s1, char *s2);
#else
#define  yf_strcasecmp(s1, s2)  strcasecmp(s1, s2)
#endif
#ifndef  HAVE_STRNCASECMP
yf_int_t yf_strncasecmp(char *s1, char *s2, size_t n);
#else
#define  yf_strncasecmp(s1, s2, n)  strncasecmp(s1, s2, n)
#endif


#ifdef   HAVE_BZERO
#define yf_memzero(buf, n)       (void)bzero(buf, n)
#else
#define yf_memzero(buf, n)       (void)memset(buf, 0, n)
#endif
#define yf_memset(buf, c, n)     (void)memset(buf, c, n)

#define yf_memzero_st(st)         (void)yf_memzero(&(st), sizeof(st))

#define yf_memcpy(dst, src, n)   (void)memcpy(dst, src, n)
#define yf_cpymem(dst, src, n)   (((char *)memcpy(dst, src, n)) + (n))

#define yf_memmove(dst, src, n)   (void)memmove(dst, src, n)
#define yf_movemem(dst, src, n)   (((char *)memmove(dst, src, n)) + (n))

#define yf_memcmp(s1, s2, n)  memcmp((const char *)s1, (const char *)s2, n)


char *yf_bin_2_hex(char *dst, char *src, size_t len);
ssize_t yf_hex_2_bin(char *dst, char *src, size_t len);

struct  yf_str_s
{
        size_t len;
        char * data;
};


typedef struct
{
        struct  yf_str_s key;
        struct  yf_str_s value;
}
yf_kv_t;


#define yf_str(str)     { sizeof(str) - 1, str }
#define yf_null_str     { 0, NULL }
#define yf_str_set(str, text)  (str)->len = sizeof(text) - 1; (str)->data = text
#define yf_str_null(str)   (str)->len = 0; (str)->data = NULL

char *yf_sprintf(char *buf, const char *fmt, ...);
char *yf_snprintf(char *buf, size_t max, const char *fmt, ...);

char *yf_vslprintf(char *buf, char *last, const char *fmt, va_list args);

#define yf_vsnprintf(buf, max, fmt, args)  yf_vslprintf(buf, buf + (max), fmt, args)

char *yf_sprintf_num(char *buf, char *last, yf_u64_t ui64
                , char zero, yf_uint_t hexadecimal, yf_uint_t width);

#define yf_strncpy2(buf, last, data, len, rlen) \
                rlen = yf_min(last - buf, len); \
                buf = yf_cpymem(buf, data, rlen);

#endif
