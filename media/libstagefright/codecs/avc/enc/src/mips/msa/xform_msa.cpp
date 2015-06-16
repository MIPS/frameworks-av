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

#include "avcenc_lib.h"
#include "common_macros_msa.h"

#define MAX_VAL 999999

#ifdef H264ENC_MSA
const static int32 dequant_coef[6][16] =
{
    { 10, 13, 13, 10, 16, 10, 13, 13, 13, 13, 16, 10, 16, 13, 13, 16 },
    { 11, 14, 14, 11, 18, 11, 14, 14, 14, 14, 18, 11, 18, 14, 14, 18 },
    { 13, 16, 16, 13, 20, 13, 16, 16, 16, 16, 20, 13, 20, 16, 16, 20 },
    { 14, 18, 18, 14, 23, 14, 18, 18, 18, 18, 23, 14, 23, 18, 18, 23 },
    { 16, 20, 20, 16, 25, 16, 20, 20, 20, 20, 25, 16, 25, 20, 20, 25 },
    { 18, 23, 23, 18, 29, 18, 23, 23, 23, 23, 29, 18, 29, 23, 23, 29 }
};

const static uint8 coef_cost_arr[2][16] =
{
    { 3, 2, 2, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9 }
};

const static uint8 blk_id_xy[4][4] =
{
    { 0, 1, 4, 5 },
    { 2, 3, 6, 7 },
    { 8, 9, 12, 13 },
    { 10, 11, 14, 15 }
};

const static uint8 zz_scan_order_dc[16] =
{
    0, 4, 64, 128, 68, 8, 12, 72, 132, 192, 196, 136, 76, 140, 200, 204
};

const static uint8 zz_scan_order[16] =
{
    0, 1, 16, 32, 17, 2, 3, 18, 33, 48, 49, 34, 19, 35, 50, 51
};

int32 avc_transform_4x4_msa(uint8 *src_ptr, int32 src_stride,
                            uint8 *pred_ptr, int32 pred_stride,
                            uint8 *dst, int32 dst_stride,
                            int16 *coef, int32 coef_stride,
                            int32 *coef_cost, int32 *level,
                            int32 *run, uint8 is_intra_mb,
                            uint8 rq, uint8 qq, int32 qp_const)
{
    uint8 lp_cnt;
    int32 q_bits, zero_run, num_coeff, lev;
    uint32 src0, src1, src2, src3;
    uint32 pred0, pred1, pred2, pred3;
    uint64_t coeff0, coeff1, coeff2, coeff3;
    int16 temp_coef[16];
    v16i8 src = { 0 };
    v16i8 pred = { 0 };
    v16u8 inp0, inp1;
    v8i16 residue0, residue1, residue2, residue3;
    v8i16 tp0, tp1, tp2, tp3;
    v8i16 res0, res1, res2, res3;
    v8i16 vec0, vec1, vec2, vec3;
    v4i32 dst0, dst1;
    v8i16 zigzag_shf1 = { 0, 1, 4, 8, 5, 2, 3, 6 };
    v8i16 zigzag_shf2 = { 9, 12, 13, 10, 7, 11, 14, 15 };
    v8i16 antizigzag_shf1 = { 0, 1, 5, 6, 2, 4, 7, 12 };
    v8i16 antizigzag_shf2 = { 3, 8, 11, 13, 9, 10, 14, 15 };

    LW4(src_ptr, src_stride, src0, src1, src2, src3);
    LW4(pred_ptr, pred_stride, pred0, pred1, pred2, pred3);

    INSERT_W4_SB(src0, src1, src2, src3, src);
    INSERT_W4_SB(pred0, pred1, pred2, pred3, pred);

    ILVRL_B2_UB(src, pred, inp0, inp1);

    /* Copy Predicted */
    if (is_intra_mb)
    {
        SW4(pred0, pred1, pred2, pred3, dst, dst_stride);
    }

    HSUB_UB2_SH(inp0, inp1, residue0, residue2);
    PCKOD_D2_SH(residue0, residue0, residue2, residue2, residue1, residue3);
    TRANSPOSE4x4_SH_SH(residue0, residue1, residue2, residue3,
                       residue0, residue1, residue2, residue3);
    BUTTERFLY_4(residue0, residue1, residue2, residue3, tp0, tp1, tp2, tp3);

    res0 = tp0 + tp1;
    res1 = tp3 << 1;
    res1 = res1 + tp2;
    res2 = tp0 - tp1;
    res3 = tp2 << 1;
    res3 = tp3 - res3;

    TRANSPOSE4x4_SH_SH(res0, res1, res2, res3, res0, res1, res2, res3);
    BUTTERFLY_4(res0, res1, res2, res3, vec0, vec1, vec2, vec3);

    res0 = vec0 + vec1;
    res1 = vec3 << 1;
    res1 += vec2;
    res2 = vec0 - vec1;
    res3 = vec2 << 1;
    res3 = vec3 - res3;

    PCKEV_D2_SH(res1, res0, res3, res2, tp0, tp1);

    /* Zig Zag Scan 4x4 */
    VSHF_H2_SH(tp0, tp1, tp0, tp1, zigzag_shf1, zigzag_shf2, tp2, tp3);

    ST_SH2(tp2, tp3, temp_coef, 8);

    q_bits = qq + 15;
    zero_run = 0;
    num_coeff = 0;

    /* Run Length Calculation */
    for (lp_cnt = 0; lp_cnt < 16; lp_cnt++)
    {
        lev = abs(temp_coef[lp_cnt]);
        lev *= quant_coef[rq][lp_cnt];
        lev += qp_const;
        lev >>= q_bits;

        if (lev)
        {
            if (lev > 1)
            {
                *coef_cost += MAX_VAL;
            }
            else
            {
                *coef_cost += coef_cost_arr[0][zero_run];
            }

            if (temp_coef[lp_cnt] > 0)
            {
                level[num_coeff] = lev;
            }
            else
            {
                level[num_coeff] = -lev;
            }

            run[num_coeff++] = zero_run;
            zero_run = 0;
        }
        else
        {
            zero_run++;
        }

        if (lev)
        {
            if (temp_coef[lp_cnt] > 0)
            {
                temp_coef[lp_cnt] = (lev * dequant_coef[rq][lp_cnt]) << qq;
            }
            else
            {
                temp_coef[lp_cnt] = (-lev * dequant_coef[rq][lp_cnt]) << qq;
            }
        }
        else
        {
            temp_coef[lp_cnt] = 0;
        }
    }

    LD_SH2(temp_coef, 8, tp1, tp3);

    VSHF_H2_SH(tp1, tp3, tp1, tp3, antizigzag_shf1, antizigzag_shf2, tp0, tp2);
    PCKOD_D2_SH(tp0, tp0, tp2, tp2, tp1, tp3);

    if (is_intra_mb)
    {
        if (num_coeff)
        {
            /* Inverse Transform 4x4 */
            /* Horizontal */
            TRANSPOSE4x4_SH_SH(tp0, tp1, tp2, tp3, tp0, tp1, tp2, tp3);
            vec0 = tp0 + tp2;
            vec1 = tp0 - tp2;
            vec2 = tp1 >> 1;
            vec2 = vec2 - tp3;
            vec3 = tp3 >> 1;
            vec3 = tp1 + vec3;
            BUTTERFLY_4(vec0, vec1, vec2, vec3, res0, res1, res2, res3);

            /* Vertical */
            TRANSPOSE4x4_SH_SH(res0, res1, res2, res3, res0, res1, res2, res3);
            vec0 = res0 + res2;
            vec1 = res0 - res2;
            vec2 = res1 >> 1;
            vec2 = vec2 - res3;
            vec3 = res3 >> 1;
            vec3 = res1 + vec3;
            BUTTERFLY_4(vec0, vec1, vec2, vec3, res0, res1, res2, res3);

            /* Add Block */
            SRARI_H4_SH(res0, res1, res2, res3, 6);
            UNPCK_UB_SH(pred, vec0, vec2);
            PCKOD_D2_SH(vec0, vec0, vec2, vec2, vec1, vec3);
            ADD4(res0, vec0, res1, vec1, res2, vec2, res3, vec3,
                 res0, res1, res2, res3);

            /* Clip pixel between 0 to 255 and store*/
            CLIP_SH4_0_255(res0, res1, res2, res3);
            PCKEV_B2_SW(res1, res0, res3, res2, dst0, dst1);
            ST4x4_UB(dst0, dst1, 0, 2, 0, 2, dst, dst_stride);
        }
    }
    else
    {
        coeff0 = __msa_copy_u_d((v2i64) tp0, 0);
        coeff1 = __msa_copy_u_d((v2i64) tp1, 0);
        coeff2 = __msa_copy_u_d((v2i64) tp2, 0);
        coeff3 = __msa_copy_u_d((v2i64) tp3, 0);
        SD4(coeff0, coeff1, coeff2, coeff3, coef, coef_stride);
    }

    return num_coeff;
}

