/*
 * Copyright (C) 2015 Imagination Technologies Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdlib.h>
#include <stdint.h>

#include "avc_types.h"
#include "common_macros_msa.h"

#ifdef H264ENC_MSA
static inline uint8_t clip_pixel(int i32Val)
{
    return ((i32Val) > 255) ? 255u : ((i32Val) < 0) ? 0u : (i32Val);
}

#define ROUND_POWER_OF_TWO(value, n)  (((value) + (1 << ((n) - 1))) >> (n))

#define AVC_CALC_DPADD_H_6PIX_2COEFF_SH(in0, in1, in2, in3, in4, in5)    \
( {                                                                      \
    v4i32 tmp0_m, tmp1_m;                                                \
    v8i16 out0_m, out1_m, out2_m, out3_m;                                \
    v8i16 minus5h_m = __msa_ldi_h(-5);                                   \
    v8i16 plus20h_m = __msa_ldi_h(20);                                   \
                                                                         \
    ILVRL_H2_SW(in5, in0, tmp0_m, tmp1_m);                               \
                                                                         \
    tmp0_m = __msa_hadd_s_w((v8i16) tmp0_m, (v8i16) tmp0_m);             \
    tmp1_m = __msa_hadd_s_w((v8i16) tmp1_m, (v8i16) tmp1_m);             \
                                                                         \
    ILVRL_H2_SH(in1, in4, out0_m, out1_m);                               \
    DPADD_SH2_SW(out0_m, out1_m, minus5h_m, minus5h_m, tmp0_m, tmp1_m);  \
    ILVRL_H2_SH(in2, in3, out2_m, out3_m);                               \
    DPADD_SH2_SW(out2_m, out3_m, plus20h_m, plus20h_m, tmp0_m, tmp1_m);  \
                                                                         \
    SRARI_W2_SW(tmp0_m, tmp1_m, 10);                                     \
    SAT_SW2_SW(tmp0_m, tmp1_m, 7);                                       \
    out0_m = __msa_pckev_h((v8i16) tmp1_m, (v8i16) tmp0_m);              \
                                                                         \
    out0_m;                                                              \
} )

#define AVC_HORZ_FILTER_SH(in, mask0, mask1, mask2)     \
( {                                                     \
    v8i16 out0_m, out1_m;                               \
    v16i8 tmp0_m, tmp1_m;                               \
    v16i8 minus5b = __msa_ldi_b(-5);                    \
    v16i8 plus20b = __msa_ldi_b(20);                    \
                                                        \
    tmp0_m = __msa_vshf_b((v16i8) mask0, in, in);       \
    out0_m = __msa_hadd_s_h(tmp0_m, tmp0_m);            \
                                                        \
    tmp0_m = __msa_vshf_b((v16i8) mask1, in, in);       \
    out0_m = __msa_dpadd_s_h(out0_m, minus5b, tmp0_m);  \
                                                        \
    tmp1_m = __msa_vshf_b((v16i8) (mask2), in, in);     \
    out1_m = __msa_dpadd_s_h(out0_m, plus20b, tmp1_m);  \
                                                        \
    out1_m;                                             \
} )

#define AVC_LUME_FILT_ELE(ele0, ele1, ele2, ele3, ele4, ele5,    \
                          out0, out1)                            \
{                                                                \
    int32 ele0_sum_ele5, ele1_sum_ele4, ele2_sum_ele3;         \
                                                                 \
    ele0_sum_ele5 = ele0 + ele5;                                 \
    ele1_sum_ele4 = ele1 + ele4;                                 \
    ele2_sum_ele3 = ele2 + ele3;                                 \
                                                                 \
    ele1_sum_ele4 = 5 * ele1_sum_ele4;                           \
    ele2_sum_ele3 = 20 * ele2_sum_ele3;                          \
    ele0_sum_ele5 += ele2_sum_ele3;                              \
                                                                 \
    out0 = (ele0_sum_ele5 & 0xffff) - (ele1_sum_ele4 & 0xffff);  \
    out1 = (ele0_sum_ele5 >> 16) - (ele1_sum_ele4 >> 16);        \
                                                                 \
    out0 = ROUND_POWER_OF_TWO(out0, 5);                          \
    out1 = ROUND_POWER_OF_TWO(out1, 5);                          \
}

void avc_calc_qpel_data_msa(uint8 **pref_data, uint8 *qpel_buf,
                            uint8 hpel_pos)
{
    uint8 row_cnt;
    uint8 *qpel_buf1, *qpel_buf2, *qpel_buf3, *qpel_buf4;
    uint8 *qpel_buf5, *qpel_buf6, *qpel_buf7, *qpel_buf8;
    uint8 *top_left_ptr, *top_right_ptr;
    uint8 *bottom_left_ptr, *bottom_right_ptr;
    v16u8 top_left, top_left_plus_one, top_left_next_line;
    v16u8 top_left_next_line_plus_one;
    v16u8 top_right, top_right_next_line;
    v16u8 bottom_left, bottom_left_plus_one, bottom_right;
    v16u8 qpel_cand1, qpel_cand2, qpel_cand3, qpel_cand4;
    v16u8 qpel_cand5, qpel_cand6, qpel_cand7, qpel_cand8;

    qpel_buf1 = qpel_buf;
    qpel_buf2 = qpel_buf + 1 * 384;
    qpel_buf3 = qpel_buf + 2 * 384;
    qpel_buf4 = qpel_buf + 3 * 384;
    qpel_buf5 = qpel_buf + 4 * 384;
    qpel_buf6 = qpel_buf + 5 * 384;
    qpel_buf7 = qpel_buf + 6 * 384;
    qpel_buf8 = qpel_buf + 7 * 384;

    top_left_ptr = pref_data[0];
    top_right_ptr = pref_data[1];
    bottom_left_ptr = pref_data[2];
    bottom_right_ptr = pref_data[3];

    // diamond pattern
    if (!(hpel_pos & 1))
    {
        for (row_cnt = 8; row_cnt--;)
        {
            top_right = LD_UB(top_right_ptr);
            top_right_ptr += 24;
            top_right_next_line = LD_UB(top_right_ptr);
            bottom_left = LD_UB(bottom_left_ptr);
            bottom_left_plus_one = LD_UB(bottom_left_ptr + 1);
            bottom_right = LD_UB(bottom_right_ptr);

            AVER_UB4_UB(bottom_right, top_right, bottom_left_plus_one,
                        top_right, bottom_left_plus_one, bottom_right,
                        bottom_left_plus_one,  top_right_next_line,
                        qpel_cand1, qpel_cand2, qpel_cand3, qpel_cand4);
            AVER_UB4_UB(bottom_right, top_right_next_line, bottom_left,
                        top_right_next_line, bottom_left, bottom_right,
                        bottom_left, top_right, qpel_cand5, qpel_cand6,
                        qpel_cand7, qpel_cand8);

            bottom_right_ptr += 24;
            bottom_left_ptr += 24;

            top_right = LD_UB(top_right_ptr);
            top_right_ptr += 24;
            top_right_next_line = LD_UB(top_right_ptr);
            bottom_left = LD_UB(bottom_left_ptr);
            bottom_left_plus_one = LD_UB(bottom_left_ptr + 1);
            bottom_right = LD_UB(bottom_right_ptr);

            ST_UB(qpel_cand1, qpel_buf1);
            ST_UB(qpel_cand2, qpel_buf2);
            ST_UB(qpel_cand3, qpel_buf3);
            ST_UB(qpel_cand4, qpel_buf4);
            ST_UB(qpel_cand5, qpel_buf5);
            ST_UB(qpel_cand6, qpel_buf6);
            ST_UB(qpel_cand7, qpel_buf7);
            ST_UB(qpel_cand8, qpel_buf8);

            qpel_buf1 += 24;
            qpel_buf2 += 24;
            qpel_buf3 += 24;
            qpel_buf4 += 24;
            qpel_buf5 += 24;
            qpel_buf6 += 24;
            qpel_buf7 += 24;
            qpel_buf8 += 24;

            AVER_UB4_UB(bottom_right, top_right, bottom_left_plus_one,
                        top_right, bottom_left_plus_one, bottom_right,
                        bottom_left_plus_one,  top_right_next_line,
                        qpel_cand1, qpel_cand2, qpel_cand3, qpel_cand4);
            AVER_UB4_UB(bottom_right, top_right_next_line, bottom_left,
                        top_right_next_line, bottom_left, bottom_right,
                        bottom_left, top_right, qpel_cand5, qpel_cand6,
                        qpel_cand7, qpel_cand8);

            ST_UB(qpel_cand1, qpel_buf1);
            ST_UB(qpel_cand2, qpel_buf2);
            ST_UB(qpel_cand3, qpel_buf3);
            ST_UB(qpel_cand4, qpel_buf4);
            ST_UB(qpel_cand5, qpel_buf5);
            ST_UB(qpel_cand6, qpel_buf6);
            ST_UB(qpel_cand7, qpel_buf7);
            ST_UB(qpel_cand8, qpel_buf8);

            qpel_buf1 += 24;
            qpel_buf2 += 24;
            qpel_buf3 += 24;
            qpel_buf4 += 24;
            qpel_buf5 += 24;
            qpel_buf6 += 24;
            qpel_buf7 += 24;
            qpel_buf8 += 24;
            bottom_right_ptr += 24;
            bottom_left_ptr += 24;
        }
    }
    // star pattern
    else
    {
        for (row_cnt = 8; row_cnt--;)
        {
            top_left = LD_UB(top_left_ptr);
            top_left_plus_one = LD_UB(top_left_ptr + 1);
            top_left_ptr += 24;
            top_left_next_line = LD_UB(top_left_ptr);
            top_left_next_line_plus_one = LD_UB(top_left_ptr + 1);
            top_right = LD_UB(top_right_ptr);
            top_right_ptr += 24;
            top_right_next_line = LD_UB(top_right_ptr);
            bottom_left = LD_UB(bottom_left_ptr);
            bottom_left_plus_one = LD_UB(bottom_left_ptr + 1);
            bottom_right = LD_UB(bottom_right_ptr);

            AVER_UB4_UB(bottom_right, top_right, bottom_right,
                        top_left_plus_one, bottom_right, bottom_left_plus_one,
                        bottom_right, top_left_next_line_plus_one,
                        qpel_cand1, qpel_cand2, qpel_cand3, qpel_cand4);
            AVER_UB4_UB(bottom_right, top_right_next_line, bottom_right,
                        top_left_next_line, bottom_right, bottom_left,
                        bottom_right, top_left, qpel_cand5, qpel_cand6,
                        qpel_cand7, qpel_cand8);

            bottom_right_ptr += 24;
            bottom_left_ptr += 24;

            top_left = LD_UB(top_left_ptr);
            top_left_plus_one = LD_UB(top_left_ptr + 1);
            top_left_ptr += 24;
            top_left_next_line = LD_UB(top_left_ptr);
            top_left_next_line_plus_one = LD_UB(top_left_ptr + 1);
            top_right = LD_UB(top_right_ptr);
            top_right_ptr += 24;
            top_right_next_line = LD_UB(top_right_ptr);
            bottom_left = LD_UB(bottom_left_ptr);
            bottom_left_plus_one = LD_UB(bottom_left_ptr + 1);
            bottom_right = LD_UB(bottom_right_ptr);

            ST_UB(qpel_cand1, qpel_buf1);
            ST_UB(qpel_cand2, qpel_buf2);
            ST_UB(qpel_cand3, qpel_buf3);
            ST_UB(qpel_cand4, qpel_buf4);
            ST_UB(qpel_cand5, qpel_buf5);
            ST_UB(qpel_cand6, qpel_buf6);
            ST_UB(qpel_cand7, qpel_buf7);
            ST_UB(qpel_cand8, qpel_buf8);

            qpel_buf1 += 24;
            qpel_buf2 += 24;
            qpel_buf3 += 24;
            qpel_buf4 += 24;
            qpel_buf5 += 24;
            qpel_buf6 += 24;
            qpel_buf7 += 24;
            qpel_buf8 += 24;

            AVER_UB4_UB(bottom_right, top_right, bottom_right,
                        top_left_plus_one, bottom_right, bottom_left_plus_one,
                        bottom_right, top_left_next_line_plus_one,
                        qpel_cand1, qpel_cand2, qpel_cand3, qpel_cand4);
            AVER_UB4_UB(bottom_right, top_right_next_line, bottom_right,
                        top_left_next_line, bottom_right, bottom_left,
                        bottom_right, top_left, qpel_cand5, qpel_cand6,
                        qpel_cand7, qpel_cand8);

            ST_UB(qpel_cand1, qpel_buf1);
            ST_UB(qpel_cand2, qpel_buf2);
            ST_UB(qpel_cand3, qpel_buf3);
            ST_UB(qpel_cand4, qpel_buf4);
            ST_UB(qpel_cand5, qpel_buf5);
            ST_UB(qpel_cand6, qpel_buf6);
            ST_UB(qpel_cand7, qpel_buf7);
            ST_UB(qpel_cand8, qpel_buf8);

            qpel_buf1 += 24;
            qpel_buf2 += 24;
            qpel_buf3 += 24;
            qpel_buf4 += 24;
            qpel_buf5 += 24;
            qpel_buf6 += 24;
            qpel_buf7 += 24;
            qpel_buf8 += 24;
            bottom_right_ptr += 24;
            bottom_left_ptr += 24;
        }
    }
    return;
}

void avc_horiz_filter_6taps_17width_msa(uint8 *src, int32 src_stride,
                                        uint8 *dst, int32 dst_stride,
                                        int32 height)
{
    uint32 loop_cnt;
    int32 filt_sum0, filt_sum1;
    v16i8 src0, src1, src2, src3;
    v16i8 mask0 = { 0, 5, 1, 6, 2, 7, 3, 8, 4, 9, 5, 10, 6, 11, 7, 12 };
    v16i8 mask1 = { 1, 4, 2, 5, 3, 6, 4, 7, 5, 8, 6, 9, 7, 10, 8, 11 };
    v16i8 mask2 = { 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10 };
    v16i8 filt = { 0, 0, 0, 0, 0, 0, 0, 0, 1, -5, 20, 20, -5, 1, 0, 0 };
    v8i16 dst0, dst1, dst2, dst3, dst4, dst5;
    v16i8 vec0, vec1, vec2, vec3, vec4, vec5, vec6, vec7, vec8, vec9, vec10;
    v16i8 vec11;
    v16i8 minus5b = __msa_ldi_b(-5);
    v16i8 plus20b = __msa_ldi_b(20);

    for (loop_cnt = (height >> 2); loop_cnt--;)
    {
        LD_SB2(src, 8, src0, src1);
        src += src_stride;
        LD_SB2(src, 8, src2, src3);
        src += src_stride;

        XORI_B4_128_SB(src0, src1, src2, src3);
        VSHF_B2_SB(src0, src0, src1, src1, mask0, mask0, vec0, vec1);
        VSHF_B2_SB(src2, src2, src3, src3, mask0, mask0, vec2, vec3);
        VSHF_B2_SB(src0, src0, src1, src1, mask1, mask1, vec4, vec5);
        VSHF_B2_SB(src2, src2, src3, src3, mask1, mask1, vec6, vec7);
        VSHF_B2_SB(src0, src0, src1, src1, mask2, mask2, vec8, vec9);
        VSHF_B2_SB(src2, src2, src3, src3, mask2, mask2, vec10, vec11);
        HADD_SB4_SH(vec0, vec1, vec2, vec3, dst0, dst1, dst2, dst3);
        DPADD_SB4_SH(vec4, vec5, vec6, vec7, minus5b, minus5b, minus5b, minus5b,
                     dst0, dst1, dst2, dst3);
        DPADD_SB4_SH(vec8, vec9, vec10, vec11, plus20b, plus20b, plus20b,
                     plus20b, dst0, dst1, dst2, dst3);
        SRARI_H4_SH(dst0, dst1, dst2, dst3, 5);
        SAT_SH4_SH(dst0, dst1, dst2, dst3, 7);
        PCKEV_B2_SH(dst1, dst0, dst3, dst2, dst0, dst2);
        XORI_B2_128_SH(dst0, dst2);

        /* do the 17th column here */
        DOTP_SB2_SH(src1, src3, filt, filt, dst4, dst5);

        filt_sum0 = __msa_copy_s_h(dst4, 4);
        filt_sum1 = __msa_copy_s_h(dst5, 4);
        filt_sum0 += __msa_copy_s_h(dst4, 5);
        filt_sum1 += __msa_copy_s_h(dst5, 5);
        filt_sum0 += __msa_copy_s_h(dst4, 6);
        filt_sum1 += __msa_copy_s_h(dst5, 6);
        filt_sum0 = (filt_sum0 + 16) >> 5;
        filt_sum1 = (filt_sum1 + 16) >> 5;

        if (filt_sum0 > 127)
        {
            filt_sum0 = 127;
        }
        else if (filt_sum0 < -128)
        {
            filt_sum0 = -128;
        }

        if (filt_sum1 > 127)
        {
            filt_sum1 = 127;
        }
        else if (filt_sum1 < -128)
        {
            filt_sum1 = -128;
        }

        filt_sum0 = (uint8) filt_sum0 ^ 128;
        filt_sum1 = (uint8) filt_sum1 ^ 128;

        /* 1st row 1st 8 pixels */
        LD_SB2(src, 8, src0, src1);
        src += src_stride;
        LD_SB2(src, 8, src2, src3);
        src += src_stride;

        ST_UB((v16u8) dst0, dst);
        *(dst + 16) = (uint8) filt_sum0;
        dst += dst_stride;
        ST_UB((v16u8) dst2, dst);
        *(dst + 16) = (uint8) filt_sum1;
        dst += dst_stride;

        XORI_B4_128_SB(src0, src1, src2, src3);
        VSHF_B2_SB(src0, src0, src1, src1, mask0, mask0, vec0, vec1);
        VSHF_B2_SB(src2, src2, src3, src3, mask0, mask0, vec2, vec3);
        VSHF_B2_SB(src0, src0, src1, src1, mask1, mask1, vec4, vec5);
        VSHF_B2_SB(src2, src2, src3, src3, mask1, mask1, vec6, vec7);
        VSHF_B2_SB(src0, src0, src1, src1, mask2, mask2, vec8, vec9);
        VSHF_B2_SB(src2, src2, src3, src3, mask2, mask2, vec10, vec11);
        HADD_SB4_SH(vec0, vec1, vec2, vec3, dst0, dst1, dst2, dst3);
        DPADD_SB4_SH(vec4, vec5, vec6, vec7, minus5b, minus5b, minus5b, minus5b,
                     dst0, dst1, dst2, dst3);
        DPADD_SB4_SH(vec8, vec9, vec10, vec11, plus20b, plus20b, plus20b,
                     plus20b, dst0, dst1, dst2, dst3);
        SRARI_H4_SH(dst0, dst1, dst2, dst3, 5);
        SAT_SH4_SH(dst0, dst1, dst2, dst3, 7);
        PCKEV_B2_SH(dst1, dst0, dst3, dst2, dst0, dst2);
        XORI_B2_128_SH(dst0, dst2);

        /* do the 17th column here */
        DOTP_SB2_SH(src1, src3, filt, filt, dst4, dst5);

        filt_sum0 = __msa_copy_s_h(dst4, 4);
        filt_sum1 = __msa_copy_s_h(dst5, 4);
        filt_sum0 += __msa_copy_s_h(dst4, 5);
        filt_sum1 += __msa_copy_s_h(dst5, 5);
        filt_sum0 += __msa_copy_s_h(dst4, 6);
        filt_sum1 += __msa_copy_s_h(dst5, 6);
        filt_sum0 = (filt_sum0 + 16) >> 5;
        filt_sum1 = (filt_sum1 + 16) >> 5;

        if (filt_sum0 > 127)
        {
            filt_sum0 = 127;
        }
        else if (filt_sum0 < -128)
        {
            filt_sum0 = -128;
        }

        if (filt_sum1 > 127)
        {
            filt_sum1 = 127;
        }
        else if (filt_sum1 < -128)
        {
            filt_sum1 = -128;
        }

        filt_sum0 = (uint8) filt_sum0 ^ 128;
        filt_sum1 = (uint8) filt_sum1 ^ 128;

        ST_UB((v16u8) dst0, dst);
        *(dst + 16) = (uint8) filt_sum0;
        dst += dst_stride;
        ST_UB((v16u8) dst2, dst);
        *(dst + 16) = (uint8) filt_sum1;
        dst += dst_stride;
    }

    LD_SB2(src, 8, src0, src1);
    src += src_stride;
    LD_SB2(src, 8, src2, src3);
    src += src_stride;

    XORI_B4_128_SB(src0, src1, src2, src3);
    VSHF_B2_SB(src0, src0, src1, src1, mask0, mask0, vec0, vec1);
    VSHF_B2_SB(src2, src2, src3, src3, mask0, mask0, vec2, vec3);
    VSHF_B2_SB(src0, src0, src1, src1, mask1, mask1, vec4, vec5);
    VSHF_B2_SB(src2, src2, src3, src3, mask1, mask1, vec6, vec7);
    VSHF_B2_SB(src0, src0, src1, src1, mask2, mask2, vec8, vec9);
    VSHF_B2_SB(src2, src2, src3, src3, mask2, mask2, vec10, vec11);
    HADD_SB4_SH(vec0, vec1, vec2, vec3, dst0, dst1, dst2, dst3);
    DPADD_SB4_SH(vec4, vec5, vec6, vec7, minus5b, minus5b, minus5b, minus5b,
                 dst0, dst1, dst2, dst3);
    DPADD_SB4_SH(vec8, vec9, vec10, vec11, plus20b, plus20b, plus20b,
                 plus20b, dst0, dst1, dst2, dst3);
    SRARI_H4_SH(dst0, dst1, dst2, dst3, 5);
    SAT_SH4_SH(dst0, dst1, dst2, dst3, 7);
    PCKEV_B2_SH(dst1, dst0, dst3, dst2, dst0, dst2);
    XORI_B2_128_SH(dst0, dst2);

    /* do the 17th column here */
    DOTP_SB2_SH(src1, src3, filt, filt, dst4, dst5);

    filt_sum0 = __msa_copy_s_h(dst4, 4);
    filt_sum1 = __msa_copy_s_h(dst5, 4);
    filt_sum0 += __msa_copy_s_h(dst4, 5);
    filt_sum1 += __msa_copy_s_h(dst5, 5);
    filt_sum0 += __msa_copy_s_h(dst4, 6);
    filt_sum1 += __msa_copy_s_h(dst5, 6);
    filt_sum0 = (filt_sum0 + 16) >> 5;
    filt_sum1 = (filt_sum1 + 16) >> 5;

    if (filt_sum0 > 127)
    {
        filt_sum0 = 127;
    }
    else if (filt_sum0 < -128)
    {
        filt_sum0 = -128;
    }

    if (filt_sum1 > 127)
    {
        filt_sum1 = 127;
    }
    else if (filt_sum1 < -128)
    {
        filt_sum1 = -128;
    }

    filt_sum0 = (uint8) filt_sum0 ^ 128;
    filt_sum1 = (uint8) filt_sum1 ^ 128;

    ST_UB((v16u8) dst0, dst);
    *(dst + 16) = (uint8) filt_sum0;
    dst += dst_stride;
    ST_UB((v16u8) dst2, dst);
    *(dst + 16) = (uint8) filt_sum1;
}

