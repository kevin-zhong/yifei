#include <ppc/yf_header.h>
#include <base_struct/yf_core.h>

char *
yf_cpystrn(char *dst, char *src, size_t n)
{
        if (n == 0)
        {
                return dst;
        }

        while (--n)
        {
                *dst = *src;

                if (*dst == '\0')
                {
                        return dst;
                }

                dst++;
                src++;
        }
        *dst = '\0';

        return dst;
}


#ifndef  HAVE_STRCASECMP
yf_int_t yf_strcasecmp(char *s1, char *s2)
{
        char c1, c2;

        for (;; )
        {
                c1 = *s1++;
                c2 = *s2++;
                c1 = yf_tolower(c1);
                c2 = yf_tolower(c2);

                if (c1 == c2)
                {
                        if (c1)
                                continue;
                        return 0;
                }

                return c1 - c2;
        }
}
#endif


#ifndef  HAVE_STRNCASECMP
yf_int_t yf_strncasecmp(char *s1, char *s2, size_t n)
{
        char c1, c2;

        while (n)
        {
                c1 = *s1++;
                c2 = *s2++;
                c1 = yf_tolower(c1);
                c2 = yf_tolower(c2);

                if (c1 == c2)
                {
                        if (c1)
                        {
                                n--;
                                continue;
                        }
                        return 0;
                }
                return c1 - c2;
        }
        return 0;
}
#endif

char *yf_bin_2_hex(char *dst, char *src, size_t len)
{
        static char hex[] = "0123456789abcdef";

        while (len--)
        {
                *dst++ = hex[*src >> 4];
                *dst++ = hex[*src++ & 0xf];
        }

        return dst;
}

ssize_t yf_hex_2_bin(char *dst, char *src, size_t len)
{
        if (len & 1)
                return -1;

        ssize_t cnt = 0;
        for (; len; len -= 2)
        {
                *dst = (*src++ << 4);
                *dst++ |= (*src++);
                ++cnt;
        }
        return cnt;
}


//%l have a bug... add on 20130223 04:40
/*
 * supported formats:
 *    %[0][width][x][X]O        off_t
 *    %[0][width]T              time_t
 *    %[0][width][u][x|X]z      ssize_t/size_t
 *    %[0][width][u][x|X]d      int/u_int
 *    %[0][width][u][x|X]l      long
 *    %[0][width|m][u][x|X]i    yf_int_t/yf_uint_t
 *    %[0][width][u][x|X]D      yf_s32_t/yf_u32_t
 *    %[0][width][u][x|X]L      yf_s64_t/yf_u64_t
 *    %[0][width][.width]f      double, max valid number fits to %18.15f
 *    %p                        void *
 *    %V                        yf_str_t *
 *    %s                        null-terminated string
 *    %*s                       length and string
 *    %Z                        '\0'
 *    %N                        '\n'
 *    %c                        char
 *    %%                        %
 *
 */
char *yf_sprintf(char *buf, const char *fmt, ...)
{
        char *p;
        va_list args;

        va_start(args, fmt);
        p = yf_vslprintf(buf, (void *)-1, fmt, args);
        va_end(args);

        return p;
}


char *yf_snprintf(char *buf, size_t max, const char *fmt, ...)
{
        char *p;
        va_list args;

        va_start(args, fmt);
        p = yf_vslprintf(buf, buf + max, fmt, args);
        va_end(args);

        return p;
}


