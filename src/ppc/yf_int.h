#ifndef  _YF_INT_H__
#define _YF_INT_H__

#define _HAVE_INT_TYPES_

#if defined  HAVE_STDINT_H
#include <stdint.h>
#elif defined  HAVE_INTTYPES_H
#include <inttypes.h>
#else
#undef  _HAVE_INT_TYPES_
#endif

#ifdef _HAVE_INT_TYPES_

typedef int8_t yf_s8_t;
typedef uint8_t yf_u8_t;
typedef int16_t yf_s16_t;
typedef uint16_t yf_u16_t;
typedef int32_t yf_s32_t;
typedef uint32_t yf_u32_t;
typedef int64_t yf_s64_t;
typedef uint64_t yf_u64_t;

#else

typedef signed char yf_s8_t;
typedef unsigned char yf_u8_t;
typedef short int yf_s16_t;
typedef unsigned short int yf_u16_t;
typedef int yf_s32_t;
typedef unsigned int yf_u32_t;

#if SIZEOF_LONG == 4
typedef long long int yf_s64_t;
typedef unsigned long long int yf_u64_t;
#elif SIZEOF_LONG == 8
typedef long int yf_s64_t;
typedef unsigned long int yf_u64_t;
#else
#error too old operating system
#endif

#endif  /*_HAVE_INT_TYPES_*/

#if SIZEOF_LONG == 4
typedef yf_u32_t   yf_uint_ptr_t;
#elif SIZEOF_LONG == 8
typedef yf_u64_t   yf_uint_ptr_t;
#endif

#ifndef INT8_MIN
#define INT8_MIN               (-128)
#endif
#ifndef INT16_MIN
#define INT16_MIN              (-32767 - 1)
#endif
#ifndef INT32_MIN
#define INT32_MIN              (-2147483647 - 1)
#endif
#ifndef INT8_MAX
#define INT8_MAX               (127)
#endif
#ifndef INT16_MAX
#define INT16_MAX              (32767)
#endif
#ifndef INT32_MAX
#define INT32_MAX              (2147483647)
#endif
#ifndef UINT8_MAX
#define UINT8_MAX              (255U)
#endif
#ifndef UINT16_MAX
#define UINT16_MAX             (65535U)
#endif
#ifndef UINT32_MAX
#define UINT32_MAX             (4294967295U)
#endif

#define YF_INT16_LEN   sizeof("-32768") - 1
#define YF_INT32_LEN   sizeof("-2147483648") - 1
#define YF_INT64_LEN   sizeof("-9223372036854775808") - 1


#define  YF_SIZEOF_PTR  SIZEOF_VOID_P

typedef  int yf_int_t;
typedef  unsigned int yf_uint_t;
typedef  unsigned char  yf_uchar_t;

typedef int  yf_err_t;

#endif