void avc_mb_inter_idct_16x16_msa(uint8 *src_ptr, int32 src_stride,
                                 int16 *coef, int32 coef_stride,
                                 uint8 *mb_nz_coef, int16 cbp)
{
    uint8 lp_cnt0, lp_cnt1;
    uint8 *src_ptr_tmp = src_ptr;
    int16 *coef_tmp = coef;
    uint64_t load0, load1, load2, load3;
    uint32 src0, src1, src2, src3;
    v8i16 coef0, coef1, coef2, coef3;
    v2i64 tmp0 = { 0 };
    v2i64 tmp1 = { 0 };
    v2i64 tmp2 = { 0 };
    v2i64 tmp3 = { 0 };
    v8i16 temp0, temp1, temp2, temp3;
    v8i16 vec0, vec1, vec2, vec3;
    v16u8 src = { 0 };
    v8i16 src0_r, src1_r, src2_r, src3_r;
    v4i32 res0, res1;

    for (lp_cnt0 = 0; lp_cnt0 < 4; lp_cnt0++)
    {
        if (cbp & (1 << lp_cnt0))
        {
            for (lp_cnt1 = 0; lp_cnt1 < 4; lp_cnt1++)
            {
                if (mb_nz_coef[blk_id_xy[lp_cnt0][lp_cnt1]])
                {
                    LD4(coef, coef_stride, load0, load1, load2, load3);

                    tmp0 = __msa_insert_d(tmp0, 0, load0);
                    tmp1 = __msa_insert_d(tmp1, 0, load1);
                    tmp2 = __msa_insert_d(tmp2, 0, load2);
                    tmp3 = __msa_insert_d(tmp3, 0, load3);

                    TRANSPOSE4x4_SH_SH(tmp0, tmp1, tmp2, tmp3,
                                       coef0, coef1, coef2, coef3);

                    temp0 = coef0 + coef2;
                    temp1 = coef0 - coef2;
                    temp2 = coef1 >> 1;
                    temp2 = temp2 - coef3;
                    temp3 = coef3 >> 1;
                    temp3 = coef1 + temp3;

                    BUTTERFLY_4(temp0, temp1, temp2, temp3,
                                vec0, vec1, vec2, vec3);
                    TRANSPOSE4x4_SH_SH(vec0, vec1, vec2, vec3,
                                       vec0, vec1, vec2, vec3);

                    temp0 = vec0 + vec2;
                    temp1 = vec0 - vec2;
                    temp2 = vec1 >> 1;
                    temp2 = temp2 - vec3;
                    temp3 = vec3 >> 1;
                    temp3 = vec1 + temp3;

                    BUTTERFLY_4(temp0, temp1, temp2, temp3,
                                vec0, vec1, vec2, vec3);
                    SRARI_H4_SH(vec0, vec1, vec2, vec3, 6);
                    LW4(src_ptr, src_stride, src0, src1, src2, src3);
                    INSERT_W4_UB(src0, src1, src2, src3, src);
                    UNPCK_UB_SH(src, src0_r, src2_r);
                    PCKOD_D2_SH(src0_r, src0_r, src2_r, src2_r, src1_r, src3_r);
                    ADD4(vec0, src0_r, vec1, src1_r, vec2, src2_r, vec3, src3_r,
                         vec0, vec1, vec2, vec3);
                    CLIP_SH4_0_255(vec0, vec1, vec2, vec3);
                    PCKEV_B2_SW(vec1, vec0, vec3, vec2, res0, res1);
                    ST4x4_UB(res0, res1, 0, 2, 0, 2, src_ptr, src_stride);
                }

                if (1 == lp_cnt1)
                {
                    src_ptr += (4 * src_stride - 4);
                    coef += (4 * coef_stride - 4);
                }
                else
                {
                    src_ptr += 4;
                    coef += 4;
                }
            }
        }

        if (1 == lp_cnt0)
        {
            src_ptr_tmp = src_ptr_tmp + 8 * src_stride;
            src_ptr = src_ptr_tmp;

            coef_tmp = coef_tmp + 8 * coef_stride;
            coef = coef_tmp;
        }
        else
        {
            src_ptr = src_ptr_tmp + 8;
            coef = coef_tmp + 8;
        }
    }
}