char *
yf_vslprintf(char *buf, char *last, const char *fmt, va_list args)
{
        char *p, zero;
        int d;
        double f, scale;
        size_t len, slen;
        yf_s64_t i64 = 0;
        yf_u64_t ui64;
        yf_uint_t width, sign, hex, max_width, frac_width, n;
        yf_str_t *v;

        while (*fmt && buf < last)
        {
                if (*fmt == '%')
                {
                        i64 = 0;
                        ui64 = 0;

                        zero = (char)((*++fmt == '0') ? '0' : ' ');
                        width = 0;
                        sign = 1;
                        hex = 0;
                        max_width = 0;
                        frac_width = 0;
                        slen = (size_t)-1;

                        while (*fmt >= '0' && *fmt <= '9')
                                width = width * 10 + *fmt++ - '0';


                        for (;; )
                        {
                                switch (*fmt)
                                {
                                case 'u':
                                        sign = 0;
                                        fmt++;
                                        continue;

                                case 'm':
                                        max_width = 1;
                                        fmt++;
                                        continue;

                                case 'X':
                                        hex = 2;
                                        sign = 0;
                                        fmt++;
                                        continue;

                                case 'x':
                                        hex = 1;
                                        sign = 0;
                                        fmt++;
                                        continue;

                                case '.':
                                        fmt++;

                                        while (*fmt >= '0' && *fmt <= '9')
                                                frac_width = frac_width * 10 + *fmt++ - '0';

                                        break;

                                case '*':
                                        slen = va_arg(args, size_t);
                                        fmt++;
                                        continue;

                                default:
                                        break;
                                }

                                break;
                        }


                        switch (*fmt)
                        {
                        case 'V':
                                v = va_arg(args, yf_str_t *);

                                len = yf_min(((size_t)(last - buf)), v->len);
                                buf = yf_cpymem(buf, v->data, len);
                                fmt++;

                                continue;
                        case 's':
                                p = va_arg(args, char *);

                                if (slen == (size_t)-1)
                                {
                                        while (*p && buf < last)
                                                *buf++ = *p++;
                                }
                                else {
                                        len = yf_min(((size_t)(last - buf)), slen);
                                        buf = yf_cpymem(buf, p, len);
                                }

                                fmt++;

                                continue;

                        case 'O':
                                i64 = (yf_s64_t)va_arg(args, off_t);
                                sign = 1;
                                break;

                        case 'P':
                                i64 = (yf_s64_t)va_arg(args, yf_pid_t);
                                sign = 1;
                        break;

                        case 'T':
                                i64 = (yf_s64_t)va_arg(args, time_t);
                                sign = 1;
                                break;

                        case 'z':
                                if (sign)
                                {
                                        i64 = (yf_s64_t)va_arg(args, ssize_t);
                                }
                                else {
                                        ui64 = (yf_u64_t)va_arg(args, size_t);
                                }
                                break;

                        case 'i':
                                if (sign)
                                {
                                        i64 = (yf_s64_t)va_arg(args, int);
                                }
                                else {
                                        ui64 = (yf_u64_t)va_arg(args, unsigned int);
                                }

                                if (max_width)
                                {
                                        width = YF_INT32_LEN;
                                }

                                break;

                        case 'd':
                                if (sign)
                                {
                                        i64 = (yf_s64_t)va_arg(args, yf_int_t);
                                }
                                else {
                                        ui64 = (yf_u64_t)va_arg(args, yf_uint_t);
                                }
                                break;

                        case 'l':
                                if (sign)
                                {
                                        i64 = (yf_s64_t)va_arg(args, long);
                                }
                                else {
                                        ui64 = (yf_u64_t)va_arg(args, unsigned long);
                                }
                                break;

                        case 'D':
                                if (sign)
                                {
                                        i64 = (yf_s64_t)va_arg(args, yf_s32_t);
                                }
                                else {
                                        ui64 = (yf_u64_t)va_arg(args, yf_u32_t);
                                }
                                break;

                        case 'L':
                                if (sign)
                                {
                                        i64 = va_arg(args, yf_s64_t);
                                }
                                else {
                                        ui64 = va_arg(args, yf_u64_t);
                                }
                                break;

                        case 'f':
                                f = va_arg(args, double);

                                if (f < 0)
                                {
                                        *buf++ = '-';
                                        f = -f;
                                }

                                ui64 = (yf_s64_t)f;

                                buf = yf_sprintf_num(buf, last, ui64, zero, 0, width);

                                if (frac_width)
                                {
                                        if (buf < last)
                                        {
                                                *buf++ = '.';
                                        }

                                        scale = 1.0;

                                        for (n = frac_width; n; n--)
                                        {
                                                scale *= 10.0;
                                        }

                                        ui64 = (yf_u64_t)((f - (yf_s64_t)ui64) * scale + 0.5);

                                        buf = yf_sprintf_num(buf, last, ui64, '0', 0, frac_width);
                                }

                                fmt++;

                                continue;

                        case 'p':
                                ui64 = (yf_u64_t)va_arg(args, void *);
                                hex = 2;
                                sign = 0;
                                zero = '0';
                                width = YF_SIZEOF_PTR * 2;
                                break;

                        case 'c':
                                d = va_arg(args, int);
                                *buf++ = (char)(d & 0xff);
                                fmt++;

                                continue;

                        case 'Z':
                                *buf++ = '\0';
                                fmt++;

                                continue;

                        case 'N':
                                *buf++ = '\n';
                                fmt++;

                                continue;

                        case '%':
                                *buf++ = '%';
                                fmt++;

                                continue;

                        default:
                                *buf++ = *fmt++;

                                continue;
                        }

                        if (sign)
                        {
                                if (i64 < 0)
                                {
                                        *buf++ = '-';
                                        ui64 = (yf_u64_t)-i64;
                                }
                                else {
                                        ui64 = (yf_u64_t)i64;
                                }
                        }

                        buf = yf_sprintf_num(buf, last, ui64, zero, hex, width);

                        fmt++;
                }
                else {
                        *buf++ = *fmt++;
                }
        }

        return buf;
}


char * yf_sprintf_num(char *buf, char *last, yf_u64_t ui64
        , char zero, yf_uint_t hexadecimal, yf_uint_t width)
{
        char *p, temp[YF_INT64_LEN + 1];

        size_t len;
        yf_u32_t ui32;
        static char hex[] = "0123456789abcdef";
        static char HEX[] = "0123456789ABCDEF";

        p = temp + YF_INT64_LEN;

        if (hexadecimal == 0)
        {
                if (ui64 <= UINT32_MAX)
                {
                        ui32 = (yf_u32_t)ui64;

                        do
                                *--p = (char)(ui32 % 10 + '0');
                        while (ui32 /= 10);
                }
                else {
                        do
                                *--p = (char)(ui64 % 10 + '0');
                        while (ui64 /= 10);
                }
        }
        else if (hexadecimal == 1)
        {
                do
                        *--p = hex[(yf_u32_t)(ui64 & 0xf)];
                while (ui64 >>= 4);
        }
        else {  /* hexadecimal == 2 */
                do
                        *--p = HEX[(yf_u32_t)(ui64 & 0xf)];
                while (ui64 >>= 4);
        }

        /* zero or space padding */
        len = (temp + YF_INT64_LEN) - p;

        while (len++ < width && buf < last)
                *buf++ = zero;

        /* number safe copy */

        len = (temp + YF_INT64_LEN) - p;

        if (buf + len > last)
        {
                len = last - buf;
        }

        return yf_cpymem(buf, p, len);
}
