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

#include <stdint.h>
#include <assert.h>

#include "common_macros_msa.h"

#ifdef H264ENC_MSA
uint8 luma_mask_arr[16 * 8] =
{
    /* 8 width cases */
    0, 5, 1, 6, 2, 7, 3, 8, 4, 9, 5, 10, 6, 11, 7, 12,
    1, 4, 2, 5, 3, 6, 4, 7, 5, 8, 6, 9, 7, 10, 8, 11,
    2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10,

    /* 4 width cases */
    0, 5, 1, 6, 2, 7, 3, 8, 16, 21, 17, 22, 18, 23, 19, 24,
    1, 4, 2, 5, 3, 6, 4, 7, 17, 20, 18, 21, 19, 22, 20, 23,
    2, 3, 3, 4, 4, 5, 5, 6, 18, 19, 19, 20, 20, 21, 21, 22,

    2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 24, 25,
    3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 24, 25, 26
};

uint8 chroma_mask_arr[16 * 5] =
{
    0, 1, 1, 2, 2, 3, 3, 4, 16, 17, 17, 18, 18, 19, 19, 20,
    0, 2, 2, 4, 4, 6, 6, 8, 16, 18, 18, 20, 20, 22, 22, 24,
    0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8,
    0, 1, 1, 2, 16, 17, 17, 18, 4, 5, 5, 6, 6, 7, 7, 8,
    0, 1, 1, 2, 16, 17, 17, 18, 16, 17, 17, 18, 18, 19, 19, 20
};

static const uint32_t mb4x4_index[16] =
{
    0, 1, 4, 5, 2, 3, 6, 7, 8, 9, 12, 13, 10, 11, 14, 15
};

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

#define AVC_CALC_DPADD_B_6PIX_2COEFF_SH(vec0, vec1, vec2, vec3, vec4, vec5,  \
                                        out1, out2)                          \
{                                                                            \
    v16i8 tmp0_m, tmp1_m;                                                    \
    v16i8 minus5b_m = __msa_ldi_b(-5);                                       \
    v16i8 plus20b_m = __msa_ldi_b(20);                                       \
                                                                             \
    ILVRL_B2_SB(vec5, vec0, tmp0_m, tmp1_m);                                 \
    HADD_SB2_SH(tmp0_m, tmp1_m, out1, out2);                                 \
    ILVRL_B2_SB(vec4, vec1, tmp0_m, tmp1_m);                                 \
    DPADD_SB2_SH(tmp0_m, tmp1_m, minus5b_m, minus5b_m, out1, out2);          \
    ILVRL_B2_SB(vec3, vec2, tmp0_m, tmp1_m);                                 \
    DPADD_SB2_SH(tmp0_m, tmp1_m, plus20b_m, plus20b_m, out1, out2);          \
}

#define AVC_CALC_DPADD_B_6PIX_2COEFF_R_SH(vec0, vec1, vec2, vec3, vec4, vec5)  \
( {                                                                            \
    v8i16 tmp1_m;                                                              \
    v16i8 tmp0_m, tmp2_m;                                                      \
    v16i8 minus5b_m = __msa_ldi_b(-5);                                         \
    v16i8 plus20b_m = __msa_ldi_b(20);                                         \
                                                                               \
    tmp1_m = (v8i16) __msa_ilvr_b((v16i8) vec5, (v16i8) vec0);                 \
    tmp1_m = __msa_hadd_s_h((v16i8) tmp1_m, (v16i8) tmp1_m);                   \
                                                                               \
    ILVR_B2_SB(vec4, vec1, vec3, vec2, tmp0_m, tmp2_m);                        \
    DPADD_SB2_SH(tmp0_m, tmp2_m, minus5b_m, plus20b_m, tmp1_m, tmp1_m);        \
                                                                               \
    tmp1_m;                                                                    \
} )

#define AVC_CALC_DPADD_H_6PIX_2COEFF_R_SH(vec0, vec1, vec2, vec3, vec4, vec5)  \
( {                                                                            \
    v4i32 tmp1_m;                                                              \
    v8i16 tmp2_m, tmp3_m;                                                      \
    v8i16 minus5h_m = __msa_ldi_h(-5);                                         \
    v8i16 plus20h_m = __msa_ldi_h(20);                                         \
                                                                               \
    tmp1_m = (v4i32) __msa_ilvr_h((v8i16) vec5, (v8i16) vec0);                 \
    tmp1_m = __msa_hadd_s_w((v8i16) tmp1_m, (v8i16) tmp1_m);                   \
                                                                               \
    ILVR_H2_SH(vec1, vec4, vec2, vec3, tmp2_m, tmp3_m);                        \
    DPADD_SH2_SW(tmp2_m, tmp3_m, minus5h_m, plus20h_m, tmp1_m, tmp1_m);        \
                                                                               \
    tmp1_m = __msa_srari_w(tmp1_m, 10);                                        \
    tmp1_m = __msa_sat_s_w(tmp1_m, 7);                                         \
                                                                               \
    tmp2_m = __msa_pckev_h((v8i16) tmp1_m, (v8i16) tmp1_m);                    \
                                                                               \
    tmp2_m;                                                                    \
} )

#define AVC_XOR_VSHF_B_AND_APPLY_6TAP_HORIZ_FILT_SH(src0, src1,              \
                                                    mask0, mask1, mask2)     \
( {                                                                          \
    v8i16 hz_out_m;                                                          \
    v16i8 vec0_m, vec1_m, vec2_m;                                            \
    v16i8 minus5b_m = __msa_ldi_b(-5);                                       \
    v16i8 plus20b_m = __msa_ldi_b(20);                                       \
                                                                             \
    vec0_m = __msa_vshf_b((v16i8) mask0, (v16i8) src1, (v16i8) src0);        \
    hz_out_m = __msa_hadd_s_h(vec0_m, vec0_m);                               \
                                                                             \
    VSHF_B2_SB(src0, src1, src0, src1, mask1, mask2, vec1_m, vec2_m);        \
    DPADD_SB2_SH(vec1_m, vec2_m, minus5b_m, plus20b_m, hz_out_m, hz_out_m);  \
                                                                             \
    hz_out_m;                                                                \
} )

void avc_luma_hz_4w_msa(uint8 *src, int32 src_stride,
                        uint8 *dst, int32 dst_stride,
                        int32 height)
{
    uint32 loop_cnt;
    v16i8 src0, src1, src2, src3;
    v8i16 res0, res1;
    v16u8 out;
    v16i8 mask0, mask1, mask2;
    v16i8 vec0, vec1, vec2, vec3, vec4, vec5;
    v16i8 minus5b = __msa_ldi_b(-5);
    v16i8 plus20b = __msa_ldi_b(20);

    LD_SB3(&luma_mask_arr[48], 16, mask0, mask1, mask2);
    for (loop_cnt = (height >> 2); loop_cnt--;)
    {
        LD_SB4(src, src_stride, src0, src1, src2, src3);
        src += (4 * src_stride);

        XORI_B4_128_SB(src0, src1, src2, src3);
        VSHF_B2_SB(src0, src1, src2, src3, mask0, mask0, vec0, vec1);
        HADD_SB2_SH(vec0, vec1, res0, res1);
        VSHF_B2_SB(src0, src1, src2, src3, mask1, mask1, vec2, vec3);
        DPADD_SB2_SH(vec2, vec3, minus5b, minus5b, res0, res1);
        VSHF_B2_SB(src0, src1, src2, src3, mask2, mask2, vec4, vec5);
        DPADD_SB2_SH(vec4, vec5, plus20b, plus20b, res0, res1);
        SRARI_H2_SH(res0, res1, 5);
        SAT_SH2_SH(res0, res1, 7);
        out = PCKEV_XORI128_UB(res0, res1);
        ST4x4_UB(out, out, 0, 1, 2, 3, dst, dst_stride);
        dst += (4 * dst_stride);
    }
}

void avc_luma_hz_8w_msa(uint8 *src, int32 src_stride,
                        uint8 *dst, int32 dst_stride,
                        int32 height)
{
    uint32 loop_cnt;
    v16i8 src0, src1, src2, src3;
    v8i16 res0, res1, res2, res3;
    v16i8 mask0, mask1, mask2;
    v16i8 vec0, vec1, vec2, vec3, vec4, vec5;
    v16i8 vec6, vec7, vec8, vec9, vec10, vec11;
    v16i8 minus5b = __msa_ldi_b(-5);
    v16i8 plus20b = __msa_ldi_b(20);
    v16u8 out0, out1;

    LD_SB3(&luma_mask_arr[0], 16, mask0, mask1, mask2);

    for (loop_cnt = (height >> 2); loop_cnt--;)
    {
        LD_SB4(src, src_stride, src0, src1, src2, src3);
        src += (4 * src_stride);

        XORI_B4_128_SB(src0, src1, src2, src3);
        VSHF_B2_SB(src0, src0, src1, src1, mask0, mask0, vec0, vec1);
        VSHF_B2_SB(src2, src2, src3, src3, mask0, mask0, vec2, vec3);
        HADD_SB4_SH(vec0, vec1, vec2, vec3, res0, res1, res2, res3);
        VSHF_B2_SB(src0, src0, src1, src1, mask1, mask1, vec4, vec5);
        VSHF_B2_SB(src2, src2, src3, src3, mask1, mask1, vec6, vec7);
        DPADD_SB4_SH(vec4, vec5, vec6, vec7, minus5b, minus5b, minus5b, minus5b,
                     res0, res1, res2, res3);
        VSHF_B2_SB(src0, src0, src1, src1, mask2, mask2, vec8, vec9);
        VSHF_B2_SB(src2, src2, src3, src3, mask2, mask2, vec10, vec11);
        DPADD_SB4_SH(vec8, vec9, vec10, vec11, plus20b, plus20b, plus20b,
                     plus20b, res0, res1, res2, res3);
        SRARI_H4_SH(res0, res1, res2, res3, 5);
        SAT_SH4_SH(res0, res1, res2, res3, 7);
        out0 = PCKEV_XORI128_UB(res0, res1);
        out1 = PCKEV_XORI128_UB(res2, res3);
        ST8x4_UB(out0, out1, dst, dst_stride);
        dst += (4 * dst_stride);
    }
}

void avc_luma_hz_16w_msa(uint8 *p_src, int32 i_src_stride,
                         uint8 *p_dst, int32 i_dst_stride,
                         int32 i_height)
{
    uint32 u_loop_cnt, i_h4w;
    v16u8 dst0;
    v16i8 src0, src1, src2, src3, src4, src5, src6, src7;
    v8i16 res0, res1, res2, res3, res4, res5, res6, res7;
    v16i8 mask0, mask1, mask2;
    v16i8 vec0, vec1, vec2, vec3, vec4, vec5;
    v16i8 vec6, vec7, vec8, vec9, vec10, vec11;
    v16i8 minus5b = __msa_ldi_b(-5);
    v16i8 plus20b = __msa_ldi_b(20);

    i_h4w = i_height % 4;
    LD_SB3(&luma_mask_arr[0], 16, mask0, mask1, mask2);

    for (u_loop_cnt = (i_height >> 2); u_loop_cnt--;)
    {
        LD_SB2(p_src, 8, src0, src1);
        p_src += i_src_stride;
        LD_SB2(p_src, 8, src2, src3);
        p_src += i_src_stride;

        XORI_B4_128_SB(src0, src1, src2, src3);
        VSHF_B2_SB(src0, src0, src1, src1, mask0, mask0, vec0, vec3);
        VSHF_B2_SB(src2, src2, src3, src3, mask0, mask0, vec6, vec9);
        VSHF_B2_SB(src0, src0, src1, src1, mask1, mask1, vec1, vec4);
        VSHF_B2_SB(src2, src2, src3, src3, mask1, mask1, vec7, vec10);
        VSHF_B2_SB(src0, src0, src1, src1, mask2, mask2, vec2, vec5);
        VSHF_B2_SB(src2, src2, src3, src3, mask2, mask2, vec8, vec11);
        HADD_SB4_SH(vec0, vec3, vec6, vec9, res0, res1, res2, res3);
        DPADD_SB4_SH(vec1, vec4, vec7, vec10, minus5b, minus5b, minus5b,
                     minus5b, res0, res1, res2, res3);
        DPADD_SB4_SH(vec2, vec5, vec8, vec11, plus20b, plus20b, plus20b,
                     plus20b, res0, res1, res2, res3);

        LD_SB2(p_src, 8, src4, src5);
        p_src += i_src_stride;
        LD_SB2(p_src, 8, src6, src7);
        p_src += i_src_stride;

        XORI_B4_128_SB(src4, src5, src6, src7);
        VSHF_B2_SB(src4, src4, src5, src5, mask0, mask0, vec0, vec3);
        VSHF_B2_SB(src6, src6, src7, src7, mask0, mask0, vec6, vec9);
        VSHF_B2_SB(src4, src4, src5, src5, mask1, mask1, vec1, vec4);
        VSHF_B2_SB(src6, src6, src7, src7, mask1, mask1, vec7, vec10);
        VSHF_B2_SB(src4, src4, src5, src5, mask2, mask2, vec2, vec5);
        VSHF_B2_SB(src6, src6, src7, src7, mask2, mask2, vec8, vec11);
        HADD_SB4_SH(vec0, vec3, vec6, vec9, res4, res5, res6, res7);
        DPADD_SB4_SH(vec1, vec4, vec7, vec10, minus5b, minus5b, minus5b,
                     minus5b, res4, res5, res6, res7);
        DPADD_SB4_SH(vec2, vec5, vec8, vec11, plus20b, plus20b, plus20b,
                     plus20b, res4, res5, res6, res7);
        SRARI_H4_SH(res0, res1, res2, res3, 5);
        SRARI_H4_SH(res4, res5, res6, res7, 5);
        SAT_SH4_SH(res0, res1, res2, res3, 7);
        SAT_SH4_SH(res4, res5, res6, res7, 7);
        PCKEV_B4_SB(res1, res0, res3, res2, res5, res4, res7, res6,
                    vec0, vec1, vec2, vec3);
        XORI_B4_128_SB(vec0, vec1, vec2, vec3);

        ST_SB4(vec0, vec1, vec2, vec3, p_dst, i_dst_stride);
        p_dst += (4 * i_dst_stride);
    }

    for (u_loop_cnt = i_h4w; u_loop_cnt--;)
    {
        LD_SB2(p_src, 8, src0, src1);
        p_src += i_src_stride;

        XORI_B2_128_SB(src0, src1);
        VSHF_B2_SB(src0, src0, src1, src1, mask0, mask0, vec0, vec3);
        VSHF_B2_SB(src0, src0, src1, src1, mask1, mask1, vec1, vec4);
        VSHF_B2_SB(src0, src0, src1, src1, mask2, mask2, vec2, vec5);
        res0 = __msa_hadd_s_h(vec0, vec0);
        DPADD_SB2_SH(vec1, vec2, minus5b, plus20b, res0, res0);
        res1 = __msa_hadd_s_h(vec3, vec3);
        DPADD_SB2_SH(vec4, vec5, minus5b, plus20b, res1, res1);
        SRARI_H2_SH(res0, res1, 5);
        SAT_SH2_SH(res0, res1, 7);
        dst0 = PCKEV_XORI128_UB(res0, res1);
        ST_UB(dst0, p_dst);
        p_dst += i_dst_stride;
    }
}