void avc_luma_transform_16x16_msa(uint8 *src, int32 src_stride,
                                  uint8 *pred, int32 pred_stride,
                                  uint8 *dst, int32 dst_stride,
                                  int16 *coef, int32 coef_stride,
                                  int32 *level_dc, int32 *run_dc,
                                  int32 *level_ac, int32 *run_ac,
                                  uint8 *mb_nz_coef, uint32 *cbp,
                                  uint8 rq, uint8 qq,
                                  int32 qp_const,
                                  int32 *num_coef_dc)
{
    uint8 lp_cnt0, lp_cnt1, lp_cnt2;
    uint8 zero_run, num_coeff, q_bits;
    int16 tmp0;
    int32 lev;
    uint64_t coef0, coef1, coef2, coef3;
    int32 curr0, curr1, curr2, curr3;
    v16i8 src0, src1, src2, src3;
    v16i8 pred0, pred1, pred2, pred3;
    v16u8 inp0, inp1;
    v8i16 residue0, residue1, residue2, residue3;
    v8i16 temp0, temp1, temp2, temp3;
    v8i16 res00, res01, res02, res03, res10, res11, res12, res13;
    v8i16 res20, res21, res22, res23, res30, res31, res32, res33;
    v8i16 vec_dc0 = { 0 };
    v8i16 vec_dc1 = { 0 };
    v8i16 vec_dc2 = { 0 };
    v8i16 vec_dc3 = { 0 };
    v4i32 vec_dc0_r, vec_dc1_r, vec_dc2_r, vec_dc3_r;
    v4i32 dq_coef, qq_vec, offset;
    v8i16 vec0 = { 0 };
    v8i16 vec1 = { 0 };
    v8i16 vec2 = { 0 };
    v8i16 vec3 = { 0 };
    v4i32 vec4, vec5, vec6, vec7;
    v4i32 vec8, vec9, vec10, vec11;
    v2i64 in0 = { 0 };
    v2i64 in1 = { 0 };
    v2i64 in2 = { 0 };
    v2i64 in3 = { 0 };
    v16i8 zeros = { 0 };

    /* horizontal */
    for (lp_cnt0 = 0; lp_cnt0 < 4; lp_cnt0++)
    {
        LD_SB4(src, src_stride, src0, src1, src2, src3);
        src += (4 * src_stride);
        LD_SB4(pred, pred_stride, pred0, pred1, pred2, pred3);
        pred += (4 * pred_stride);

        ILVRL_B2_UB(src0, pred0, inp0, inp1);
        HSUB_UB2_SH(inp0, inp1, residue0, residue2);
        PCKOD_D2_SH(residue0, residue0, residue2, residue2, residue1, residue3);
        TRANSPOSE4x4_SH_SH(residue0, residue1, residue2, residue3,
                           residue0, residue1, residue2, residue3);
        BUTTERFLY_4(residue0, residue1, residue2, residue3,
                    temp0, temp1, temp2, temp3);

        res00 = temp0 + temp1;
        res01 = temp3 << 1;
        res01 = res01 + temp2;
        res02 = temp0 - temp1;
        res03 = temp2 << 1;
        res03 = temp3 - res03;

        ILVRL_B2_UB(src1, pred1, inp0, inp1);
        HSUB_UB2_SH(inp0, inp1, residue0, residue2);
        PCKOD_D2_SH(residue0, residue0, residue2, residue2, residue1, residue3);
        TRANSPOSE4x4_SH_SH(residue0, residue1, residue2, residue3,
                           residue0, residue1, residue2, residue3);
        BUTTERFLY_4(residue0, residue1, residue2, residue3,
                    temp0, temp1, temp2, temp3);

        res10 = temp0 + temp1;
        res11 = temp3 << 1;
        res11 += temp2;
        res12 = temp0 - temp1;
        res13 = temp2 << 1;
        res13 = temp3 - res13;

        ILVRL_B2_UB(src2, pred2, inp0, inp1);
        HSUB_UB2_SH(inp0, inp1, residue0, residue2);
        PCKOD_D2_SH(residue0, residue0, residue2, residue2, residue1, residue3);
        TRANSPOSE4x4_SH_SH(residue0, residue1, residue2, residue3,
                           residue0, residue1, residue2, residue3);
        BUTTERFLY_4(residue0, residue1, residue2, residue3,
                    temp0, temp1, temp2, temp3);

        res20 = temp0 + temp1;
        res21 = temp3 << 1;
        res21 += temp2;
        res22 = temp0 - temp1;
        res23 = temp2 << 1;
        res23 = temp3 - res23;

        ILVRL_B2_UB(src3, pred3, inp0, inp1);
        HSUB_UB2_SH(inp0, inp1, residue0, residue2);
        PCKOD_D2_SH(residue0, residue0, residue2, residue2, residue1, residue3);
        TRANSPOSE4x4_SH_SH(residue0, residue1, residue2, residue3,
                           residue0, residue1, residue2, residue3);
        BUTTERFLY_4(residue0, residue1, residue2, residue3,
                    temp0, temp1, temp2, temp3);

        res30 = temp0 + temp1;
        res31 = temp3 << 1;
        res31 += temp2;
        res32 = temp0 - temp1;
        res33 = temp2 << 1;
        res33 = temp3 - res33;

        TRANSPOSE4x4_SH_SH(res00, res01, res02, res03,
                           res00, res01, res02, res03);
        TRANSPOSE4x4_SH_SH(res10, res11, res12, res13,
                           res10, res11, res12, res13);
        TRANSPOSE4x4_SH_SH(res20, res21, res22, res23,
                           res20, res21, res22, res23);
        TRANSPOSE4x4_SH_SH(res30, res31, res32, res33,
                           res30, res31, res32, res33);
        PCKEV_D2_SH(res01, res00, res03, res02, res00, res02);
        PCKEV_D2_SH(res11, res10, res13, res12, res10, res12);
        PCKEV_D2_SH(res21, res20, res23, res22, res20, res22);
        PCKEV_D2_SH(res31, res30, res33, res32, res30, res32);
        BUTTERFLY_4(res00, res10, res20, res30, temp0, temp1, temp2, temp3);

        res00 = temp0 + temp1;
        res10 = temp3 << 1;
        res10 += temp2;
        res20 = temp0 - temp1;
        res30 = temp2 << 1;
        res30 = temp3 - res30;

        BUTTERFLY_4(res02, res12, res22, res32, temp0, temp1, temp2, temp3);

        res02 = temp0 + temp1;
        res12 = temp3 << 1;
        res12 += temp2;
        res22 = temp0 - temp1;
        res32 = temp2 << 1;
        res32 = temp3 - res32;

        vec_dc0[lp_cnt0] = __msa_copy_s_h(res00, 0);
        vec_dc1[lp_cnt0] = __msa_copy_s_h(res00, 4);
        vec_dc2[lp_cnt0] = __msa_copy_s_h(res02, 0);
        vec_dc3[lp_cnt0] = __msa_copy_s_h(res02, 4);

        ST_SH2(res00, res02, coef, 8);
        coef += coef_stride;
        ST_SH2(res10, res12, coef, 8);
        coef += coef_stride;
        ST_SH2(res20, res22, coef, 8);
        coef += coef_stride;
        ST_SH2(res30, res32, coef, 8);
        coef += coef_stride;
    }
    pred -= 16 * pred_stride;
    coef -= 16 * coef_stride;

    /* then perform DC transform */
    BUTTERFLY_4(vec_dc0, vec_dc1, vec_dc2, vec_dc3, temp0, temp1, temp2, temp3);
    BUTTERFLY_4(temp0, temp3, temp2, temp1, vec_dc0, vec_dc1, vec_dc3, vec_dc2);
    TRANSPOSE4x4_SH_SH(vec_dc0, vec_dc1, vec_dc2, vec_dc3,
                       vec_dc0, vec_dc1, vec_dc2, vec_dc3);
    BUTTERFLY_4(vec_dc0, vec_dc1, vec_dc2, vec_dc3, temp0, temp1, temp2, temp3);
    UNPCK_R_SH_SW(temp0, vec8);
    UNPCK_R_SH_SW(temp1, vec9);
    UNPCK_R_SH_SW(temp2, vec10);
    UNPCK_R_SH_SW(temp3, vec11);
    BUTTERFLY_4(vec8, vec11, vec10, vec9, vec4, vec5, vec7, vec6);
    SRA_4V(vec4, vec5, vec6, vec7, 1);

    coef[0] = __msa_copy_s_h((v8i16) vec4, 0);
    coef[4] = __msa_copy_s_h((v8i16) vec4, 2);
    coef[8] = __msa_copy_s_h((v8i16) vec4, 4);
    coef[12] = __msa_copy_s_h((v8i16) vec4, 6);
    coef += 4 * coef_stride;
    coef[0] = __msa_copy_s_h((v8i16) vec5, 0);
    coef[4] = __msa_copy_s_h((v8i16) vec5, 2);
    coef[8] = __msa_copy_s_h((v8i16) vec5, 4);
    coef[12] = __msa_copy_s_h((v8i16) vec5, 6);
    coef += 4 * coef_stride;
    coef[0] = __msa_copy_s_h((v8i16) vec6, 0);
    coef[4] = __msa_copy_s_h((v8i16) vec6, 2);
    coef[8] = __msa_copy_s_h((v8i16) vec6, 4);
    coef[12] = __msa_copy_s_h((v8i16) vec6, 6);
    coef += 4 * coef_stride;
    coef[0] = __msa_copy_s_h((v8i16) vec7, 0);
    coef[4] = __msa_copy_s_h((v8i16) vec7, 2);
    coef[8] = __msa_copy_s_h((v8i16) vec7, 4);
    coef[12] = __msa_copy_s_h((v8i16) vec7, 6);
    coef -= (12 * coef_stride);

    q_bits = 15 + qq;
    zero_run = 0;
    num_coeff = 0;

    for (lp_cnt0 = 0; lp_cnt0 < 16; lp_cnt0++)
    {
        if (coef[zz_scan_order_dc[lp_cnt0]] > 0)
        {
            lev = coef[zz_scan_order_dc[lp_cnt0]] * quant_coef[rq][0] +
                  (qp_const << 1);
        }
        else
        {
            lev = -(coef[zz_scan_order_dc[lp_cnt0]]) * quant_coef[rq][0] +
                   (qp_const << 1);
        }

        lev >>= (q_bits + 1);

        if (lev)
        {
            if (coef[zz_scan_order_dc[lp_cnt0]] > 0)
            {
                level_dc[num_coeff] = lev;
                coef[zz_scan_order_dc[lp_cnt0]] = lev;
            }
            else
            {
                level_dc[num_coeff] = -lev;
                coef[zz_scan_order_dc[lp_cnt0]] = -lev;
            }

            run_dc[num_coeff++] = zero_run;
            zero_run = 0;
        }
        else
        {
            zero_run++;
            coef[zz_scan_order_dc[lp_cnt0]] = 0;
        }
    }

    /* inverse transform DC */
    *num_coef_dc = num_coeff;

    if (num_coeff)
    {
        vec_dc0 = __msa_insert_h(vec_dc0, 0, coef[0]);
        vec_dc1 = __msa_insert_h(vec_dc1, 0, coef[4]);
        vec_dc2 = __msa_insert_h(vec_dc2, 0, coef[8]);
        vec_dc3 = __msa_insert_h(vec_dc3, 0, coef[12]);
        coef += 4 * coef_stride;

        vec_dc0 = __msa_insert_h(vec_dc0, 1, coef[0]);
        vec_dc1 = __msa_insert_h(vec_dc1, 1, coef[4]);
        vec_dc2 = __msa_insert_h(vec_dc2, 1, coef[8]);
        vec_dc3 = __msa_insert_h(vec_dc3, 1, coef[12]);
        coef += 4 * coef_stride;

        vec_dc0 = __msa_insert_h(vec_dc0, 2, coef[0]);
        vec_dc1 = __msa_insert_h(vec_dc1, 2, coef[4]);
        vec_dc2 = __msa_insert_h(vec_dc2, 2, coef[8]);
        vec_dc3 = __msa_insert_h(vec_dc3, 2, coef[12]);
        coef += 4 * coef_stride;

        vec_dc0 = __msa_insert_h(vec_dc0, 3, coef[0]);
        vec_dc1 = __msa_insert_h(vec_dc1, 3, coef[4]);
        vec_dc2 = __msa_insert_h(vec_dc2, 3, coef[8]);
        vec_dc3 = __msa_insert_h(vec_dc3, 3, coef[12]);
        coef -= 12 * coef_stride;

        BUTTERFLY_4(vec_dc0, vec_dc2, vec_dc3, vec_dc1,
                    temp0, temp2, temp3, temp1);
        BUTTERFLY_4(temp0, temp1, temp3, temp2,
                    vec_dc0, vec_dc3, vec_dc2, vec_dc1);
        TRANSPOSE4x4_SH_SH(vec_dc0, vec_dc1, vec_dc2, vec_dc3,
                           vec_dc0, vec_dc1, vec_dc2, vec_dc3);
        BUTTERFLY_4(vec_dc0, vec_dc2, vec_dc3, vec_dc1,
                    temp0, temp2, temp3, temp1);
        BUTTERFLY_4(temp0, temp1, temp3, temp2,
                    vec_dc0, vec_dc3, vec_dc2, vec_dc1);

        dq_coef = __msa_fill_w(dequant_coef[rq][0]);

        UNPCK_R_SH_SW(vec_dc0, vec_dc0_r);
        UNPCK_R_SH_SW(vec_dc1, vec_dc1_r);
        UNPCK_R_SH_SW(vec_dc2, vec_dc2_r);
        UNPCK_R_SH_SW(vec_dc3, vec_dc3_r);

        vec_dc0_r *= dq_coef;
        vec_dc1_r *= dq_coef;
        vec_dc2_r *= dq_coef;
        vec_dc3_r *= dq_coef;

        if (qq >= 2)
        {
            qq -= 2;

            qq_vec = __msa_fill_w(qq);

            SLLI_4V(vec_dc0_r, vec_dc1_r, vec_dc2_r, vec_dc3_r, qq_vec);

            qq += 2;
        }
        else
        {
            qq = 2 - qq;

            qq_vec = __msa_fill_w(qq);
            offset = __msa_fill_w(1 << (qq - 1));

            ADD4(vec_dc0_r, offset, vec_dc1_r, offset, vec_dc2_r, offset,
                 vec_dc3_r, offset, vec_dc0_r, vec_dc1_r, vec_dc2_r, vec_dc3_r);
            SRA_4V(vec_dc0_r, vec_dc1_r, vec_dc2_r, vec_dc3_r, qq_vec);

            qq = 2 - qq;
        }

        coef[0] = __msa_copy_s_h((v8i16) vec_dc0_r, 0);
        coef[4] = __msa_copy_s_h((v8i16) vec_dc0_r, 2);
        coef[8] = __msa_copy_s_h((v8i16) vec_dc0_r, 4);
        coef[12] = __msa_copy_s_h((v8i16) vec_dc0_r, 6);
        coef += 4 * coef_stride;

        coef[0] = __msa_copy_s_h((v8i16) vec_dc1_r, 0);
        coef[4] = __msa_copy_s_h((v8i16) vec_dc1_r, 2);
        coef[8] = __msa_copy_s_h((v8i16) vec_dc1_r, 4);
        coef[12] = __msa_copy_s_h((v8i16) vec_dc1_r, 6);
        coef += 4 * coef_stride;

        coef[0] = __msa_copy_s_h((v8i16) vec_dc2_r, 0);
        coef[4] = __msa_copy_s_h((v8i16) vec_dc2_r, 2);
        coef[8] = __msa_copy_s_h((v8i16) vec_dc2_r, 4);
        coef[12] = __msa_copy_s_h((v8i16) vec_dc2_r, 6);
        coef += 4 * coef_stride;

        coef[0] = __msa_copy_s_h((v8i16) vec_dc3_r, 0);
        coef[4] = __msa_copy_s_h((v8i16) vec_dc3_r, 2);
        coef[8] = __msa_copy_s_h((v8i16) vec_dc3_r, 4);
        coef[12] = __msa_copy_s_h((v8i16) vec_dc3_r, 6);
        coef -= 12 * coef_stride;
    }
    *cbp = 0;

    for (lp_cnt0 = 0; lp_cnt0 < 4; lp_cnt0++)
    {
        for (lp_cnt1 = 0; lp_cnt1 < 4; lp_cnt1++)
        {
            zero_run = 0;
            num_coeff = 0;

            for (lp_cnt2 = 1; lp_cnt2 < 16; lp_cnt2++)
            {
                if (coef[zz_scan_order[lp_cnt2]] > 0)
                {
                    lev = coef[zz_scan_order[lp_cnt2]] *
                          quant_coef[rq][lp_cnt2] + qp_const;
                }
                else
                {
                    lev = -(coef[zz_scan_order[lp_cnt2]]) *
                          quant_coef[rq][lp_cnt2] + qp_const;
                }

                lev >>= q_bits;

                if (lev)
                {
                    if (coef[zz_scan_order[lp_cnt2]] > 0)
                    {
                        level_ac[num_coeff] = lev;
                        coef[zz_scan_order[lp_cnt2]] =
                            (lev * dequant_coef[rq][lp_cnt2]) << qq;
                    }
                    else
                    {
                        level_ac[num_coeff] = -lev;
                        coef[zz_scan_order[lp_cnt2]] =
                            (-lev * dequant_coef[rq][lp_cnt2]) << qq;
                    }

                    run_ac[num_coeff++] = zero_run;
                    zero_run = 0;
                }
                else
                {
                    zero_run++;
                    coef[zz_scan_order[lp_cnt2]] = 0;
                }
            }

            mb_nz_coef[blk_id_xy[lp_cnt0][lp_cnt1]] = num_coeff;

            if (num_coeff)
            {
                *cbp |= (1 << lp_cnt0);

                LD4(coef, coef_stride, coef0, coef1, coef2, coef3);

                in0 = __msa_insert_d(in0, 0, coef0);
                in1 = __msa_insert_d(in1, 0, coef1);
                in2 = __msa_insert_d(in2, 0, coef2);
                in3 = __msa_insert_d(in3, 0, coef3);

                TRANSPOSE4x4_SH_SH(in0, in1, in2, in3,
                                   temp0, temp1, temp2, temp3);

                vec0 = temp0 + temp2;
                vec1 = temp0 - temp2;
                vec2 = temp1 >> 1;
                vec2 = vec2 - temp3;
                vec3 = temp3 >> 1;
                vec3 = temp1 + vec3;

                BUTTERFLY_4(vec0, vec1, vec2, vec3, temp0, temp1, temp2, temp3);
                TRANSPOSE4x4_SH_SH(temp0, temp1, temp2, temp3,
                                   temp0, temp1, temp2, temp3);

                coef0 = __msa_copy_u_d((v2i64) temp0, 0);
                coef1 = __msa_copy_u_d((v2i64) temp1, 0);
                coef2 = __msa_copy_u_d((v2i64) temp2, 0);
                coef3 = __msa_copy_u_d((v2i64) temp3, 0);

                SD4(coef0, coef1, coef2, coef3, coef, coef_stride);

                vec0 = temp0 + temp2;
                vec1 = temp0 - temp2;
                vec2 = temp1 >> 1;
                vec2 = vec2 - temp3;
                vec3 = temp3 >> 1;
                vec3 = temp1 + vec3;

                UNPCK_R_SH_SW(vec0, vec4);
                UNPCK_R_SH_SW(vec1, vec5);
                UNPCK_R_SH_SW(vec2, vec6);
                UNPCK_R_SH_SW(vec3, vec7);

                BUTTERFLY_4(vec4, vec5, vec6, vec7, vec8, vec9, vec10, vec11);
                SRARI_W4_SW(vec8, vec9, vec10, vec11, 6);
                PCKEV_H4_SH(vec8, vec8, vec9, vec9, vec10, vec10, vec11, vec11,
                            temp0, temp1, temp2, temp3);
                LW4(pred, pred_stride, curr0, curr1, curr2, curr3);

                vec0 = (v8i16) __msa_insert_w((v4i32) vec0, 0, curr0);
                vec1 = (v8i16) __msa_insert_w((v4i32) vec1, 0, curr1);
                vec2 = (v8i16) __msa_insert_w((v4i32) vec2, 0, curr2);
                vec3 = (v8i16) __msa_insert_w((v4i32) vec3, 0, curr3);

                ILVR_B4_SH(zeros, vec0, zeros, vec1, zeros, vec2, zeros, vec3,
                           vec0, vec1, vec2, vec3);
                ADD4(vec0, temp0, vec1, temp1, vec2, temp2, vec3, temp3,
                     vec0, vec1, vec2, vec3);
            }
            else
            {
                tmp0 = (coef[0] + 32) >> 6;

                temp0 = __msa_fill_h(tmp0);

                LW4(pred, pred_stride, curr0, curr1, curr2, curr3);

                vec0 = (v8i16) __msa_insert_w((v4i32) vec0, 0, curr0);
                vec1 = (v8i16) __msa_insert_w((v4i32) vec1, 0, curr1);
                vec2 = (v8i16) __msa_insert_w((v4i32) vec2, 0, curr2);
                vec3 = (v8i16) __msa_insert_w((v4i32) vec3, 0, curr3);

                ILVR_B4_SH(zeros, vec0, zeros, vec1, zeros, vec2, zeros, vec3,
                           vec0, vec1, vec2, vec3);
                ADD4(vec0, temp0, vec1, temp0, vec2, temp0, vec3, temp0,
                     vec0, vec1, vec2, vec3);
            }

            CLIP_SH4_0_255(vec0, vec1, vec2, vec3);
            PCKEV_ST4x4_UB(vec0, vec1, vec2, vec3, dst, dst_stride);

            run_ac += 16;
            level_ac += 16;
            dst += 4;
            pred += 4;
            coef += 4;

            dst += (lp_cnt1 & 1) * ((dst_stride << 2) - 8);
            pred += (lp_cnt1 & 1) * 56;
            coef += (lp_cnt1 & 1) * 56;
        }

        if (0 == lp_cnt0 || 2 == lp_cnt0)
        {
            dst += 8 - (dst_stride << 3);
            pred -= 120;
            coef -= 120;
        }
        else
        {
            dst -= 8;
            pred -= 8;
            coef -= 8;
        }
    }
    return;
}

