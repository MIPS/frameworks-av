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

#include "basetype.h"
#include "h264bsd_transform.h"
#include "h264bsd_util.h"
#include "h264bsd_macros_msa.h"
#include "h264bsd_types_msa.h"

#ifdef H264DEC_MSA
#define AVC_ITRANS_H(in0, in1, in2, in3, out0, out1, out2, out3)          \
{                                                                         \
    v8i16 tmp0_m, tmp1_m, tmp2_m, tmp3_m;                                 \
                                                                          \
    tmp0_m = in0 + in2;                                                   \
    tmp1_m = in0 - in2;                                                   \
    tmp2_m = in1 >> 1;                                                    \
    tmp2_m = tmp2_m - in3;                                                \
    tmp3_m = in3 >> 1;                                                    \
    tmp3_m = in1 + tmp3_m;                                                \
                                                                          \
    BUTTERFLY_4(tmp0_m, tmp1_m, tmp2_m, tmp3_m, out0, out1, out2, out3);  \
}

#define AVC_ITRANS_W(in0, in1, in2, in3, out0, out1, out2, out3)          \
{                                                                         \
    v4i32 tmp0_m, tmp1_m, tmp2_m, tmp3_m;                                 \
                                                                          \
    tmp0_m = in0 + in2;                                                   \
    tmp1_m = in0 - in2;                                                   \
    tmp2_m = in1 >> 1;                                                    \
    tmp2_m = tmp2_m - in3;                                                \
    tmp3_m = in3 >> 1;                                                    \
    tmp3_m = in1 + tmp3_m;                                                \
                                                                          \
    BUTTERFLY_4(tmp0_m, tmp1_m, tmp2_m, tmp3_m, out0, out1, out2, out3);  \
}

#define AVC_ZIGZAG_SCAN_16_DATA_H(inp1, inp2, out1, out2)                      \
{                                                                              \
    v8i16 zigzag_shf1 = {0, 2, 3, 9, 1, 4, 8, 10};                             \
    v8i16 zigzag_shf2 = {5, 7, 11, 14, 6, 12, 13, 15};                         \
                                                                               \
    VSHF_H2_SH(inp1, inp2, inp1, inp2, zigzag_shf1, zigzag_shf2, out1, out2);  \
}

/* LevelScale function */
static const i8 level_scale[6][3] =
{
    {10, 13, 16},
    {11, 14, 18},
    {13, 16, 20},
    {14, 18, 23},
    {16, 20, 25},
    {18, 23, 29}
};

/* qp % 6 as a function of qp */
static const u8 qp_mod6[52] =
{
    0, 1, 2, 3, 4, 5, 0, 1, 2, 3, 4, 5,
    0, 1, 2, 3, 4, 5, 0, 1, 2, 3, 4, 5,
    0, 1, 2, 3, 4, 5, 0, 1, 2, 3, 4, 5,
    0, 1, 2, 3, 4, 5, 0, 1, 2, 3, 4, 5,
    0, 1, 2, 3
};

/* qp / 6 as a function of qp */
static const u8 qp_div6[52] =
{
    0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1,
    2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3,
    4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5,
    6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7,
    8, 8, 8, 8
};

/* mapping of dc coefficients array to luma blocks */
static const u8 arr_dc_coeff_index[16] =
{
    0, 1, 4, 5, 2, 3, 6, 7, 8, 9, 12, 13, 10, 11, 14, 15
};