void avc_vert_filter_6taps_18width_msa(uint8 *src, int32 src_stride,
                                       uint8 *dst, int32 dst_stride,
                                       int32 height)
{
    int32 loop_cnt;
    int16 filt_const0 = 0xfb01;
    int16 filt_const1 = 0x1414;
    int16 filt_const2 = 0x1fb;
    uint32 ele0, ele1, ele2, ele3, ele4, ele5, ele6, ele7, ele8;
    uint32 tmp;
    int32 out0, out1, out2, out3, out4, out5, out6, out7;
    v16i8 src0, src1, src2, src3, src4, src5, src6, src7, src8;
    v16i8 src10_r, src32_r, src54_r, src76_r, src21_r, src43_r, src65_r;
    v16i8 src87_r, src10_l, src32_l, src54_l, src76_l, src21_l, src43_l;
    v16i8 src65_l, src87_l, filt0, filt1, filt2;
    v8i16 out0_r, out1_r, out2_r, out3_r, out0_l, out1_l, out2_l, out3_l;
    v16u8 res0, res1, res2, res3;

    filt0 = (v16i8) __msa_fill_h(filt_const0);
    filt1 = (v16i8) __msa_fill_h(filt_const1);
    filt2 = (v16i8) __msa_fill_h(filt_const2);

    LD_SB5(src, src_stride, src0, src1, src2, src3, src4);
    XORI_B5_128_SB(src0, src1, src2, src3, src4);
    ILVR_B4_SB(src1, src0, src2, src1, src3, src2, src4, src3,
               src10_r, src21_r, src32_r, src43_r);
    ILVL_B4_SB(src1, src0, src2, src1, src3, src2, src4, src3,
               src10_l, src21_l, src32_l, src43_l);

    ele0 = *(src + 16);
    tmp = *(src + 17);
    tmp = tmp << 16;
    ele0 = ele0 | tmp;
    src += src_stride;

    ele1 = *(src + 16);
    tmp = *(src + 17);
    tmp = tmp << 16;
    ele1 = ele1 | tmp;
    src += src_stride;

    ele2 = *(src + 16);
    tmp = *(src + 17);
    tmp = tmp << 16;
    ele2 = ele2 | tmp;
    src += src_stride;

    ele3 = *(src + 16);
    tmp = *(src + 17);
    tmp = tmp << 16;
    ele3 = ele3 | tmp;
    src += src_stride;

    ele4 = *(src + 16);
    tmp = *(src + 17);
    tmp = tmp << 16;
    ele4 = ele4 | tmp;
    src += src_stride;

    for (loop_cnt = (height >> 2); loop_cnt--;)
    {
        LD_SB4(src, src_stride, src5, src6, src7, src8);

        ele5 = *(src + 16);
        tmp = *(src + 17);
        tmp = tmp << 16;
        ele5 = ele5 | tmp;
        src += src_stride;

        ele6 = *(src + 16);
        tmp = *(src + 17);
        tmp = tmp << 16;
        ele6 = ele6 | tmp;
        src += src_stride;

        ele7 = *(src + 16);
        tmp = *(src + 17);
        tmp = tmp << 16;
        ele7 = ele7 | tmp;
        src += src_stride;

        ele8 = *(src + 16);
        tmp = *(src + 17);
        tmp = tmp << 16;
        ele8 = ele8 | tmp;
        src += src_stride;

        XORI_B4_128_SB(src5, src6, src7, src8);
        ILVR_B4_SB(src5, src4, src6, src5, src7, src6, src8, src7,
                   src54_r, src65_r, src76_r, src87_r);
        ILVL_B4_SB(src5, src4, src6, src5, src7, src6, src8, src7,
                   src54_l, src65_l, src76_l, src87_l);
        out0_r = DPADD_SH3_SH(src10_r, src32_r, src54_r, filt0, filt1, filt2);
        out1_r = DPADD_SH3_SH(src21_r, src43_r, src65_r, filt0, filt1, filt2);
        out2_r = DPADD_SH3_SH(src32_r, src54_r, src76_r, filt0, filt1, filt2);
        out3_r = DPADD_SH3_SH(src43_r, src65_r, src87_r, filt0, filt1, filt2);
        out0_l = DPADD_SH3_SH(src10_l, src32_l, src54_l, filt0, filt1, filt2);
        out1_l = DPADD_SH3_SH(src21_l, src43_l, src65_l, filt0, filt1, filt2);
        out2_l = DPADD_SH3_SH(src32_l, src54_l, src76_l, filt0, filt1, filt2);
        out3_l = DPADD_SH3_SH(src43_l, src65_l, src87_l, filt0, filt1, filt2);
        SRARI_H4_SH(out0_r, out1_r, out2_r, out3_r, 5);
        SRARI_H4_SH(out0_l, out1_l, out2_l, out3_l, 5);
        SAT_SH4_SH(out0_r, out1_r, out2_r, out3_r, 7);
        SAT_SH4_SH(out0_l, out1_l, out2_l, out3_l, 7);
        PCKEV_B4_UB(out0_l, out0_r, out1_l, out1_r, out2_l, out2_r, out3_l,
                    out3_r, res0, res1, res2, res3);
        XORI_B4_128_UB(res0, res1, res2, res3);

        AVC_LUME_FILT_ELE(ele0, ele1, ele2, ele3, ele4, ele5, out0, out1);
        AVC_LUME_FILT_ELE(ele1, ele2, ele3, ele4, ele5, ele6, out2, out3);
        AVC_LUME_FILT_ELE(ele2, ele3, ele4, ele5, ele6, ele7, out4, out5);
        AVC_LUME_FILT_ELE(ele3, ele4, ele5, ele6, ele7, ele8, out6, out7);

        ST_UB(res0, dst);
        dst[16] = clip_pixel(out0);
        dst[17] = clip_pixel(out1);
        dst += dst_stride;

        ST_UB(res1, dst);
        dst[16] = clip_pixel(out2);
        dst[17] = clip_pixel(out3);
        dst += dst_stride;

        ST_UB(res2, dst);
        dst[16] = clip_pixel(out4);
        dst[17] = clip_pixel(out5);
        dst += dst_stride;

        ST_UB(res3, dst);
        dst[16] = clip_pixel(out6);
        dst[17] = clip_pixel(out7);
        dst += dst_stride;

        src10_r = src54_r;
        src32_r = src76_r;
        src21_r = src65_r;
        src43_r = src87_r;
        src10_l = src54_l;
        src32_l = src76_l;
        src21_l = src65_l;
        src43_l = src87_l;
        src4 = src8;
        ele0 = ele4;
        ele1 = ele5;
        ele2 = ele6;
        ele3 = ele7;
        ele4 = ele8;
    }

    src5 = LD_SB(src);

    ele5 = *(src + 16);
    tmp = *(src + 17);
    tmp = tmp << 16;
    ele5 = ele5 | tmp;

    src5 = (v16i8) __msa_xori_b((v16u8) src5, 128);

    ILVRL_B2_SB(src5, src4, src54_r, src54_l);

    out0_r = DPADD_SH3_SH(src10_r, src32_r, src54_r, filt0, filt1, filt2);
    out0_l = DPADD_SH3_SH(src10_l, src32_l, src54_l, filt0, filt1, filt2);
    SRARI_H2_SH(out0_r, out0_l, 5);
    SAT_SH2_SH(out0_r, out0_l, 7);

    res0 = PCKEV_XORI128_UB(out0_r, out0_l);

    AVC_LUME_FILT_ELE(ele0, ele1, ele2, ele3, ele4, ele5, out0, out1);

    ST_UB(res0, dst);
    dst[16] = clip_pixel(out0);
    dst[17] = clip_pixel(out1);
}

