#include <ppc/yf_header.h>
#include <base_struct/yf_core.h>

yf_bit_index_t  yf_bit_indexs[65536];

inline yf_uint_t yf_bit_cnt(yf_u64_t val)
{
        yf_uint_t cnt = 0;
        while (val)
        {
                ++cnt;
                val >>= 1;
        }
        return cnt;
}

inline yf_u64_t yf_align_2pow(yf_u64_t val)
{
        yf_uint_t bits = yf_bit_cnt(val);
        if (yf_bitcnt2val(bits) < val)
                ++bits;
        
        return yf_bitcnt2val(bits);        
}


void yf_init_bit_indexs()
{
        static yf_int_t  inited = 0;
        if (inited)
                return;
        inited = 1;
        yf_u32_t i, bit_val;

        for (i = 0; i < YF_ARRAY_SIZE(yf_bit_indexs); ++i)
        {
                yf_s8_t  index = 0, offset = 0;
                yf_s8_t *indexs = yf_bit_indexs[i].indexs;
                
                for (bit_val = i; bit_val; bit_val >>= 1, ++index)
                {
                        if (!yf_test_bit(bit_val, 0))
                                continue;

                        indexs[offset++] = index;
                }

                indexs[offset] = YF_END_INDEX;
        }
}


void yf_get_set_bits(yf_bit_set_t* bit_set, yf_set_bits set_bits)
{
        yf_int_t  i1, i2;
        yf_int_t  offset = 0, begin_index;
        yf_u16_t  test_val;
        yf_s8_t*  unempty_indexs;

#define  _YF_TEST_BIT16(_first_part, _next_part) \
        test_val = bit_set->ubits[_first_part].bit_16[_next_part]; \
        if (test_val) { \
                begin_index = (YF_IS_LT_PART(_first_part) ? 0 : 32) \
                        + (YF_IS_LT_PART(_next_part) ? 0 : 16); \
                ; \
                unempty_indexs = yf_bit_indexs[test_val].indexs; \
                for (i2 = 0; !yf_index_end(unempty_indexs, i2); ++i2) \
                        set_bits[offset++] = begin_index + unempty_indexs[i2]; \
        }

        if (bit_set->bit_64)
        {
#define  _YF_TEST_BIT32(_first_part) \
                if (bit_set->ubits[_first_part].bit_32) \
                { \
                        _YF_TEST_BIT16(_first_part, WORDS_LITTLE_PART); \
                        _YF_TEST_BIT16(_first_part, 1 - WORDS_LITTLE_PART); \
                }
                _YF_TEST_BIT32(WORDS_LITTLE_PART);
                _YF_TEST_BIT32(1 - WORDS_LITTLE_PART);
        }

        set_bits[offset] = YF_END_INDEX;
}