void avc_luma_hz_qrt_4w_msa(uint8 *src, int32 src_stride,
                            uint8 *dst, int32 dst_stride,
                            int32 height, uint8 hor_offset)
{
    uint8 slide;
    uint32 loop_cnt;
    v16i8 src0, src1, src2, src3;
    v8i16 res0, res1;
    v16i8 res, mask0, mask1, mask2;
    v16i8 vec0, vec1, vec2, vec3, vec4, vec5;
    v16i8 minus5b = __msa_ldi_b(-5);
    v16i8 plus20b = __msa_ldi_b(20);

    LD_SB3(&luma_mask_arr[48], 16, mask0, mask1, mask2);
    slide = 2 + hor_offset;

    for (loop_cnt = (height >> 2); loop_cnt--;)
    {
        LD_SB4(src, src_stride, src0, src1, src2, src3);
        src += (4 * src_stride);

        XORI_B4_128_SB(src0, src1, src2, src3);
        VSHF_B2_SB(src0, src1, src2, src3, mask0, mask0, vec0, vec1);
        HADD_SB2_SH(vec0, vec1, res0, res1);
        VSHF_B2_SB(src0, src1, src2, src3, mask1, mask1, vec2, vec3);
        DPADD_SB2_SH(vec2, vec3, minus5b, minus5b, res0, res1);
        VSHF_B2_SB(src0, src1, src2, src3, mask2, mask2, vec4, vec5);
        DPADD_SB2_SH(vec4, vec5, plus20b, plus20b, res0, res1);
        SRARI_H2_SH(res0, res1, 5);
        SAT_SH2_SH(res0, res1, 7);

        res = __msa_pckev_b((v16i8) res1, (v16i8) res0);
        src0 = __msa_sld_b(src0, src0, slide);
        src1 = __msa_sld_b(src1, src1, slide);
        src2 = __msa_sld_b(src2, src2, slide);
        src3 = __msa_sld_b(src3, src3, slide);
        src0 = (v16i8) __msa_insve_w((v4i32) src0, 1, (v4i32) src1);
        src1 = (v16i8) __msa_insve_w((v4i32) src2, 1, (v4i32) src3);
        src0 = (v16i8) __msa_insve_d((v2i64) src0, 1, (v2i64) src1);
        res = __msa_aver_s_b(res, src0);
        res = (v16i8) __msa_xori_b((v16u8) res, 128);

        ST4x4_UB(res, res, 0, 1, 2, 3, dst, dst_stride);
        dst += (4 * dst_stride);
    }
}

void avc_luma_hz_qrt_8w_msa(uint8 *src, int32 src_stride,
                            uint8 *dst, int32 dst_stride,
                            int32 height, uint8 hor_offset)
{
    uint8 slide;
    uint32 loop_cnt;
    v16i8 src0, src1, src2, src3;
    v16i8 tmp0, tmp1;
    v8i16 res0, res1, res2, res3;
    v16i8 mask0, mask1, mask2;
    v16i8 vec0, vec1, vec2, vec3, vec4, vec5;
    v16i8 vec6, vec7, vec8, vec9, vec10, vec11;
    v16i8 minus5b = __msa_ldi_b(-5);
    v16i8 plus20b = __msa_ldi_b(20);

    LD_SB3(&luma_mask_arr[0], 16, mask0, mask1, mask2);
    slide = 2 + hor_offset;

    for (loop_cnt = height >> 2; loop_cnt--;)
    {
        LD_SB4(src, src_stride, src0, src1, src2, src3);
        src += (4 * src_stride);

        XORI_B4_128_SB(src0, src1, src2, src3);
        VSHF_B2_SB(src0, src0, src1, src1, mask0, mask0, vec0, vec1);
        VSHF_B2_SB(src2, src2, src3, src3, mask0, mask0, vec2, vec3);
        HADD_SB4_SH(vec0, vec1, vec2, vec3, res0, res1, res2, res3);
        VSHF_B2_SB(src0, src0, src1, src1, mask1, mask1, vec4, vec5);
        VSHF_B2_SB(src2, src2, src3, src3, mask1, mask1, vec6, vec7);
        DPADD_SB4_SH(vec4, vec5, vec6, vec7, minus5b, minus5b, minus5b, minus5b,
                     res0, res1, res2, res3);
        VSHF_B2_SB(src0, src0, src1, src1, mask2, mask2, vec8, vec9);
        VSHF_B2_SB(src2, src2, src3, src3, mask2, mask2, vec10, vec11);
        DPADD_SB4_SH(vec8, vec9, vec10, vec11, plus20b, plus20b, plus20b,
                     plus20b, res0, res1, res2, res3);

        src0 = __msa_sld_b(src0, src0, slide);
        src1 = __msa_sld_b(src1, src1, slide);
        src2 = __msa_sld_b(src2, src2, slide);
        src3 = __msa_sld_b(src3, src3, slide);

        SRARI_H4_SH(res0, res1, res2, res3, 5);
        SAT_SH4_SH(res0, res1, res2, res3, 7);
        PCKEV_B2_SB(res1, res0, res3, res2, tmp0, tmp1);
        PCKEV_D2_SB(src1, src0, src3, src2, src0, src1);

        tmp0 = __msa_aver_s_b(tmp0, src0);
        tmp1 = __msa_aver_s_b(tmp1, src1);

        XORI_B2_128_SB(tmp0, tmp1);
        ST8x4_UB(tmp0, tmp1, dst, dst_stride);

        dst += (4 * dst_stride);
    }
}

void avc_luma_hz_qrt_16w_msa(uint8 *src, int32 src_stride,
                             uint8 *dst, int32 dst_stride,
                             int32 height, uint8 hor_offset)
{
    uint32 loop_cnt;
    v16i8 dst0, dst1;
    v16i8 src0, src1, src2, src3;
    v16i8 mask0, mask1, mask2, vshf;
    v8i16 res0, res1, res2, res3;
    v16i8 vec0, vec1, vec2, vec3, vec4, vec5;
    v16i8 vec6, vec7, vec8, vec9, vec10, vec11;
    v16i8 minus5b = __msa_ldi_b(-5);
    v16i8 plus20b = __msa_ldi_b(20);

    LD_SB3(&luma_mask_arr[0], 16, mask0, mask1, mask2);

    if (hor_offset)
    {
        vshf = LD_SB(&luma_mask_arr[16 + 96]);
    }
    else
    {
        vshf = LD_SB(&luma_mask_arr[96]);
    }

    for (loop_cnt = height >> 1; loop_cnt--;)
    {
        LD_SB2(src, 8, src0, src1);
        src += src_stride;
        LD_SB2(src, 8, src2, src3);
        src += src_stride;

        XORI_B4_128_SB(src0, src1, src2, src3);
        VSHF_B2_SB(src0, src0, src1, src1, mask0, mask0, vec0, vec3);
        VSHF_B2_SB(src2, src2, src3, src3, mask0, mask0, vec6, vec9);
        VSHF_B2_SB(src0, src0, src1, src1, mask1, mask1, vec1, vec4);
        VSHF_B2_SB(src2, src2, src3, src3, mask1, mask1, vec7, vec10);
        VSHF_B2_SB(src0, src0, src1, src1, mask2, mask2, vec2, vec5);
        VSHF_B2_SB(src2, src2, src3, src3, mask2, mask2, vec8, vec11);
        HADD_SB4_SH(vec0, vec3, vec6, vec9, res0, res1, res2, res3);
        DPADD_SB4_SH(vec1, vec4, vec7, vec10, minus5b, minus5b, minus5b,
                     minus5b, res0, res1, res2, res3);
        DPADD_SB4_SH(vec2, vec5, vec8, vec11, plus20b, plus20b, plus20b,
                     plus20b, res0, res1, res2, res3);
        VSHF_B2_SB(src0, src1, src2, src3, vshf, vshf, src0, src2);
        SRARI_H4_SH(res0, res1, res2, res3, 5);
        SAT_SH4_SH(res0, res1, res2, res3, 7);
        PCKEV_B2_SB(res1, res0, res3, res2, dst0, dst1);

        dst0 = __msa_aver_s_b(dst0, src0);
        dst1 = __msa_aver_s_b(dst1, src2);

        XORI_B2_128_SB(dst0, dst1);

        ST_SB2(dst0, dst1, dst, dst_stride);
        dst += (2 * dst_stride);
    }
}

void avc_luma_vt_4w_msa(uint8 *src, int32 src_stride,
                        uint8 *dst, int32 dst_stride,
                        int32 height)
{
    int32 loop_cnt;
    int16 filt_const0 = 0xfb01;
    int16 filt_const1 = 0x1414;
    int16 filt_const2 = 0x1fb;
    v16i8 src0, src1, src2, src3, src4, src5, src6, src7, src8;
    v16i8 src10_r, src32_r, src54_r, src76_r, src21_r, src43_r, src65_r;
    v16i8 src87_r, src2110, src4332, src6554, src8776;
    v16i8 filt0, filt1, filt2;
    v8i16 out10, out32;
    v16u8 out;

    filt0 = (v16i8) __msa_fill_h(filt_const0);
    filt1 = (v16i8) __msa_fill_h(filt_const1);
    filt2 = (v16i8) __msa_fill_h(filt_const2);

    LD_SB5(src, src_stride, src0, src1, src2, src3, src4);
    src += (5 * src_stride);

    ILVR_B4_SB(src1, src0, src2, src1, src3, src2, src4, src3,
               src10_r, src21_r, src32_r, src43_r);
    ILVR_D2_SB(src21_r, src10_r, src43_r, src32_r, src2110, src4332);
    XORI_B2_128_SB(src2110, src4332);

    for (loop_cnt = (height >> 2); loop_cnt--;)
    {
        LD_SB4(src, src_stride, src5, src6, src7, src8);
        src += (4 * src_stride);

        ILVR_B4_SB(src5, src4, src6, src5, src7, src6, src8, src7,
                   src54_r, src65_r, src76_r, src87_r);
        ILVR_D2_SB(src65_r, src54_r, src87_r, src76_r, src6554, src8776);
        XORI_B2_128_SB(src6554, src8776);
        out10 = DPADD_SH3_SH(src2110, src4332, src6554, filt0, filt1, filt2);
        out32 = DPADD_SH3_SH(src4332, src6554, src8776, filt0, filt1, filt2);
        SRARI_H2_SH(out10, out32, 5);
        SAT_SH2_SH(out10, out32, 7);
        out = PCKEV_XORI128_UB(out10, out32);
        ST4x4_UB(out, out, 0, 1, 2, 3, dst, dst_stride);

        dst += (4 * dst_stride);
        src2110 = src6554;
        src4332 = src8776;
        src4 = src8;
    }
}

void avc_luma_vt_8w_msa(uint8 *src, int32 src_stride,
                        uint8 *dst, int32 dst_stride,
                        int32 height)
{
    int32 loop_cnt;
    int16 filt_const0 = 0xfb01;
    int16 filt_const1 = 0x1414;
    int16 filt_const2 = 0x1fb;
    v16i8 src0, src1, src2, src3, src4, src7, src8, src9, src10;
    v16i8 src10_r, src32_r, src76_r, src98_r;
    v16i8 src21_r, src43_r, src87_r, src109_r;
    v8i16 out0_r, out1_r, out2_r, out3_r;
    v16i8 filt0, filt1, filt2;
    v16u8 out0, out1;

    filt0 = (v16i8) __msa_fill_h(filt_const0);
    filt1 = (v16i8) __msa_fill_h(filt_const1);
    filt2 = (v16i8) __msa_fill_h(filt_const2);

    LD_SB5(src, src_stride, src0, src1, src2, src3, src4);
    src += (5 * src_stride);

    XORI_B5_128_SB(src0, src1, src2, src3, src4);
    ILVR_B4_SB(src1, src0, src2, src1, src3, src2, src4, src3,
               src10_r, src21_r, src32_r, src43_r);

    for (loop_cnt = (height >> 2); loop_cnt--;)
    {
        LD_SB4(src, src_stride, src7, src8, src9, src10);
        src += (4 * src_stride);

        XORI_B4_128_SB(src7, src8, src9, src10);
        ILVR_B4_SB(src7, src4, src8, src7, src9, src8, src10, src9,
                   src76_r, src87_r, src98_r, src109_r);
        out0_r = DPADD_SH3_SH(src10_r, src32_r, src76_r, filt0, filt1, filt2);
        out1_r = DPADD_SH3_SH(src21_r, src43_r, src87_r, filt0, filt1, filt2);
        out2_r = DPADD_SH3_SH(src32_r, src76_r, src98_r, filt0, filt1, filt2);
        out3_r = DPADD_SH3_SH(src43_r, src87_r, src109_r, filt0, filt1, filt2);
        SRARI_H4_SH(out0_r, out1_r, out2_r, out3_r, 5);
        SAT_SH4_SH(out0_r, out1_r, out2_r, out3_r, 7);
        out0 = PCKEV_XORI128_UB(out0_r, out1_r);
        out1 = PCKEV_XORI128_UB(out2_r, out3_r);
        ST8x4_UB(out0, out1, dst, dst_stride);
        dst += (4 * dst_stride);

        src10_r = src76_r;
        src32_r = src98_r;
        src21_r = src87_r;
        src43_r = src109_r;
        src4 = src10;
    }
}