void avc_chroma_transform_8x8_msa(uint8 *src, int32 src_stride,
                                  uint8 *pred, int32 pred_stride,
                                  uint8 *dst, int32 dst_stride,
                                  int16 *coef, int32 coef_stride,
                                  int32 *coef_cost,
                                  int32 *level_dc, int32 *run_dc,
                                  int32 *level_ac, int32 *run_ac,
                                  uint8 is_intra_mb, int32 *num_coef_cdc,
                                  uint8 *mb_nz_coef, uint32 *cbp,
                                  uint8 cr, uint8 rq,
                                  uint8 qq, int32 qp_const)
{
    uint8 lp_cnt0, lp_cnt1;
    uint8 idx, q_bits, zero_run, num_coeff;
    int16 temp0, temp1, temp2, temp3;
    uint32 pred0, pred1, pred2, pred3;
    int32 lev, nz_tmp[4];
    uint64_t coef0, coef1, coef2, coef3;
    uint64_t src0, src1, src2, src3;
    v16u8 inp0, inp1, inp2, inp3;
    v8i16 residue0, residue1, residue2, residue3;
    v8i16 vec0, vec1, vec2, vec3, vec4, vec5, vec6, vec7;
    v8i16 res0, res1, res2, res3;
    v4i32 tmp0 = { 0 };
    v4i32 tmp1 = { 0 };
    v4i32 tmp2 = { 0 };
    v4i32 tmp3 = { 0 };
    v2i64 inp4 = { 0 };
    v2i64 inp5 = { 0 };
    v2i64 inp6 = { 0 };
    v2i64 inp7 = { 0 };
    v16i8 zeros = { 0 };

    if (cr)
    {
        coef += 8;
        pred += 8;
    }

    if (0 == is_intra_mb)
    {
        pred = dst;
        pred_stride = dst_stride;
    }

    /* do 4x4 transform */
    /* horizontal */
    for (lp_cnt0 = 2; lp_cnt0--;)
    {
        uint64_t pred0, pred1, pred2, pred3;

        LD4(src, src_stride, src0, src1, src2, src3);
        src += (4 * src_stride);
        LD4(pred, pred_stride, pred0, pred1, pred2, pred3);
        pred += (4 * pred_stride);

        inp4 = __msa_insert_d(inp4, 0, src0);
        inp5 = __msa_insert_d(inp5, 0, src1);
        inp6 = __msa_insert_d(inp6, 0, pred0);
        inp7 = __msa_insert_d(inp7, 0, pred1);
        ILVR_B2_UB(inp4, inp6, inp5, inp7, inp0, inp1);

        inp4 = __msa_insert_d(inp4, 0, src2);
        inp5 = __msa_insert_d(inp5, 0, src3);
        inp6 = __msa_insert_d(inp6, 0, pred2);
        inp7 = __msa_insert_d(inp7, 0, pred3);
        ILVR_B2_UB(inp4, inp6, inp5, inp7, inp2, inp3);

        HSUB_UB2_SH(inp0, inp1, residue0, residue1);
        HSUB_UB2_SH(inp2, inp3, residue2, residue3);

        TRANSPOSE4x4_SH_SH(residue0, residue1, residue2, residue3,
                           vec4, vec5, vec6, vec7);
        PCKOD_D2_SH(residue0, residue0, residue1, residue1, residue0, residue1);
        PCKOD_D2_SH(residue2, residue2, residue3, residue3, residue2, residue3);
        TRANSPOSE4x4_SH_SH(residue0, residue1, residue2, residue3,
                           residue0, residue1, residue2, residue3);
        BUTTERFLY_4(vec4, vec5, vec6, vec7, vec0, vec1, vec2, vec3);
        BUTTERFLY_4(residue0, residue1, residue2, residue3,
                    vec4, vec5, vec6, vec7);

        res0 = vec0 + vec1;
        res1 = vec3 << 1;
        res1 = res1 + vec2;
        res2 = vec0 - vec1;
        res3 = vec2 << 1;
        res3 = vec3 - res3;
        vec0 = vec4 + vec5;
        vec1 = vec7 << 1;
        vec1 = vec1 + vec6;
        vec2 = vec4 - vec5;
        vec3 = vec6 << 1;
        vec3 = vec7 - vec3;

        TRANSPOSE4x4_SH_SH(res0, res1, res2, res3, res0, res1, res2, res3);
        TRANSPOSE4x4_SH_SH(vec0, vec1, vec2, vec3, vec0, vec1, vec2, vec3);
        BUTTERFLY_4(res0, res1, res2, res3, vec4, vec5, vec6, vec7);
        BUTTERFLY_4(vec0, vec1, vec2, vec3, res0, res1, res2, res3);

        vec0 = vec4 + vec5;
        vec1 = vec7 << 1;
        vec1 = vec1 + vec6;
        vec2 = vec4 - vec5;
        vec3 = vec6 << 1;
        vec3 = vec7 - vec3;
        vec4 = res0 + res1;
        vec5 = res3 << 1;
        vec5 = vec5 + res2;
        vec6 = res0 - res1;
        vec7 = res2 << 1;
        vec7 = res3 - vec7;

        PCKEV_D2_SH(vec4, vec0, vec5, vec1, res0, res1);
        PCKEV_D2_SH(vec6, vec2, vec7, vec3, res2, res3);
        ST_SH4(res0, res1, res2, res3, coef, coef_stride);
        coef += (4 * coef_stride);
    }
    /* DC transform */
    pred -= 8 * pred_stride;
    coef -= 8 * coef_stride;

    /* 2x2 transform of DC components */
    temp0 = coef[0];
    temp1 = coef[4];
    temp2 = coef[64];
    temp3 = coef[68];

    coef[0] = temp0 + temp1 + temp2 + temp3;
    coef[4] = temp0 - temp1 + temp2 - temp3;
    coef[64] = temp0 + temp1 - temp2 - temp3;
    coef[68] = temp0 - temp1 - temp2 + temp3;

    q_bits = qq + 15;
    zero_run = 0;
    num_coeff = 0;
    run_dc += (cr << 2);
    level_dc += (cr << 2);

    for (lp_cnt0 = 0; lp_cnt0 < 4; lp_cnt0++)
    {
        idx = ((lp_cnt0 >> 1) << 6) + ((lp_cnt0 & 1) << 2);

        if (coef[idx] > 0)
        {
            lev = coef[idx] * quant_coef[rq][0] + (qp_const << 1);
        }
        else
        {
            lev = -coef[idx] * quant_coef[rq][0] + (qp_const << 1);
        }

        lev >>= (q_bits + 1);

        if (lev)
        {
            if (coef[idx] > 0)
            {
                level_dc[num_coeff] = lev;
                coef[idx] = lev;
            }
            else
            {
                level_dc[num_coeff] = -lev;
                coef[idx] = -lev;
            }
            run_dc[num_coeff++] = zero_run;
            zero_run = 0;
        }
        else
        {
            zero_run++;
            coef[idx] = 0;
        }
    }
    num_coef_cdc[cr] = num_coeff;
    if (num_coeff)
    {
        int16 res0, res1, res2, res3;

        *cbp |= (1 << 4);
        temp0 = coef[0] + coef[4];
        temp1 = coef[0] - coef[4];
        temp2 = coef[64] + coef[68];
        temp3 = coef[64] - coef[68];

        BUTTERFLY_4(temp0, temp1, temp3, temp2, res0, res1, res3, res2);

        if (qq >= 1)
        {
            qq -= 1;
            coef[0] = (res0 * dequant_coef[rq][0]) << qq;
            coef[4] = (res1 * dequant_coef[rq][0]) << qq;
            coef[64] = (res2 * dequant_coef[rq][0]) << qq;
            coef[68] = (res3 * dequant_coef[rq][0]) << qq;
            qq++;
        }
        else
        {
            coef[0] = (res0 * dequant_coef[rq][0]) >> 1;
            coef[4] = (res1 * dequant_coef[rq][0]) >> 1;
            coef[64] = (res2 * dequant_coef[rq][0]) >> 1;
            coef[68] = (res3 * dequant_coef[rq][0]) >> 1;
        }
    }

    /* now do AC zigzag scan, quant, iquant and itrans */
    *coef_cost = 0;
    for (lp_cnt0 = 0; lp_cnt0 < 4; lp_cnt0++)
    {
        zero_run = 0;
        num_coeff = 0;

        for (lp_cnt1 = 1; lp_cnt1 < 16; lp_cnt1++)
        {
            if (coef[zz_scan_order[lp_cnt1]] > 0)
            {
                lev = coef[zz_scan_order[lp_cnt1]] *
                      quant_coef[rq][lp_cnt1] + qp_const;
            }
            else
            {
                lev = -coef[zz_scan_order[lp_cnt1]] *
                      quant_coef[rq][lp_cnt1] + qp_const;
            }

            lev >>= q_bits;

            if (lev)
            {
                if (lev > 1)
                {
                    *coef_cost += MAX_VAL;
                }
                else
                {
                    *coef_cost += coef_cost_arr[0][zero_run];
                }

                if (coef[zz_scan_order[lp_cnt1]] > 0)
                {
                    level_ac[num_coeff] = lev;
                    coef[zz_scan_order[lp_cnt1]] =
                        (lev * dequant_coef[rq][lp_cnt1]) << qq;
                }
                else
                {
                    level_ac[num_coeff] = -lev;
                    coef[zz_scan_order[lp_cnt1]] =
                        (-lev * dequant_coef[rq][lp_cnt1]) << qq;
                }

                run_ac[num_coeff++] = zero_run;
                zero_run = 0;
            }
            else
            {
                zero_run++;
                coef[zz_scan_order[lp_cnt1]] = 0;
            }
        }
        nz_tmp[lp_cnt0] = num_coeff;
        if (lp_cnt0 & 1)
        {
            coef += 4 * coef_stride - 4;
        }
        else
        {
            coef += 4;
        }

        run_ac += 16;
        level_ac += 16;
    }
    coef -= 8 * coef_stride;
    if (*coef_cost < _CHROMA_COEFF_COST_)
    {
        mb_nz_coef[16 + (cr << 1)] = 0;
        mb_nz_coef[17 + (cr << 1)] = 0;
        mb_nz_coef[20 + (cr << 1)] = 0;
        mb_nz_coef[21 + (cr << 1)] = 0;

        for (lp_cnt0 = 0; lp_cnt0 < 4; lp_cnt0++)
        {
            temp0 = (coef[0] + 32) >> 6;
            vec4 = __msa_fill_h(temp0);

            LW4(pred, pred_stride, pred0, pred1, pred2, pred3);

            tmp0 = __msa_insert_w(tmp0, 0, pred0);
            tmp1 = __msa_insert_w(tmp1, 0, pred1);
            tmp2 = __msa_insert_w(tmp2, 0, pred2);
            tmp3 = __msa_insert_w(tmp3, 0, pred3);

            ILVR_B4_SH(zeros, tmp0, zeros, tmp1, zeros, tmp2, zeros, tmp3,
                       vec0, vec1, vec2, vec3);
            ADD4(vec0, vec4, vec1, vec4, vec2, vec4, vec3, vec4,
                 vec0, vec1, vec2, vec3);
            CLIP_SH4_0_255(vec0, vec1, vec2, vec3);
            PCKEV_ST4x4_UB(vec0, vec1, vec2, vec3, dst, dst_stride);

            if (lp_cnt0 & 1)
            {
                coef += 4 * coef_stride - 4;
                pred += 4 * pred_stride - 4;
                dst += 4 * dst_stride - 4;
            }
            else
            {
                coef += 4;
                pred += 4;
                dst += 4;
            }
        }
    }
    else
    {
        for (lp_cnt0 = 0; lp_cnt0 < 4; lp_cnt0++)
        {
            num_coeff = nz_tmp[lp_cnt0];
            mb_nz_coef[16 + (lp_cnt0 & 1) + (cr << 1) + ((lp_cnt0 >> 1) << 2)]
                = num_coeff;

            if (num_coeff)
            {
                uint64_t coeff0, coeff1, coeff2, coeff3;

                *cbp |= (2 << 4);

                LD4(coef, coef_stride, coef0, coef1, coef2, coef3);

                vec4 = (v8i16) __msa_insert_d((v2i64) vec4, 0, coef0);
                vec5 = (v8i16) __msa_insert_d((v2i64) vec5, 0, coef1);
                vec6 = (v8i16) __msa_insert_d((v2i64) vec6, 0, coef2);
                vec7 = (v8i16) __msa_insert_d((v2i64) vec7, 0, coef3);

                TRANSPOSE4x4_SH_SH(vec4, vec5, vec6, vec7,
                                   vec4, vec5, vec6, vec7);

                vec0 = vec4 + vec6;
                vec1 = vec4 - vec6;
                vec2 = vec5 >> 1;
                vec2 = vec2 - vec7;
                vec3 = vec7 >> 1;
                vec3 = vec5 + vec3;

                BUTTERFLY_4(vec0, vec1, vec2, vec3, vec4, vec5, vec6, vec7);
                TRANSPOSE4x4_SH_SH(vec4, vec5, vec6, vec7,
                                   vec4, vec5, vec6, vec7);

                coeff0 = __msa_copy_s_d((v2i64) vec4, 0);
                coeff1 = __msa_copy_s_d((v2i64) vec5, 0);
                coeff2 = __msa_copy_s_d((v2i64) vec6, 0);
                coeff3 = __msa_copy_s_d((v2i64) vec7, 0);

                SD4(coeff0, coeff1, coeff2, coeff3, coef, coef_stride);

                vec0 = vec4 + vec6;
                vec1 = vec4 - vec6;
                vec2 = vec5 >> 1;
                vec2 = vec2 - vec7;
                vec3 = vec7 >> 1;
                vec3 = vec5 + vec3;

                BUTTERFLY_4(vec0, vec1, vec2, vec3, vec4, vec5, vec6, vec7);
                SRARI_H4_SH(vec4, vec5, vec6, vec7, 6);
                LW4(pred, pred_stride, pred0, pred1, pred2, pred3);

                tmp0 = __msa_insert_w(tmp0, 0, pred0);
                tmp1 = __msa_insert_w(tmp1, 0, pred1);
                tmp2 = __msa_insert_w(tmp2, 0, pred2);
                tmp3 = __msa_insert_w(tmp3, 0, pred3);

                ILVR_B4_SH(zeros, tmp0, zeros, tmp1, zeros, tmp2, zeros, tmp3,
                           vec0, vec1, vec2, vec3);
                ADD4(vec0, vec4, vec1, vec5, vec2, vec6, vec3, vec7,
                     vec0, vec1, vec2, vec3);
                CLIP_SH4_0_255(vec0, vec1, vec2, vec3);
                PCKEV_ST4x4_UB(vec0, vec1, vec2, vec3, dst, dst_stride);
            }
            else
            {
                temp0 = (coef[0] + 32) >> 6;
                vec4 = __msa_fill_h(temp0);

                LW4(pred, pred_stride, pred0, pred1, pred2, pred3);

                vec0 = (v8i16) __msa_insert_w((v4i32) vec0, 0, pred0);
                vec1 = (v8i16) __msa_insert_w((v4i32) vec1, 0, pred1);
                vec2 = (v8i16) __msa_insert_w((v4i32) vec2, 0, pred2);
                vec3 = (v8i16) __msa_insert_w((v4i32) vec3, 0, pred3);

                ILVR_B4_SH(zeros, vec0, zeros, vec1, zeros, vec2, zeros, vec3,
                           vec0, vec1, vec2, vec3);
                ADD4(vec0, vec4, vec1, vec4, vec2, vec4, vec3, vec4,
                     vec0, vec1, vec2, vec3);
                CLIP_SH4_0_255(vec0, vec1, vec2, vec3);
                PCKEV_ST4x4_UB(vec0, vec1, vec2, vec3, dst, dst_stride);
            }

            if (lp_cnt0 & 1)
            {
                coef += 4 * coef_stride - 4;
                pred += 4 * pred_stride - 4;
                dst += 4 * dst_stride - 4;
            }
            else
            {
                coef += 4;
                pred += 4;
                dst += 4;
            }
        }
    }
}