static void avc_mid_filter_6taps_8width_msa(uint8 *src, int32 src_stride,
                                            uint8 *dst, int32 dst_stride,
                                            int32 height)
{
    uint32 loop_cnt;
    v16i8 src0, src1, src2, src3, src4;
    v16u8 mask0 = { 0, 5, 1, 6, 2, 7, 3, 8, 4, 9, 5, 10, 6, 11, 7, 12 };
    v16u8 mask1 = { 1, 4, 2, 5, 3, 6, 4, 7, 5, 8, 6, 9, 7, 10, 8, 11 };
    v16u8 mask2 = { 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10 };
    v8i16 hz_out0, hz_out1, hz_out2, hz_out3;
    v8i16 hz_out4, hz_out5, hz_out6, hz_out7, hz_out8;
    v8i16 dst0, dst1, dst2, dst3;
    v16u8 out0, out1;

    LD_SB5(src, src_stride, src0, src1, src2, src3, src4);
    XORI_B5_128_SB(src0, src1, src2, src3, src4);
    src += (5 * src_stride);

    hz_out0 = AVC_HORZ_FILTER_SH(src0, mask0, mask1, mask2);
    hz_out1 = AVC_HORZ_FILTER_SH(src1, mask0, mask1, mask2);
    hz_out2 = AVC_HORZ_FILTER_SH(src2, mask0, mask1, mask2);
    hz_out3 = AVC_HORZ_FILTER_SH(src3, mask0, mask1, mask2);
    hz_out4 = AVC_HORZ_FILTER_SH(src4, mask0, mask1, mask2);

    for (loop_cnt = (height >> 2); loop_cnt--;)
    {
        LD_SB4(src, src_stride, src0, src1, src2, src3);
        XORI_B4_128_SB(src0, src1, src2, src3);
        src += (4 * src_stride);

        hz_out5 = AVC_HORZ_FILTER_SH(src0, mask0, mask1, mask2);
        hz_out6 = AVC_HORZ_FILTER_SH(src1, mask0, mask1, mask2);
        hz_out7 = AVC_HORZ_FILTER_SH(src2, mask0, mask1, mask2);
        hz_out8 = AVC_HORZ_FILTER_SH(src3, mask0, mask1, mask2);

        dst0 = AVC_CALC_DPADD_H_6PIX_2COEFF_SH(hz_out0, hz_out1, hz_out2,
                                               hz_out3, hz_out4, hz_out5);
        dst1 = AVC_CALC_DPADD_H_6PIX_2COEFF_SH(hz_out1, hz_out2, hz_out3,
                                               hz_out4, hz_out5, hz_out6);
        dst2 = AVC_CALC_DPADD_H_6PIX_2COEFF_SH(hz_out2, hz_out3, hz_out4,
                                               hz_out5, hz_out6, hz_out7);
        dst3 = AVC_CALC_DPADD_H_6PIX_2COEFF_SH(hz_out3, hz_out4, hz_out5,
                                               hz_out6, hz_out7, hz_out8);

        out0 = PCKEV_XORI128_UB(dst0, dst1);
        out1 = PCKEV_XORI128_UB(dst2, dst3);
        ST8x4_UB(out0, out1, dst, dst_stride);
        dst += (4 * dst_stride);

        hz_out3 = hz_out7;
        hz_out1 = hz_out5;
        hz_out5 = hz_out4;
        hz_out4 = hz_out8;
        hz_out2 = hz_out6;
        hz_out0 = hz_out5;
    }

    src0 = LD_SB(src);
    src0 = (v16i8) __msa_xori_b((v16u8) src0, 128);
    hz_out5 = AVC_HORZ_FILTER_SH(src0, mask0, mask1, mask2);
    dst0 = AVC_CALC_DPADD_H_6PIX_2COEFF_SH(hz_out0, hz_out1, hz_out2, hz_out3,
                                           hz_out4, hz_out5);

    out0 = PCKEV_XORI128_UB(dst0, dst0);
    ST8x1_UB(out0, dst)
}