void avc_zigzag_idct4x4_luma_dc_msa(i16 *src, u32 qp)
{
    u8 qp_div, qp_mod;
    i16 level_scale_val;
    v8i16 load_data1, load_data2, data1, data2;
    v8i16 vec0, vec1, vec2, vec3;
    v4i32 vec0_r, vec1_r, vec2_r, vec3_r, shift_cnt;
    v8i16 tmp0, tmp1, tmp2, tmp3;
    v8i16 hout0, hout1, hout2, hout3;
    v8i16 vin0, vin1, vin2, vin3;
    v8i16 vout0, vout1;

    qp_div = qp_div6[qp];
    qp_mod = qp_mod6[qp];
    level_scale_val = level_scale[qp_mod][0];

    LD_SH2(src, 8, load_data1, load_data2);
    AVC_ZIGZAG_SCAN_16_DATA_H(load_data1, load_data2, data1, data2);
    PCKEV_D2_SH(data1, data1, data2, data2, vec0, vec2);
    PCKOD_D2_SH(data1, data1, data2, data2, vec1, vec3);
    BUTTERFLY_4(vec0, vec1, vec3, vec2, tmp0, tmp3, tmp2, tmp1);
    BUTTERFLY_4(tmp0, tmp1, tmp2, tmp3, hout0, hout1, hout2, hout3);
    TRANSPOSE4x4_SH_SH(hout0, hout1, hout2, hout3, vin0, vin1, vin2, vin3);
    BUTTERFLY_4(vin0, vin1, vin3, vin2, tmp0, tmp3, tmp2, tmp1);
    BUTTERFLY_4(tmp0, tmp1, tmp2, tmp3, vec0, vec1, vec2, vec3);

    if (qp >= 12)
    {
        v8i16 level_scale0;

        level_scale_val = level_scale_val << (qp_div - 2);
        level_scale0 = __msa_fill_h(level_scale_val);
        vec0 *= level_scale0;
        vec1 *= level_scale0;
        vec2 *= level_scale0;
        vec3 *= level_scale0;

        PCKEV_D2_SH(vec1, vec0, vec3, vec2, vout0, vout1);
    }
    else
    {
        v4i32 level_scale0;

        shift_cnt = __msa_fill_w(2 - qp_div);
        level_scale0 = __msa_fill_w(level_scale_val);

        UNPCK_R_SH_SW(vec0, vec0_r);
        UNPCK_R_SH_SW(vec1, vec1_r);
        UNPCK_R_SH_SW(vec2, vec2_r);
        UNPCK_R_SH_SW(vec3, vec3_r);

        vec0_r *= level_scale0;
        vec1_r *= level_scale0;
        vec2_r *= level_scale0;
        vec3_r *= level_scale0;

        if (qp_div != 1)
        {
            vec0_r += 2;
            vec1_r += 2;
            vec2_r += 2;
            vec3_r += 2;
        }
        else
        {
            vec0_r += 1;
            vec1_r += 1;
            vec2_r += 1;
            vec3_r += 1;
        }
        SRA_4V(vec0_r, vec1_r, vec2_r, vec3_r, shift_cnt);
        PCKEV_H2_SH(vec1_r, vec0_r, vec3_r, vec2_r, vout0, vout1);
    }
    ST_SH(vout0, src);
    ST_SH(vout1, src + 8);
}

void avc_zigzag_idct4x2_chroma_dc_msa(i16 *src, u32 qp)
{
    u8 qp_div;
    i16 level_scale_val;
    v8i16 load_data1;
    v8i16 src0, src1, src2, src3;
    v8i16 vec0, vec1, vec2, vec3, vec4, vec5, vec6, vec7;
    v4i32 vec0_r, vec1_r, vec2_r, vec3_r, vec4_r, vec5_r, vec6_r, vec7_r;
    v8i16 res, res0, res1;
    v8u16 shf_mask = { 0, 4, 1, 5, 2, 6, 3, 7 };

    load_data1 = LD_SH(src);
    load_data1 = __msa_vshf_h((v8i16) shf_mask, load_data1, load_data1);

    qp_div = qp_div6[qp];
    level_scale_val = level_scale[qp_mod6[qp]][0];

    SPLATI_W4_SH(load_data1, src0, src1, src2, src3);
    BUTTERFLY_4(src0, src1, src3, src2, vec0, vec3, vec2, vec1);

    if (qp >= 6)
    {
        v8i16 level_scale0;

        level_scale_val = level_scale_val << (qp_div - 1);
        level_scale0 = __msa_fill_h(level_scale_val);

        BUTTERFLY_4(vec0, vec1, vec2, vec3, vec4, vec6, vec7, vec5);

        vec4 *= level_scale0;
        vec5 *= level_scale0;
        vec6 *= level_scale0;
        vec7 *= level_scale0;
    }
    else
    {
        v4i32 level_scale0;

        level_scale0 = __msa_fill_w(level_scale_val);

        UNPCK_R_SH_SW(vec0, vec0_r);
        UNPCK_R_SH_SW(vec1, vec1_r);
        UNPCK_R_SH_SW(vec2, vec2_r);
        UNPCK_R_SH_SW(vec3, vec3_r);
        BUTTERFLY_4(vec0_r, vec1_r, vec2_r, vec3_r,
                    vec4_r, vec6_r, vec7_r, vec5_r);

        vec4_r *= level_scale0;
        vec5_r *= level_scale0;
        vec6_r *= level_scale0;
        vec7_r *= level_scale0;

        SRA_4V(vec4_r, vec5_r, vec6_r, vec7_r, 1);
        PCKEV_H4_SH(vec4_r, vec4_r, vec5_r, vec5_r, vec6_r, vec6_r, vec7_r,
                    vec7_r, vec4, vec5, vec6, vec7);
    }

    ILVR_H2_SH(vec5, vec4, vec7, vec6, res0, res1);
    res = (v8i16) __msa_ilvr_w((v4i32) res1, (v4i32) res0);
    ST_SH(res, src);
}