uint32 avc_calc_residue_satd_4x4_msa(uint8 *src_ptr, int32 src_stride,
                                     uint8 *pred_ptr, int32 pred_stride)
{
    uint32 satd_out = 0;
    uint32 src0, src1, src2, src3;
    uint32 pred0, pred1, pred2, pred3;
    v16i8 src = { 0 };
    v16i8 pred = { 0 };
    v16u8 inp0, inp1;
    v8i16 residue0, residue1, residue2, residue3;
    v8i16 temp0, temp1, temp2, temp3;
    v8u16 satd_vec3, satd_vec1, satd_vec2;
    v4u32 satd_vec4;
    v4u32 const_one = (v4u32) __msa_ldi_w(1);
    v2u64 satd_vec5;

    LW4(src_ptr, src_stride, src0, src1, src2, src3);
    LW4(pred_ptr, pred_stride, pred0, pred1, pred2, pred3);

    INSERT_W4_SB(src0, src1, src2, src3, src);
    INSERT_W4_SB(pred0, pred1, pred2, pred3, pred);

    /* Residue calculation */
    ILVRL_B2_UB(src, pred, inp0, inp1);
    HSUB_UB2_SH(inp0, inp1, residue0, residue2);
    PCKOD_D2_SH(residue0, residue0, residue2, residue2, residue1, residue3);

    /* horizontal transform */
    TRANSPOSE4x4_SH_SH(residue0, residue1, residue2, residue3,
                       residue0, residue1, residue2, residue3);
    BUTTERFLY_4(residue0, residue1, residue2, residue3,
                temp0, temp1, temp2, temp3);
    BUTTERFLY_4(temp0, temp3, temp2, temp1,
                residue0, residue1, residue3, residue2);

    /* vertical transform */
    TRANSPOSE4x4_SH_SH(residue0, residue1, residue2, residue3,
                       residue0, residue1, residue2, residue3);
    BUTTERFLY_4(residue0, residue1, residue2, residue3,
                temp0, temp1, temp2, temp3);
    BUTTERFLY_4(temp0, temp3, temp2, temp1,
                residue0, residue1, residue3, residue2);

    satd_vec1 = (v8u16) __msa_add_a_h(residue0, residue1);
    satd_vec2 = (v8u16) __msa_add_a_h(residue3, residue2);

    satd_vec3 = satd_vec1 + satd_vec2;
    satd_vec4 = __msa_hadd_u_w(satd_vec3, satd_vec3);
    satd_vec5 = __msa_hadd_u_d(satd_vec4, satd_vec4);
    satd_vec4 = __msa_ave_u_w((v4u32) satd_vec5, const_one);
    satd_out = __msa_copy_u_w((v4i32) satd_vec4, 0);

    return satd_out;
}