void avc_luma_vt_16w_msa(uint8 *p_src, int32 i_src_stride,
                         uint8 *p_dst, int32 i_dst_stride,
                         int32 i_height)
{
    int32 i_loop_cnt, h4w;
    int16 i_filt_const0 = 0xfb01;
    int16 i_filt_const1 = 0x1414;
    int16 i_filt_const2 = 0x1fb;
    v16i8 src0, src1, src2, src3, src4, src5, src6, src7, src8;
    v16i8 src10_r, src32_r, src54_r, src76_r, src21_r, src43_r, src65_r;
    v16i8 src87_r, src10_l, src32_l, src54_l, src76_l, src21_l, src43_l;
    v16i8 src65_l, src87_l;
    v8i16 out0_r, out1_r, out2_r, out3_r, out0_l, out1_l, out2_l, out3_l;
    v16u8 res0, res1, res2, res3;
    v16i8 filt0, filt1, filt2;

    h4w = i_height % 4;
    filt0 = (v16i8) __msa_fill_h(i_filt_const0);
    filt1 = (v16i8) __msa_fill_h(i_filt_const1);
    filt2 = (v16i8) __msa_fill_h(i_filt_const2);

    LD_SB5(p_src, i_src_stride, src0, src1, src2, src3, src4);
    p_src += (5 * i_src_stride);

    XORI_B5_128_SB(src0, src1, src2, src3, src4);
    ILVR_B4_SB(src1, src0, src2, src1, src3, src2, src4, src3,
               src10_r, src21_r, src32_r, src43_r);
    ILVL_B4_SB(src1, src0, src2, src1, src3, src2, src4, src3,
               src10_l, src21_l, src32_l, src43_l);

    for (i_loop_cnt = (i_height >> 2); i_loop_cnt--;)
    {
        LD_SB4(p_src, i_src_stride, src5, src6, src7, src8);
        p_src += (4 * i_src_stride);

        XORI_B4_128_SB(src5, src6, src7, src8);
        ILVR_B4_SB(src5, src4, src6, src5, src7, src6, src8, src7,
                   src54_r, src65_r, src76_r, src87_r);
        ILVL_B4_SB(src5, src4, src6, src5, src7, src6, src8, src7,
                   src54_l, src65_l, src76_l, src87_l);
        out0_r = DPADD_SH3_SH(src10_r, src32_r, src54_r,
                              filt0, filt1, filt2);
        out1_r = DPADD_SH3_SH(src21_r, src43_r, src65_r,
                              filt0, filt1, filt2);
        out2_r = DPADD_SH3_SH(src32_r, src54_r, src76_r,
                              filt0, filt1, filt2);
        out3_r = DPADD_SH3_SH(src43_r, src65_r, src87_r,
                              filt0, filt1, filt2);
        out0_l = DPADD_SH3_SH(src10_l, src32_l, src54_l,
                              filt0, filt1, filt2);
        out1_l = DPADD_SH3_SH(src21_l, src43_l, src65_l,
                              filt0, filt1, filt2);
        out2_l = DPADD_SH3_SH(src32_l, src54_l, src76_l,
                              filt0, filt1, filt2);
        out3_l = DPADD_SH3_SH(src43_l, src65_l, src87_l,
                              filt0, filt1, filt2);
        SRARI_H4_SH(out0_r, out1_r, out2_r, out3_r, 5);
        SAT_SH4_SH(out0_r, out1_r, out2_r, out3_r, 7);
        SRARI_H4_SH(out0_l, out1_l, out2_l, out3_l, 5);
        SAT_SH4_SH(out0_l, out1_l, out2_l, out3_l, 7);
        PCKEV_B4_UB(out0_l, out0_r, out1_l, out1_r, out2_l, out2_r, out3_l,
                    out3_r, res0, res1, res2, res3);
        XORI_B4_128_UB(res0, res1, res2, res3);

        ST_UB4(res0, res1, res2, res3, p_dst, i_dst_stride);
        p_dst += (4 * i_dst_stride);

        src10_r = src54_r;
        src32_r = src76_r;
        src21_r = src65_r;
        src43_r = src87_r;
        src10_l = src54_l;
        src32_l = src76_l;
        src21_l = src65_l;
        src43_l = src87_l;
        src4 = src8;
    }

    for (i_loop_cnt = h4w; i_loop_cnt--;)
    {
        src5 = LD_SB(p_src);
        p_src += (i_src_stride);
        src5 = (v16i8) __msa_xori_b((v16u8) src5, 128);
        ILVRL_B2_SB(src5, src4, src54_r, src54_l);
        out0_r = DPADD_SH3_SH(src10_r, src32_r, src54_r,
                              filt0, filt1, filt2);
        out0_l = DPADD_SH3_SH(src10_l, src32_l, src54_l,
                              filt0, filt1, filt2);
        SRARI_H2_SH(out0_r, out0_l, 5);
        SAT_SH2_SH(out0_r, out0_l, 7);
        out0_r = (v8i16) __msa_pckev_b((v16i8) out0_l, (v16i8) out0_r);
        res0 = __msa_xori_b((v16u8) out0_r, 128);
        ST_UB(res0, p_dst);
        p_dst += i_dst_stride;

        src10_r = src21_r;
        src21_r = src32_r;
        src32_r = src43_r;
        src43_r = src54_r;

        src10_l = src21_l;
        src21_l = src32_l;
        src32_l = src43_l;
        src43_l = src54_l;

        src4 = src5;
    }
}

void avc_luma_vt_qrt_4w_msa(uint8 *src, int32 src_stride,
                            uint8 *dst, int32 dst_stride,
                            int32 height, uint8 ver_offset)
{
    int32 loop_cnt;
    int16 filt_const0 = 0xfb01;
    int16 filt_const1 = 0x1414;
    int16 filt_const2 = 0x1fb;
    v16i8 src0, src1, src2, src3, src4, src5, src6, src7, src8;
    v16i8 src10_r, src32_r, src54_r, src76_r, src21_r, src43_r, src65_r;
    v16i8 src87_r, src2110, src4332, src6554, src8776;
    v8i16 out10, out32;
    v16i8 filt0, filt1, filt2;
    v16u8 out;

    filt0 = (v16i8) __msa_fill_h(filt_const0);
    filt1 = (v16i8) __msa_fill_h(filt_const1);
    filt2 = (v16i8) __msa_fill_h(filt_const2);

    LD_SB5(src, src_stride, src0, src1, src2, src3, src4);
    src += (5 * src_stride);

    ILVR_B4_SB(src1, src0, src2, src1, src3, src2, src4, src3,
               src10_r, src21_r, src32_r, src43_r);
    ILVR_D2_SB(src21_r, src10_r, src43_r, src32_r, src2110, src4332);
    XORI_B2_128_SB(src2110, src4332);

    for (loop_cnt = (height >> 2); loop_cnt--;)
    {
        LD_SB4(src, src_stride, src5, src6, src7, src8);
        src += (4 * src_stride);

        ILVR_B4_SB(src5, src4, src6, src5, src7, src6, src8, src7,
                   src54_r, src65_r, src76_r, src87_r);
        ILVR_D2_SB(src65_r, src54_r, src87_r, src76_r, src6554, src8776);
        XORI_B2_128_SB(src6554, src8776);
        out10 = DPADD_SH3_SH(src2110, src4332, src6554, filt0, filt1, filt2);
        out32 = DPADD_SH3_SH(src4332, src6554, src8776, filt0, filt1, filt2);
        SRARI_H2_SH(out10, out32, 5);
        SAT_SH2_SH(out10, out32, 7);

        out = PCKEV_XORI128_UB(out10, out32);

        if (ver_offset)
        {
            src32_r = (v16i8) __msa_insve_w((v4i32) src3, 1, (v4i32) src4);
            src54_r = (v16i8) __msa_insve_w((v4i32) src5, 1, (v4i32) src6);
        }
        else
        {
            src32_r = (v16i8) __msa_insve_w((v4i32) src2, 1, (v4i32) src3);
            src54_r = (v16i8) __msa_insve_w((v4i32) src4, 1, (v4i32) src5);
        }

        src32_r = (v16i8) __msa_insve_d((v2i64) src32_r, 1, (v2i64) src54_r);
        out = __msa_aver_u_b(out, (v16u8) src32_r);

        ST4x4_UB(out, out, 0, 1, 2, 3, dst, dst_stride);
        dst += (4 * dst_stride);
        src2110 = src6554;
        src4332 = src8776;
        src2 = src6;
        src3 = src7;
        src4 = src8;
    }
}

void avc_luma_vt_qrt_8w_msa(uint8 *src, int32 src_stride,
                            uint8 *dst, int32 dst_stride,
                            int32 height, uint8 ver_offset)
{
    int32 loop_cnt;
    int16 filt_const0 = 0xfb01;
    int16 filt_const1 = 0x1414;
    int16 filt_const2 = 0x1fb;
    v16i8 src0, src1, src2, src3, src4, src7, src8, src9, src10;
    v16i8 src10_r, src32_r, src76_r, src98_r;
    v16i8 src21_r, src43_r, src87_r, src109_r;
    v8i16 out0_r, out1_r, out2_r, out3_r;
    v16i8 res0, res1;
    v16i8 filt0, filt1, filt2;

    filt0 = (v16i8) __msa_fill_h(filt_const0);
    filt1 = (v16i8) __msa_fill_h(filt_const1);
    filt2 = (v16i8) __msa_fill_h(filt_const2);

    LD_SB5(src, src_stride, src0, src1, src2, src3, src4);
    src += (5 * src_stride);

    XORI_B5_128_SB(src0, src1, src2, src3, src4);
    ILVR_B4_SB(src1, src0, src2, src1, src3, src2, src4, src3,
               src10_r, src21_r, src32_r, src43_r);

    for (loop_cnt = (height >> 2); loop_cnt--;)
    {
        LD_SB4(src, src_stride, src7, src8, src9, src10);
        src += (4 * src_stride);

        XORI_B4_128_SB(src7, src8, src9, src10);
        ILVR_B4_SB(src7, src4, src8, src7, src9, src8, src10, src9,
                   src76_r, src87_r, src98_r, src109_r);
        out0_r = DPADD_SH3_SH(src10_r, src32_r, src76_r, filt0, filt1, filt2);
        out1_r = DPADD_SH3_SH(src21_r, src43_r, src87_r, filt0, filt1, filt2);
        out2_r = DPADD_SH3_SH(src32_r, src76_r, src98_r, filt0, filt1, filt2);
        out3_r = DPADD_SH3_SH(src43_r, src87_r, src109_r, filt0, filt1, filt2);
        SRARI_H4_SH(out0_r, out1_r, out2_r, out3_r, 5);
        SAT_SH4_SH(out0_r, out1_r, out2_r, out3_r, 7);
        PCKEV_B2_SB(out1_r, out0_r, out3_r, out2_r, res0, res1);

        if (ver_offset)
        {
            PCKEV_D2_SB(src4, src3, src8, src7, src10_r, src32_r);
        }
        else
        {
            PCKEV_D2_SB(src3, src2, src7, src4, src10_r, src32_r);
        }

        res0 = __msa_aver_s_b(res0, (v16i8) src10_r);
        res1 = __msa_aver_s_b(res1, (v16i8) src32_r);

        XORI_B2_128_SB(res0, res1);
        ST8x4_UB(res0, res1, dst, dst_stride);

        dst += (4 * dst_stride);
        src10_r = src76_r;
        src32_r = src98_r;
        src21_r = src87_r;
        src43_r = src109_r;
        src2 = src8;
        src3 = src9;
        src4 = src10;
    }
}