static void avc_mid_filter_6taps_9width_msa(uint8 *src, int32 src_stride,
                                            uint8 *dst, int32 dst_stride,
                                            int32 height)
{
    uint32 loop_cnt;
    int32 hz_out_w0, hz_out_w1, hz_out_w2, hz_out_w3, hz_out_w4, hz_out_w5;
    int32 filt_sum;
    v16i8 src0, src1, src2, src3, src4;
    v16i8 mask0 = { 0, 5, 1, 6, 2, 7, 3, 8, 4, 9, 5, 10, 6, 11, 7, 12 };
    v16i8 mask1 = { 1, 4, 2, 5, 3, 6, 4, 7, 5, 8, 6, 9, 7, 10, 8, 11 };
    v16i8 mask2 = { 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10 };
    v16i8 filt = { 0, 0, 0, 0, 0, 0, 0, 0, 1, -5, 20, 20, -5, 1, 0, 0 };
    v8i16 hz_out0, hz_out1, hz_out2, hz_out3;
    v8i16 hz_out4, hz_out5, hz_out6, hz_out7, hz_out8;
    v8i16 dst0, dst1, dst2, dst3, tmp;
    v16u8 out;

    LD_SB5(src, src_stride, src0, src1, src2, src3, src4);
    XORI_B5_128_SB(src0, src1, src2, src3, src4);
    src += (5 * src_stride);

    hz_out0 = AVC_HORZ_FILTER_SH(src0, mask0, mask1, mask2);

    tmp = __msa_dotp_s_h(src0, filt);
    hz_out_w0 = __msa_copy_s_h(tmp, 4);
    hz_out_w0 += __msa_copy_s_h(tmp, 5);
    hz_out_w0 += __msa_copy_s_h(tmp, 6);

    hz_out1 = AVC_HORZ_FILTER_SH(src1, mask0, mask1, mask2);

    tmp = __msa_dotp_s_h(src1, filt);
    hz_out_w1 = __msa_copy_s_h(tmp, 4);
    hz_out_w1 += __msa_copy_s_h(tmp, 5);
    hz_out_w1 += __msa_copy_s_h(tmp, 6);

    hz_out2 = AVC_HORZ_FILTER_SH(src2, mask0, mask1, mask2);

    tmp = __msa_dotp_s_h(src2, filt);
    hz_out_w2 = __msa_copy_s_h(tmp, 4);
    hz_out_w2 += __msa_copy_s_h(tmp, 5);
    hz_out_w2 += __msa_copy_s_h(tmp, 6);

    hz_out3 = AVC_HORZ_FILTER_SH(src3, mask0, mask1, mask2);

    tmp = __msa_dotp_s_h(src3, filt);
    hz_out_w3 = __msa_copy_s_h(tmp, 4);
    hz_out_w3 += __msa_copy_s_h(tmp, 5);
    hz_out_w3 += __msa_copy_s_h(tmp, 6);

    hz_out4 = AVC_HORZ_FILTER_SH(src4, mask0, mask1, mask2);

    tmp = __msa_dotp_s_h(src4, filt);
    hz_out_w4 = __msa_copy_s_h(tmp, 4);
    hz_out_w4 += __msa_copy_s_h(tmp, 5);
    hz_out_w4 += __msa_copy_s_h(tmp, 6);

    for (loop_cnt = (height >> 2); loop_cnt--;)
    {
        LD_SB4(src, src_stride, src0, src1, src2, src3);
        XORI_B4_128_SB(src0, src1, src2, src3);
        src += (4 * src_stride);

        hz_out5 = AVC_HORZ_FILTER_SH(src0, mask0, mask1, mask2);
        hz_out6 = AVC_HORZ_FILTER_SH(src1, mask0, mask1, mask2);
        hz_out7 = AVC_HORZ_FILTER_SH(src2, mask0, mask1, mask2);
        hz_out8 = AVC_HORZ_FILTER_SH(src3, mask0, mask1, mask2);

        dst0 = AVC_CALC_DPADD_H_6PIX_2COEFF_SH(hz_out0, hz_out1, hz_out2,
                                               hz_out3, hz_out4, hz_out5);
        dst1 = AVC_CALC_DPADD_H_6PIX_2COEFF_SH(hz_out1, hz_out2, hz_out3,
                                               hz_out4, hz_out5, hz_out6);
        dst2 = AVC_CALC_DPADD_H_6PIX_2COEFF_SH(hz_out2, hz_out3, hz_out4,
                                               hz_out5, hz_out6, hz_out7);
        dst3 = AVC_CALC_DPADD_H_6PIX_2COEFF_SH(hz_out3, hz_out4, hz_out5,
                                               hz_out6, hz_out7, hz_out8);

        tmp = __msa_dotp_s_h(src0, filt);
        hz_out_w5 = __msa_copy_s_h(tmp, 4);
        hz_out_w5 += __msa_copy_s_h(tmp, 5);
        hz_out_w5 += __msa_copy_s_h(tmp, 6);

        out = PCKEV_XORI128_UB(dst0, dst0);
        ST8x1_UB(out, dst)

        filt_sum = hz_out_w0 + hz_out_w5 - 5 * (hz_out_w1 + hz_out_w4) +
                   20 * (hz_out_w2 + hz_out_w3);
        filt_sum = (filt_sum + 512) >> 10;

        if (filt_sum > 127)
        {
            filt_sum = 127;
        }
        else if (filt_sum < -128)
        {
            filt_sum = -128;
        }

        filt_sum = (uint8) filt_sum ^ 128;
        *(dst + 8) = (uint8) filt_sum;

        dst += dst_stride;

        tmp = __msa_dotp_s_h(src1, filt);
        hz_out_w0 = __msa_copy_s_h(tmp, 4);
        hz_out_w0 += __msa_copy_s_h(tmp, 5);
        hz_out_w0 += __msa_copy_s_h(tmp, 6);

        out = PCKEV_XORI128_UB(dst1, dst1);
        ST8x1_UB(out, dst)

        filt_sum = hz_out_w1 + hz_out_w0 - 5 * (hz_out_w2 + hz_out_w5) +
                   20 * (hz_out_w3 + hz_out_w4);
        filt_sum = (filt_sum + 512) >> 10;

        if (filt_sum > 127)
        {
            filt_sum = 127;
        }
        else if (filt_sum < -128)
        {
            filt_sum = -128;
        }

        filt_sum = (uint8) filt_sum ^ 128;
        *(dst + 8) = (uint8) filt_sum;

        dst += dst_stride;

        /* 3rd row */
        tmp = __msa_dotp_s_h(src2, filt);
        hz_out_w1 = __msa_copy_s_h(tmp, 4);
        hz_out_w1 += __msa_copy_s_h(tmp, 5);
        hz_out_w1 += __msa_copy_s_h(tmp, 6);

        out = PCKEV_XORI128_UB(dst2, dst2);
        ST8x1_UB(out, dst)

        filt_sum = hz_out_w2 + hz_out_w1 - 5 * (hz_out_w3 + hz_out_w0) +
                   20 * (hz_out_w4 + hz_out_w5);
        filt_sum = (filt_sum + 512) >> 10;

        if (filt_sum > 127)
        {
            filt_sum = 127;
        }
        else if (filt_sum < -128)
        {
            filt_sum = -128;
        }

        filt_sum = (uint8) filt_sum ^ 128;
        *(dst + 8) = (uint8) filt_sum;

        dst += dst_stride;

        /* 4th row */
        tmp = __msa_dotp_s_h(src3, filt);
        hz_out_w2 = __msa_copy_s_h(tmp, 4);
        hz_out_w2 += __msa_copy_s_h(tmp, 5);
        hz_out_w2 += __msa_copy_s_h(tmp, 6);

        out = PCKEV_XORI128_UB(dst3, dst3);
        ST8x1_UB(out, dst)

        filt_sum = hz_out_w3 + hz_out_w2 - 5 * (hz_out_w4 + hz_out_w1) +
                   20 * (hz_out_w5 + hz_out_w0);
        filt_sum = (filt_sum + 512) >> 10;

        if (filt_sum > 127)
        {
            filt_sum = 127;
        }
        else if (filt_sum < -128)
        {
            filt_sum = -128;
        }

        filt_sum = (uint8) filt_sum ^ 128;
        *(dst + 8) = (uint8) filt_sum;

        dst += dst_stride;

        hz_out3 = hz_out7;
        hz_out1 = hz_out5;
        hz_out5 = hz_out4;
        hz_out4 = hz_out8;
        hz_out2 = hz_out6;
        hz_out0 = hz_out5;
        hz_out_w3 = hz_out_w1;
        hz_out_w1 = hz_out_w5;
        hz_out_w5 = hz_out_w4;
        hz_out_w4 = hz_out_w2;
        hz_out_w2 = hz_out_w0;
        hz_out_w0 = hz_out_w5;
    }

    src0 = LD_SB(src);
    src0 = (v16i8) __msa_xori_b((v16u8) src0, 128);
    hz_out5 = AVC_HORZ_FILTER_SH(src0, mask0, mask1, mask2);

    tmp = __msa_dotp_s_h(src0, filt);
    hz_out_w5 = __msa_copy_s_h(tmp, 4);
    hz_out_w5 += __msa_copy_s_h(tmp, 5);
    hz_out_w5 += __msa_copy_s_h(tmp, 6);

    dst0 = AVC_CALC_DPADD_H_6PIX_2COEFF_SH(hz_out0, hz_out1, hz_out2, hz_out3,
                                           hz_out4, hz_out5);
    out = PCKEV_XORI128_UB(dst0, dst0);
    ST8x1_UB(out, dst)

    filt_sum = hz_out_w0 + hz_out_w5 - 5 * (hz_out_w1 + hz_out_w4) +
               20 * (hz_out_w2 + hz_out_w3);
    filt_sum = (filt_sum + 512) >> 10;

    if (filt_sum > 127)
    {
        filt_sum = 127;
    }
    else if (filt_sum < -128)
    {
        filt_sum = -128;
    }

    filt_sum = (uint8) filt_sum ^ 128;
    *(dst + 8) = (uint8) filt_sum;
}