u32 avc_zigzag_idct16x16_luma_msa(i16 src_in[][16],
                                  u32 qp,
                                  i16 *total_coeff,
                                  u32 *coeff_map)
{
    i8 one = 1;
    i32 tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
    i16 level_scale_val0, level_scale_val1, level_scale_val2;
    u32 count, coeff;
    const u8 *dc_coeff_idx;
    u8 qp_div;
    i16 *src;
    i16 (*block_dc)[16] = src_in + 24;
    i16 (*block)[16] = src_in;
    v8i16 load_data1, load_data2;
    v8i16 data2, data1 = { 0 };
    v4i32 vec4 = { 0 };
    v8i16 level_scale0, level_scale1;
    v8i16 vec0, vec1, vec2, vec3, vec5;
    v8i16 hout0, hout1, hout2, hout3;
    v4i32 vin0, vin1, vin2, vin3;
    v4i32 vout0, vout1, vout2, vout3;
    v4i32 hout0_r, hout1_r, hout2_r, hout3_r;

    if (total_coeff[24])
    {
        avc_zigzag_idct4x4_luma_dc_msa(*block_dc, qp);
    }

    qp_div = qp_div6[qp];
    level_scale_val0 = level_scale[qp_mod6[qp]][0] << qp_div;
    level_scale_val1 = level_scale[qp_mod6[qp]][1] << qp_div;
    level_scale_val2 = level_scale[qp_mod6[qp]][2] << qp_div;

    vec5 = __msa_fill_h(level_scale_val1);

    level_scale1 = __msa_insert_h(vec5, 0, level_scale_val0);
    level_scale1 = __msa_insert_h(level_scale1, 2, level_scale_val0);
    level_scale1 = __msa_insert_h(level_scale1, 5, level_scale_val2);
    level_scale1 = __msa_insert_h(level_scale1, 7, level_scale_val2);

    level_scale0 = level_scale1;
    level_scale0 = __msa_insert_h(level_scale0, 0, one);
    dc_coeff_idx = arr_dc_coeff_index;

    for (count = 16; count--; block++, total_coeff++, coeff_map++)
    {
        (*block)[0] = (*block_dc)[*dc_coeff_idx++];
        if ((*block)[0] || *total_coeff)
        {
            src = *block;
            coeff = *coeff_map;

            if (coeff & 0xFF9C)
            {
                load_data1 = LD_SH(src);
                load_data2 = LD_SH(src + 8);

                AVC_ZIGZAG_SCAN_16_DATA_H(load_data1, load_data2, data1, data2);

                data1 *= level_scale0;
                data2 *= level_scale1;

                PCKEV_D2_SH(data1, data1, data2, data2, vec0, vec2);
                PCKOD_D2_SH(data1, data1, data2, data2, vec1, vec3);
                AVC_ITRANS_H(vec0, vec1, vec2, vec3,
                             hout0, hout1, hout2, hout3);
                UNPCK_R_SH_SW(hout0, hout0_r);
                UNPCK_R_SH_SW(hout1, hout1_r);
                UNPCK_R_SH_SW(hout2, hout2_r);
                UNPCK_R_SH_SW(hout3, hout3_r);
                TRANSPOSE4x4_SW_SW(hout0_r, hout1_r, hout2_r, hout3_r,
                                   vin0, vin1, vin2, vin3);
                AVC_ITRANS_W(vin0, vin1, vin2, vin3,
                             vout0, vout1, vout2, vout3);
                SRARI_W4_SW(vout0, vout1, vout2, vout3, 6);
                PCKEV_H2_SH(vout1, vout0, vout3, vout2, vec0, vec1);
                ST_SH2(vec0, vec1, src, 8);
            }
            else
            {
                if (0 == (coeff & 0x62))
                {
                    data1 = __msa_fill_h(src[0]);
                    data1 = __msa_srari_h(data1, 6);
                }
                else
                {
                    src[1] *= level_scale_val1;
                    src[2] = src[5] * level_scale_val0;
                    src[3] = src[6] * level_scale_val1;

                    tmp0 = src[0] + src[2];
                    tmp1 = src[0] - src[2];
                    tmp2 = (src[1] >> 1) - src[3];
                    tmp3 = src[1] + (src[3] >> 1);

                    BUTTERFLY_4(tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7);
                    INSERT_W4_SW(tmp4, tmp5, tmp6, tmp7, vec4);

                    vec4 = __msa_srari_w(vec4, 6);
                    data1 = __msa_pckev_h((v8i16) vec4, (v8i16) vec4);
                }
                ST_SH2(data1, data1, src, 8);
            }
        }
        else
        {
            MARK_RESIDUAL_EMPTY(*block);
        }
    }
    return 0;
}