void avc_luma_vt_qrt_16w_msa(uint8 *src, int32 src_stride,
                             uint8 *dst, int32 dst_stride,
                             int32 height, uint8 ver_offset)
{
    int32 loop_cnt;
    int16 filt_const0 = 0xfb01;
    int16 filt_const1 = 0x1414;
    int16 filt_const2 = 0x1fb;
    v16i8 src0, src1, src2, src3, src4, src5, src6, src7, src8;
    v16i8 src10_r, src32_r, src54_r, src76_r, src21_r, src43_r, src65_r;
    v16i8 src87_r, src10_l, src32_l, src54_l, src76_l, src21_l, src43_l;
    v16i8 src65_l, src87_l;
    v8i16 out0_r, out1_r, out2_r, out3_r, out0_l, out1_l, out2_l, out3_l;
    v16u8 res0, res1, res2, res3;
    v16i8 filt0, filt1, filt2;

    filt0 = (v16i8) __msa_fill_h(filt_const0);
    filt1 = (v16i8) __msa_fill_h(filt_const1);
    filt2 = (v16i8) __msa_fill_h(filt_const2);

    LD_SB5(src, src_stride, src0, src1, src2, src3, src4);
    src += (5 * src_stride);

    XORI_B5_128_SB(src0, src1, src2, src3, src4);
    ILVR_B4_SB(src1, src0, src2, src1, src3, src2, src4, src3,
               src10_r, src21_r, src32_r, src43_r);
    ILVL_B4_SB(src1, src0, src2, src1, src3, src2, src4, src3,
               src10_l, src21_l, src32_l, src43_l);

    for (loop_cnt = (height >> 2); loop_cnt--;)
    {
        LD_SB4(src, src_stride, src5, src6, src7, src8);
        src += (4 * src_stride);

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
        SAT_SH4_SH(out0_r, out1_r, out2_r, out3_r, 7);
        SRARI_H4_SH(out0_l, out1_l, out2_l, out3_l, 5);
        SAT_SH4_SH(out0_l, out1_l, out2_l, out3_l, 7);
        PCKEV_B4_UB(out0_l, out0_r, out1_l, out1_r, out2_l, out2_r, out3_l,
                    out3_r, res0, res1, res2, res3);

        if (ver_offset)
        {
            res0 = (v16u8) __msa_aver_s_b((v16i8) res0, src3);
            res1 = (v16u8) __msa_aver_s_b((v16i8) res1, src4);
            res2 = (v16u8) __msa_aver_s_b((v16i8) res2, src5);
            res3 = (v16u8) __msa_aver_s_b((v16i8) res3, src6);
        }
        else
        {
            res0 = (v16u8) __msa_aver_s_b((v16i8) res0, src2);
            res1 = (v16u8) __msa_aver_s_b((v16i8) res1, src3);
            res2 = (v16u8) __msa_aver_s_b((v16i8) res2, src4);
            res3 = (v16u8) __msa_aver_s_b((v16i8) res3, src5);
        }

        XORI_B4_128_UB(res0, res1, res2, res3);
        ST_UB4(res0, res1, res2, res3, dst, dst_stride);

        dst += (4 * dst_stride);

        src10_r = src54_r;
        src32_r = src76_r;
        src21_r = src65_r;
        src43_r = src87_r;
        src10_l = src54_l;
        src32_l = src76_l;
        src21_l = src65_l;
        src43_l = src87_l;
        src2 = src6;
        src3 = src7;
        src4 = src8;
    }
}

void avc_luma_mid_4w_msa(uint8 *src, int32 src_stride,
                         uint8 *dst, int32 dst_stride,
                         int32 height)
{
    uint32 loop_cnt;
    v16i8 src0, src1, src2, src3, src4;
    v16i8 mask0, mask1, mask2;
    v8i16 hz_out0, hz_out1, hz_out2, hz_out3;
    v8i16 hz_out4, hz_out5, hz_out6, hz_out7, hz_out8;
    v8i16 dst0, dst1, dst2, dst3;

    LD_SB3(&luma_mask_arr[48], 16, mask0, mask1, mask2);
    LD_SB5(src, src_stride, src0, src1, src2, src3, src4);
    src += (5 * src_stride);

    XORI_B5_128_SB(src0, src1, src2, src3, src4);

    hz_out0 = AVC_XOR_VSHF_B_AND_APPLY_6TAP_HORIZ_FILT_SH(src0, src1,
                                                          mask0, mask1, mask2);
    hz_out2 = AVC_XOR_VSHF_B_AND_APPLY_6TAP_HORIZ_FILT_SH(src2, src3,
                                                          mask0, mask1, mask2);

    PCKOD_D2_SH(hz_out0, hz_out0, hz_out2, hz_out2, hz_out1, hz_out3);

    hz_out4 = AVC_HORZ_FILTER_SH(src4, mask0, mask1, mask2);

    for (loop_cnt = (height >> 2); loop_cnt--;)
    {
        LD_SB4(src, src_stride, src0, src1, src2, src3);
        src += (4 * src_stride);

        XORI_B4_128_SB(src0, src1, src2, src3);

        hz_out5 = AVC_XOR_VSHF_B_AND_APPLY_6TAP_HORIZ_FILT_SH(src0, src1,
                                                              mask0, mask1,
                                                              mask2);
        hz_out7 = AVC_XOR_VSHF_B_AND_APPLY_6TAP_HORIZ_FILT_SH(src2, src3,
                                                              mask0, mask1,
                                                              mask2);

        PCKOD_D2_SH(hz_out5, hz_out5, hz_out7, hz_out7, hz_out6, hz_out8);

        dst0 = AVC_CALC_DPADD_H_6PIX_2COEFF_R_SH(hz_out0, hz_out1, hz_out2,
                                                 hz_out3, hz_out4, hz_out5);
        dst1 = AVC_CALC_DPADD_H_6PIX_2COEFF_R_SH(hz_out1, hz_out2, hz_out3,
                                                 hz_out4, hz_out5, hz_out6);
        dst2 = AVC_CALC_DPADD_H_6PIX_2COEFF_R_SH(hz_out2, hz_out3, hz_out4,
                                                 hz_out5, hz_out6, hz_out7);
        dst3 = AVC_CALC_DPADD_H_6PIX_2COEFF_R_SH(hz_out3, hz_out4, hz_out5,
                                                 hz_out6, hz_out7, hz_out8);

        PCKEV_B2_SB(dst1, dst0, dst3, dst2, src0, src1);
        XORI_B2_128_SB(src0, src1);

        ST4x4_UB(src0, src1, 0, 2, 0, 2, dst, dst_stride);

        dst += (4 * dst_stride);

        hz_out0 = hz_out4;
        hz_out1 = hz_out5;
        hz_out2 = hz_out6;
        hz_out3 = hz_out7;
        hz_out4 = hz_out8;
    }
}

void avc_luma_mid_8w_msa(uint8 *p_src, int32 i_src_stride,
                         uint8 *p_dst, int32 i_dst_stride,
                         int32 i_height)
{
    uint32 u_loop_cnt, h4w;
    uint64_t u_out0;
    v16i8 tmp0;
    v16i8 src0, src1, src2, src3, src4;
    v16i8 mask0, mask1, mask2;
    v8i16 hz_out0, hz_out1, hz_out2, hz_out3;
    v8i16 hz_out4, hz_out5, hz_out6, hz_out7, hz_out8;
    v8i16 dst0, dst1, dst2, dst3;
    v16u8 out0, out1;

    h4w = i_height % 4;
    LD_SB3(&luma_mask_arr[0], 16, mask0, mask1, mask2);

    LD_SB5(p_src, i_src_stride, src0, src1, src2, src3, src4);
    XORI_B5_128_SB(src0, src1, src2, src3, src4);
    p_src += (5 * i_src_stride);

    hz_out0 = AVC_HORZ_FILTER_SH(src0, mask0, mask1, mask2);
    hz_out1 = AVC_HORZ_FILTER_SH(src1, mask0, mask1, mask2);
    hz_out2 = AVC_HORZ_FILTER_SH(src2, mask0, mask1, mask2);
    hz_out3 = AVC_HORZ_FILTER_SH(src3, mask0, mask1, mask2);
    hz_out4 = AVC_HORZ_FILTER_SH(src4, mask0, mask1, mask2);

    for (u_loop_cnt = (i_height >> 2); u_loop_cnt--;)
    {
        LD_SB4(p_src, i_src_stride, src0, src1, src2, src3);
        XORI_B4_128_SB(src0, src1, src2, src3);
        p_src += (4 * i_src_stride);

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
        ST8x4_UB(out0, out1, p_dst, i_dst_stride);

        p_dst += (4 * i_dst_stride);
        hz_out3 = hz_out7;
        hz_out1 = hz_out5;
        hz_out5 = hz_out4;
        hz_out4 = hz_out8;
        hz_out2 = hz_out6;
        hz_out0 = hz_out5;
    }

    for (u_loop_cnt = h4w; u_loop_cnt--;)
    {
        src0 = LD_SB(p_src);
        p_src += i_src_stride;

        src0 = (v16i8) __msa_xori_b((v16u8) src0, 128);
        hz_out5 = AVC_HORZ_FILTER_SH(src0, mask0, mask1, mask2);

        dst0 = AVC_CALC_DPADD_H_6PIX_2COEFF_SH(hz_out0, hz_out1,
                                               hz_out2, hz_out3,
                                               hz_out4, hz_out5);

        tmp0 = __msa_pckev_b((v16i8) (dst0), (v16i8) (dst0));
        tmp0 = (v16i8) __msa_xori_b((v16u8) tmp0, 128);
        u_out0 = __msa_copy_u_d((v2i64) tmp0, 0);
        SD(u_out0, p_dst);
        p_dst += i_dst_stride;

        hz_out0 = hz_out1;
        hz_out1 = hz_out2;
        hz_out2 = hz_out3;
        hz_out3 = hz_out4;
        hz_out4 = hz_out5;
    }
}

void avc_luma_mid_16w_msa(uint8 *src, int32 src_stride,
                          uint8 *dst, int32 dst_stride,
                          int32 height)
{
    uint32 multiple8_cnt;

    for (multiple8_cnt = 2; multiple8_cnt--;)
    {
        avc_luma_mid_8w_msa(src, src_stride, dst, dst_stride, height);
        src += 8;
        dst += 8;
    }
}

void avc_luma_midh_qrt_4w_msa(uint8 *src, int32 src_stride,
                              uint8 *dst, int32 dst_stride,
                              int32 height, uint8 horiz_offset)
{
    uint32 row;
    v16i8 src0, src1, src2, src3, src4, src5, src6;
    v8i16 vt_res0, vt_res1, vt_res2, vt_res3;
    v4i32 hz_res0, hz_res1;
    v8i16 dst0, dst1;
    v8i16 shf_vec0, shf_vec1, shf_vec2, shf_vec3, shf_vec4, shf_vec5;
    v8i16 mask0 = { 0, 5, 1, 6, 2, 7, 3, 8 };
    v8i16 mask1 = { 1, 4, 2, 5, 3, 6, 4, 7 };
    v8i16 mask2 = { 2, 3, 3, 4, 4, 5, 5, 6 };
    v8i16 minus5h = __msa_ldi_h(-5);
    v8i16 plus20h = __msa_ldi_h(20);
    v8i16 zeros = { 0 };
    v16u8 out;

    LD_SB5(src, src_stride, src0, src1, src2, src3, src4);
    src += (5 * src_stride);
    XORI_B5_128_SB(src0, src1, src2, src3, src4);

    for (row = (height >> 1); row--;)
    {
        LD_SB2(src, src_stride, src5, src6);
        src += (2 * src_stride);

        XORI_B2_128_SB(src5, src6);
        AVC_CALC_DPADD_B_6PIX_2COEFF_SH(src0, src1, src2, src3, src4, src5,
                                        vt_res0, vt_res1);
        AVC_CALC_DPADD_B_6PIX_2COEFF_SH(src1, src2, src3, src4, src5, src6,
                                        vt_res2, vt_res3);
        VSHF_H3_SH(vt_res0, vt_res1, vt_res0, vt_res1, vt_res0, vt_res1,
                   mask0, mask1, mask2, shf_vec0, shf_vec1, shf_vec2);
        VSHF_H3_SH(vt_res2, vt_res3, vt_res2, vt_res3, vt_res2, vt_res3,
                   mask0, mask1, mask2, shf_vec3, shf_vec4, shf_vec5);
        hz_res0 = __msa_hadd_s_w(shf_vec0, shf_vec0);
        DPADD_SH2_SW(shf_vec1, shf_vec2, minus5h, plus20h, hz_res0, hz_res0);
        hz_res1 = __msa_hadd_s_w(shf_vec3, shf_vec3);
        DPADD_SH2_SW(shf_vec4, shf_vec5, minus5h, plus20h, hz_res1, hz_res1);

        SRARI_W2_SW(hz_res0, hz_res1, 10);
        SAT_SW2_SW(hz_res0, hz_res1, 7);

        dst0 = __msa_srari_h(shf_vec2, 5);
        dst1 = __msa_srari_h(shf_vec5, 5);

        SAT_SH2_SH(dst0, dst1, 7);

        if (horiz_offset)
        {
            dst0 = __msa_ilvod_h(zeros, dst0);
            dst1 = __msa_ilvod_h(zeros, dst1);
        }
        else
        {
            ILVEV_H2_SH(dst0, zeros, dst1, zeros, dst0, dst1);
        }

        hz_res0 = __msa_aver_s_w(hz_res0, (v4i32) dst0);
        hz_res1 = __msa_aver_s_w(hz_res1, (v4i32) dst1);
        dst0 = __msa_pckev_h((v8i16) hz_res1, (v8i16) hz_res0);

        out = PCKEV_XORI128_UB(dst0, dst0);
        ST4x2_UB(out, dst, dst_stride);

        dst += (2 * dst_stride);

        src0 = src2;
        src1 = src3;
        src2 = src4;
        src3 = src5;
        src4 = src6;
    }
}

void avc_luma_midh_qrt_8w_msa(uint8 *src, int32 src_stride,
                              uint8 *dst, int32 dst_stride,
                              int32 height, uint8 horiz_offset)
{
    uint32 multiple8_cnt;

    for (multiple8_cnt = 2; multiple8_cnt--;)
    {
        avc_luma_midh_qrt_4w_msa(src, src_stride, dst, dst_stride, height,
                                 horiz_offset);

        src += 4;
        dst += 4;
    }
}