void avc_mid_filter_6taps_17width_msa(uint8 *src, int32 src_stride,
                                      uint8 *dst, int32 dst_stride,
                                      int32 height)
{
    avc_mid_filter_6taps_8width_msa(src, src_stride, dst, dst_stride, height);

    src += 8;
    dst += 8;

    avc_mid_filter_6taps_9width_msa(src, src_stride, dst, dst_stride, height);
}

uint32 sad_16width_msa(uint8 *src, int32 src_stride,
                       uint8 *ref, int32 ref_stride,
                       int32 height)
{
    int32 ht_cnt;
    v16u8 src0, src1, ref0, ref1;
    v8u16 sad = { 0 };

    for (ht_cnt = (height >> 2); ht_cnt--;)
    {
        LD_UB2(src, src_stride, src0, src1);
        src += (2 * src_stride);
        LD_UB2(ref, ref_stride, ref0, ref1);
        ref += (2 * ref_stride);
        sad += SAD_UB2_UH(src0, src1, ref0, ref1);

        LD_UB2(src, src_stride, src0, src1);
        src += (2 * src_stride);
        LD_UB2(ref, ref_stride, ref0, ref1);
        ref += (2 * ref_stride);
        sad += SAD_UB2_UH(src0, src1, ref0, ref1);
    }

    return (HADD_UH_U32(sad));
}

uint32 sad16x16_msa(uint8 *src_ptr, int32 src_stride,
                    uint8 *ref_ptr, int32 ref_stride)
{
    return (sad_16width_msa(src_ptr, src_stride, ref_ptr, ref_stride, 16));
}
#endif /* #ifdef H264ENC_MSA */
