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

#include "common_macros_msa.h"
#include "prototypes_msa.h"
#include "mp4dec_lib.h"

#ifdef PV_POSTPROC_ON
#ifdef M4VH263DEC_MSA
#define TRANSPOSE16x8_B_DUAL(in0, in1, in2, in3,            \
                             in4, in5, in6, in7,            \
                             out0, out1, out2, out3,        \
                             out4, out5, out6, out7)        \
{                                                           \
    v16i8 tmp0, tmp1, tmp2, tmp3;                           \
    v8i16 tmp4, tmp5;                                       \
    v2i64 s0, s1, s2, s3, s4, s5, s6, s7;                   \
                                                            \
    ILVL_B4_SB(in1, in0, in3, in2, in5, in4, in7, in6,      \
               tmp0, tmp1, tmp2, tmp3);                     \
    ILVL_H2_SH(tmp1, tmp0, tmp3, tmp2, tmp4, tmp5);         \
    ILVRL_W2_SD(tmp5, tmp4, s1, s0);                        \
    ILVR_H2_SH(tmp1, tmp0, tmp3, tmp2, tmp4, tmp5);         \
    ILVRL_W2_SD(tmp5, tmp4, s3, s2);                        \
    ILVR_B4_SB(in1, in0, in3, in2, in5, in4, in7, in6,      \
               tmp0, tmp1, tmp2, tmp3);                     \
    ILVL_H2_SH(tmp1, tmp0, tmp3, tmp2, tmp4, tmp5);         \
    ILVRL_W2_SD(tmp5, tmp4, s5, s4);                        \
    ILVR_H2_SH(tmp1, tmp0, tmp3, tmp2, tmp4, tmp5);         \
    ILVRL_W2_SD(tmp5, tmp4, s7, s6);                        \
    ILVRL_D2_UB(s0, s4, out6, out7);                        \
    ILVRL_D2_UB(s1, s5, out4, out5);                        \
    ILVRL_D2_UB(s2, s6, out2, out3);                        \
    ILVRL_D2_UB(s3, s7, out0, out1);                        \
}