void avc_luma_midh_qrt_16w_msa(uint8 *src, int32 src_stride,
                               uint8 *dst, int32 dst_stride,
                               int32 height, uint8 horiz_offset)
{
    uint32 multiple8_cnt;

    for (multiple8_cnt = 4; multiple8_cnt--;)
    {
        avc_luma_midh_qrt_4w_msa(src, src_stride, dst, dst_stride, height,
                                 horiz_offset);

        src += 4;
        dst += 4;
    }
}

void avc_luma_midv_qrt_4w_msa(uint8 *src, int32 src_stride,
                              uint8 *dst, int32 dst_stride,
                              int32 height, uint8 ver_offset)
{
    uint32 loop_cnt;
    v16i8 src0, src1, src2, src3, src4;
    v16i8 mask0, mask1, mask2;
    v8i16 hz_out0, hz_out1, hz_out2, hz_out3;
    v8i16 hz_out4, hz_out5, hz_out6, hz_out7, hz_out8;
    v8i16 dst0, dst1, dst2, dst3, dst4, dst5, dst6, dst7;

    LD_SB3(&luma_mask_arr[48], 16, mask0, mask1, mask2);
    LD_SB5(src, src_stride, src0, src1, src2, src3, src4);
    src += (5 * src_stride);

    XORI_B5_128_SB(src0, src1, src2, src3, src4);

    hz_out0 = AVC_XOR_VSHF_B_AND_APPLY_6TAP_HORIZ_FILT_SH(src0, src1,
                                                          mask0, mask1, mask2);
    hz_out2 = AVC_XOR_VSHF_B_AND_APPLY_6TAP_HORIZ_FILT_SH(src2, src3,
                                                          mask0, mask1, mask2);

    PCKOD_D2_SH(hz_out0, hz_out0, hz_out2, hz_out2, hz_out1, hz_out3);

    hz_out4 = AVC_HORZ_FILTER_SH(src4, mask0, mask1, mask2);

    for (loop_cnt = (height >> 2); loop_cnt--;)
    {
        LD_SB4(src, src_stride, src0, src1, src2, src3);
        src += (4 * src_stride);
        XORI_B4_128_SB(src0, src1, src2, src3);

        hz_out5 = AVC_XOR_VSHF_B_AND_APPLY_6TAP_HORIZ_FILT_SH(src0, src1,
                                                              mask0, mask1,
                                                              mask2);
        hz_out7 = AVC_XOR_VSHF_B_AND_APPLY_6TAP_HORIZ_FILT_SH(src2, src3,
                                                              mask0, mask1,
                                                              mask2);

        PCKOD_D2_SH(hz_out5, hz_out5, hz_out7, hz_out7, hz_out6, hz_out8);

        dst0 = AVC_CALC_DPADD_H_6PIX_2COEFF_SH(hz_out0, hz_out1, hz_out2,
                                               hz_out3, hz_out4, hz_out5);
        dst2 = AVC_CALC_DPADD_H_6PIX_2COEFF_SH(hz_out1, hz_out2, hz_out3,
                                               hz_out4, hz_out5, hz_out6);
        dst4 = AVC_CALC_DPADD_H_6PIX_2COEFF_SH(hz_out2, hz_out3, hz_out4,
                                               hz_out5, hz_out6, hz_out7);
        dst6 = AVC_CALC_DPADD_H_6PIX_2COEFF_SH(hz_out3, hz_out4, hz_out5,
                                               hz_out6, hz_out7, hz_out8);

        if (ver_offset)
        {
            dst1 = __msa_srari_h(hz_out3, 5);
            dst3 = __msa_srari_h(hz_out4, 5);
            dst5 = __msa_srari_h(hz_out5, 5);
            dst7 = __msa_srari_h(hz_out6, 5);
        }
        else
        {
            dst1 = __msa_srari_h(hz_out2, 5);
            dst3 = __msa_srari_h(hz_out3, 5);
            dst5 = __msa_srari_h(hz_out4, 5);
            dst7 = __msa_srari_h(hz_out5, 5);
        }

        SAT_SH4_SH(dst1, dst3, dst5, dst7, 7);

        dst0 = __msa_aver_s_h(dst0, dst1);
        dst1 = __msa_aver_s_h(dst2, dst3);
        dst2 = __msa_aver_s_h(dst4, dst5);
        dst3 = __msa_aver_s_h(dst6, dst7);

        PCKEV_B2_SB(dst1, dst0, dst3, dst2, src0, src1);
        XORI_B2_128_SB(src0, src1);

        ST4x4_UB(src0, src1, 0, 2, 0, 2, dst, dst_stride);

        dst += (4 * dst_stride);
        hz_out0 = hz_out4;
        hz_out1 = hz_out5;
        hz_out2 = hz_out6;
        hz_out3 = hz_out7;
        hz_out4 = hz_out8;
    }
}

void avc_luma_midv_qrt_8w_msa(uint8 *src, int32 src_stride,
                              uint8 *dst, int32 dst_stride,
                              int32 height, uint8 ver_offset)
{
    uint32 loop_cnt;
    v16i8 src0, src1, src2, src3, src4;
    v16i8 mask0, mask1, mask2;
    v8i16 hz_out0, hz_out1, hz_out2, hz_out3;
    v8i16 hz_out4, hz_out5, hz_out6, hz_out7, hz_out8;
    v8i16 dst0, dst1, dst2, dst3, dst4, dst5, dst6, dst7;
    v16u8 out;

    LD_SB3(&luma_mask_arr[0], 16, mask0, mask1, mask2);

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
        dst2 = AVC_CALC_DPADD_H_6PIX_2COEFF_SH(hz_out1, hz_out2, hz_out3,
                                               hz_out4, hz_out5, hz_out6);
        dst4 = AVC_CALC_DPADD_H_6PIX_2COEFF_SH(hz_out2, hz_out3, hz_out4,
                                               hz_out5, hz_out6, hz_out7);
        dst6 = AVC_CALC_DPADD_H_6PIX_2COEFF_SH(hz_out3, hz_out4, hz_out5,
                                               hz_out6, hz_out7, hz_out8);

        if (ver_offset)
        {
            dst1 = __msa_srari_h(hz_out3, 5);
            dst3 = __msa_srari_h(hz_out4, 5);
            dst5 = __msa_srari_h(hz_out5, 5);
            dst7 = __msa_srari_h(hz_out6, 5);
        }
        else
        {
            dst1 = __msa_srari_h(hz_out2, 5);
            dst3 = __msa_srari_h(hz_out3, 5);
            dst5 = __msa_srari_h(hz_out4, 5);
            dst7 = __msa_srari_h(hz_out5, 5);
        }

        SAT_SH4_SH(dst1, dst3, dst5, dst7, 7);

        dst0 = __msa_aver_s_h(dst0, dst1);
        dst1 = __msa_aver_s_h(dst2, dst3);
        dst2 = __msa_aver_s_h(dst4, dst5);
        dst3 = __msa_aver_s_h(dst6, dst7);

        out = PCKEV_XORI128_UB(dst0, dst0);
        ST8x1_UB(out, dst);
        dst += dst_stride;
        out = PCKEV_XORI128_UB(dst1, dst1);
        ST8x1_UB(out, dst);
        dst += dst_stride;
        out = PCKEV_XORI128_UB(dst2, dst2);
        ST8x1_UB(out, dst);
        dst += dst_stride;
        out = PCKEV_XORI128_UB(dst3, dst3);
        ST8x1_UB(out, dst);
        dst += dst_stride;

        hz_out0 = hz_out4;
        hz_out1 = hz_out5;
        hz_out2 = hz_out6;
        hz_out3 = hz_out7;
        hz_out4 = hz_out8;
    }
}

void avc_luma_midv_qrt_16w_msa(uint8 *src, int32 src_stride,
                               uint8 *dst, int32 dst_stride,
                               int32 height, uint8 vert_offset)
{
    uint32 multiple8_cnt;

    for (multiple8_cnt = 2; multiple8_cnt--;)
    {
        avc_luma_midv_qrt_8w_msa(src, src_stride, dst, dst_stride, height,
                                 vert_offset);

        src += 8;
        dst += 8;
    }
}

void avc_luma_hv_qrt_4w_msa(uint8 *src_x, uint8 *src_y,
                            int32 src_stride,
                            uint8 *dst, int32 dst_stride,
                            int32 height)
{
    uint32 loop_cnt;
    v16i8 src_hz0, src_hz1, src_hz2, src_hz3;
    v16i8 src_vt0, src_vt1, src_vt2, src_vt3, src_vt4;
    v16i8 src_vt5, src_vt6, src_vt7, src_vt8;
    v16i8 mask0, mask1, mask2;
    v8i16 hz_out0, hz_out1, vert_out0, vert_out1;
    v8i16 out0, out1;
    v16u8 out;

    LD_SB3(&luma_mask_arr[48], 16, mask0, mask1, mask2);

    LD_SB5(src_y, src_stride, src_vt0, src_vt1, src_vt2, src_vt3, src_vt4);
    src_y += (5 * src_stride);

    src_vt0 = (v16i8) __msa_insve_w((v4i32) src_vt0, 1, (v4i32) src_vt1);
    src_vt1 = (v16i8) __msa_insve_w((v4i32) src_vt1, 1, (v4i32) src_vt2);
    src_vt2 = (v16i8) __msa_insve_w((v4i32) src_vt2, 1, (v4i32) src_vt3);
    src_vt3 = (v16i8) __msa_insve_w((v4i32) src_vt3, 1, (v4i32) src_vt4);

    XORI_B4_128_SB(src_vt0, src_vt1, src_vt2, src_vt3);

    for (loop_cnt = (height >> 2); loop_cnt--;)
    {
        LD_SB4(src_x, src_stride, src_hz0, src_hz1, src_hz2, src_hz3);
        src_x += (4 * src_stride);

        XORI_B4_128_SB(src_hz0, src_hz1, src_hz2, src_hz3);

        hz_out0 = AVC_XOR_VSHF_B_AND_APPLY_6TAP_HORIZ_FILT_SH(src_hz0,
                                                              src_hz1, mask0,
                                                              mask1, mask2);
        hz_out1 = AVC_XOR_VSHF_B_AND_APPLY_6TAP_HORIZ_FILT_SH(src_hz2,
                                                              src_hz3, mask0,
                                                              mask1, mask2);

        SRARI_H2_SH(hz_out0, hz_out1, 5);
        SAT_SH2_SH(hz_out0, hz_out1, 7);

        LD_SB4(src_y, src_stride, src_vt5, src_vt6, src_vt7, src_vt8);
        src_y += (4 * src_stride);

        src_vt4 = (v16i8) __msa_insve_w((v4i32) src_vt4, 1, (v4i32) src_vt5);
        src_vt5 = (v16i8) __msa_insve_w((v4i32) src_vt5, 1, (v4i32) src_vt6);
        src_vt6 = (v16i8) __msa_insve_w((v4i32) src_vt6, 1, (v4i32) src_vt7);
        src_vt7 = (v16i8) __msa_insve_w((v4i32) src_vt7, 1, (v4i32) src_vt8);

        XORI_B4_128_SB(src_vt4, src_vt5, src_vt6, src_vt7);

        /* filter calc */
        vert_out0 = AVC_CALC_DPADD_B_6PIX_2COEFF_R_SH(src_vt0, src_vt1,
                                                      src_vt2, src_vt3,
                                                      src_vt4, src_vt5);
        vert_out1 = AVC_CALC_DPADD_B_6PIX_2COEFF_R_SH(src_vt2, src_vt3,
                                                      src_vt4, src_vt5,
                                                      src_vt6, src_vt7);

        SRARI_H2_SH(vert_out0, vert_out1, 5);
        SAT_SH2_SH(vert_out0, vert_out1, 7);

        out0 = __msa_srari_h((hz_out0 + vert_out0), 1);
        out1 = __msa_srari_h((hz_out1 + vert_out1), 1);

        SAT_SH2_SH(out0, out1, 7);
        out = PCKEV_XORI128_UB(out0, out1);
        ST4x4_UB(out, out, 0, 1, 2, 3, dst, dst_stride);
        dst += (4 * dst_stride);

        src_vt3 = src_vt7;
        src_vt1 = src_vt5;
        src_vt0 = src_vt4;
        src_vt4 = src_vt8;
        src_vt2 = src_vt6;
    }
}