u32 avc_zigzag_idct4x4_luma_msa(i16 src_in[][16],
                                u32 qp,
                                i16 *total_coeff,
                                u32 *coeff_map)
{
    u32 count;
    i16 data;
    i32 tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
    i16 level_scale_val0, level_scale_val1, level_scale_val2;
    u32 coeff;
    u8 qp_div;
    i16 *src;
    i16 (*block)[16] = src_in;
    v8i16 load_data1, load_data2;
    v8i16 data2, data1 = { 0 };
    v8i16 vec0, vec1, vec2, vec3, vec5;
    v4i32 vec4 = { 0 };
    v8i16 level_scale0, level_scale1;
    v8i16 hout0, hout1, hout2, hout3;
    v4i32 vin0, vin1, vin2, vin3;
    v4i32 vout0, vout1, vout2, vout3;
    v4i32 hout0_r, hout1_r, hout2_r, hout3_r;

    qp_div = qp_div6[qp];
    level_scale_val0 = level_scale[qp_mod6[qp]][0] << qp_div;
    level_scale_val1 = level_scale[qp_mod6[qp]][1] << qp_div;
    level_scale_val2 = level_scale[qp_mod6[qp]][2] << qp_div;
    vec5 = __msa_fill_h(level_scale_val1);
    level_scale1 = __msa_insert_h(vec5, 0, level_scale_val0);
    level_scale1 = __msa_insert_h(level_scale1, 2, level_scale_val0);
    level_scale1 = __msa_insert_h(level_scale1, 5, level_scale_val2);
    level_scale1 = __msa_insert_h(level_scale1, 7, level_scale_val2);
    level_scale0 = level_scale1;

    for (count = 16; count--; block++, total_coeff++, coeff_map++)
    {
        if (total_coeff)
        {
            src = *block;
            coeff = *coeff_map;

            if (coeff & 0xFF9C)
            {
                load_data1 = LD_SH(src);
                load_data2 = LD_SH(src + 8);

                AVC_ZIGZAG_SCAN_16_DATA_H(load_data1, load_data2, data1, data2);

                data1 *= level_scale0;
                data2 *= level_scale1;

                PCKEV_D2_SH(data1, data1, data2, data2, vec0, vec2);
                PCKOD_D2_SH(data1, data1, data2, data2, vec1, vec3);
                AVC_ITRANS_H(vec0, vec1, vec2, vec3,
                             hout0, hout1, hout2, hout3);
                UNPCK_R_SH_SW(hout0, hout0_r);
                UNPCK_R_SH_SW(hout1, hout1_r);
                UNPCK_R_SH_SW(hout2, hout2_r);
                UNPCK_R_SH_SW(hout3, hout3_r);
                TRANSPOSE4x4_SW_SW(hout0_r, hout1_r, hout2_r, hout3_r,
                                   vin0, vin1, vin2, vin3);
                AVC_ITRANS_W(vin0, vin1, vin2, vin3,
                             vout0, vout1, vout2, vout3);
                SRARI_W4_SW(vout0, vout1, vout2, vout3, 6);
                PCKEV_H2_SH(vout1, vout0, vout3, vout2, vec0, vec1);
                ST_SH2(vec0, vec1, src, 8);
            }
            else
            {
                if (0 == (coeff & 0x62))
                {
                    data = level_scale_val0 * src[0];

                    data1 = __msa_fill_h(data);
                    data1 = __msa_srari_h(data1, 6);
                }
                else
                {
                    src[0] *= level_scale_val0;
                    src[1] *= level_scale_val1;
                    src[2] = src[5] * level_scale_val0;
                    src[3] = src[6] * level_scale_val1;

                    tmp0 = src[0] + src[2];
                    tmp1 = src[0] - src[2];
                    tmp2 = (src[1] >> 1) - src[3];
                    tmp3 = src[1] + (src[3] >> 1);

                    BUTTERFLY_4(tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7);
                    INSERT_W4_SW(tmp4, tmp5, tmp6, tmp7, vec4);

                    vec4 = __msa_srari_w(vec4, 6);
                    data1 = __msa_pckev_h((v8i16) vec4, (v8i16) vec4);
                }
                ST_SH2(data1, data1, src, 8);
            }
        }
        else
        {
            MARK_RESIDUAL_EMPTY(*block);
        }
    }
    return 0;
}