void m4v_h263_CombinedHorzVertFilter_msa(uint8 *rec, int width, int height,
                                         int16 *qp_store, int chr, uint8 *pp_mod)
{
    uint8 *ptr, *ptr_dup;
    int row, blk_row_cnt, blk_col_cnt, col;
    int qp = 0;
    int qp0, qp1, blk_in_row, blk_in_col;
    int br_width;
    int jval0, jval1;
    uint32 val0, val1, val2, val3;
    uint16 num0, num1;
    v16u8 pix, pix_r, pix_l, pix0_0, pix0_1, pix1_0, pix1_1;
    v16u8 pix_a, pix_b, pix_c, pix_d, pix_e, pix_f, pix_g, pix_h;
    v16u8 mask, qp_mask, e_mask;
    v8i16 zero = { 0 };
    v8i16 mask0, mask1, mask_r, mask_l, add_r, add_l, val_r, val_l;
    v8i16 sub_r, sub_l, qp_vec, q_pneg, qp_r, qp_l;

    /*--------------------------------------------------------------------------
    ; Function body here
    --------------------------------------------------------------------------*/
    blk_in_row = (width >> 3);
    blk_in_col = (height >> 3);

    /* row of blocks */
    for (blk_row_cnt = 0; blk_row_cnt < blk_in_col; blk_row_cnt += 2)
    {
        /* number of blocks above current block row */
        br_width = blk_row_cnt * blk_in_row;

        /* column of blocks */
        for (blk_col_cnt = 0; blk_col_cnt < blk_in_row; blk_col_cnt += 2)
        {
            if (!chr)
                qp = qp_store[(br_width >> 2) + (blk_col_cnt >> 1)];

            /* horizontal filtering of 2x2 blocks */
            for (row = (blk_row_cnt + 1); row < (blk_row_cnt + 3); row++)
            {
                /* number of blocks above & left current block row */
                br_width += blk_in_row;

                if (row < blk_in_col)
                {
                    ptr = rec + (br_width << 6) + (blk_col_cnt << 3);

                    jval0 = br_width + blk_col_cnt;
                    jval1 = jval0 + 1;

                    LD_UB6(ptr - (width << 1) - width, width,
                           pix_a, pix_b, pix_c, pix_d, pix_e, pix_f);

                    if (((pp_mod[jval0] & 0x02) && (pp_mod[jval0 - blk_in_row] & 0x02)) &&
                        ((pp_mod[jval1] & 0x02) && (pp_mod[jval1 - blk_in_row] & 0x02)) &&
                        ((blk_col_cnt + 1) < blk_in_row)
                       )
                    {
                        if (chr)
                        {
                            qp0 = qp_store[jval0];
                            qp1 = qp_store[jval1];

                            qp_r = __msa_fill_h(qp0 << 1);
                            qp_l = __msa_fill_h(qp1 << 1);
                        }
                        else
                        {
                            qp_r = __msa_fill_h(qp << 1);
                            qp_l = __msa_fill_h(qp << 1);
                        }

                        ILVRL_B2_UB(pix_d, pix_c, pix_r, pix_l);

                        /* (D - C) */
                        HSUB_UB2_SH(pix_r, pix_l, sub_r, sub_l);

                        mask0 = __msa_clt_s_h(zero, sub_r);
                        mask0 &= __msa_clt_s_h(sub_r, qp_r);

                        q_pneg = -qp_r;
                        mask1 = __msa_clt_s_h(sub_r, zero);
                        mask1 &= __msa_clt_s_h(q_pneg, sub_r);

                        mask_r = mask0 | mask1;

                        mask0 = __msa_clt_s_h(zero, sub_l);
                        mask0 &= __msa_clt_s_h(sub_l, qp_l);

                        q_pneg = -qp_l;
                        mask1 = __msa_clt_s_h(sub_l, zero);
                        mask1 &= __msa_clt_s_h(q_pneg, sub_l);

                        mask_l = mask0 | mask1;

                        /* if condition */
                        qp_mask = (v16u8) __msa_pckev_b((v16i8) mask_l, (v16i8) mask_r);

                        HADD_UB2_SH(pix_r, pix_l, add_r, add_l);

                        add_r >>= 1;
                        add_l >>= 1;

                        pix0_1 = (v16u8) __msa_pckev_b((v16i8) add_l, (v16i8) add_r);

                        pix_c = __msa_bmnz_v(pix_c, pix0_1, qp_mask);
                        pix_d = __msa_bmnz_v(pix_d, pix0_1, qp_mask);

                        ST_UB(pix_c, ptr - width);
                        ST_UB(pix_d, ptr);

                        ILVRL_B2_UB(pix_e, pix_b, pix_r, pix_l);

                        /* val2 = E - B */
                        HSUB_UB2_SH(pix_r, pix_l, sub_r, sub_l);

                        mask_r = __msa_clt_s_h(zero, sub_r);
                        mask_l = __msa_clt_s_h(zero, sub_l);

                        /* if val2 > 0 */
                        mask = (v16u8) __msa_pckev_b((v16i8) mask_l, (v16i8) mask_r);
                        mask &= qp_mask;

                        val_r = (sub_r + 3) >> 2;
                        val_l = (sub_l + 3) >> 2;

                        pix = (v16u8) __msa_pckev_b((v16i8) val_l, (v16i8) val_r);

                        pix0_0 = pix_b + pix;
                        pix1_0 = pix_e - pix;

                        /* else */
                        val_r = (3 - sub_r) >> 2;
                        val_l = (3 - sub_l) >> 2;

                        pix = (v16u8) __msa_pckev_b((v16i8) val_l, (v16i8) val_r);

                        pix0_1 = pix_b - pix;
                        pix1_1 = pix_e + pix;

                        mask_r = __msa_clt_s_h(sub_r, zero);
                        mask_l = __msa_clt_s_h(sub_l, zero);

                        /* else if val2 < 0 */
                        e_mask = (v16u8) __msa_pckev_b((v16i8) mask_l, (v16i8) mask_r);
                        e_mask &= qp_mask;

                        pix0_1 = __msa_bmnz_v(pix_b, pix0_1, e_mask);
                        pix1_1 = __msa_bmnz_v(pix_e, pix1_1, e_mask);

                        pix_b = __msa_bmnz_v(pix0_1, pix0_0, mask);
                        pix_e = __msa_bmnz_v(pix1_1, pix1_0, mask);

                        ST_UB(pix_b, ptr - (width << 1));
                        ST_UB(pix_e, ptr + width);

                        ILVRL_B2_UB(pix_f, pix_a, pix_r, pix_l);

                        /* val2 = F - A */
                        HSUB_UB2_SH(pix_r, pix_l, sub_r, sub_l);

                        mask_r = __msa_clt_s_h(zero, sub_r);
                        mask_l = __msa_clt_s_h(zero, sub_l);

                        /* if val2 > 0 */
                        mask = (v16u8) __msa_pckev_b((v16i8) mask_l, (v16i8) mask_r);
                        mask &= qp_mask;

                        val_r = (sub_r + 7) >> 3;
                        val_l = (sub_l + 7) >> 3;

                        pix = (v16u8) __msa_pckev_b((v16i8) val_l, (v16i8) val_r);

                        pix0_0 = pix_a + pix;
                        pix1_0 = pix_f - pix;

                        /* else */
                        val_r = (7 - sub_r) >> 3;
                        val_l = (7 - sub_l) >> 3;

                        pix = (v16u8) __msa_pckev_b((v16i8) val_l, (v16i8) val_r);

                        pix0_1 = pix_a - pix;
                        pix1_1 = pix_f + pix;

                        mask_r = __msa_clt_s_h(sub_r, zero);
                        mask_l = __msa_clt_s_h(sub_l, zero);

                        /* else if val2 < 0 */
                        e_mask = (v16u8) __msa_pckev_b((v16i8) mask_l, (v16i8) mask_r);
                        e_mask &= qp_mask;

                        pix0_1 = __msa_bmnz_v(pix_a, pix0_1, e_mask);
                        pix1_1 = __msa_bmnz_v(pix_f, pix1_1, e_mask);

                        pix_a = __msa_bmnz_v(pix0_1, pix0_0, mask);
                        pix_f = __msa_bmnz_v(pix1_1, pix1_0, mask);

                        ST_UB(pix_a, ptr - (width << 1) - width);
                        ST_UB(pix_f, ptr + (width << 1));
                    }
                    else if (!
                             ((pp_mod[jval0] & 0x02)
                              && (pp_mod[jval0 - blk_in_row] & 0x02))
                             && !((pp_mod[jval1] & 0x02)
                                  && (pp_mod[jval1 - blk_in_row] & 0x02))
                             && ((blk_col_cnt + 1) < blk_in_row))
                    {
                        if (chr)
                        {
                            qp0 = qp_store[jval0];
                            qp1 = qp_store[jval1];
                            qp_r = __msa_fill_h(qp0);
                            qp_l = __msa_fill_h(qp1);
                        }
                        else
                        {
                            qp_r = __msa_fill_h(qp);
                            qp_l = __msa_fill_h(qp);
                        }

                        ILVRL_B2_UB(pix_d, pix_c, pix_r, pix_l);

                        /* (D - C) */
                        HSUB_UB2_SH(pix_r, pix_l, sub_r, sub_l);

                        /* mask calc */
                        mask0 = __msa_clt_s_h(zero, sub_r);
                        mask0 &= __msa_clt_s_h(sub_r, qp_r);

                        q_pneg = -qp_r;
                        mask1 = __msa_clt_s_h(sub_r, zero);
                        mask1 &= __msa_clt_s_h(q_pneg, sub_r);

                        mask_r = mask0 | mask1;

                        q_pneg = -qp_l;
                        mask0 = __msa_clt_s_h(zero, sub_l);
                        mask0 &= __msa_clt_s_h(sub_l, qp_l);

                        mask1 = __msa_clt_s_h(sub_l, zero);
                        mask1 &= __msa_clt_s_h(q_pneg, sub_l);

                        mask_l = mask0 | mask1;

                        /* if condition */
                        qp_mask = (v16u8) __msa_pckev_b((v16i8) mask_l, (v16i8) mask_r);

                        HADD_UB2_SH(pix_r, pix_l, add_r, add_l);

                        add_r >>= 1;
                        add_l >>= 1;

                        pix0_1 = (v16u8) __msa_pckev_b((v16i8) add_l, (v16i8) add_r);

                        pix_c = __msa_bmnz_v(pix_c, pix0_1, qp_mask);
                        pix_d = __msa_bmnz_v(pix_d, pix0_1, qp_mask);

                        ST_UB(pix_c, ptr - width);
                        ST_UB(pix_d, ptr);

                        ILVRL_B2_UB(pix_e, pix_b, pix_r, pix_l);

                        /* val2 = E-B */
                        HSUB_UB2_SH(pix_r, pix_l, sub_r, sub_l);

                        mask_r = __msa_clt_s_h(zero, sub_r);
                        mask_l = __msa_clt_s_h(zero, sub_l);

                        /* if val2 > 0 */
                        mask = (v16u8) __msa_pckev_b((v16i8) mask_l, (v16i8) mask_r);
                        mask &= qp_mask;

                        val_r = (sub_r + 7) >> 3;
                        val_l = (sub_l + 7) >> 3;

                        pix = (v16u8) __msa_pckev_b((v16i8) val_l, (v16i8) val_r);

                        pix0_0 = pix_b + pix;
                        pix1_0 = pix_e - pix;

                        /* else */
                        val_r = (7 - sub_r) >> 3;
                        val_l = (7 - sub_l) >> 3;

                        pix = (v16u8) __msa_pckev_b((v16i8) val_l, (v16i8) val_r);

                        pix0_1 = pix_b - pix;
                        pix1_1 = pix_e + pix;

                        mask_r = __msa_clt_s_h(sub_r, zero);
                        mask_l = __msa_clt_s_h(sub_l, zero);

                        /* else if val2 < 0 */
                        e_mask = (v16u8) __msa_pckev_b((v16i8) mask_l, (v16i8) mask_r);
                        e_mask &= qp_mask;

                        pix0_1 = __msa_bmnz_v(pix_b, pix0_1, e_mask);
                        pix1_1 = __msa_bmnz_v(pix_e, pix1_1, e_mask);

                        pix_b = __msa_bmnz_v(pix0_1, pix0_0, mask);
                        pix_e = __msa_bmnz_v(pix1_1, pix1_0, mask);

                        ST_UB(pix_b, ptr - (width << 1));
                        ST_UB(pix_e, ptr + width);
                    }
                    else
                    {
                        for (col = blk_col_cnt; col < (blk_col_cnt + 2); col++)
                        {
                            if (col < blk_in_row)
                            {
                                ptr = rec + (br_width << 6) + (col << 3);

                                jval0 = br_width + col;
                                if (chr)
                                    qp = qp_store[jval0];

                                if (col == (blk_col_cnt + 1))
                                {
                                    LD_UB6(ptr - (width << 1) - width, width,
                                           pix_a, pix_b, pix_c, pix_d, pix_e, pix_f);
                                }

                                /* horizontal hard filter */
                                if (((pp_mod[jval0] & 0x02)) &&
                                    ((pp_mod[jval0 - blk_in_row] & 0x02)))
                                {
                                    qp_vec = __msa_fill_h((qp << 1));

                                    ILVRL_B2_UB(pix_d, pix_c, pix_r, pix_l);

                                    /* (D - C) */
                                    HSUB_UB2_SH(pix_r, pix_l, sub_r, sub_l);

                                    /* mask calc */
                                    mask0 = __msa_clt_s_h(zero, sub_r);
                                    mask0 &= __msa_clt_s_h(sub_r, qp_vec);

                                    q_pneg = -qp_vec;
                                    mask1 = __msa_clt_s_h(sub_r, zero);
                                    mask1 &= __msa_clt_s_h(q_pneg, sub_r);

                                    mask_r = mask0 | mask1;

                                    mask0 = __msa_clt_s_h(zero, sub_l);
                                    mask0 &= __msa_clt_s_h(sub_l, qp_vec);

                                    mask1 = __msa_clt_s_h(sub_l, zero);
                                    mask1 &= __msa_clt_s_h(q_pneg, sub_l);

                                    mask_l = mask0 | mask1;

                                    /* if condition */
                                    qp_mask = (v16u8) __msa_pckev_b((v16i8) mask_l, (v16i8) mask_r);

                                    HADD_UB2_SH(pix_r, pix_l, add_r, add_l);

                                    add_r >>= 1;
                                    add_l >>= 1;

                                    pix0_1 = (v16u8) __msa_pckev_b((v16i8) add_l, (v16i8) add_r);

                                    pix_c = __msa_bmnz_v(pix_c, pix0_1, qp_mask);
                                    pix_d = __msa_bmnz_v(pix_d, pix0_1, qp_mask);

                                    val0 = __msa_copy_u_w((v4i32) pix_c, 0);
                                    val1 = __msa_copy_u_w((v4i32) pix_c, 1);
                                    val2 = __msa_copy_u_w((v4i32) pix_d, 0);
                                    val3 = __msa_copy_u_w((v4i32) pix_d, 1);
                                    SW(val0, (ptr - width));
                                    SW(val1, (ptr - width + 4));
                                    SW(val2, ptr);
                                    SW(val3, (ptr + 4));

                                    ILVRL_B2_UB(pix_e, pix_b, pix_r, pix_l);

                                    /* val2 = E - B */
                                    HSUB_UB2_SH(pix_r, pix_l, sub_r, sub_l);

                                    mask_r = __msa_clt_s_h(zero, sub_r);
                                    mask_l = __msa_clt_s_h(zero, sub_l);

                                    /* if val2 > 0 */
                                    mask = (v16u8) __msa_pckev_b((v16i8) mask_l, (v16i8) mask_r);
                                    mask &= qp_mask;

                                    val_r = (sub_r + 3) >> 2;
                                    val_l = (sub_l + 3) >> 2;

                                    pix = (v16u8) __msa_pckev_b((v16i8) val_l, (v16i8) val_r);

                                    pix0_0 = pix_b + pix;
                                    pix1_0 = pix_e - pix;

                                    /* else */
                                    val_r = (3 - sub_r) >> 2;
                                    val_l = (3 - sub_l) >> 2;

                                    pix = (v16u8) __msa_pckev_b((v16i8) val_l, (v16i8) val_r);

                                    pix0_1 = pix_b - pix;
                                    pix1_1 = pix_e + pix;

                                    mask_r = __msa_clt_s_h(sub_r, zero);
                                    mask_l = __msa_clt_s_h(sub_l, zero);

                                    /* else if val2 < 0 */
                                    e_mask = (v16u8) __msa_pckev_b((v16i8) mask_l, (v16i8) mask_r);
                                    e_mask &= qp_mask;

                                    pix0_1 = __msa_bmnz_v(pix_b, pix0_1, e_mask);
                                    pix1_1 = __msa_bmnz_v(pix_e, pix1_1, e_mask);

                                    pix_b = __msa_bmnz_v(pix0_1, pix0_0, mask);
                                    pix_e = __msa_bmnz_v(pix1_1, pix1_0, mask);

                                    val0 = __msa_copy_u_w((v4i32) pix_b, 0);
                                    val1 = __msa_copy_u_w((v4i32) pix_b, 1);
                                    val2 = __msa_copy_u_w((v4i32) pix_e, 0);
                                    val3 = __msa_copy_u_w((v4i32) pix_e, 1);
                                    SW(val0, ptr - (width << 1));
                                    SW(val1, (ptr - (width << 1) + 4));
                                    SW(val2, ptr + width);
                                    SW(val3, (ptr + width + 4));

                                    ILVRL_B2_UB(pix_f, pix_a, pix_r, pix_l);

                                    /* val2 = F-A */
                                    HSUB_UB2_SH(pix_r, pix_l, sub_r, sub_l);

                                    mask_r = __msa_clt_s_h(zero, sub_r);
                                    mask_l = __msa_clt_s_h(zero, sub_l);

                                    /* if val2 > 0 */
                                    mask = (v16u8) __msa_pckev_b((v16i8) mask_l, (v16i8) mask_r);
                                    mask &= qp_mask;

                                    val_r = (sub_r + 7) >> 3;
                                    val_l = (sub_l + 7) >> 3;

                                    pix = (v16u8) __msa_pckev_b((v16i8) val_l, (v16i8) val_r);

                                    pix0_0 = pix_a + pix;
                                    pix1_0 = pix_f - pix;

                                    /* else */
                                    val_r = (7 - sub_r) >> 3;
                                    val_l = (7 - sub_l) >> 3;

                                    pix = (v16u8) __msa_pckev_b((v16i8) val_l, (v16i8) val_r);

                                    pix0_1 = pix_a - pix;
                                    pix1_1 = pix_f + pix;

                                    mask_r = __msa_clt_s_h(sub_r, zero);
                                    mask_l = __msa_clt_s_h(sub_l, zero);

                                    /* else if val2 < 0 */
                                    e_mask = (v16u8) __msa_pckev_b((v16i8) mask_l, (v16i8) mask_r);
                                    e_mask &= qp_mask;

                                    pix0_1 = __msa_bmnz_v(pix_a, pix0_1, e_mask);
                                    pix1_1 = __msa_bmnz_v(pix_f, pix1_1, e_mask);

                                    pix_a = __msa_bmnz_v(pix0_1, pix0_0, mask);
                                    pix_f = __msa_bmnz_v(pix1_1, pix1_0, mask);

                                    val0 = __msa_copy_u_w((v4i32) pix_a, 0);
                                    val1 = __msa_copy_u_w((v4i32) pix_a, 1);
                                    val2 = __msa_copy_u_w((v4i32) pix_f, 0);
                                    val3 = __msa_copy_u_w((v4i32) pix_f, 1);
                                    SW(val0, (ptr - (width << 1) - width));
                                    SW(val1, (ptr - (width << 1) - width + 4));
                                    SW(val2, (ptr + (width << 1)));
                                    SW(val3, (ptr + (width << 1) + 4));
                                }
                                else /* Horiz soft filter */
                                {
                                    qp_vec = __msa_fill_h(qp);

                                    ILVRL_B2_UB(pix_d, pix_c, pix_r, pix_l);

                                    /* (D - C) */
                                    HSUB_UB2_SH(pix_r, pix_l, sub_r, sub_l);

                                    /* mask calc */
                                    mask0 = __msa_clt_s_h(zero, sub_r);
                                    mask0 &= __msa_clt_s_h(sub_r, qp_vec);

                                    q_pneg = zero - qp_vec;
                                    mask1 = __msa_clt_s_h(sub_r, zero);
                                    mask1 &= __msa_clt_s_h(q_pneg, sub_r);

                                    mask_r = mask0 | mask1;

                                    mask0 = __msa_clt_s_h(zero, sub_l);
                                    mask0 &= __msa_clt_s_h(sub_l, qp_vec);

                                    mask1 = __msa_clt_s_h(sub_l, zero);
                                    mask1 &= __msa_clt_s_h(q_pneg, sub_l);

                                    mask_l = mask0 | mask1;

                                    /* if condition */
                                    qp_mask = (v16u8) __msa_pckev_b((v16i8) mask_l, (v16i8) mask_r);

                                    HADD_UB2_SH(pix_r, pix_l, add_r, add_l);

                                    add_r >>= 1;
                                    add_l >>= 1;

                                    pix0_1 = (v16u8) __msa_pckev_b((v16i8) add_l, (v16i8) add_r);

                                    pix_c = __msa_bmnz_v(pix_c, pix0_1, qp_mask);
                                    pix_d = __msa_bmnz_v(pix_d, pix0_1, qp_mask);

                                    val0 = __msa_copy_u_w((v4i32) pix_c, 0);
                                    val1 = __msa_copy_u_w((v4i32) pix_c, 1);
                                    val2 = __msa_copy_u_w((v4i32) pix_d, 0);
                                    val3 = __msa_copy_u_w((v4i32) pix_d, 1);
                                    SW(val0, (ptr - width));
                                    SW(val1, (ptr - width + 4));
                                    SW(val2, ptr);
                                    SW(val3, (ptr + 4));

                                    ILVRL_B2_UB(pix_e, pix_b, pix_r, pix_l);

                                    /* val2 = E-B */
                                    HSUB_UB2_SH(pix_r, pix_l, sub_r, sub_l);

                                    mask_r = __msa_clt_s_h(zero, sub_r);
                                    mask_l = __msa_clt_s_h(zero, sub_l);

                                    /* if val2 > 0 */
                                    mask = (v16u8) __msa_pckev_b((v16i8) mask_l, (v16i8) mask_r);
                                    mask &= qp_mask;

                                    val_r = (sub_r + 7) >> 3;
                                    val_l = (sub_l + 7) >> 3;

                                    pix = (v16u8) __msa_pckev_b((v16i8) val_l, (v16i8) val_r);

                                    pix0_0 = pix_b + pix;
                                    pix1_0 = pix_e - pix;

                                    /* else */
                                    val_r = (7 - sub_r) >> 3;
                                    val_l = (7 - sub_l) >> 3;

                                    pix = (v16u8) __msa_pckev_b((v16i8) val_l, (v16i8) val_r);

                                    pix0_1 = pix_b - pix;
                                    pix1_1 = pix_e + pix;

                                    mask_r = __msa_clt_s_h(sub_r, zero);
                                    mask_l = __msa_clt_s_h(sub_l, zero);

                                    /* else if val2 < 0 */
                                    e_mask = (v16u8) __msa_pckev_b((v16i8) mask_l, (v16i8) mask_r);
                                    e_mask &= qp_mask;

                                    pix0_1 = __msa_bmnz_v(pix_b, pix0_1, e_mask);
                                    pix1_1 = __msa_bmnz_v(pix_e, pix1_1, e_mask);

                                    pix_b = __msa_bmnz_v(pix0_1, pix0_0, mask);
                                    pix_e = __msa_bmnz_v(pix1_1, pix1_0, mask);

                                    val0 = __msa_copy_u_w((v4i32) pix_b, 0);
                                    val1 = __msa_copy_u_w((v4i32) pix_b, 1);
                                    val2 = __msa_copy_u_w((v4i32) pix_e, 0);
                                    val3 = __msa_copy_u_w((v4i32) pix_e, 1);
                                    SW(val0, (ptr - (width << 1)));
                                    SW(val1, (ptr - (width << 1) + 4));
                                    SW(val2, (ptr + width));
                                    SW(val3, (ptr + width + 4));
                                }
                            }
                        } /* col */
                    }
                } /* blk_in_col */
            } /* row */

            br_width -= (blk_in_row << 1);

            /* Vert. Filtering */
            v16u8 pix_a_org, pix_b_org, pix_c_org, pix_d_org;
            v16u8 pix_e_org, pix_f_org, pix_g_org, pix_h_org;

            for (row = blk_row_cnt; row < (blk_row_cnt + 2); row++)
            {
                if (row < blk_in_col)
                {
                    jval0 = br_width + blk_col_cnt + 1;
                    jval1 = jval0 + 1;

                    ptr = rec + (br_width << 6) + ((blk_col_cnt + 1) << 3);

                    /* Vert Hard filter */
                    if ((pp_mod[jval0 - 1] & 0x01) &&
                        (pp_mod[jval0] & 0x01) &&
                        (pp_mod[jval1 - 1] & 0x01) &&
                        (pp_mod[jval1] & 0x01) &&
                        (blk_col_cnt + 2 < blk_in_row))
                    {
                        LD_UB8(ptr - 3, width, pix_a, pix_b, pix_c, pix_d,
                               pix_e, pix_f, pix_g, pix_h);

                        pix_a_org = pix_a;
                        pix_b_org = pix_b;
                        pix_c_org = pix_c;
                        pix_d_org = pix_d;
                        pix_e_org = pix_e;
                        pix_f_org = pix_f;
                        pix_g_org = pix_g;
                        pix_h_org = pix_h;

                        TRANSPOSE16x8_B_DUAL(pix_a, pix_b, pix_c, pix_d,
                                             pix_e, pix_f, pix_g, pix_h,
                                             pix_a, pix_b, pix_c, pix_d,
                                             pix_e, pix_f, pix_g, pix_h);
                        if (chr)
                        {
                            qp0 = qp_store[jval0];
                            qp1 = qp_store[jval1];
                            qp_r = __msa_fill_h(qp0 << 1);
                            qp_l = __msa_fill_h(qp1 << 1);
                        }
                        else
                        {
                            qp_r = __msa_fill_h(qp << 1);
                            qp_l = __msa_fill_h(qp << 1);
                        }

                        ILVRL_B2_UB(pix_d, pix_c, pix_r, pix_l);

                        /* (D-C) */
                        HSUB_UB2_SH(pix_r, pix_l, sub_r, sub_l);

                        /* mask calc */
                        mask0 = __msa_clt_s_h(zero, sub_r);
                        mask0 &= __msa_clt_s_h(sub_r, qp_r);

                        q_pneg = zero - qp_r;
                        mask1 = __msa_clt_s_h(sub_r, zero);
                        mask1 &= __msa_clt_s_h(q_pneg, sub_r);

                        mask_r = mask0 | mask1;

                        mask0 = __msa_clt_s_h(zero, sub_l);
                        mask0 &= __msa_clt_s_h(sub_l, qp_l);

                        q_pneg = zero - qp_r;
                        mask1 = __msa_clt_s_h(sub_l, zero);
                        mask1 &= __msa_clt_s_h(q_pneg, sub_l);

                        mask_l = mask0 | mask1;

                        /* if condition */
                        qp_mask = (v16u8) __msa_pckev_b((v16i8) mask_l, (v16i8) mask_r);

                        HADD_UB2_SH(pix_r, pix_l, add_r, add_l);

                        add_r >>= 1;
                        add_l >>= 1;

                        pix0_1 = (v16u8) __msa_pckev_b((v16i8) add_l, (v16i8) add_r);

                        pix_c = __msa_bmnz_v(pix_c, pix0_1, qp_mask);
                        pix_d = __msa_bmnz_v(pix_d, pix0_1, qp_mask);

                        ILVRL_B2_UB(pix_e, pix_b, pix_r, pix_l);

                        /* val2 = E-B */
                        HSUB_UB2_SH(pix_r, pix_l, sub_r, sub_l);

                        mask_r = __msa_clt_s_h(zero, sub_r);
                        mask_l = __msa_clt_s_h(zero, sub_l);

                        /* if val2 > 0 */
                        mask = (v16u8) __msa_pckev_b((v16i8) mask_l, (v16i8) mask_r);
                        mask &= qp_mask;

                        val_r = (sub_r + 3) >> 2;
                        val_l = (sub_l + 3) >> 2;

                        pix = (v16u8) __msa_pckev_b((v16i8) val_l, (v16i8) val_r);

                        pix0_0 = pix_b + pix;
                        pix1_0 = pix_e - pix;

                        /* else */
                        val_r = (3 - sub_r) >> 2;
                        val_l = (3 - sub_l) >> 2;

                        pix = (v16u8) __msa_pckev_b((v16i8) val_l, (v16i8) val_r);

                        pix0_1 = pix_b - pix;
                        pix1_1 = pix_e + pix;

                        mask_r = __msa_clt_s_h(sub_r, zero);
                        mask_l = __msa_clt_s_h(sub_l, zero);

                        /* else if val2 < 0 */
                        e_mask = (v16u8) __msa_pckev_b((v16i8) mask_l, (v16i8) mask_r);
                        e_mask &= qp_mask;

                        pix0_1 = __msa_bmnz_v(pix_b, pix0_1, e_mask);
                        pix1_1 = __msa_bmnz_v(pix_e, pix1_1, e_mask);

                        pix_b = __msa_bmnz_v(pix0_1, pix0_0, mask);
                        pix_e = __msa_bmnz_v(pix1_1, pix1_0, mask);

                        ILVRL_B2_UB(pix_f, pix_a, pix_r, pix_l);

                        /* val2 = F-A */
                        HSUB_UB2_SH(pix_r, pix_l, sub_r, sub_l);

                        mask_r = __msa_clt_s_h(zero, sub_r);
                        mask_l = __msa_clt_s_h(zero, sub_l);

                        /* if val2 > 0 */
                        mask = (v16u8) __msa_pckev_b((v16i8) mask_l, (v16i8) mask_r);
                        mask &= qp_mask;

                        val_r = (sub_r + 7) >> 3;
                        val_l = (sub_l + 7) >> 3;

                        pix = (v16u8) __msa_pckev_b((v16i8) val_l, (v16i8) val_r);

                        pix0_0 = pix_a + pix;
                        pix1_0 = pix_f - pix;

                        /* else */
                        val_r = (sub_r - 7) >> 3;
                        val_l = (sub_l - 7) >> 3;

                        pix = (v16u8) __msa_pckev_b((v16i8) val_l, (v16i8) val_r);

                        pix0_1 = pix_a + pix;
                        pix1_1 = pix_f - pix;

                        mask_r = __msa_clt_s_h(sub_r, zero);
                        mask_l = __msa_clt_s_h(sub_l, zero);

                        /* else if val2 < 0 */
                        e_mask = (v16u8) __msa_pckev_b((v16i8) mask_l, (v16i8) mask_r);
                        e_mask &= qp_mask;

                        pix0_1 = __msa_bmnz_v(pix_a, pix0_1, e_mask);
                        pix1_1 = __msa_bmnz_v(pix_f, pix1_1, e_mask);

                        pix_a = __msa_bmnz_v(pix0_1, pix0_0, mask);
                        pix_f = __msa_bmnz_v(pix1_1, pix1_0, mask);

                        TRANSPOSE16x8_B_DUAL(pix_a, pix_b, pix_c, pix_d,
                                             pix_e, pix_f, pix_g, pix_h,
                                             pix_a, pix_b, pix_c, pix_d,
                                             pix_e, pix_f, pix_g, pix_h);

                        v16u8 mask = { 255, 255, 255, 255, 255, 255, 0, 0,
                                       255, 255, 255, 255, 255, 255, 0, 0 };

                        pix_a = __msa_bmnz_v(pix_a_org, pix_a, mask);
                        pix_b = __msa_bmnz_v(pix_b_org, pix_b, mask);
                        pix_c = __msa_bmnz_v(pix_c_org, pix_c, mask);
                        pix_d = __msa_bmnz_v(pix_d_org, pix_d, mask);
                        pix_e = __msa_bmnz_v(pix_e_org, pix_e, mask);
                        pix_f = __msa_bmnz_v(pix_f_org, pix_f, mask);
                        pix_g = __msa_bmnz_v(pix_g_org, pix_g, mask);
                        pix_h = __msa_bmnz_v(pix_h_org, pix_h, mask);

                        ST_UB8(pix_a, pix_b, pix_c, pix_d, pix_e, pix_f, pix_g, pix_h,
                               ptr - 3, width);
                    }
                    else if (!
                             ((pp_mod[jval0 - 1] & 0x01)
                              && (pp_mod[jval0] & 0x01))
                             && !((pp_mod[jval1 - 1] & 0x01)
                                  && (pp_mod[jval1] & 0x01))
                             && (blk_col_cnt + 2 < blk_in_row))
                    {
                        LD_UB8(ptr - 2, width, pix_a, pix_b, pix_c, pix_d,
                               pix_e, pix_f, pix_g, pix_h);

                        pix_a_org = pix_a;
                        pix_b_org = pix_b;
                        pix_c_org = pix_c;
                        pix_d_org = pix_d;
                        pix_e_org = pix_e;
                        pix_f_org = pix_f;
                        pix_g_org = pix_g;
                        pix_h_org = pix_h;

                        TRANSPOSE16x8_B_DUAL(pix_a, pix_b, pix_c, pix_d,
                                             pix_e, pix_f, pix_g, pix_h,
                                             pix_a, pix_b, pix_c, pix_d,
                                             pix_e, pix_f, pix_g, pix_h);

                        if (chr)
                        {
                            qp0 = qp_store[jval0];
                            qp1 = qp_store[jval1];
                            qp_r = __msa_fill_h(qp0);
                            qp_l = __msa_fill_h(qp1);
                        }
                        else
                        {
                            qp_r = __msa_fill_h(qp);
                            qp_l = __msa_fill_h(qp);
                        }

                        ptr_dup = ptr;

                        ILVRL_B2_UB(pix_c, pix_b, pix_r, pix_l);

                        /* (C - B) */
                        HSUB_UB2_SH(pix_r, pix_l, sub_r, sub_l);

                        /* mask calc */
                        mask0 = __msa_clt_s_h(zero, sub_r);
                        mask0 &= __msa_clt_s_h(sub_r, qp_r);

                        q_pneg = zero - qp_r;
                        mask1 = __msa_clt_s_h(sub_r, zero);
                        mask1 &= __msa_clt_s_h(q_pneg, sub_r);

                        mask_r = mask0 | mask1;

                        mask0 = __msa_clt_s_h(zero, sub_l);
                        mask0 &= __msa_clt_s_h(sub_l, qp_l);

                        q_pneg = zero - qp_l;
                        mask1 = __msa_clt_s_h(sub_l, zero);
                        mask1 &= __msa_clt_s_h(q_pneg, sub_l);

                        mask_l = mask0 | mask1;

                        /* if condition */
                        qp_mask = (v16u8) __msa_pckev_b((v16i8) mask_l, (v16i8) mask_r);

                        pix0_1 = __msa_aver_u_b(pix_b, pix_c);

                        pix_b = __msa_bmnz_v(pix_b, pix0_1, qp_mask);
                        pix_c = __msa_bmnz_v(pix_c, pix0_1, qp_mask);

                        ILVRL_B2_UB(pix_d, pix_a, pix_r, pix_l);

                        /* val2 = D-A */
                        HSUB_UB2_SH(pix_r, pix_l, sub_r, sub_l);

                        mask_r = __msa_clt_s_h(zero, sub_r);
                        mask_l = __msa_clt_s_h(zero, sub_l);

                        /* if val2 > 0 */
                        mask = (v16u8) __msa_pckev_b((v16i8) mask_l, (v16i8) mask_r);
                        mask &= qp_mask;

                        val_r = (sub_r + 7) >> 3;
                        val_l = (sub_l + 7) >> 3;

                        pix = (v16u8) __msa_pckev_b((v16i8) val_l, (v16i8) val_r);

                        pix0_0 = pix_a + pix;
                        pix1_0 = pix_d - pix;

                        /* else */
                        val_r = (7 - sub_r) >> 3;
                        val_l = (7 - sub_l) >> 3;

                        pix = (v16u8) __msa_pckev_b((v16i8) val_l, (v16i8) val_r);

                        pix0_1 = pix_a - pix;
                        pix1_1 = pix_d + pix;

                        mask_r = __msa_clt_s_h(sub_r, zero);
                        mask_l = __msa_clt_s_h(sub_l, zero);

                        /* else if val2 < 0 */
                        e_mask = (v16u8) __msa_pckev_b((v16i8) mask_l, (v16i8) mask_r);
                        e_mask &= qp_mask;

                        pix0_1 = __msa_bmnz_v(pix_a, pix0_1, e_mask);
                        pix1_1 = __msa_bmnz_v(pix_d, pix1_1, e_mask);

                        pix_a = __msa_bmnz_v(pix0_1, pix0_0, mask);
                        pix_d = __msa_bmnz_v(pix1_1, pix1_0, mask);

                        /* STORE */
                        TRANSPOSE16x8_B_DUAL(pix_a, pix_b, pix_c, pix_d,
                                             pix_e, pix_f, pix_g, pix_h,
                                             pix_a, pix_b, pix_c, pix_d,
                                             pix_e, pix_f, pix_g, pix_h);

                        v16u8 mask = { 255, 255, 255, 255, 0, 0, 0, 0,
                                       255, 255, 255, 255, 0, 0, 0, 0 };

                        pix_a = __msa_bmnz_v(pix_a_org, pix_a, mask);
                        pix_b = __msa_bmnz_v(pix_b_org, pix_b, mask);
                        pix_c = __msa_bmnz_v(pix_c_org, pix_c, mask);
                        pix_d = __msa_bmnz_v(pix_d_org, pix_d, mask);
                        pix_e = __msa_bmnz_v(pix_e_org, pix_e, mask);
                        pix_f = __msa_bmnz_v(pix_f_org, pix_f, mask);
                        pix_g = __msa_bmnz_v(pix_g_org, pix_g, mask);
                        pix_h = __msa_bmnz_v(pix_h_org, pix_h, mask);

                        ST_UB8(pix_a, pix_b, pix_c, pix_d, pix_e, pix_f, pix_g, pix_h, ptr - 2, width);
                    }
                    else
                    {
                        for (col = (blk_col_cnt + 1); col < (blk_col_cnt + 3);
                             col++)
                        {
                            /* check boundary for deblocking */
                            if (col < blk_in_row)
                            {
                                ptr = rec + (br_width << 6) + (col << 3);
                                jval0 = br_width + col;
                                if (chr)
                                    qp = qp_store[jval0];

                                ptr_dup = ptr;

                                /* vertical hard filter */
                                if ((pp_mod[jval0 - 1] & 0x01) &&
                                    (pp_mod[jval0] & 0x01))
                                {
                                    ptr = ptr_dup - 3;

                                    LD_UB8(ptr, width, pix_a, pix_b, pix_c, pix_d,
                                           pix_e, pix_f, pix_g, pix_h);

                                    TRANSPOSE8x8_UB_UB(pix_a, pix_b, pix_c, pix_d,
                                                       pix_e, pix_f, pix_g, pix_h,
                                                       pix_a, pix_b, pix_c, pix_d,
                                                       pix_e, pix_f, pix_g, pix_h);

                                    qp_vec = __msa_fill_h(qp << 1);

                                    ILVRL_B2_UB(pix_d, pix_c, pix_r, pix_l);

                                    /* (D-C) */
                                    HSUB_UB2_SH(pix_r, pix_l, sub_r, sub_l);

                                    /* mask calc */
                                    mask0 = __msa_clt_s_h(zero, sub_r);
                                    mask0 &= __msa_clt_s_h(sub_r, qp_vec);

                                    q_pneg = zero - qp_vec;
                                    mask1 = __msa_clt_s_h(sub_r, zero);
                                    mask1 &= __msa_clt_s_h(q_pneg, sub_r);

                                    mask_r = mask0 | mask1;

                                    mask0 = __msa_clt_s_h(zero, sub_l);
                                    mask0 &= __msa_clt_s_h(sub_l, qp_vec);
                                    mask1 = __msa_clt_s_h(sub_l, zero);
                                    mask1 &= __msa_clt_s_h(q_pneg, sub_l);

                                    mask_l = mask0 | mask1;

                                    /* if condition */
                                    qp_mask = (v16u8) __msa_pckev_b((v16i8) mask_l,
                                                                    (v16i8) mask_r);

                                    HADD_UB2_SH(pix_r, pix_l, add_r, add_l);

                                    add_r >>= 1;
                                    add_l >>= 1;

                                    pix0_1 = (v16u8) __msa_pckev_b((v16i8) add_l,
                                                                   (v16i8) add_r);

                                    pix_c = __msa_bmnz_v(pix_c, pix0_1, qp_mask);
                                    pix_d = __msa_bmnz_v(pix_d, pix0_1, qp_mask);

                                    ILVRL_B2_UB(pix_e, pix_b, pix_r, pix_l);

                                    /* val2 = E-B */
                                    HSUB_UB2_SH(pix_r, pix_l, sub_r, sub_l);

                                    mask_r = __msa_clt_s_h(zero, sub_r);
                                    mask_l = __msa_clt_s_h(zero, sub_l);

                                    /* if val2 > 0 */
                                    mask =
                                        (v16u8) __msa_pckev_b((v16i8) mask_l,
                                                              (v16i8) mask_r);
                                    mask &= qp_mask;

                                    val_r = (sub_r + 3) >> 2;
                                    val_l = (sub_l + 3) >> 2;

                                    pix = (v16u8) __msa_pckev_b((v16i8) val_l,
                                                                (v16i8) val_r);

                                    pix0_0 = pix_b + pix;
                                    pix1_0 = pix_e - pix;

                                    /* else */
                                    val_r = (3 - sub_r) >> 2;
                                    val_l = (3 - sub_l) >> 2;

                                    pix = (v16u8) __msa_pckev_b((v16i8) val_l,
                                                                (v16i8) val_r);

                                    pix0_1 = pix_b - pix;
                                    pix1_1 = pix_e + pix;

                                    mask_r = __msa_clt_s_h(sub_r, zero);
                                    mask_l = __msa_clt_s_h(sub_l, zero);

                                    /* else if val2 < 0 */
                                    e_mask =
                                        (v16u8) __msa_pckev_b((v16i8) mask_l,
                                                              (v16i8) mask_r);
                                    e_mask &= qp_mask;

                                    pix0_1 = __msa_bmnz_v(pix_b, pix0_1, e_mask);
                                    pix1_1 = __msa_bmnz_v(pix_e, pix1_1, e_mask);

                                    pix_b = __msa_bmnz_v(pix0_1, pix0_0, mask);
                                    pix_e = __msa_bmnz_v(pix1_1, pix1_0, mask);

                                    ILVRL_B2_UB(pix_f, pix_a, pix_r, pix_l);

                                    /* val2 = F-A */
                                    HSUB_UB2_SH(pix_r, pix_l, sub_r, sub_l);

                                    mask_r = __msa_clt_s_h(zero, sub_r);
                                    mask_l = __msa_clt_s_h(zero, sub_l);

                                    /* if val2 > 0 */
                                    mask =
                                        (v16u8) __msa_pckev_b((v16i8) mask_l,
                                                              (v16i8) mask_r);
                                    mask &= qp_mask;

                                    val_r = (sub_r + 7) >> 3;
                                    val_l = (sub_l + 7) >> 3;

                                    pix = (v16u8) __msa_pckev_b((v16i8) val_l,
                                                                (v16i8) val_r);

                                    pix0_0 = pix_a + pix;
                                    pix1_0 = pix_f - pix;

                                    /* else */
                                    val_r = (sub_r - 7) >> 3;
                                    val_l = (sub_l - 7) >> 3;

                                    pix = (v16u8) __msa_pckev_b((v16i8) val_l,
                                                                (v16i8) val_r);

                                    pix0_1 = pix_a + pix;
                                    pix1_1 = pix_f - pix;

                                    mask_r = __msa_clt_s_h(sub_r, zero);
                                    mask_l = __msa_clt_s_h(sub_l, zero);

                                    /* else if val2 < 0 */
                                    e_mask =
                                        (v16u8) __msa_pckev_b((v16i8) mask_l,
                                                              (v16i8) mask_r);
                                    e_mask &= qp_mask;

                                    pix0_1 = __msa_bmnz_v(pix_a, pix0_1, e_mask);
                                    pix1_1 = __msa_bmnz_v(pix_f, pix1_1, e_mask);

                                    pix_a = __msa_bmnz_v(pix0_1, pix0_0, mask);
                                    pix_f = __msa_bmnz_v(pix1_1, pix1_0, mask);

                                    TRANSPOSE8x8_UB_UB(pix_a, pix_b, pix_c, pix_d,
                                                       pix_e, pix_f, pix_g, pix_h,
                                                       pix_a, pix_b, pix_c, pix_d,
                                                       pix_e, pix_f, pix_g, pix_h);

                                    val0 = __msa_copy_u_w((v4i32) pix_a, 0);
                                    num0 = __msa_copy_u_h((v8i16) pix_a, 2);
                                    val1 = __msa_copy_u_w((v4i32) pix_b, 0);
                                    num1 = __msa_copy_u_h((v8i16) pix_b, 2);
                                    SW(val0, ptr);
                                    SH(num0, ptr + 4);
                                    ptr += width;
                                    SW(val1, ptr);
                                    SH(num1, ptr + 4);
                                    ptr += width;

                                    val0 = __msa_copy_u_w((v4i32) pix_c, 0);
                                    num0 = __msa_copy_u_h((v8i16) pix_c, 2);
                                    val1 = __msa_copy_u_w((v4i32) pix_d, 0);
                                    num1 = __msa_copy_u_h((v8i16) pix_d, 2);
                                    SW(val0, ptr);
                                    SH(num0, ptr + 4);
                                    ptr += width;
                                    SW(val1, ptr);
                                    SH(num1, ptr + 4);
                                    ptr += width;

                                    val0 = __msa_copy_u_w((v4i32) pix_e, 0);
                                    num0 = __msa_copy_u_h((v8i16) pix_e, 2);
                                    val1 = __msa_copy_u_w((v4i32) pix_f, 0);
                                    num1 = __msa_copy_u_h((v8i16) pix_f, 2);
                                    SW(val0, ptr);
                                    SH(num0, ptr + 4);
                                    ptr += width;
                                    SW(val1, ptr);
                                    SH(num1, ptr + 4);
                                    ptr += width;

                                    val0 = __msa_copy_u_w((v4i32) pix_g, 0);
                                    num0 = __msa_copy_u_h((v8i16) pix_g, 2);
                                    val1 = __msa_copy_u_w((v4i32) pix_h, 0);
                                    num1 = __msa_copy_u_h((v8i16) pix_h, 2);
                                    SW(val0, ptr);
                                    SH(num0, ptr + 4);
                                    ptr += width;
                                    SW(val1, ptr);
                                    SH(num1, ptr + 4);
                                    ptr += width;
                                }
                                else /* vertical soft filter */
                                {
                                    ptr = ptr_dup - 2;

                                    LD_UB8(ptr, width, pix_a, pix_b, pix_c, pix_d,
                                           pix_e, pix_f, pix_g, pix_h);

                                    TRANSPOSE8x8_UB_UB(pix_a, pix_b, pix_c, pix_d,
                                                       pix_e, pix_f, pix_g, pix_h,
                                                       pix_a, pix_b, pix_c, pix_d,
                                                       pix_e, pix_f, pix_g, pix_h);

                                    qp_vec = __msa_fill_h(qp);

                                    ILVRL_B2_UB(pix_c, pix_b, pix_r, pix_l);

                                    /* (C-B) */
                                    HSUB_UB2_SH(pix_r, pix_l, sub_r, sub_l);

                                    /* mask calc */
                                    mask0 = __msa_clt_s_h(zero, sub_r);
                                    mask0 &= __msa_clt_s_h(sub_r, qp_vec);

                                    q_pneg = zero - qp_vec;
                                    mask1 = __msa_clt_s_h(sub_r, zero);
                                    mask1 &= __msa_clt_s_h(q_pneg, sub_r);

                                    mask_r = mask0 | mask1;

                                    mask0 = __msa_clt_s_h(zero, sub_l);
                                    mask0 &= __msa_clt_s_h(sub_l, qp_vec);

                                    mask1 = __msa_clt_s_h(sub_l, zero);
                                    mask1 &= __msa_clt_s_h(q_pneg, sub_l);

                                    mask_l = mask0 | mask1;

                                    /* if condition */
                                    qp_mask =
                                        (v16u8) __msa_pckev_b((v16i8) mask_l,
                                                              (v16i8) mask_r);

                                    pix0_1 = __msa_aver_u_b(pix_b, pix_c);

                                    pix_b = __msa_bmnz_v(pix_b, pix0_1, qp_mask);
                                    pix_c = __msa_bmnz_v(pix_c, pix0_1, qp_mask);

                                    ILVRL_B2_UB(pix_d, pix_a, pix_r, pix_l);

                                    /* val2 = D - A */
                                    HSUB_UB2_SH(pix_r, pix_l, sub_r, sub_l);

                                    mask_r = __msa_clt_s_h(zero, sub_r);
                                    mask_l = __msa_clt_s_h(zero, sub_l);

                                    /* if val2 > 0 */
                                    mask =
                                        (v16u8) __msa_pckev_b((v16i8) mask_l,
                                                              (v16i8) mask_r);
                                    mask &= qp_mask;

                                    val_r = (sub_r + 7) >> 3;
                                    val_l = (sub_l + 7) >> 3;

                                    pix = (v16u8) __msa_pckev_b((v16i8) val_l,
                                                                (v16i8) val_r);

                                    pix0_0 = pix_a + pix;
                                    pix1_0 = pix_d - pix;

                                    /* else */
                                    val_r = (7 - sub_r) >> 3;
                                    val_l = (7 - sub_l) >> 3;

                                    pix = (v16u8) __msa_pckev_b((v16i8) val_l,
                                                                (v16i8) val_r);

                                    pix0_1 = pix_a - pix;
                                    pix1_1 = pix_d + pix;

                                    mask_r = __msa_clt_s_h(sub_r, zero);
                                    mask_l = __msa_clt_s_h(sub_l, zero);

                                    /* else if val2 < 0 */
                                    e_mask =
                                        (v16u8) __msa_pckev_b((v16i8) mask_l,
                                                              (v16i8) mask_r);
                                    e_mask &= qp_mask;

                                    pix0_1 = __msa_bmnz_v(pix_a, pix0_1, e_mask);
                                    pix1_1 = __msa_bmnz_v(pix_d, pix1_1, e_mask);

                                    pix_a = __msa_bmnz_v(pix0_1, pix0_0, mask);
                                    pix_d = __msa_bmnz_v(pix1_1, pix1_0, mask);

                                    TRANSPOSE8x8_UB_UB(pix_a, pix_b, pix_c, pix_d,
                                                       pix_e, pix_f, pix_g, pix_h,
                                                       pix_a, pix_b, pix_c, pix_d,
                                                       pix_e, pix_f, pix_g, pix_h);

                                    val0 = __msa_copy_u_w((v4i32) pix_a, 0);
                                    val1 = __msa_copy_u_w((v4i32) pix_b, 0);
                                    SW(val0, ptr);
                                    ptr += width;
                                    SW(val1, ptr);
                                    ptr += width;

                                    val0 = __msa_copy_u_w((v4i32) pix_c, 0);
                                    val1 = __msa_copy_u_w((v4i32) pix_d, 0);
                                    SW(val0, ptr);
                                    ptr += width;
                                    SW(val1, ptr);
                                    ptr += width;

                                    val0 = __msa_copy_u_w((v4i32) pix_e, 0);
                                    val1 = __msa_copy_u_w((v4i32) pix_f, 0);
                                    SW(val0, ptr);
                                    ptr += width;
                                    SW(val1, ptr);
                                    ptr += width;

                                    val0 = __msa_copy_u_w((v4i32) pix_g, 0);
                                    val1 = __msa_copy_u_w((v4i32) pix_h, 0);
                                    SW(val0, ptr);
                                    ptr += width;
                                    SW(val1, ptr);
                                    ptr += width;
                                } /* soft filter */
                            } /* boundary */
                        } /* col */
                    }
                }

                br_width += blk_in_row;
            } /* row */

            br_width -= (blk_in_row << 1);
        } /* blk_col_cnt */

        br_width += (blk_in_row << 1);
    } /* blk_row_cnt */
}

