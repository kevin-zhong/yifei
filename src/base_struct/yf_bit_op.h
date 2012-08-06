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


#ifdef  WORDS_BIGENDIAN
#define  WORDS_LITTLE_PART  1
#else
#define  WORDS_LITTLE_PART  0
#endif

#define  YF_IS_LT_PART(p) (p == WORDS_LITTLE_PART)

typedef  struct
{
        yf_s8_t  indexs[16];
}
yf_bit_index_t;

#define  YF_END_INDEX  -1
#define  yf_index_end(bit_indexs, i) (i == 16 || bit_indexs[i] == YF_END_INDEX)

extern yf_bit_index_t  yf_bit_indexs[65536];
extern void yf_init_bit_indexs();


typedef  union
{
        yf_u64_t  bit_64;
        union {
                yf_u32_t  bit_32;
                yf_u16_t  bit_16[2];
        } ubits[2];
}
yf_bit_set_t;

#define YF_BITS_CNT 64

typedef  yf_s8_t  yf_set_bits[YF_BITS_CNT];

extern  void yf_get_set_bits(yf_bit_set_t* bit_set, yf_set_bits set_bits);


#endif