void avc_luma_hv_qrt_8w_msa(uint8 *src_x, uint8 *src_y,
                            int32 src_stride,
                            uint8 *dst, int32 dst_stride,
                            int32 height)
{
    uint32 loop_cnt;
    v16i8 src_hz0, src_hz1, src_hz2, src_hz3;
    v16i8 src_vt0, src_vt1, src_vt2, src_vt3, src_vt4;
    v16i8 src_vt5, src_vt6, src_vt7, src_vt8;
    v16i8 mask0, mask1, mask2;
    v8i16 hz_out0, hz_out1, hz_out2, hz_out3;
    v8i16 vert_out0, vert_out1, vert_out2, vert_out3;
    v8i16 out0, out1, out2, out3;
    v16u8 tmp0, tmp1;

    LD_SB3(&luma_mask_arr[0], 16, mask0, mask1, mask2);
    LD_SB5(src_y, src_stride, src_vt0, src_vt1, src_vt2, src_vt3, src_vt4);
    src_y += (5 * src_stride);

    src_vt0 = (v16i8) __msa_insve_d((v2i64) src_vt0, 1, (v2i64) src_vt1);
    src_vt1 = (v16i8) __msa_insve_d((v2i64) src_vt1, 1, (v2i64) src_vt2);
    src_vt2 = (v16i8) __msa_insve_d((v2i64) src_vt2, 1, (v2i64) src_vt3);
    src_vt3 = (v16i8) __msa_insve_d((v2i64) src_vt3, 1, (v2i64) src_vt4);

    XORI_B4_128_SB(src_vt0, src_vt1, src_vt2, src_vt3);

    for (loop_cnt = (height >> 2); loop_cnt--;)
    {
        LD_SB4(src_x, src_stride, src_hz0, src_hz1, src_hz2, src_hz3);
        XORI_B4_128_SB(src_hz0, src_hz1, src_hz2, src_hz3);
        src_x += (4 * src_stride);

        hz_out0 = AVC_HORZ_FILTER_SH(src_hz0, mask0, mask1, mask2);
        hz_out1 = AVC_HORZ_FILTER_SH(src_hz1, mask0, mask1, mask2);
        hz_out2 = AVC_HORZ_FILTER_SH(src_hz2, mask0, mask1, mask2);
        hz_out3 = AVC_HORZ_FILTER_SH(src_hz3, mask0, mask1, mask2);

        SRARI_H4_SH(hz_out0, hz_out1, hz_out2, hz_out3, 5);
        SAT_SH4_SH(hz_out0, hz_out1, hz_out2, hz_out3, 7);

        LD_SB4(src_y, src_stride, src_vt5, src_vt6, src_vt7, src_vt8);
        src_y += (4 * src_stride);

        src_vt4 = (v16i8) __msa_insve_d((v2i64) src_vt4, 1, (v2i64) src_vt5);
        src_vt5 = (v16i8) __msa_insve_d((v2i64) src_vt5, 1, (v2i64) src_vt6);
        src_vt6 = (v16i8) __msa_insve_d((v2i64) src_vt6, 1, (v2i64) src_vt7);
        src_vt7 = (v16i8) __msa_insve_d((v2i64) src_vt7, 1, (v2i64) src_vt8);

        XORI_B4_128_SB(src_vt4, src_vt5, src_vt6, src_vt7);

        /* filter calc */
        AVC_CALC_DPADD_B_6PIX_2COEFF_SH(src_vt0, src_vt1, src_vt2, src_vt3,
                                        src_vt4, src_vt5, vert_out0, vert_out1);
        AVC_CALC_DPADD_B_6PIX_2COEFF_SH(src_vt2, src_vt3, src_vt4, src_vt5,
                                        src_vt6, src_vt7, vert_out2, vert_out3);

        SRARI_H4_SH(vert_out0, vert_out1, vert_out2, vert_out3, 5);
        SAT_SH4_SH(vert_out0, vert_out1, vert_out2, vert_out3, 7);

        out0 = __msa_srari_h((hz_out0 + vert_out0), 1);
        out1 = __msa_srari_h((hz_out1 + vert_out1), 1);
        out2 = __msa_srari_h((hz_out2 + vert_out2), 1);
        out3 = __msa_srari_h((hz_out3 + vert_out3), 1);

        SAT_SH4_SH(out0, out1, out2, out3, 7);
        tmp0 = PCKEV_XORI128_UB(out0, out1);
        tmp1 = PCKEV_XORI128_UB(out2, out3);
        ST8x4_UB(tmp0, tmp1, dst, dst_stride);

        dst += (4 * dst_stride);
        src_vt3 = src_vt7;
        src_vt1 = src_vt5;
        src_vt5 = src_vt4;
        src_vt4 = src_vt8;
        src_vt2 = src_vt6;
        src_vt0 = src_vt5;
    }
}

void avc_luma_hv_qrt_16w_msa(uint8 *src_x, uint8 *src_y,
                             int32 src_stride,
                             uint8 *dst, int32 dst_stride,
                             int32 height)
{
    uint32 multiple8_cnt;

    for (multiple8_cnt = 2; multiple8_cnt--;)
    {
        avc_luma_hv_qrt_8w_msa(src_x, src_y, src_stride, dst, dst_stride,
                               height);

        src_x += 8;
        src_y += 8;
        dst += 8;
    }
}

void avc_chroma_hz_2x2_msa(uint8 *src, int32 src_stride,
                           uint8 *dst, int32 dst_stride,
                           uint32 coeff0, uint32 coeff1)
{
    uint16 out0, out1;
    v16i8 src0, src1;
    v8u16 res_r;
    v8i16 res;
    v16i8 mask;
    v16i8 coeff_vec0 = __msa_fill_b(coeff0);
    v16i8 coeff_vec1 = __msa_fill_b(coeff1);
    v16u8 coeff_vec = (v16u8) __msa_ilvr_b(coeff_vec0, coeff_vec1);

    mask = LD_SB(&chroma_mask_arr[0]);

    LD_SB2(src, src_stride, src0, src1);

    src0 = __msa_vshf_b(mask, src1, src0);
    res_r = __msa_dotp_u_h((v16u8) src0, coeff_vec);
    res_r <<= 3;
    res_r = (v8u16) __msa_srari_h((v8i16) res_r, 6);
    res_r = __msa_sat_u_h(res_r, 7);
    res = (v8i16) __msa_pckev_b((v16i8) res_r, (v16i8) res_r);

    out0 = __msa_copy_u_h(res, 0);
    out1 = __msa_copy_u_h(res, 2);

    SH(out0, dst);
    dst += dst_stride;
    SH(out1, dst);
}

void avc_chroma_hz_2x4_msa(uint8 *src, int32 src_stride,
                           uint8 *dst, int32 dst_stride,
                           uint32 coeff0, uint32 coeff1)
{
    v16u8 src0, src1, src2, src3;
    v8u16 res_r;
    v8i16 res;
    v16i8 mask;
    v16i8 coeff_vec0 = __msa_fill_b(coeff0);
    v16i8 coeff_vec1 = __msa_fill_b(coeff1);
    v16u8 coeff_vec = (v16u8) __msa_ilvr_b(coeff_vec0, coeff_vec1);

    mask = LD_SB(&chroma_mask_arr[64]);

    LD_UB4(src, src_stride, src0, src1, src2, src3);

    VSHF_B2_UB(src0, src1, src2, src3, mask, mask, src0, src2);

    src0 = (v16u8) __msa_ilvr_d((v2i64) src2, (v2i64) src0);

    res_r = __msa_dotp_u_h(src0, coeff_vec);
    res_r <<= 3;
    res_r = (v8u16) __msa_srari_h((v8i16) res_r, 6);
    res_r = __msa_sat_u_h(res_r, 7);
    res = (v8i16) __msa_pckev_b((v16i8) res_r, (v16i8) res_r);

    ST2x4_UB(res, 0, dst, dst_stride);
}

void avc_chroma_hz_2x8_msa(uint8 *src, int32 src_stride,
                           uint8 *dst, int32 dst_stride,
                           uint32 coeff0, uint32 coeff1)
{
    v16u8 src0, src1, src2, src3, src4, src5, src6, src7;
    v8u16 res_r;
    v8i16 res;
    v16i8 mask;
    v16i8 coeff_vec0 = __msa_fill_b(coeff0);
    v16i8 coeff_vec1 = __msa_fill_b(coeff1);
    v16u8 coeff_vec = (v16u8) __msa_ilvr_b(coeff_vec0, coeff_vec1);

    mask = LD_SB(&chroma_mask_arr[64]);

    LD_UB8(src, src_stride, src0, src1, src2, src3, src4, src5, src6, src7);

    VSHF_B2_UB(src0, src1, src2, src3, mask, mask, src0, src2);
    VSHF_B2_UB(src4, src5, src6, src7, mask, mask, src4, src6);

    ILVR_D2_UB(src2, src0, src6, src4, src0, src4);

    res_r = __msa_dotp_u_h(src0, coeff_vec);
    res_r <<= 3;
    res_r = (v8u16) __msa_srari_h((v8i16) res_r, 6);
    res_r = __msa_sat_u_h(res_r, 7);
    res = (v8i16) __msa_pckev_b((v16i8) res_r, (v16i8) res_r);

    ST2x4_UB(res, 0, dst, dst_stride);
    dst += (4 * dst_stride);

    res_r = __msa_dotp_u_h(src4, coeff_vec);
    res_r <<= 3;
    res_r = (v8u16) __msa_srari_h((v8i16) res_r, 6);
    res_r = __msa_sat_u_h(res_r, 7);
    res = (v8i16) __msa_pckev_b((v16i8) res_r, (v16i8) res_r);

    ST2x4_UB(res, 0, dst, dst_stride);
}

void avc_chroma_hz_2w_msa(uint8 *src, int32 src_stride,
                          uint8 *dst, int32 dst_stride,
                          uint32 coeff0, uint32 coeff1,
                          int32 height)
{
    if (2 == height)
    {
        avc_chroma_hz_2x2_msa(src, src_stride, dst, dst_stride, coeff0, coeff1);
    }
    else if (4 == height)
    {
        avc_chroma_hz_2x4_msa(src, src_stride, dst, dst_stride, coeff0, coeff1);
    }
    else if (8 == height)
    {
        avc_chroma_hz_2x8_msa(src, src_stride, dst, dst_stride, coeff0, coeff1);
    }
}

void avc_chroma_hz_4x2_msa(uint8 *src, int32 src_stride,
                           uint8 *dst, int32 dst_stride,
                           uint32 coeff0, uint32 coeff1)
{
    v16i8 src0, src1;
    v8u16 res_r;
    v4i32 res;
    v16i8 mask;
    v16i8 coeff_vec0 = __msa_fill_b(coeff0);
    v16i8 coeff_vec1 = __msa_fill_b(coeff1);
    v16u8 coeff_vec = (v16u8) __msa_ilvr_b(coeff_vec0, coeff_vec1);

    mask = LD_SB(&chroma_mask_arr[0]);

    LD_SB2(src, src_stride, src0, src1);

    src0 = __msa_vshf_b(mask, src1, src0);
    res_r = __msa_dotp_u_h((v16u8) src0, coeff_vec);
    res_r <<= 3;
    res_r = (v8u16) __msa_srari_h((v8i16) res_r, 6);
    res_r = __msa_sat_u_h(res_r, 7);
    res = (v4i32) __msa_pckev_b((v16i8) res_r, (v16i8) res_r);

    ST4x2_UB(res, dst, dst_stride);
}

void avc_chroma_hz_4x4multiple_msa(uint8 *src, int32 src_stride,
                                   uint8 *dst, int32 dst_stride,
                                   uint32 coeff0, uint32 coeff1,
                                   int32 height)
{
    uint32 row;
    v16u8 src0, src1, src2, src3;
    v8u16 res0_r, res1_r;
    v4i32 res0, res1;
    v16i8 mask;
    v16i8 coeff_vec0 = __msa_fill_b(coeff0);
    v16i8 coeff_vec1 = __msa_fill_b(coeff1);
    v16u8 coeff_vec = (v16u8) __msa_ilvr_b(coeff_vec0, coeff_vec1);

    mask = LD_SB(&chroma_mask_arr[0]);

    for (row = (height >> 2); row--;)
    {
        LD_UB4(src, src_stride, src0, src1, src2, src3);
        src += (4 * src_stride);

        VSHF_B2_UB(src0, src1, src2, src3, mask, mask, src0, src2);
        DOTP_UB2_UH(src0, src2, coeff_vec, coeff_vec, res0_r, res1_r);

        res0_r <<= 3;
        res1_r <<= 3;

        SRARI_H2_UH(res0_r, res1_r, 6);
        SAT_UH2_UH(res0_r, res1_r, 7);
        PCKEV_B2_SW(res0_r, res0_r, res1_r, res1_r, res0, res1);

        ST4x4_UB(res0, res1, 0, 1, 0, 1, dst, dst_stride);
        dst += (4 * dst_stride);
    }
}

void avc_chroma_hz_4w_msa(uint8 *src, int32 src_stride,
                          uint8 *dst, int32 dst_stride,
                          uint32 coeff0, uint32 coeff1,
                          int32 height)
{
    if (2 == height)
    {
        avc_chroma_hz_4x2_msa(src, src_stride, dst, dst_stride, coeff0, coeff1);
    }
    else
    {
        avc_chroma_hz_4x4multiple_msa(src, src_stride, dst, dst_stride, coeff0,
                                      coeff1, height);
    }
}

void avc_chroma_hz_8w_msa(uint8 *src, int32 src_stride,
                          uint8 *dst, int32 dst_stride,
                          uint32 coeff0, uint32 coeff1,
                          int32 height)
{
    uint32 row;
    v16u8 src0, src1, src2, src3, out0, out1;
    v8u16 res0, res1, res2, res3;
    v16i8 mask;
    v16i8 coeff_vec0 = __msa_fill_b(coeff0);
    v16i8 coeff_vec1 = __msa_fill_b(coeff1);
    v16u8 coeff_vec = (v16u8) __msa_ilvr_b(coeff_vec0, coeff_vec1);

    mask = LD_SB(&chroma_mask_arr[32]);

    for (row = height >> 2; row--;)
    {
        LD_UB4(src, src_stride, src0, src1, src2, src3);
        src += (4 * src_stride);

        VSHF_B2_UB(src0, src0, src1, src1, mask, mask, src0, src1);
        VSHF_B2_UB(src2, src2, src3, src3, mask, mask, src2, src3);
        DOTP_UB4_UH(src0, src1, src2, src3, coeff_vec, coeff_vec, coeff_vec,
                    coeff_vec, res0, res1, res2, res3);
        SLLI_4V(res0, res1, res2, res3, 3);
        SRARI_H4_UH(res0, res1, res2, res3, 6);
        SAT_UH4_UH(res0, res1, res2, res3, 7);
        PCKEV_B2_UB(res1, res0, res3, res2, out0, out1);
        ST8x4_UB(out0, out1, dst, dst_stride);
        dst += (4 * dst_stride);
    }

    if (0 != (height % 4))
    {
        for (row = (height % 4); row--;)
        {
            src0 = LD_UB(src);
            src += src_stride;

            src0 = (v16u8) __msa_vshf_b(mask, (v16i8) src0, (v16i8) src0);

            res0 = __msa_dotp_u_h(src0, coeff_vec);
            res0 <<= 3;
            res0 = (v8u16) __msa_srari_h((v8i16) res0, 6);
            res0 = __msa_sat_u_h(res0, 7);
            res0 = (v8u16) __msa_pckev_b((v16i8) res0, (v16i8) res0);

            ST8x1_UB(res0, dst);
            dst += dst_stride;
        }
    }
}

