#ifndef  _YF_BIT_OP_H
#define _YF_BIT_OP_H

#include <base_struct/yf_core.h>

#define yf_align(d, a)     (((d) + ((a) - 1)) & ~((a) - 1))
#define yf_align_ptr(p, a)  \
        (char *)(((yf_uint_ptr_t)(p) + ((yf_uint_ptr_t)a - 1)) & ~((yf_uint_ptr_t)a - 1))

#define yf_set_bit(val, index) ((val) |= ((yf_u64_t)1 << (index)))
#define yf_reset_bit(val, index) ((val) &= ~((yf_u64_t)1 << (index)))
#define yf_revert_bit(val, index) ((val) ^= ((yf_u64_t)1 << (index)))
#define yf_test_bit(val, index) ((val) & ((yf_u64_t)1 << (index)))


#define  yf_mod(val, mod) ((val) & ((mod)-1))

#define  yf_cicur_add(org_val, add_val, cicur_size) yf_mod((org_val) + (add_val), (cicur_size))


#endif