void m4v_h263_CombinedHorzVertFilter_NoSoftDeblocking_msa(uint8 *rec,
                                                          int width,
                                                          int height,
                                                          int16 *qp_store,
                                                          int chr,
                                                          uint8 *pp_mod)
{
    uint8 *ptr, *ptr_dup;
    int row, blk_row_cnt, blk_col_cnt, col;
    int qp = 0;
    int qp0, qp1, blk_in_row, blk_in_col;
    int br_width;
    int jval0, jval1;
    uint32 val0, val1, val2, val3;
    uint16 num0, num1;
    v16u8 pix, pix_r, pix_l, pix0_0, pix0_1, pix1_0, pix1_1;
    v16u8 pix_a, pix_b, pix_c, pix_d, pix_e, pix_f, pix_g, pix_h;
    v16u8 mask, qp_mask, e_mask;
    v8i16 zero = { 0 };
    v8i16 mask0, mask1, mask_r, mask_l;
    v8i16 add_r, add_l, val_r, val_l;
    v8i16 sub_r, sub_l, qp_vec, q_pneg, qp_r, qp_l;

    /*--------------------------------------------------------------------------
    ; Function body here
    --------------------------------------------------------------------------*/
    blk_in_row = (width >> 3);
    blk_in_col = (height >> 3);

    /* row of blocks */
    for (blk_row_cnt = 0; blk_row_cnt < blk_in_col; blk_row_cnt += 2)
    {
        /* number of blocks above current block row */
        br_width = blk_row_cnt * blk_in_row;

        /* column of blocks */
        for (blk_col_cnt = 0; blk_col_cnt < blk_in_row; blk_col_cnt += 2)
        {
            if (!chr)
                qp = qp_store[(br_width >> 2) + (blk_col_cnt >> 1)];

            /* horizontal filtering for 2x2 blocks */
            for (row = (blk_row_cnt + 1); row < (blk_row_cnt + 3); row++)
            {
                br_width += blk_in_row;

                if (row < blk_in_col)
                {
                    ptr = rec + (br_width << 6) + (blk_col_cnt << 3);

                    jval0 = br_width + blk_col_cnt;
                    jval1 = jval0 + 1;

                    if (((pp_mod[jval0] & 0x02)
                         && (pp_mod[jval0 - blk_in_row] & 0x02))
                        && ((pp_mod[jval1] & 0x02)
                            && (pp_mod[jval1 - blk_in_row] & 0x02))
                        && ((blk_col_cnt + 1) < blk_in_row))
                    {
                        LD_UB6(ptr - (width << 1) - width, width,
                               pix_a, pix_b, pix_c, pix_d, pix_e, pix_f);

                        if (chr)
                        {
                            qp0 = qp_store[jval0];
                            qp1 = qp_store[jval1];
                            qp_r = __msa_fill_h(qp0 << 1);
                            qp_l = __msa_fill_h(qp1 << 1);
                        }
                        else
                        {
                            qp_r = __msa_fill_h(qp << 1);
                            qp_l = __msa_fill_h(qp << 1);
                        }

                        ILVRL_B2_UB(pix_d, pix_c, pix_r, pix_l);

                        /*(D - C) */
                        HSUB_UB2_SH(pix_r, pix_l, sub_r, sub_l);

                        /* mask calc */
                        mask0 = __msa_clt_s_h(zero, sub_r);
                        mask0 &= __msa_clt_s_h(sub_r, qp_r);

                        q_pneg = -qp_r;
                        mask1 = __msa_clt_s_h(sub_r, zero);
                        mask1 &= __msa_clt_s_h(q_pneg, sub_r);

                        mask_r = mask0 | mask1;

                        mask0 = __msa_clt_s_h(zero, sub_l);
                        mask0 &= __msa_clt_s_h(sub_l, qp_l);

                        q_pneg = -qp_l;
                        mask1 = __msa_clt_s_h(sub_l, zero);
                        mask1 &= __msa_clt_s_h(q_pneg, sub_l);

                        mask_l = mask0 | mask1;

                        /* if condition */
                        qp_mask = (v16u8) __msa_pckev_b((v16i8) mask_l, (v16i8) mask_r);

                        HADD_UB2_SH(pix_r, pix_l, add_r, add_l);

                        add_r >>= 1;
                        add_l >>= 1;

                        pix0_1 = (v16u8) __msa_pckev_b((v16i8) add_l, (v16i8) add_r);

                        pix_c = __msa_bmnz_v(pix_c, pix0_1, qp_mask);
                        pix_d = __msa_bmnz_v(pix_d, pix0_1, qp_mask);

                        ST_UB(pix_c, ptr - width);
                        ST_UB(pix_d, ptr);

                        ILVRL_B2_UB(pix_e, pix_b, pix_r, pix_l);

                        /* val2 = E-B */
                        HSUB_UB2_SH(pix_r, pix_l, sub_r, sub_l);

                        mask_r = __msa_clt_s_h(zero, sub_r);
                        mask_l = __msa_clt_s_h(zero, sub_l);

                        /* if val2 > 0 */
                        mask = (v16u8) __msa_pckev_b((v16i8) mask_l, (v16i8) mask_r);
                        mask &= qp_mask;

                        val_r = (sub_r + 3) >> 2;
                        val_l = (sub_l + 3) >> 2;

                        pix = (v16u8) __msa_pckev_b((v16i8) val_l, (v16i8) val_r);

                        pix0_0 = pix_b + pix;
                        pix1_0 = pix_e - pix;

                        /* else */
                        val_r = (3 - sub_r) >> 2;
                        val_l = (3 - sub_l) >> 2;

                        pix = (v16u8) __msa_pckev_b((v16i8) val_l, (v16i8) val_r);

                        pix0_1 = pix_b - pix;
                        pix1_1 = pix_e + pix;

                        mask_r = __msa_clt_s_h(sub_r, zero);
                        mask_l = __msa_clt_s_h(sub_l, zero);

                        /* else if val2 < 0 */
                        e_mask = (v16u8) __msa_pckev_b((v16i8) mask_l, (v16i8) mask_r);
                        e_mask &= qp_mask;

                        pix0_1 = __msa_bmnz_v(pix_b, pix0_1, e_mask);
                        pix1_1 = __msa_bmnz_v(pix_e, pix1_1, e_mask);

                        pix_b = __msa_bmnz_v(pix0_1, pix0_0, mask);
                        pix_e = __msa_bmnz_v(pix1_1, pix1_0, mask);

                        ST_UB(pix_b, ptr - (width << 1));
                        ST_UB(pix_e, ptr + width);

                        ILVRL_B2_UB(pix_f, pix_a, pix_r, pix_l);

                        /* val2 = F-A */
                        HSUB_UB2_SH(pix_r, pix_l, sub_r, sub_l);

                        mask_r = __msa_clt_s_h(zero, sub_r);
                        mask_l = __msa_clt_s_h(zero, sub_l);

                        /* if val2 > 0 */
                        mask = (v16u8) __msa_pckev_b((v16i8) mask_l, (v16i8) mask_r);
                        mask &= qp_mask;

                        val_r = (sub_r + 7) >> 3;
                        val_l = (sub_l + 7) >> 3;

                        pix = (v16u8) __msa_pckev_b((v16i8) val_l, (v16i8) val_r);

                        pix0_0 = pix_a + pix;
                        pix1_0 = pix_f - pix;

                        /* else */
                        val_r = (7 - sub_r) >> 3;
                        val_l = (7 - sub_l) >> 3;

                        pix = (v16u8) __msa_pckev_b((v16i8) val_l, (v16i8) val_r);

                        pix0_1 = pix_a - pix;
                        pix1_1 = pix_f + pix;

                        mask_r = __msa_clt_s_h(sub_r, zero);
                        mask_l = __msa_clt_s_h(sub_l, zero);

                        /* else if val2 < 0 */
                        e_mask = (v16u8) __msa_pckev_b((v16i8) mask_l, (v16i8) mask_r);
                        e_mask &= qp_mask;

                        pix0_1 = __msa_bmnz_v(pix_a, pix0_1, e_mask);
                        pix1_1 = __msa_bmnz_v(pix_f, pix1_1, e_mask);

                        pix_a = __msa_bmnz_v(pix0_1, pix0_0, mask);
                        pix_f = __msa_bmnz_v(pix1_1, pix1_0, mask);

                        ST_UB(pix_a, ptr - (width << 1) - width);
                        ST_UB(pix_f, ptr + (width << 1));
                    }
                    else
                    {
                        for (col = blk_col_cnt; col < (blk_col_cnt + 2); col++)
                        {
                            if (col < blk_in_row)
                            {
                                ptr = rec + (br_width << 6) + (col << 3);

                                jval0 = br_width + col;
                                if (chr)
                                    qp = qp_store[jval0];

                                LD_UB6((ptr - (width << 1) - width), width,
                                       pix_a, pix_b, pix_c, pix_d, pix_e, pix_f);

                                /* Horiz Hard filter */
                                if ((pp_mod[jval0] & 0x02) &&
                                    (pp_mod[jval0 - blk_in_row] & 0x02))
                                {
                                    qp_vec = __msa_fill_h((qp << 1));

                                    ILVRL_B2_UB(pix_d, pix_c, pix_r, pix_l);

                                    /* (D-C) */
                                    HSUB_UB2_SH(pix_r, pix_l, sub_r, sub_l);

                                    /* mask calc */
                                    mask0 = __msa_clt_s_h(zero, sub_r);
                                    mask0 &= __msa_clt_s_h(sub_r, qp_vec);

                                    q_pneg = -qp_vec;
                                    mask1 = __msa_clt_s_h(sub_r, zero);
                                    mask1 &= __msa_clt_s_h(q_pneg, sub_r);

                                    mask_r = mask0 | mask1;

                                    mask0 = __msa_clt_s_h(zero, sub_l);
                                    mask0 &= __msa_clt_s_h(sub_l, qp_vec);

                                    mask1 = __msa_clt_s_h(sub_l, zero);
                                    mask1 &= __msa_clt_s_h(q_pneg, sub_l);

                                    mask_l = mask0 | mask1;

                                    /* if condition */
                                    qp_mask =
                                        (v16u8) __msa_pckev_b((v16i8) mask_l,
                                                              (v16i8) mask_r);

                                    HADD_UB2_SH(pix_r, pix_l, add_r, add_l);

                                    add_r >>= 1;
                                    add_l >>= 1;

                                    pix0_1 =
                                        (v16u8) __msa_pckev_b((v16i8) add_l,
                                                              (v16i8) add_r);

                                    pix_c = __msa_bmnz_v(pix_c, pix0_1,
                                                         qp_mask);
                                    pix_d = __msa_bmnz_v(pix_d, pix0_1,
                                                         qp_mask);

                                    val0 = __msa_copy_u_w((v4i32) pix_c, 0);
                                    val1 = __msa_copy_u_w((v4i32) pix_c, 1);
                                    val2 = __msa_copy_u_w((v4i32) pix_d, 0);
                                    val3 = __msa_copy_u_w((v4i32) pix_d, 1);
                                    SW(val0, (ptr - width));
                                    SW(val1, (ptr - width + 4));
                                    SW(val2, ptr);
                                    SW(val3, (ptr + 4));

                                    ILVRL_B2_UB(pix_e, pix_b, pix_r, pix_l);

                                    /* val2 = E-B */
                                    HSUB_UB2_SH(pix_r, pix_l, sub_r, sub_l);

                                    mask_r = __msa_clt_s_h(zero, sub_r);
                                    mask_l = __msa_clt_s_h(zero, sub_l);

                                    /* if val2 > 0 */
                                    mask =
                                        (v16u8) __msa_pckev_b((v16i8) mask_l,
                                                              (v16i8) mask_r);
                                    mask &= qp_mask;

                                    val_r = (sub_r + 3) >> 2;
                                    val_l = (sub_l + 3) >> 2;

                                    pix = (v16u8) __msa_pckev_b((v16i8) val_l,
                                                                (v16i8) val_r);

                                    pix0_0 = pix_b + pix;
                                    pix1_0 = pix_e - pix;

                                    /* else */
                                    val_r = (3 - sub_r) >> 2;
                                    val_l = (3 - sub_l) >> 2;

                                    pix = (v16u8) __msa_pckev_b((v16i8) val_l,
                                                                (v16i8) val_r);

                                    pix0_1 = pix_b - pix;
                                    pix1_1 = pix_e + pix;

                                    mask_r = __msa_clt_s_h(sub_r, zero);
                                    mask_l = __msa_clt_s_h(sub_l, zero);

                                    /* else if val2 < 0 */
                                    e_mask =
                                        (v16u8) __msa_pckev_b((v16i8) mask_l,
                                                              (v16i8) mask_r);
                                    e_mask &= qp_mask;

                                    pix0_1 = __msa_bmnz_v(pix_b, pix0_1,
                                                          e_mask);
                                    pix1_1 = __msa_bmnz_v(pix_e, pix1_1,
                                                          e_mask);

                                    pix_b = __msa_bmnz_v(pix0_1, pix0_0, mask);
                                    pix_e = __msa_bmnz_v(pix1_1, pix1_0, mask);

                                    val0 = __msa_copy_u_w((v4i32) pix_b, 0);
                                    val1 = __msa_copy_u_w((v4i32) pix_b, 1);
                                    val2 = __msa_copy_u_w((v4i32) pix_e, 0);
                                    val3 = __msa_copy_u_w((v4i32) pix_e, 1);
                                    SW(val0, ptr - (width << 1));
                                    SW(val1, (ptr - (width << 1) + 4));
                                    SW(val2, ptr + width);
                                    SW(val3, (ptr + width + 4));

                                    ILVRL_B2_UB(pix_f, pix_a, pix_r, pix_l);

                                    /* val2 = F-A */
                                    HSUB_UB2_SH(pix_r, pix_l, sub_r, sub_l);

                                    mask_r = __msa_clt_s_h(zero, sub_r);
                                    mask_l = __msa_clt_s_h(zero, sub_l);

                                    /* if val2 > 0 */
                                    mask =
                                        (v16u8) __msa_pckev_b((v16i8) mask_l,
                                                              (v16i8) mask_r);
                                    mask &= qp_mask;

                                    val_r = (sub_r + 7) >> 3;
                                    val_l = (sub_l + 7) >> 3;

                                    pix = (v16u8) __msa_pckev_b((v16i8) val_l,
                                                                (v16i8) val_r);

                                    pix0_0 = pix_a + pix;
                                    pix1_0 = pix_f - pix;

                                    /* else */
                                    val_r = (7 - sub_r) >> 3;
                                    val_l = (7 - sub_l) >> 3;

                                    pix = (v16u8) __msa_pckev_b((v16i8) val_l,
                                                                (v16i8) val_r);

                                    pix0_1 = pix_a - pix;
                                    pix1_1 = pix_f + pix;

                                    mask_r = __msa_clt_s_h(sub_r, zero);
                                    mask_l = __msa_clt_s_h(sub_l, zero);

                                    /* else if val2 < 0 */
                                    e_mask =
                                        (v16u8) __msa_pckev_b((v16i8) mask_l,
                                                              (v16i8) mask_r);
                                    e_mask &= qp_mask;

                                    pix0_1 = __msa_bmnz_v(pix_a, pix0_1, e_mask);
                                    pix1_1 = __msa_bmnz_v(pix_f, pix1_1, e_mask);

                                    pix_a = __msa_bmnz_v(pix0_1, pix0_0, mask);
                                    pix_f = __msa_bmnz_v(pix1_1, pix1_0, mask);

                                    val0 = __msa_copy_u_w((v4i32) pix_a, 0);
                                    val1 = __msa_copy_u_w((v4i32) pix_a, 1);
                                    val2 = __msa_copy_u_w((v4i32) pix_f, 0);
                                    val3 = __msa_copy_u_w((v4i32) pix_f, 1);
                                    SW(val0, (ptr - (width << 1) - width));
                                    SW(val1, (ptr - (width << 1) - width + 4));
                                    SW(val2, (ptr + (width << 1)));
                                    SW(val3, (ptr + (width << 1) + 4));
                                }
                            }
                        }
                    } /* col */
                } /* blk_in_col */
            } /* row */

            br_width -= (blk_in_row << 1);

            /* vertical filtering */
            v16u8 pix_a_org, pix_b_org, pix_c_org, pix_d_org;
            v16u8 pix_e_org, pix_f_org, pix_g_org, pix_h_org;

            for (row = blk_row_cnt; row < (blk_row_cnt + 2); row++)
            {
                if (row < blk_in_col)
                {
                    jval0 = br_width + blk_col_cnt + 1;
                    jval1 = jval0 + 1;

                    ptr = rec + (br_width << 6) + ((blk_col_cnt + 1) << 3);

                    if ((pp_mod[jval0 - 1] & 0x01) &&
                        (pp_mod[jval0] & 0x01) &&
                        (pp_mod[jval1 - 1] & 0x01) &&
                        (pp_mod[jval1] & 0x01) &&
                        (blk_col_cnt + 2 < blk_in_row))
                    {
                        LD_UB8(ptr - 3, width, pix_a, pix_b, pix_c, pix_d,
                               pix_e, pix_f, pix_g, pix_h);

                        pix_a_org = pix_a;
                        pix_b_org = pix_b;
                        pix_c_org = pix_c;
                        pix_d_org = pix_d;
                        pix_e_org = pix_e;
                        pix_f_org = pix_f;
                        pix_g_org = pix_g;
                        pix_h_org = pix_h;

                        TRANSPOSE16x8_B_DUAL(pix_a, pix_b, pix_c, pix_d,
                                             pix_e, pix_f, pix_g, pix_h,
                                             pix_a, pix_b, pix_c, pix_d,
                                             pix_e, pix_f, pix_g, pix_h);

                        if (chr)
                        {
                            qp0 = qp_store[jval0];
                            qp1 = qp_store[jval1];
                            qp_r = __msa_fill_h(qp0 << 1);
                            qp_l = __msa_fill_h(qp1 << 1);
                        }
                        else
                        {
                            qp_r = __msa_fill_h(qp << 1);
                            qp_l = __msa_fill_h(qp << 1);
                        }

                        ILVRL_B2_UB(pix_d, pix_c, pix_r, pix_l);

                        /* (D - C) */
                        HSUB_UB2_SH(pix_r, pix_l, sub_r, sub_l);

                        /* mask calc */
                        mask0 = __msa_clt_s_h(zero, sub_r);
                        mask0 &= __msa_clt_s_h(sub_r, qp_r);

                        q_pneg = zero - qp_r;
                        mask1 = __msa_clt_s_h(sub_r, zero);
                        mask1 &= __msa_clt_s_h(q_pneg, sub_r);

                        mask_r = mask0 | mask1;

                        mask0 = __msa_clt_s_h(zero, sub_l);
                        mask0 &= __msa_clt_s_h(sub_l, qp_l);

                        q_pneg = zero - qp_r;
                        mask1 = __msa_clt_s_h(sub_l, zero);
                        mask1 &= __msa_clt_s_h(q_pneg, sub_l);

                        mask_l = mask0 | mask1;

                        /* if condition */
                        qp_mask = (v16u8) __msa_pckev_b((v16i8) mask_l,
                                                        (v16i8) mask_r);

                        HADD_UB2_SH(pix_r, pix_l, add_r, add_l);

                        add_r >>= 1;
                        add_l >>= 1;

                        pix0_1 = (v16u8) __msa_pckev_b((v16i8) add_l,
                                                       (v16i8) add_r);

                        pix_c = __msa_bmnz_v(pix_c, pix0_1, qp_mask);
                        pix_d = __msa_bmnz_v(pix_d, pix0_1, qp_mask);

                        ILVRL_B2_UB(pix_e, pix_b, pix_r, pix_l);

                        /* val2 = E-B */
                        HSUB_UB2_SH(pix_r, pix_l, sub_r, sub_l);

                        mask_r = __msa_clt_s_h(zero, sub_r);
                        mask_l = __msa_clt_s_h(zero, sub_l);

                        /* if val2 > 0 */
                        mask = (v16u8) __msa_pckev_b((v16i8) mask_l,
                                                     (v16i8) mask_r);
                        mask &= qp_mask;

                        val_r = (sub_r + 3) >> 2;
                        val_l = (sub_l + 3) >> 2;

                        pix = (v16u8) __msa_pckev_b((v16i8) val_l,
                                                    (v16i8) val_r);

                        pix0_0 = pix_b + pix;
                        pix1_0 = pix_e - pix;

                        /* else */
                        val_r = (3 - sub_r) >> 2;
                        val_l = (3 - sub_l) >> 2;

                        pix = (v16u8) __msa_pckev_b((v16i8) val_l,
                                                    (v16i8) val_r);

                        pix0_1 = pix_b - pix;
                        pix1_1 = pix_e + pix;

                        mask_r = __msa_clt_s_h(sub_r, zero);
                        mask_l = __msa_clt_s_h(sub_l, zero);

                        /* else if val2 < 0 */
                        e_mask = (v16u8) __msa_pckev_b((v16i8) mask_l,
                                                       (v16i8) mask_r);
                        e_mask &= qp_mask;

                        pix0_1 = __msa_bmnz_v(pix_b, pix0_1, e_mask);
                        pix1_1 = __msa_bmnz_v(pix_e, pix1_1, e_mask);

                        pix_b = __msa_bmnz_v(pix0_1, pix0_0, mask);
                        pix_e = __msa_bmnz_v(pix1_1, pix1_0, mask);

                        ILVRL_B2_UB(pix_f, pix_a, pix_r, pix_l);

                        /* val2 = F-A */
                        HSUB_UB2_SH(pix_r, pix_l, sub_r, sub_l);

                        mask_r = __msa_clt_s_h(zero, sub_r);
                        mask_l = __msa_clt_s_h(zero, sub_l);

                        /* if val2 > 0 */
                        mask = (v16u8) __msa_pckev_b((v16i8) mask_l,
                                                     (v16i8) mask_r);
                        mask &= qp_mask;

                        val_r = (sub_r + 7) >> 3;
                        val_l = (sub_l + 7) >> 3;

                        pix = (v16u8) __msa_pckev_b((v16i8) val_l,
                                                    (v16i8) val_r);

                        pix0_0 = pix_a + pix;
                        pix1_0 = pix_f - pix;

                        /* else */
                        val_r = (sub_r - 7) >> 3;
                        val_l = (sub_l - 7) >> 3;

                        pix = (v16u8) __msa_pckev_b((v16i8) val_l,
                                                    (v16i8) val_r);

                        pix0_1 = pix_a + pix;
                        pix1_1 = pix_f - pix;

                        mask_r = __msa_clt_s_h(sub_r, zero);
                        mask_l = __msa_clt_s_h(sub_l, zero);

                        /* else if val2 < 0 */
                        e_mask = (v16u8) __msa_pckev_b((v16i8) mask_l,
                                                       (v16i8) mask_r);
                        e_mask &= qp_mask;

                        pix0_1 = __msa_bmnz_v(pix_a, pix0_1, e_mask);
                        pix1_1 = __msa_bmnz_v(pix_f, pix1_1, e_mask);

                        pix_a = __msa_bmnz_v(pix0_1, pix0_0, mask);
                        pix_f = __msa_bmnz_v(pix1_1, pix1_0, mask);

                        TRANSPOSE16x8_B_DUAL(pix_a, pix_b, pix_c, pix_d,
                                             pix_e, pix_f, pix_g, pix_h,
                                             pix_a, pix_b, pix_c, pix_d,
                                             pix_e, pix_f, pix_g, pix_h);

                        v16u8 mask = { 255, 255, 255, 255, 255, 255, 0, 0,
                                       255, 255, 255, 255, 255, 255, 0, 0 };

                        pix_a = __msa_bmnz_v(pix_a_org, pix_a, mask);
                        pix_b = __msa_bmnz_v(pix_b_org, pix_b, mask);
                        pix_c = __msa_bmnz_v(pix_c_org, pix_c, mask);
                        pix_d = __msa_bmnz_v(pix_d_org, pix_d, mask);
                        pix_e = __msa_bmnz_v(pix_e_org, pix_e, mask);
                        pix_f = __msa_bmnz_v(pix_f_org, pix_f, mask);
                        pix_g = __msa_bmnz_v(pix_g_org, pix_g, mask);
                        pix_h = __msa_bmnz_v(pix_h_org, pix_h, mask);

                        ST_UB8(pix_a, pix_b, pix_c, pix_d, pix_e, pix_f, pix_g, pix_h,
                               ptr - 3, width);
                    }
                    else
                    {
                        for (col = (blk_col_cnt + 1); col < (blk_col_cnt + 3);
                             col++)
                        {
                            /* check boundary for deblocking */
                            if (col < blk_in_row)
                            {
                                ptr = rec + (br_width << 6) + (col << 3);
                                jval0 = br_width + col;

                                if (chr)
                                    qp = qp_store[jval0];

                                ptr_dup = ptr;

                                /* vertical hard filter */
                                if ((pp_mod[jval0 - 1] & 0x01) &&
                                    (pp_mod[jval0] & 0x01))
                                {
                                    ptr = ptr_dup - 3;

                                    LD_UB8(ptr, width, pix_a, pix_b, pix_c, pix_d,
                                           pix_e, pix_f, pix_g, pix_h);

                                    TRANSPOSE8x8_UB_UB(pix_a, pix_b, pix_c, pix_d,
                                                       pix_e, pix_f, pix_g, pix_h,
                                                       pix_a, pix_b, pix_c, pix_d,
                                                       pix_e, pix_f, pix_g, pix_h);

                                    qp_vec = __msa_fill_h(qp << 1);

                                    ILVRL_B2_UB(pix_d, pix_c, pix_r, pix_l);

                                    /* (D-C) */
                                    HSUB_UB2_SH(pix_r, pix_l, sub_r, sub_l);

                                    /* mask calc */
                                    mask0 = __msa_clt_s_h(zero, sub_r);
                                    mask0 &= __msa_clt_s_h(sub_r, qp_vec);

                                    q_pneg = zero - qp_vec;
                                    mask1 = __msa_clt_s_h(sub_r, zero);
                                    mask1 &= __msa_clt_s_h(q_pneg, sub_r);

                                    mask_r = mask0 | mask1;

                                    mask0 = __msa_clt_s_h(zero, sub_l);
                                    mask0 &= __msa_clt_s_h(sub_l, qp_vec);

                                    mask1 = __msa_clt_s_h(sub_l, zero);
                                    mask1 &= __msa_clt_s_h(q_pneg, sub_l);

                                    mask_l = mask0 | mask1;

                                    /* if condition */
                                    qp_mask =
                                        (v16u8) __msa_pckev_b((v16i8) mask_l,
                                                              (v16i8) mask_r);

                                    HADD_UB2_SH(pix_r, pix_l, add_r, add_l);

                                    add_r >>= 1;
                                    add_l >>= 1;

                                    pix0_1 =
                                        (v16u8) __msa_pckev_b((v16i8) add_l,
                                                              (v16i8) add_r);

                                    pix_c = __msa_bmnz_v(pix_c, pix0_1,
                                                         qp_mask);
                                    pix_d = __msa_bmnz_v(pix_d, pix0_1,
                                                         qp_mask);

                                    ILVRL_B2_UB(pix_e, pix_b, pix_r, pix_l);

                                    /* val2 = E-B */
                                    HSUB_UB2_SH(pix_r, pix_l, sub_r, sub_l);

                                    mask_r = __msa_clt_s_h(zero, sub_r);
                                    mask_l = __msa_clt_s_h(zero, sub_l);

                                    /* if val2 > 0 */
                                    mask =
                                        (v16u8) __msa_pckev_b((v16i8) mask_l,
                                                              (v16i8) mask_r);
                                    mask &= qp_mask;

                                    val_r = (sub_r + 3) >> 2;
                                    val_l = (sub_l + 3) >> 2;

                                    pix = (v16u8) __msa_pckev_b((v16i8) val_l,
                                                                (v16i8) val_r);

                                    pix0_0 = pix_b + pix;
                                    pix1_0 = pix_e - pix;

                                    /* else */
                                    val_r = (3 - sub_r) >> 2;
                                    val_l = (3 - sub_l) >> 2;

                                    pix = (v16u8) __msa_pckev_b((v16i8) val_l,
                                                                (v16i8) val_r);

                                    pix0_1 = pix_b - pix;
                                    pix1_1 = pix_e + pix;

                                    mask_r = __msa_clt_s_h(sub_r, zero);
                                    mask_l = __msa_clt_s_h(sub_l, zero);

                                    /* else if val2 < 0 */
                                    e_mask =
                                        (v16u8) __msa_pckev_b((v16i8) mask_l,
                                                              (v16i8) mask_r);
                                    e_mask &= qp_mask;

                                    pix0_1 = __msa_bmnz_v(pix_b, pix0_1,
                                                          e_mask);
                                    pix1_1 = __msa_bmnz_v(pix_e, pix1_1,
                                                          e_mask);

                                    pix_b = __msa_bmnz_v(pix0_1, pix0_0, mask);
                                    pix_e = __msa_bmnz_v(pix1_1, pix1_0, mask);

                                    ILVRL_B2_UB(pix_f, pix_a, pix_r, pix_l);

                                    /* val2 = F-A */
                                    HSUB_UB2_SH(pix_r, pix_l, sub_r, sub_l);

                                    mask_r = __msa_clt_s_h(zero, sub_r);
                                    mask_l = __msa_clt_s_h(zero, sub_l);

                                    /* if val2 > 0 */
                                    mask =
                                        (v16u8) __msa_pckev_b((v16i8) mask_l,
                                                              (v16i8) mask_r);
                                    mask &= qp_mask;

                                    val_r = (sub_r + 7) >> 3;
                                    val_l = (sub_l + 7) >> 3;

                                    pix = (v16u8) __msa_pckev_b((v16i8) val_l,
                                                                (v16i8) val_r);

                                    pix0_0 = pix_a + pix;
                                    pix1_0 = pix_f - pix;

                                    /* else */
                                    val_r = (sub_r - 7) >> 3;
                                    val_l = (sub_l - 7) >> 3;

                                    pix = (v16u8) __msa_pckev_b((v16i8) val_l,
                                                                (v16i8) val_r);

                                    pix0_1 = pix_a + pix;
                                    pix1_1 = pix_f - pix;

                                    mask_r = __msa_clt_s_h(sub_r, zero);
                                    mask_l = __msa_clt_s_h(sub_l, zero);

                                    /* else if val2 < 0 */
                                    e_mask =
                                        (v16u8) __msa_pckev_b((v16i8) mask_l,
                                                              (v16i8) mask_r);
                                    e_mask &= qp_mask;

                                    pix0_1 = __msa_bmnz_v(pix_a, pix0_1,
                                                          e_mask);
                                    pix1_1 = __msa_bmnz_v(pix_f, pix1_1,
                                                          e_mask);

                                    pix_a = __msa_bmnz_v(pix0_1, pix0_0, mask);
                                    pix_f = __msa_bmnz_v(pix1_1, pix1_0, mask);

                                    /* STORE */
                                    TRANSPOSE8x8_UB_UB(pix_a, pix_b, pix_c, pix_d,
                                                       pix_e, pix_f, pix_g, pix_h,
                                                       pix_a, pix_b, pix_c, pix_d,
                                                       pix_e, pix_f, pix_g, pix_h);

                                    val0 = __msa_copy_u_w((v4i32) pix_a, 0);
                                    num0 = __msa_copy_u_h((v8i16) pix_a, 2);
                                    val1 = __msa_copy_u_w((v4i32) pix_b, 0);
                                    num1 = __msa_copy_u_h((v8i16) pix_b, 2);
                                    SW(val0, ptr);
                                    SH(num0, ptr + 4);
                                    ptr += width;
                                    SW(val1, ptr);
                                    SH(num1, ptr + 4);
                                    ptr += width;

                                    val0 = __msa_copy_u_w((v4i32) pix_c, 0);
                                    num0 = __msa_copy_u_h((v8i16) pix_c, 2);
                                    val1 = __msa_copy_u_w((v4i32) pix_d, 0);
                                    num1 = __msa_copy_u_h((v8i16) pix_d, 2);
                                    SW(val0, ptr);
                                    SH(num0, ptr + 4);
                                    ptr += width;
                                    SW(val1, ptr);
                                    SH(num1, ptr + 4);
                                    ptr += width;

                                    val0 = __msa_copy_u_w((v4i32) pix_e, 0);
                                    num0 = __msa_copy_u_h((v8i16) pix_e, 2);
                                    val1 = __msa_copy_u_w((v4i32) pix_f, 0);
                                    num1 = __msa_copy_u_h((v8i16) pix_f, 2);
                                    SW(val0, ptr);
                                    SH(num0, ptr + 4);
                                    ptr += width;
                                    SW(val1, ptr);
                                    SH(num1, ptr + 4);
                                    ptr += width;

                                    val0 = __msa_copy_u_w((v4i32) pix_g, 0);
                                    num0 = __msa_copy_u_h((v8i16) pix_g, 2);
                                    val1 = __msa_copy_u_w((v4i32) pix_h, 0);
                                    num1 = __msa_copy_u_h((v8i16) pix_h, 2);
                                    SW(val0, ptr);
                                    SH(num0, ptr + 4);
                                    ptr += width;
                                    SW(val1, ptr);
                                    SH(num1, ptr + 4);
                                    ptr += width;
                                } /* end if */
                            } /* boundary */
                        } /* col */
                    }
                } /* boundary check */

                br_width += blk_in_row;
            } /* row */

            br_width -= (blk_in_row << 1);
        } /* blk_col_cnt */

        br_width += (blk_in_row << 1);
    } /* blk_row_cnt */
}
#endif /* M4VH263DEC_MSA */
#endif /* PV_POSTPROC_ON */