uint32 avc_calc_residue_satd_16x16_msa(uint8 *src, int32 src_stride,
                                       uint8 *pred, int32 pred_stride)
{
    uint16 zero = 0;
    uint32 satd_out;
    uint8 row_cnt;
    v16i8 src0, src1, src2, src3;
    v16i8 pred0, pred1, pred2, pred3;
    v16u8 inp0, inp1;
    v8i16 residue0, residue1, residue2, residue3;
    v8i16 temp0, temp1, temp2, temp3, temp4, temp5;
    v8i16 res00, res01, res02, res03, res10, res11, res12, res13;
    v8i16 res20, res21, res22, res23, res30, res31, res32, res33;
    v8i16 vec_dc0 = { 0 };
    v8i16 vec_dc1 = { 0 };
    v8i16 vec_dc2 = { 0 };
    v8i16 vec_dc3 = { 0 };
    v8i16 dc_out0, dc_out1, dc_out2, dc_out3;
    v8i16 satd_vec0, satd_vec1;
    v4i32 satd_vec2 = { 0 };
    v2i64 satd_vec3;

    for (row_cnt = 0; row_cnt < 4; row_cnt++)
    {
        LD_SB4(src, src_stride, src0, src1, src2, src3);
        src += (4 * src_stride);

        LD_SB4(pred, pred_stride, pred0, pred1, pred2, pred3);
        pred += (4 * pred_stride);

        ILVRL_B2_UB(src0, pred0, inp0, inp1);
        HSUB_UB2_SH(inp0, inp1, residue0, residue2);
        PCKOD_D2_SH(residue0, residue0, residue2, residue2, residue1, residue3);
        TRANSPOSE4x4_SH_SH(residue0, residue1, residue2, residue3,
                           residue0, residue1, residue2, residue3);
        BUTTERFLY_4(residue0, residue1, residue2, residue3,
                    temp0, temp1, temp2, temp3);
        BUTTERFLY_4(temp0, temp3, temp2, temp1, res00, res01, res03, res02);
        TRANSPOSE4x4_SH_SH(res00, res01, res02, res03,
                           res00, res01, res02, res03);

        ILVRL_B2_UB(src1, pred1, inp0, inp1);
        HSUB_UB2_SH(inp0, inp1, residue0, residue2);
        PCKOD_D2_SH(residue0, residue0, residue2, residue2, residue1, residue3);
        TRANSPOSE4x4_SH_SH(residue0, residue1, residue2, residue3,
                           residue0, residue1, residue2, residue3);
        BUTTERFLY_4(residue0, residue1, residue2, residue3,
                    temp0, temp1, temp2, temp3);
        BUTTERFLY_4(temp0, temp3, temp2, temp1, res10, res11, res13, res12);
        TRANSPOSE4x4_SH_SH(res10, res11, res12, res13,
                           res10, res11, res12, res13);

        ILVRL_B2_UB(src2, pred2, inp0, inp1);
        HSUB_UB2_SH(inp0, inp1, residue0, residue2);
        PCKOD_D2_SH(residue0, residue0, residue2, residue2, residue1, residue3);
        TRANSPOSE4x4_SH_SH(residue0, residue1, residue2, residue3,
                           residue0, residue1, residue2, residue3);
        BUTTERFLY_4(residue0, residue1, residue2, residue3,
                    temp0, temp1, temp2, temp3);
        BUTTERFLY_4(temp0, temp3, temp2, temp1, res20, res21, res23, res22);
        TRANSPOSE4x4_SH_SH(res20, res21, res22, res23,
                           res20, res21, res22, res23);

        ILVRL_B2_UB(src3, pred3, inp0, inp1);
        HSUB_UB2_SH(inp0, inp1, residue0, residue2);
        PCKOD_D2_SH(residue0, residue0, residue2, residue2, residue1, residue3);
        TRANSPOSE4x4_SH_SH(residue0, residue1, residue2, residue3,
                           residue0, residue1, residue2, residue3);
        BUTTERFLY_4(residue0, residue1, residue2, residue3,
                    temp0, temp1, temp2, temp3);
        BUTTERFLY_4(temp0, temp3, temp2, temp1, res30, res31, res33, res32);
        TRANSPOSE4x4_SH_SH(res30, res31, res32, res33,
                           res30, res31, res32, res33);
        BUTTERFLY_4(res00, res10, res20, res30, temp0, temp1, temp2, temp3);
        BUTTERFLY_4(temp0, temp3, temp2, temp1, res00, res10, res30, res20);
        BUTTERFLY_4(res01, res11, res21, res31, temp0, temp1, temp2, temp3);
        BUTTERFLY_4(temp0, temp3, temp2, temp1, res01, res11, res31, res21);
        BUTTERFLY_4(res02, res12, res22, res32, temp0, temp1, temp2, temp3);
        BUTTERFLY_4(temp0, temp3, temp2, temp1, res02, res12, res32, res22);
        BUTTERFLY_4(res03, res13, res23, res33, temp0, temp1, temp2, temp3);
        BUTTERFLY_4(temp0, temp3, temp2, temp1, res03, res13, res33, res23);

        vec_dc0[row_cnt] = __msa_copy_s_h(res00, 0);
        vec_dc1[row_cnt] = __msa_copy_s_h(res01, 0);
        vec_dc2[row_cnt] = __msa_copy_s_h(res02, 0);
        vec_dc3[row_cnt] = __msa_copy_s_h(res03, 0);

        res00 = __msa_insert_h(res00, 0, zero);
        res01 = __msa_insert_h(res01, 0, zero);
        res02 = __msa_insert_h(res02, 0, zero);
        res03 = __msa_insert_h(res03, 0, zero);

        satd_vec0 = __msa_add_a_h(res00, res01);
        satd_vec1 = __msa_add_a_h(res02, res03);
        satd_vec0 += satd_vec1;
        satd_vec2 += __msa_hadd_s_w(satd_vec0, satd_vec0);

        satd_vec0 = __msa_add_a_h(res10, res11);
        satd_vec1 = __msa_add_a_h(res12, res13);
        satd_vec0 += satd_vec1;
        satd_vec2 += __msa_hadd_s_w(satd_vec0, satd_vec0);

        satd_vec0 = __msa_add_a_h(res20, res21);
        satd_vec1 = __msa_add_a_h(res22, res23);
        satd_vec0 += satd_vec1;
        satd_vec2 += __msa_hadd_s_w(satd_vec0, satd_vec0);

        satd_vec0 = __msa_add_a_h(res30, res31);
        satd_vec1 = __msa_add_a_h(res32, res33);
        satd_vec0 += satd_vec1;
        satd_vec2 += __msa_hadd_s_w(satd_vec0, satd_vec0);
    }

    satd_vec3 = __msa_hadd_s_d(satd_vec2, satd_vec2);

    /* Hadamard of the DC coefficient */
    temp4 = vec_dc2 >> 1;
    temp5 = vec_dc3 >> 1;
    vec_dc0 >>= 2;
    vec_dc1 >>= 2;
    vec_dc2 = temp4 >> 1;
    vec_dc3 = temp5 >> 1;

    temp0 = vec_dc0 + vec_dc3;
    temp1 = vec_dc1 + vec_dc2;
    temp2 = vec_dc1 + vec_dc2;
    temp3 = vec_dc0 + vec_dc3;
    temp2 -= temp4;
    temp3 -= temp5;

    BUTTERFLY_4(temp0, temp3, temp2, temp1, vec_dc0, vec_dc1, vec_dc3, vec_dc2);
    TRANSPOSE4x4_SH_SH(vec_dc0, vec_dc1, vec_dc2, vec_dc3,
                       vec_dc0, vec_dc1, vec_dc2, vec_dc3);

    BUTTERFLY_4(vec_dc0, vec_dc1, vec_dc2, vec_dc3, temp0, temp1, temp2, temp3);
    BUTTERFLY_4(temp0, temp3, temp2, temp1, dc_out0, dc_out3, dc_out2, dc_out1);

    satd_vec0 = __msa_add_a_h(dc_out0, dc_out1);
    satd_vec1 = __msa_add_a_h(dc_out2, dc_out3);
    satd_vec0 += satd_vec1;
    satd_vec2 = __msa_hadd_s_w(satd_vec0, satd_vec0);
    satd_vec3 += __msa_hadd_s_d(satd_vec2, satd_vec2);

    satd_out = __msa_copy_u_w((v4i32) satd_vec3, 0);
    satd_out >>= 1;

    return satd_out;
}
#endif /* #ifdef H264ENC_MSA */