void avc_chroma_vt_2x2_msa(uint8 *src, int32 src_stride,
                           uint8 *dst, int32 dst_stride,
                           uint32 coeff0, uint32 coeff1)
{
    uint16 out0, out1;
    v16i8 src0, src1, src2;
    v16u8 tmp0, tmp1;
    v8i16 res;
    v8u16 res_r;
    v16i8 coeff_vec0 = __msa_fill_b(coeff0);
    v16i8 coeff_vec1 = __msa_fill_b(coeff1);
    v16u8 coeff_vec = (v16u8) __msa_ilvr_b(coeff_vec0, coeff_vec1);

    LD_SB3(src, src_stride, src0, src1, src2);

    ILVR_B2_UB(src1, src0, src2, src1, tmp0, tmp1);

    tmp0 = (v16u8) __msa_ilvr_d((v2i64) tmp1, (v2i64) tmp0);

    res_r = __msa_dotp_u_h(tmp0, coeff_vec);
    res_r <<= 3;
    res_r = (v8u16) __msa_srari_h((v8i16) res_r, 6);
    res_r = __msa_sat_u_h(res_r, 7);
    res = (v8i16) __msa_pckev_b((v16i8) res_r, (v16i8) res_r);

    out0 = __msa_copy_u_h(res, 0);
    out1 = __msa_copy_u_h(res, 2);

    SH(out0, dst);
    dst += dst_stride;
    SH(out1, dst);
}

void avc_chroma_vt_2x4_msa(uint8 *src, int32 src_stride,
                           uint8 *dst, int32 dst_stride,
                           uint32 coeff0, uint32 coeff1)
{
    v16u8 src0, src1, src2, src3, src4;
    v16u8 tmp0, tmp1, tmp2, tmp3;
    v8i16 res;
    v8u16 res_r;
    v16i8 coeff_vec0 = __msa_fill_b(coeff0);
    v16i8 coeff_vec1 = __msa_fill_b(coeff1);
    v16u8 coeff_vec = (v16u8) __msa_ilvr_b(coeff_vec0, coeff_vec1);

    LD_UB5(src, src_stride, src0, src1, src2, src3, src4);
    ILVR_B4_UB(src1, src0, src2, src1, src3, src2, src4, src3,
               tmp0, tmp1, tmp2, tmp3);
    ILVR_W2_UB(tmp1, tmp0, tmp3, tmp2, tmp0, tmp2);

    tmp0 = (v16u8) __msa_ilvr_d((v2i64) tmp2, (v2i64) tmp0);

    res_r = __msa_dotp_u_h(tmp0, coeff_vec);
    res_r <<= 3;
    res_r = (v8u16) __msa_srari_h((v8i16) res_r, 6);
    res_r = __msa_sat_u_h(res_r, 7);

    res = (v8i16) __msa_pckev_b((v16i8) res_r, (v16i8) res_r);

    ST2x4_UB(res, 0, dst, dst_stride);
}

void avc_chroma_vt_2x8_msa(uint8 *src, int32 src_stride,
                           uint8 *dst, int32 dst_stride,
                           uint32 coeff0, uint32 coeff1)
{
    v16u8 src0, src1, src2, src3, src4, src5, src6, src7, src8;
    v16u8 tmp0, tmp1, tmp2, tmp3;
    v8i16 res;
    v8u16 res_r;
    v16i8 coeff_vec0 = __msa_fill_b(coeff0);
    v16i8 coeff_vec1 = __msa_fill_b(coeff1);
    v16u8 coeff_vec = (v16u8) __msa_ilvr_b(coeff_vec0, coeff_vec1);

    LD_UB5(src, src_stride, src0, src1, src2, src3, src4);
    src += (5 * src_stride);
    LD_UB4(src, src_stride, src5, src6, src7, src8);

    ILVR_B4_UB(src1, src0, src2, src1, src3, src2, src4, src3,
               tmp0, tmp1, tmp2, tmp3);
    ILVR_W2_UB(tmp1, tmp0, tmp3, tmp2, tmp0, tmp2);

    tmp0 = (v16u8) __msa_ilvr_d((v2i64) tmp2, (v2i64) tmp0);

    res_r = __msa_dotp_u_h(tmp0, coeff_vec);
    res_r <<= 3;
    res_r = (v8u16) __msa_srari_h((v8i16) res_r, 6);
    res_r = __msa_sat_u_h(res_r, 7);

    res = (v8i16) __msa_pckev_b((v16i8) res_r, (v16i8) res_r);

    ST2x4_UB(res, 0, dst, dst_stride);
    dst += (4 * dst_stride);

    ILVR_B4_UB(src5, src4, src6, src5, src7, src6, src8, src7,
               tmp0, tmp1, tmp2, tmp3);
    ILVR_W2_UB(tmp1, tmp0, tmp3, tmp2, tmp0, tmp2);

    tmp0 = (v16u8) __msa_ilvr_d((v2i64) tmp2, (v2i64) tmp0);

    res_r = __msa_dotp_u_h(tmp0, coeff_vec);
    res_r <<= 3;
    res_r = (v8u16) __msa_srari_h((v8i16) res_r, 6);
    res_r = __msa_sat_u_h(res_r, 7);

    res = (v8i16) __msa_pckev_b((v16i8) res_r, (v16i8) res_r);

    ST2x4_UB(res, 0, dst, dst_stride);
    dst += (4 * dst_stride);
}

void avc_chroma_vt_2w_msa(uint8 *src, int32 src_stride,
                          uint8 *dst, int32 dst_stride,
                          uint32 coeff0, uint32 coeff1,
                          int32 height)
{
    if (2 == height)
    {
        avc_chroma_vt_2x2_msa(src, src_stride, dst, dst_stride, coeff0, coeff1);
    }
    else if (4 == height)
    {
        avc_chroma_vt_2x4_msa(src, src_stride, dst, dst_stride, coeff0, coeff1);
    }
    else if (8 == height)
    {
        avc_chroma_vt_2x8_msa(src, src_stride, dst, dst_stride, coeff0, coeff1);
    }
}

void avc_chroma_vt_4x2_msa(uint8 *src, int32 src_stride,
                           uint8 *dst, int32 dst_stride,
                           uint32 coeff0, uint32 coeff1)
{
    v16u8 src0, src1, src2;
    v16u8 tmp0, tmp1;
    v4i32 res;
    v8u16 res_r;
    v16i8 coeff_vec0 = __msa_fill_b(coeff0);
    v16i8 coeff_vec1 = __msa_fill_b(coeff1);
    v16u8 coeff_vec = (v16u8) __msa_ilvr_b(coeff_vec0, coeff_vec1);

    LD_UB3(src, src_stride, src0, src1, src2);
    ILVR_B2_UB(src1, src0, src2, src1, tmp0, tmp1);

    tmp0 = (v16u8) __msa_ilvr_d((v2i64) tmp1, (v2i64) tmp0);
    res_r = __msa_dotp_u_h(tmp0, coeff_vec);
    res_r <<= 3;
    res_r = (v8u16) __msa_srari_h((v8i16) res_r, 6);
    res_r = __msa_sat_u_h(res_r, 7);
    res = (v4i32) __msa_pckev_b((v16i8) res_r, (v16i8) res_r);

    ST4x2_UB(res, dst, dst_stride);
}

void avc_chroma_vt_4x4multiple_msa(uint8 *src, int32 src_stride,
                                   uint8 *dst, int32 dst_stride,
                                   uint32 coeff0, uint32 coeff1,
                                   int32 height)
{
    uint32 row;
    v16u8 src0, src1, src2, src3, src4;
    v16u8 tmp0, tmp1, tmp2, tmp3;
    v8u16 res0_r, res1_r;
    v4i32 res0, res1;
    v16i8 coeff_vec0 = __msa_fill_b(coeff0);
    v16i8 coeff_vec1 = __msa_fill_b(coeff1);
    v16u8 coeff_vec = (v16u8) __msa_ilvr_b(coeff_vec0, coeff_vec1);

    src0 = LD_UB(src);
    src += src_stride;

    for (row = (height >> 2); row--;)
    {
        LD_UB4(src, src_stride, src1, src2, src3, src4);
        src += (4 * src_stride);

        ILVR_B4_UB(src1, src0, src2, src1, src3, src2, src4, src3,
                   tmp0, tmp1, tmp2, tmp3);
        ILVR_D2_UB(tmp1, tmp0, tmp3, tmp2, tmp0, tmp2);
        DOTP_UB2_UH(tmp0, tmp2, coeff_vec, coeff_vec, res0_r, res1_r);

        res0_r <<= 3;
        res1_r <<= 3;

        SRARI_H2_UH(res0_r, res1_r, 6);
        SAT_UH2_UH(res0_r, res1_r, 7);
        PCKEV_B2_SW(res0_r, res0_r, res1_r, res1_r, res0, res1);

        ST4x4_UB(res0, res1, 0, 1, 0, 1, dst, dst_stride);
        dst += (4 * dst_stride);
        src0 = src4;
    }
}

void avc_chroma_vt_4w_msa(uint8 *src, int32 src_stride,
                          uint8 *dst, int32 dst_stride,
                          uint32 coeff0, uint32 coeff1,
                          int32 height)
{
    if (2 == height)
    {
        avc_chroma_vt_4x2_msa(src, src_stride, dst, dst_stride, coeff0, coeff1);
    }
    else
    {
        avc_chroma_vt_4x4multiple_msa(src, src_stride, dst, dst_stride, coeff0,
                                      coeff1, height);
    }
}

void avc_chroma_vt_8w_msa(uint8 *src, int32 src_stride,
                          uint8 *dst, int32 dst_stride,
                          uint32 coeff0, uint32 coeff1,
                          int32 height)
{
    uint32 row;
    v16u8 src0, src1, src2, src3, src4, out0, out1;
    v8u16 res0, res1, res2, res3;
    v16i8 coeff_vec0 = __msa_fill_b(coeff0);
    v16i8 coeff_vec1 = __msa_fill_b(coeff1);
    v16u8 coeff_vec = (v16u8) __msa_ilvr_b(coeff_vec0, coeff_vec1);

    src0 = LD_UB(src);
    src += src_stride;

    for (row = height >> 2; row--;)
    {
        LD_UB4(src, src_stride, src1, src2, src3, src4);
        src += (4 * src_stride);

        ILVR_B4_UB(src1, src0, src2, src1, src3, src2, src4, src3,
                   src0, src1, src2, src3);
        DOTP_UB4_UH(src0, src1, src2, src3, coeff_vec, coeff_vec, coeff_vec,
                    coeff_vec, res0, res1, res2, res3);
        SLLI_4V(res0, res1, res2, res3, 3);
        SRARI_H4_UH(res0, res1, res2, res3, 6);
        SAT_UH4_UH(res0, res1, res2, res3, 7);
        PCKEV_B2_UB(res1, res0, res3, res2, out0, out1);

        ST8x4_UB(out0, out1, dst, dst_stride);

        dst += (4 * dst_stride);
        src0 = src4;
    }
}

void avc_chroma_hv_2x2_msa(uint8 *src, int32 src_stride,
                           uint8 *dst, int32 dst_stride,
                           uint32 coef_hor0, uint32 coef_hor1,
                           uint32 coef_ver0, uint32 coef_ver1)
{
    uint16 out0, out1;
    v16u8 src0, src1, src2;
    v8u16 res_hz0, res_hz1, res_vt0, res_vt1;
    v8i16 res_vert;
    v16i8 mask;
    v16i8 coeff_hz_vec0 = __msa_fill_b(coef_hor0);
    v16i8 coeff_hz_vec1 = __msa_fill_b(coef_hor1);
    v16u8 coeff_hz_vec = (v16u8) __msa_ilvr_b(coeff_hz_vec0, coeff_hz_vec1);
    v8u16 coeff_vt_vec0 = (v8u16) __msa_fill_h(coef_ver0);
    v8u16 coeff_vt_vec1 = (v8u16) __msa_fill_h(coef_ver1);

    mask = LD_SB(&chroma_mask_arr[48]);

    LD_UB3(src, src_stride, src0, src1, src2);
    VSHF_B2_UB(src0, src1, src1, src2, mask, mask, src0, src1);
    DOTP_UB2_UH(src0, src1, coeff_hz_vec, coeff_hz_vec, res_hz0, res_hz1);
    MUL2(res_hz0, coeff_vt_vec1, res_hz1, coeff_vt_vec0, res_vt0, res_vt1);

    res_vt0 += res_vt1;
    res_vt0 = (v8u16) __msa_srari_h((v8i16) res_vt0, 6);
    res_vt0 = __msa_sat_u_h(res_vt0, 7);
    res_vert = (v8i16) __msa_pckev_b((v16i8) res_vt0, (v16i8) res_vt0);

    out0 = __msa_copy_u_h(res_vert, 0);
    out1 = __msa_copy_u_h(res_vert, 1);

    SH(out0, dst);
    dst += dst_stride;
    SH(out1, dst);
}

