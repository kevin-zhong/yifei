#ifndef _YF_CORE_H_INCLUDED_
#define _YF_CORE_H_INCLUDED_

#define  YF_OK          0
#define  YF_ERROR      -1
#define  YF_AGAIN      -2
#define  YF_BUSY       -3
#define  YF_DONE       -4
#define  YF_DECLINED   -5
#define  YF_ABORT      -6

#define YF_MAGIC_VAL 0x5897A7CB

#define yf_set_magic(v) ((v) = YF_MAGIC_VAL)
#define yf_check_magic(v) ((v) == YF_MAGIC_VAL)

#define yf_set_be_magic(tar) \
        yf_set_magic((tar)->begin_flag); \
        yf_set_magic((tar)->end_flag);

#define yf_check_be_magic(tar) (yf_check_magic((tar)->begin_flag) \
        && yf_check_magic((tar)->end_flag))

typedef struct yf_pool_s        yf_pool_t;
typedef struct yf_chain_s       yf_chain_t;
typedef struct yf_log_s         yf_log_t;
typedef struct yf_array_s       yf_array_t;
typedef struct yf_str_s           yf_str_t;
typedef struct yf_node_pool_s  yf_node_pool_t;

#include "yf_string.h"
#include "yf_mem_pool.h"
#include "yf_buf.h"
#include "yf_array.h"
#include "yf_list.h"
#include "yf_hash.h"
#include "yf_rbtree.h"
#include "yf_node_pool.h"
#include "yf_bit_op.h"
#include "yf_log.h"


#define yf_min(a, b)   ((a < b) ? (a) : (b))
#define yf_max(a, b)  ((a > b) ? (a) : (b))

#define YF_ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))


#define yf_value_helper(n)   #n
#define yf_value(n)          yf_value_helper(n)


#ifndef  offsetof
#define offsetof(type, member) ((size_t) &((type *)0)->member)
#endif

#ifndef  container_of
#define container_of(ptr, type, member) ({          \
                                                 const typeof(((type *)0)->member) * __mptr = (ptr);    \
                                                 (type *)((char *)__mptr - offsetof(type, member)); })
#endif


#define CHECK_RV(line, rv) { if (line) return rv; }
#define CHECK_OK(line) {yf_int_t __ret = (line); if (__ret != YF_OK) return __ret;}

#endif
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            /* _YF_CORE_H_INCLUDED_ */