u32 avc_zigzag_idct4x4_chroma_msa(i16 src_in[][16],
                                  i16 *block_dc,
                                  u32 qp,
                                  i16 *total_coeff,
                                  u32 *coeff_map)
{
    i16 one = 1;
    u32 count;
    i32 tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
    i16 level_scale_val0, level_scale_val1, level_scale_val2;
    u32 coeff;
    u8 qp_div;
    i16 *src;
    i16 (*block)[16] = src_in;
    v8i16 load_data1, load_data2;
    v8i16 data2, data1 = { 0 };
    v4i32 vec4 = { 0 };
    v8i16 vec0, vec1, vec2, vec3, vec5;
    v8i16 level_scale0, level_scale1;
    v8i16 hout0, hout1, hout2, hout3;
    v4i32 vin0, vin1, vin2, vin3;
    v4i32 vout0, vout1, vout2, vout3;
    v4i32 hout0_r, hout1_r, hout2_r, hout3_r;

    qp_div = qp_div6[qp];
    level_scale_val0 = level_scale[qp_mod6[qp]][0] << qp_div;
    level_scale_val1 = level_scale[qp_mod6[qp]][1] << qp_div;
    level_scale_val2 = level_scale[qp_mod6[qp]][2] << qp_div;
    vec5 = __msa_fill_h(level_scale_val1);
    level_scale1 = __msa_insert_h(vec5, 0, level_scale_val0);
    level_scale1 = __msa_insert_h(level_scale1, 2, level_scale_val0);
    level_scale1 = __msa_insert_h(level_scale1, 5, level_scale_val2);
    level_scale1 = __msa_insert_h(level_scale1, 7, level_scale_val2);
    level_scale0 = level_scale1;
    level_scale0 = __msa_insert_h(level_scale0, 0, one);

    for (count = 8; count--; block++, total_coeff++, coeff_map++)
    {
        (*block)[0] = *block_dc++;

        if ((*block)[0] || total_coeff)
        {
            src = *block;
            coeff = *coeff_map;

            if (coeff & 0xFF9C)
            {
                load_data1 = LD_SH(src);
                load_data2 = LD_SH(src + 8);

                AVC_ZIGZAG_SCAN_16_DATA_H(load_data1, load_data2, data1, data2);

                data1 *= level_scale0;
                data2 *= level_scale1;

                PCKEV_D2_SH(data1, data1, data2, data2, vec0, vec2);
                PCKOD_D2_SH(data1, data1, data2, data2, vec1, vec3);
                AVC_ITRANS_H(vec0, vec1, vec2, vec3,
                             hout0, hout1, hout2, hout3);
                UNPCK_R_SH_SW(hout0, hout0_r);
                UNPCK_R_SH_SW(hout1, hout1_r);
                UNPCK_R_SH_SW(hout2, hout2_r);
                UNPCK_R_SH_SW(hout3, hout3_r);
                TRANSPOSE4x4_SW_SW(hout0_r, hout1_r, hout2_r, hout3_r,
                                   vin0, vin1, vin2, vin3);
                AVC_ITRANS_W(vin0, vin1, vin2, vin3,
                             vout0, vout1, vout2, vout3);
                SRARI_W4_SW(vout0, vout1, vout2, vout3, 6);
                PCKEV_H2_SH(vout1, vout0, vout3, vout2, vec0, vec1);
                ST_SH2(vec0, vec1, src, 8);
            }
            else
            {
                if (0 == (coeff & 0x62))
                {
                    data1 = __msa_fill_h(src[0]);
                    data1 = __msa_srari_h(data1, 6);
                }
                else
                {
                    src[1] *= level_scale_val1;
                    src[2] = src[5] * level_scale_val0;
                    src[3] = src[6] * level_scale_val1;

                    tmp0 = src[0] + src[2];
                    tmp1 = src[0] - src[2];
                    tmp2 = (src[1] >> 1) - src[3];
                    tmp3 = src[1] + (src[3] >> 1);

                    BUTTERFLY_4(tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7);
                    INSERT_W4_SW(tmp4, tmp5, tmp6, tmp7, vec4);

                    vec4 = __msa_srari_w(vec4, 6);
                    data1 = __msa_pckev_h((v8i16) vec4, (v8i16) vec4);
                }
                ST_SH2(data1, data1, src, 8);
            }
        }
        else
        {
            MARK_RESIDUAL_EMPTY(*block);
        }
    }
    return 0;
}
#endif /* #ifdef H264DEC_MSA */