void avc_chroma_hv_2x4_msa(uint8 *src, int32 src_stride,
                           uint8 *dst, int32 dst_stride,
                           uint32 coef_hor0, uint32 coef_hor1,
                           uint32 coef_ver0, uint32 coef_ver1)
{
    v16u8 src0, src1, src2, src3, src4;
    v16u8 tmp0, tmp1, tmp2, tmp3;
    v8u16 res_hz0, res_hz1, res_vt0, res_vt1;
    v8i16 res;
    v16i8 mask;
    v16i8 coeff_hz_vec0 = __msa_fill_b(coef_hor0);
    v16i8 coeff_hz_vec1 = __msa_fill_b(coef_hor1);
    v16u8 coeff_hz_vec = (v16u8) __msa_ilvr_b(coeff_hz_vec0, coeff_hz_vec1);
    v8u16 coeff_vt_vec0 = (v8u16) __msa_fill_h(coef_ver0);
    v8u16 coeff_vt_vec1 = (v8u16) __msa_fill_h(coef_ver1);

    mask = LD_SB(&chroma_mask_arr[48]);

    LD_UB5(src, src_stride, src0, src1, src2, src3, src4);

    VSHF_B2_UB(src0, src1, src2, src3, mask, mask, tmp0, tmp1);
    VSHF_B2_UB(src1, src2, src3, src4, mask, mask, tmp2, tmp3);
    ILVR_D2_UB(tmp1, tmp0, tmp3, tmp2, src0, src1);
    DOTP_UB2_UH(src0, src1, coeff_hz_vec, coeff_hz_vec, res_hz0, res_hz1);
    MUL2(res_hz0, coeff_vt_vec1, res_hz1, coeff_vt_vec0, res_vt0, res_vt1);

    res_vt0 += res_vt1;
    res_vt0 = (v8u16) __msa_srari_h((v8i16) res_vt0, 6);
    res_vt0 = __msa_sat_u_h(res_vt0, 7);
    res = (v8i16) __msa_pckev_b((v16i8) res_vt0, (v16i8) res_vt0);

    ST2x4_UB(res, 0, dst, dst_stride);
}

void avc_chroma_hv_2x8_msa(uint8 *src, int32 src_stride,
                           uint8 *dst, int32 dst_stride,
                           uint32 coef_hor0, uint32 coef_hor1,
                           uint32 coef_ver0, uint32 coef_ver1)
{
    v16u8 src0, src1, src2, src3, src4, src5, src6, src7, src8;
    v16u8 tmp0, tmp1, tmp2, tmp3;
    v8u16 res_hz0, res_hz1, res_vt0, res_vt1;
    v8i16 res;
    v16i8 mask;
    v16i8 coeff_hz_vec0 = __msa_fill_b(coef_hor0);
    v16i8 coeff_hz_vec1 = __msa_fill_b(coef_hor1);
    v16u8 coeff_hz_vec = (v16u8) __msa_ilvr_b(coeff_hz_vec0, coeff_hz_vec1);
    v8u16 coeff_vt_vec0 = (v8u16) __msa_fill_h(coef_ver0);
    v8u16 coeff_vt_vec1 = (v8u16) __msa_fill_h(coef_ver1);

    mask = LD_SB(&chroma_mask_arr[48]);

    LD_UB5(src, src_stride, src0, src1, src2, src3, src4);
    src += (5 * src_stride);
    LD_UB4(src, src_stride, src5, src6, src7, src8);

    VSHF_B2_UB(src0, src1, src2, src3, mask, mask, tmp0, tmp1);
    VSHF_B2_UB(src1, src2, src3, src4, mask, mask, tmp2, tmp3);
    ILVR_D2_UB(tmp1, tmp0, tmp3, tmp2, src0, src1);
    VSHF_B2_UB(src4, src5, src6, src7, mask, mask, tmp0, tmp1);
    VSHF_B2_UB(src5, src6, src7, src8, mask, mask, tmp2, tmp3);
    ILVR_D2_UB(tmp1, tmp0, tmp3, tmp2, src4, src5);
    DOTP_UB2_UH(src0, src1, coeff_hz_vec, coeff_hz_vec, res_hz0, res_hz1);
    MUL2(res_hz0, coeff_vt_vec1, res_hz1, coeff_vt_vec0, res_vt0, res_vt1);

    res_vt0 += res_vt1;
    res_vt0 = (v8u16) __msa_srari_h((v8i16) res_vt0, 6);
    res_vt0 = __msa_sat_u_h(res_vt0, 7);

    res = (v8i16) __msa_pckev_b((v16i8) res_vt0, (v16i8) res_vt0);

    ST2x4_UB(res, 0, dst, dst_stride);
    dst += (4 * dst_stride);

    DOTP_UB2_UH(src4, src5, coeff_hz_vec, coeff_hz_vec, res_hz0, res_hz1);
    MUL2(res_hz0, coeff_vt_vec1, res_hz1, coeff_vt_vec0, res_vt0, res_vt1);

    res_vt0 += res_vt1;
    res_vt0 = (v8u16) __msa_srari_h((v8i16) res_vt0, 6);
    res_vt0 = __msa_sat_u_h(res_vt0, 7);

    res = (v8i16) __msa_pckev_b((v16i8) res_vt0, (v16i8) res_vt0);

    ST2x4_UB(res, 0, dst, dst_stride);
}

void avc_chroma_hv_2w_msa(uint8 *src, int32 src_stride,
                          uint8 *dst, int32 dst_stride,
                          uint32 coef_hor0, uint32 coef_hor1,
                          uint32 coef_ver0, uint32 coef_ver1,
                          int32 height)
{
    if (2 == height)
    {
        avc_chroma_hv_2x2_msa(src, src_stride, dst, dst_stride, coef_hor0,
                              coef_hor1, coef_ver0, coef_ver1);
    }
    else if (4 == height)
    {
        avc_chroma_hv_2x4_msa(src, src_stride, dst, dst_stride, coef_hor0,
                              coef_hor1, coef_ver0, coef_ver1);
    }
    else if (8 == height)
    {
        avc_chroma_hv_2x8_msa(src, src_stride, dst, dst_stride, coef_hor0,
                              coef_hor1, coef_ver0, coef_ver1);
    }
}

void avc_chroma_hv_4x2_msa(uint8 *src, int32 src_stride,
                           uint8 *dst, int32 dst_stride,
                           uint32 coef_hor0, uint32 coef_hor1,
                           uint32 coef_ver0, uint32 coef_ver1)
{
    v16u8 src0, src1, src2;
    v8u16 res_hz0, res_hz1, res_vt0, res_vt1;
    v16i8 mask;
    v4i32 res;
    v16i8 coeff_hz_vec0 = __msa_fill_b(coef_hor0);
    v16i8 coeff_hz_vec1 = __msa_fill_b(coef_hor1);
    v16u8 coeff_hz_vec = (v16u8) __msa_ilvr_b(coeff_hz_vec0, coeff_hz_vec1);
    v8u16 coeff_vt_vec0 = (v8u16) __msa_fill_h(coef_ver0);
    v8u16 coeff_vt_vec1 = (v8u16) __msa_fill_h(coef_ver1);

    mask = LD_SB(&chroma_mask_arr[0]);
    LD_UB3(src, src_stride, src0, src1, src2);
    VSHF_B2_UB(src0, src1, src1, src2, mask, mask, src0, src1);
    DOTP_UB2_UH(src0, src1, coeff_hz_vec, coeff_hz_vec, res_hz0, res_hz1);
    MUL2(res_hz0, coeff_vt_vec1, res_hz1, coeff_vt_vec0, res_vt0, res_vt1);

    res_vt0 += res_vt1;
    res_vt0 = (v8u16) __msa_srari_h((v8i16) res_vt0, 6);
    res_vt0 = __msa_sat_u_h(res_vt0, 7);
    res = (v4i32) __msa_pckev_b((v16i8) res_vt0, (v16i8) res_vt0);

    ST4x2_UB(res, dst, dst_stride);
}

void avc_chroma_hv_4x4multiple_msa(uint8 *src, int32 src_stride,
                                   uint8 *dst, int32 dst_stride,
                                   uint32 coef_hor0, uint32 coef_hor1,
                                   uint32 coef_ver0, uint32 coef_ver1,
                                   int32 height)
{
    uint32 row;
    v16u8 src0, src1, src2, src3, src4;
    v8u16 res_hz0, res_hz1, res_hz2, res_hz3;
    v8u16 res_vt0, res_vt1, res_vt2, res_vt3;
    v16i8 mask;
    v16i8 coeff_hz_vec0 = __msa_fill_b(coef_hor0);
    v16i8 coeff_hz_vec1 = __msa_fill_b(coef_hor1);
    v16u8 coeff_hz_vec = (v16u8) __msa_ilvr_b(coeff_hz_vec0, coeff_hz_vec1);
    v8u16 coeff_vt_vec0 = (v8u16) __msa_fill_h(coef_ver0);
    v8u16 coeff_vt_vec1 = (v8u16) __msa_fill_h(coef_ver1);
    v4i32 res0, res1;

    mask = LD_SB(&chroma_mask_arr[0]);

    src0 = LD_UB(src);
    src += src_stride;

    for (row = (height >> 2); row--;)
    {
        LD_UB4(src, src_stride, src1, src2, src3, src4);
        src += (4 * src_stride);

        VSHF_B2_UB(src0, src1, src1, src2, mask, mask, src0, src1);
        VSHF_B2_UB(src2, src3, src3, src4, mask, mask, src2, src3);
        DOTP_UB4_UH(src0, src1, src2, src3, coeff_hz_vec, coeff_hz_vec,
                    coeff_hz_vec, coeff_hz_vec, res_hz0, res_hz1, res_hz2,
                    res_hz3);
        MUL4(res_hz0, coeff_vt_vec1, res_hz1, coeff_vt_vec0, res_hz2,
             coeff_vt_vec1, res_hz3, coeff_vt_vec0, res_vt0, res_vt1, res_vt2,
             res_vt3);
        ADD2(res_vt0, res_vt1, res_vt2, res_vt3, res_vt0, res_vt1);
        SRARI_H2_UH(res_vt0, res_vt1, 6);
        SAT_UH2_UH(res_vt0, res_vt1, 7);
        PCKEV_B2_SW(res_vt0, res_vt0, res_vt1, res_vt1, res0, res1);

        ST4x4_UB(res0, res1, 0, 1, 0, 1, dst, dst_stride);
        dst += (4 * dst_stride);
        src0 = src4;
    }
}

void avc_chroma_hv_4w_msa(uint8 *src, int32 src_stride,
                          uint8 *dst, int32 dst_stride,
                          uint32 coef_hor0, uint32 coef_hor1,
                          uint32 coef_ver0, uint32 coef_ver1,
                          int32 height)
{
    if (2 == height)
    {
        avc_chroma_hv_4x2_msa(src, src_stride, dst, dst_stride, coef_hor0,
                              coef_hor1, coef_ver0, coef_ver1);
    }
    else
    {
        avc_chroma_hv_4x4multiple_msa(src, src_stride, dst, dst_stride,
                                      coef_hor0, coef_hor1, coef_ver0,
                                      coef_ver1, height);
    }
}

void avc_chroma_hv_8w_msa(uint8 *src, int32 src_stride,
                          uint8 *dst, int32 dst_stride,
                          uint32 coef_hor0, uint32 coef_hor1,
                          uint32 coef_ver0, uint32 coef_ver1,
                          int32 height)
{
    uint32 row;
    v16u8 src0, src1, src2, src3, src4, out0, out1;
    v8u16 res_hz0, res_hz1, res_hz2, res_hz3, res_hz4;
    v8u16 res_vt0, res_vt1, res_vt2, res_vt3;
    v16i8 mask;
    v16i8 coeff_hz_vec0 = __msa_fill_b(coef_hor0);
    v16i8 coeff_hz_vec1 = __msa_fill_b(coef_hor1);
    v16u8 coeff_hz_vec = (v16u8) __msa_ilvr_b(coeff_hz_vec0, coeff_hz_vec1);
    v8u16 coeff_vt_vec0 = (v8u16) __msa_fill_h(coef_ver0);
    v8u16 coeff_vt_vec1 = (v8u16) __msa_fill_h(coef_ver1);

    mask = LD_SB(&chroma_mask_arr[32]);

    src0 = LD_UB(src);
    src += src_stride;

    src0 = (v16u8) __msa_vshf_b(mask, (v16i8) src0, (v16i8) src0);
    res_hz0 = __msa_dotp_u_h(src0, coeff_hz_vec);

    for (row = (height >> 2); row--;)
    {
        LD_UB4(src, src_stride, src1, src2, src3, src4);
        src += (4 * src_stride);

        VSHF_B2_UB(src1, src1, src2, src2, mask, mask, src1, src2);
        VSHF_B2_UB(src3, src3, src4, src4, mask, mask, src3, src4);
        DOTP_UB4_UH(src1, src2, src3, src4, coeff_hz_vec, coeff_hz_vec,
                    coeff_hz_vec, coeff_hz_vec, res_hz1, res_hz2, res_hz3,
                    res_hz4);
        MUL4(res_hz1, coeff_vt_vec0, res_hz2, coeff_vt_vec0, res_hz3,
             coeff_vt_vec0, res_hz4, coeff_vt_vec0, res_vt0, res_vt1, res_vt2,
             res_vt3);

        res_vt0 += (res_hz0 * coeff_vt_vec1);
        res_vt1 += (res_hz1 * coeff_vt_vec1);
        res_vt2 += (res_hz2 * coeff_vt_vec1);
        res_vt3 += (res_hz3 * coeff_vt_vec1);

        SRARI_H4_UH(res_vt0, res_vt1, res_vt2, res_vt3, 6);
        SAT_UH4_UH(res_vt0, res_vt1, res_vt2, res_vt3, 7);
        PCKEV_B2_UB(res_vt1, res_vt0, res_vt3, res_vt2, out0, out1);
        ST8x4_UB(out0, out1, dst, dst_stride);

        dst += (4 * dst_stride);

        res_hz0 = res_hz4;
    }
}
#endif /* #ifdef H264ENC_MSA */
