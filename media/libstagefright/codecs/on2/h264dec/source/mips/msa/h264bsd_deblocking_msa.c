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

#include "basetype.h"
#include "h264bsd_macros_msa.h"

#include "h264bsd_macroblock_layer.h"
#include "h264bsd_prototypes_msa.h"
#include "h264bsd_util.h"

#ifdef H264DEC_MSA
/*******************************************************************************
                                Macros
*******************************************************************************/
#define AVC_BS(mb, bs)                                                      \
{                                                                           \
    v16u8 coeff, coeff_reordered, bs_top, bs_left;                          \
    v16u8 zero = { 0 };                                                     \
    v16u8 const2;                                                           \
                                                                            \
    const2 = (v16u8) __msa_ldi_b(2);                                        \
    coeff = LD_UB(mb->totalCoeff);                                         \
    coeff_reordered = LD_UB(mb->totalCoeff + 8);                           \
                                                                            \
    coeff = (v16u8) __msa_pckev_b((v16i8) coeff_reordered, (v16i8) coeff);  \
    coeff_reordered = (v16u8) __msa_shf_h((v8i16) coeff, 0xD8);             \
                                                                            \
    bs_top = (v16u8) __msa_sldi_b((v16i8) coeff_reordered,                  \
                                  (v16i8) coeff_reordered, 12);             \
    bs_top = coeff_reordered | bs_top;                                      \
    bs_top = (zero < bs_top);                                               \
    bs_top = __msa_bmnz_v(zero, const2, bs_top);                            \
                                                                            \
    bs_left = (v16u8) __msa_sldi_b((v16i8) coeff_reordered,                 \
                                   (v16i8) coeff_reordered, 15);            \
    bs_left = coeff_reordered | bs_left;                                    \
    bs_left = (zero < bs_left);                                             \
    bs_left = __msa_bmnz_v(zero, const2, bs_left);                          \
                                                                            \
    ILVRL_B2_UB(bs_left, bs_top, coeff_reordered, coeff);                   \
                                                                            \
    ST_UB(coeff_reordered, bs);                                             \
    ST_UB(coeff, &(bs[8].top));                                             \
}

#define AVC_LPF_P0P1P2_OR_Q0Q1Q2(p3_or_q3_org_in, p0_or_q0_org_in,          \
                                 q3_or_p3_org_in, p1_or_q1_org_in,          \
                                 p2_or_q2_org_in, q1_or_p1_org_in,          \
                                 p0_or_q0_out, p1_or_q1_out, p2_or_q2_out)  \
{                                                                           \
    v8i16 threshold;                                                        \
    v8i16 const3 = __msa_ldi_h(3);                                          \
                                                                            \
    threshold = (p0_or_q0_org_in) + (q3_or_p3_org_in);                      \
    threshold += (p1_or_q1_org_in);                                         \
                                                                            \
    (p0_or_q0_out) = threshold << 1;                                        \
    (p0_or_q0_out) += (p2_or_q2_org_in);                                    \
    (p0_or_q0_out) += (q1_or_p1_org_in);                                    \
    (p0_or_q0_out) = __msa_srari_h((p0_or_q0_out), 3);                      \
                                                                            \
    (p1_or_q1_out) = (p2_or_q2_org_in) + threshold;                         \
    (p1_or_q1_out) = __msa_srari_h((p1_or_q1_out), 2);                      \
                                                                            \
    (p2_or_q2_out) = (p2_or_q2_org_in) * const3;                            \
    (p2_or_q2_out) += (p3_or_q3_org_in);                                    \
    (p2_or_q2_out) += (p3_or_q3_org_in);                                    \
    (p2_or_q2_out) += threshold;                                            \
    (p2_or_q2_out) = __msa_srari_h((p2_or_q2_out), 3);                      \
}

/* data[-u32_img_width] = (u8)((2 * p1 + p0 + q1 + 2) >> 2); */
#define AVC_LPF_P0_OR_Q0(p0_or_q0_org_in, q1_or_p1_org_in,   \
                         p1_or_q1_org_in, p0_or_q0_out)      \
{                                                            \
    (p0_or_q0_out) = (p0_or_q0_org_in) + (q1_or_p1_org_in);  \
    (p0_or_q0_out) += (p1_or_q1_org_in);                     \
    (p0_or_q0_out) += (p1_or_q1_org_in);                     \
    (p0_or_q0_out) = __msa_srari_h((p0_or_q0_out), 2);       \
}

#define AVC_LPF_P1_OR_Q1(p0_or_q0_org_in, q0_or_p0_org_in,    \
                         p1_or_q1_org_in, p2_or_q2_org_in,    \
                         negate_tc_in, tc_in, p1_or_q1_out)   \
{                                                             \
    v8i16 clip3, temp;                                        \
                                                              \
    clip3 = (v8i16) __msa_aver_u_h((v8u16) p0_or_q0_org_in,   \
                                   (v8u16) q0_or_p0_org_in);  \
    temp = p1_or_q1_org_in << 1;                              \
    clip3 = clip3 - temp;                                     \
    clip3 = __msa_ave_s_h(p2_or_q2_org_in, clip3);            \
    clip3 = CLIP_SH(clip3, negate_tc_in, tc_in);              \
    p1_or_q1_out = p1_or_q1_org_in + clip3;                   \
}

#define AVC_LPF_P0Q0(q0_or_p0_org_in, p0_or_q0_org_in,          \
                     p1_or_q1_org_in, q1_or_p1_org_in,          \
                     negate_threshold_in, threshold_in,         \
                     p0_or_q0_out, q0_or_p0_out)                \
{                                                               \
    v8i16 q0_sub_p0, p1_sub_q1, delta;                          \
                                                                \
    q0_sub_p0 = q0_or_p0_org_in - p0_or_q0_org_in;              \
    p1_sub_q1 = p1_or_q1_org_in - q1_or_p1_org_in;              \
    q0_sub_p0 <<= 2;                                            \
    p1_sub_q1 += 4;                                             \
    delta = q0_sub_p0 + p1_sub_q1;                              \
    delta >>= 3;                                                \
                                                                \
    delta = CLIP_SH(delta, negate_threshold_in, threshold_in);  \
                                                                \
    p0_or_q0_out = p0_or_q0_org_in + delta;                     \
    q0_or_p0_out = q0_or_p0_org_in - delta;                     \
                                                                \
    CLIP_SH2_0_255(p0_or_q0_out, q0_or_p0_out);                 \
}

/*******************************************************************************
                              Boundary Strength
*******************************************************************************/
u32 avc_get_boundary_strengths_msa(mbStorage_t *mb, bS_t *bs,
                                   u32 flags)
{
    u32 non_zero_bs = HANTRO_FALSE;

    /* inner edges */
    if (IS_INTRA_MB(*mb))
    {
        v16u8 const3 = (v16u8) __msa_ldi_b(3);

        ST_UB(const3, (u8 *) bs);
        ST_UB(const3, &(bs[8]));

        non_zero_bs = HANTRO_TRUE;
    }
    else
    {
        if (1 == h264bsdNumMbPart(mb->mbType))
        {
            AVC_BS(mb, bs);
        }
        else if (P_L0_L0_16x8 == mb->mbType)
        {
            AVC_BS(mb, bs);

            bs[8].top = InnerBoundaryStrength(mb, 8, 2);
            bs[9].top = InnerBoundaryStrength(mb, 9, 3);
            bs[10].top = InnerBoundaryStrength(mb, 12, 6);
            bs[11].top = InnerBoundaryStrength(mb, 13, 7);
        }
        else if (P_L0_L0_8x16 == mb->mbType)
        {
            AVC_BS(mb, bs);

            bs[2].left = InnerBoundaryStrength(mb, 4, 1);
            bs[6].left = InnerBoundaryStrength(mb, 6, 3);
            bs[10].left = InnerBoundaryStrength(mb, 12, 9);
            bs[14].left = InnerBoundaryStrength(mb, 14, 11);
        }
        else
        {
            v16u8 coeff_condition_l, coeff_condition_t;
            v16u8 addr_condition_l, addr_condition_t;
            v16i8 bs_left_addr, bs_top_addr;
            v16i8 bs_top_coeff, bs_left_coeff;
            v16u8 zero = { 0 };
            v8i16 m_v0, m_v1, m_v2, m_v3, m_v4, m_v5, m_v6, m_v7;
            v4i32 ref_addr1100, ref_addr3322, ref_addr2200, ref_addr3311;
            v4i32 ref_addr = { 0 };
            v4i32 is_equal_left, is_not_equal_left;
            v4i32 is_equal_top, is_not_equal_top;
            v16i8 coeff, coeff_reordered;
            v16i8 temp, horizontal, vertical;
            v16u8 const2 = (v16u8) __msa_ldi_b(2);
            v16u8 const1 = (v16u8) __msa_ldi_b(1);
            v8i16 const4 = __msa_ldi_h(4);

            coeff = LD_SB(mb->totalCoeff);
            coeff_reordered = LD_SB(mb->totalCoeff + 8);
            coeff = __msa_pckev_b(coeff_reordered, coeff);
            coeff_reordered = (v16i8) __msa_shf_h((v8i16) coeff, 0xD8);
            bs_top_coeff = __msa_sldi_b(coeff_reordered, coeff_reordered, 12);
            bs_top_coeff = coeff_reordered | bs_top_coeff;
            bs_top_coeff = (zero < bs_top_coeff);

            bs_left_coeff = __msa_sldi_b(coeff_reordered, coeff_reordered, 15);
            bs_left_coeff = coeff_reordered | bs_left_coeff;
            bs_left_coeff = (zero < bs_left_coeff);
            ILVRL_B2_UB(bs_left_coeff, bs_top_coeff, coeff_condition_t,
                        coeff_condition_l);
            bs_top_coeff = (v16i8) __msa_bmnz_v(zero, const2,
                                                coeff_condition_t);
            bs_left_coeff = (v16i8) __msa_bmnz_v(zero, const2,
                                                 coeff_condition_l);

            INSERT_W4_SW((uintptr_t) mb->refAddr[0],
                         (uintptr_t) mb->refAddr[1],
                         (uintptr_t) mb->refAddr[2],
                         (uintptr_t) mb->refAddr[3], ref_addr);

            ref_addr1100 = __msa_shf_w(ref_addr, 0x50);
            ref_addr3322 = __msa_shf_w(ref_addr, 0xFA);
            ref_addr2200 = __msa_shf_w(ref_addr, 0xA0);
            ref_addr3311 = __msa_shf_w(ref_addr, 0xF5);

            is_equal_top = (ref_addr1100 == ref_addr3322);
            is_equal_left = (ref_addr3311 == ref_addr2200);
            is_not_equal_top = (v4i32) __msa_xori_b((v16u8) is_equal_top, 0xFF);
            is_not_equal_left = (v4i32) __msa_xori_b((v16u8) is_equal_left,
                                                     0xFF);
            m_v0 = LD_SH(&(mb->mv[0].hor));
            m_v1 = LD_SH(&(mb->mv[4].hor));
            m_v2 = LD_SH(&(mb->mv[8].hor));
            m_v3 = LD_SH(&(mb->mv[12].hor));

            {
                v8i16 shuffle_a = { 0, 1, 2, 3, 8, 9, 10, 11 };
                VSHF_H2_SH(m_v0, m_v1, m_v2, m_v3, shuffle_a, shuffle_a,
                           m_v4, m_v6);
            }
            {
                v8i16 shuffle_b = { 4, 5, 6, 7, 12, 13, 14, 15 };
                VSHF_H2_SH(m_v0, m_v1, m_v2, m_v3, shuffle_b, shuffle_b,
                           m_v5, m_v7);
            }

            {
                v8i16 mv5_asub_mv4, mv6_asub_mv5, mv7_asub_mv6;

                mv5_asub_mv4 = __msa_asub_s_h(m_v5, m_v4);
                mv6_asub_mv5 = __msa_asub_s_h(m_v6, m_v5);
                mv7_asub_mv6 = __msa_asub_s_h(m_v7, m_v6);
                mv5_asub_mv4 = (const4 <= mv5_asub_mv4);
                mv6_asub_mv5 = (const4 <= mv6_asub_mv5);
                mv7_asub_mv6 = (const4 <= mv7_asub_mv6);

                /* 16 bit logical(1 or 0) output can interpreted in 8 bit */
                PCKEV_B2_SB(mv5_asub_mv4, mv5_asub_mv4, mv7_asub_mv6,
                            mv6_asub_mv5, bs_top_addr, temp);
            }

            /* separate horizontal and vertical mv component */
            horizontal = __msa_pckev_b((v16i8) temp, bs_top_addr);
            vertical = __msa_pckod_b((v16i8) temp, bs_top_addr);
            bs_top_addr = horizontal | vertical;
            {
                v16i8 shuffle_c =
                    { 0, 1, 2, 3, 4, 5, 6, 7, 16, 20, 24, 28, 12, 13, 14, 15 };

                is_not_equal_top =
                    (v4i32) __msa_vshf_b(shuffle_c,
                                         (v16i8) is_not_equal_top,
                                         (v16i8) zero);
            }
            /* or with address */
            bs_top_addr = bs_top_addr | (v16i8) is_not_equal_top;

            {
                v8i16 mv4_shift_r4_byte, mv5_shift_r4_byte;
                v8i16 mv6_shift_r4_byte, mv7_shift_r4_byte;
                v4i32 mv4_asub_mv4, mv5_asub_mv5, mv6asub_mv6;
                v4i32 mv7_asub_mv7;

                mv4_shift_r4_byte = (v8i16) __msa_shf_w((v4i32) m_v4, 0x90);
                mv5_shift_r4_byte = (v8i16) __msa_shf_w((v4i32) m_v5, 0x90);
                mv6_shift_r4_byte = (v8i16) __msa_shf_w((v4i32) m_v6, 0x90);
                mv7_shift_r4_byte = (v8i16) __msa_shf_w((v4i32) m_v7, 0x90);

                mv4_asub_mv4 = (v4i32) __msa_asub_s_h(mv4_shift_r4_byte,
                                                      (v8i16) m_v4);
                mv5_asub_mv5 = (v4i32) __msa_asub_s_h(mv5_shift_r4_byte,
                                                      (v8i16) m_v5);
                mv6asub_mv6 = (v4i32) __msa_asub_s_h(mv6_shift_r4_byte,
                                                     (v8i16) m_v6);
                mv7_asub_mv7 = (v4i32) __msa_asub_s_h(mv7_shift_r4_byte,
                                                      (v8i16) m_v7);

                mv4_asub_mv4 = (const4 <= (v8u16) mv4_asub_mv4);
                mv5_asub_mv5 = (const4 <= (v8u16) mv5_asub_mv5);
                mv6asub_mv6 = (const4 <= (v8u16) mv6asub_mv6);
                mv7_asub_mv7 = (const4 <= (v8u16) mv7_asub_mv7);

                PCKEV_B2_SB(mv5_asub_mv5, mv5_asub_mv5, mv7_asub_mv7,
                            mv6asub_mv6, bs_left_addr, temp);
            }
            horizontal = __msa_pckev_b(temp, bs_left_addr);
            vertical = __msa_pckod_b(temp, bs_left_addr);
            bs_left_addr = horizontal | vertical;

            {
                v16i8 shuffle_d =
                    { 0, 1, 16, 3, 4, 5, 20, 7, 8, 9, 24, 11, 12, 13, 28, 15 };

                is_not_equal_left =
                    (v4i32) __msa_vshf_b(shuffle_d,
                                         (v16i8) is_not_equal_left,
                                         (v16i8) zero);
            }
            /* or with address */
            bs_left_addr = bs_left_addr | (v16i8) is_not_equal_left;

            ILVRL_B2_UB(bs_left_addr, bs_top_addr, addr_condition_t,
                        addr_condition_l);

            /* negate */
            coeff_condition_t = __msa_xori_b(coeff_condition_t, 0xFF);
            coeff_condition_l = __msa_xori_b(coeff_condition_l, 0xFF);

            addr_condition_t = coeff_condition_t & addr_condition_t;
            addr_condition_l = coeff_condition_l & addr_condition_l;

            bs_top_addr = (v16i8) __msa_bmnz_v((v16u8) bs_top_coeff, const1,
                                               addr_condition_t);
            bs_left_addr = (v16i8) __msa_bmnz_v((v16u8) bs_left_coeff, const1,
                                                addr_condition_l);
            ST_SB(bs_top_addr, bs);
            ST_SB(bs_left_addr, &(bs[8].top));
        }

        if (bs[4].top || bs[5].top || bs[6].top ||
            bs[7].top || bs[8].top || bs[9].top ||
            bs[10].top || bs[11].top || bs[12].top ||
            bs[13].top || bs[14].top || bs[15].top ||
            bs[1].left || bs[2].left || bs[3].left ||
            bs[5].left || bs[6].left || bs[7].left ||
            bs[9].left || bs[10].left || bs[11].left ||
            bs[13].left || bs[14].left || bs[15].left)
        {
            non_zero_bs = HANTRO_TRUE;
        }
    }

    /* top edges */
    if (flags & FILTER_TOP_EDGE)
    {
        if (IS_INTRA_MB(*mb) || IS_INTRA_MB(*mb->mbB))
        {
            bs[0].top = bs[1].top = bs[2].top = bs[3].top = 4;
            non_zero_bs = HANTRO_TRUE;
        }
        else
        {
            bs[0].top = EdgeBoundaryStrength(mb, mb->mbB, 0, 10);
            bs[1].top = EdgeBoundaryStrength(mb, mb->mbB, 1, 11);
            bs[2].top = EdgeBoundaryStrength(mb, mb->mbB, 4, 14);
            bs[3].top = EdgeBoundaryStrength(mb, mb->mbB, 5, 15);

            if (!non_zero_bs &&
                (bs[0].top || bs[1].top || bs[2].top || bs[3].top))
            {
                non_zero_bs = HANTRO_TRUE;
            }
        }
    }
    else
    {
        bs[0].top = bs[1].top = bs[2].top = bs[3].top = 0;
    }

    /* left edges */
    if (flags & FILTER_LEFT_EDGE)
    {
        if (IS_INTRA_MB(*mb) || IS_INTRA_MB(*mb->mbA))
        {
            bs[0].left = bs[4].left = bs[8].left = bs[12].left = 4;
            non_zero_bs = HANTRO_TRUE;
        }
        else
        {
            bs[0].left = EdgeBoundaryStrength(mb, mb->mbA, 0, 5);
            bs[4].left = EdgeBoundaryStrength(mb, mb->mbA, 2, 7);
            bs[8].left = EdgeBoundaryStrength(mb, mb->mbA, 8, 13);
            bs[12].left = EdgeBoundaryStrength(mb, mb->mbA, 10, 15);

            if (!non_zero_bs &&
                (bs[0].left || bs[4].left || bs[8].left || bs[12].left))
            {
                non_zero_bs = HANTRO_TRUE;
            }
        }
    }
    else
    {
        bs[0].left = bs[4].left = bs[8].left = bs[12].left = 0;
    }

    return non_zero_bs;
}

/*******************************************************************************
                              Intra Luma
*******************************************************************************/
void avc_loopfilter_luma_intra_edge_hor_msa(u8 *data, u8 alpha_in,
                                            u8 beta_in, u32 img_width)
{
    v16u8 p2_asub_p0, q2_asub_q0, p0_asub_q0;
    v16u8 alpha, beta;
    v16u8 is_less_than, is_less_than_beta, negate_is_less_than_beta;
    v16u8 p2, p1, p0, q0, q1, q2;
    v16u8 p3_org, p2_org, p1_org, p0_org, q0_org, q1_org, q2_org, q3_org;
    v8i16 p1_org_r, p0_org_r, q0_org_r, q1_org_r;
    v8i16 p1_org_l, p0_org_l, q0_org_l, q1_org_l;
    v8i16 p2_r = { 0 };
    v8i16 p1_r = { 0 };
    v8i16 p0_r = { 0 };
    v8i16 q0_r = { 0 };
    v8i16 q1_r = { 0 };
    v8i16 q2_r = { 0 };
    v8i16 p2_l = { 0 };
    v8i16 p1_l = { 0 };
    v8i16 p0_l = { 0 };
    v8i16 q0_l = { 0 };
    v8i16 q1_l = { 0 };
    v8i16 q2_l = { 0 };
    v16u8 tmp_flag;
    v16i8 zero = { 0 };

    alpha = (v16u8) __msa_fill_b(alpha_in);
    beta = (v16u8) __msa_fill_b(beta_in);

    LD_UB4(data - (img_width << 1), img_width, p1_org, p0_org, q0_org, q1_org);

    {
        v16u8 p1_asub_p0, q1_asub_q0, is_less_than_alpha;

        p0_asub_q0 = __msa_asub_u_b(p0_org, q0_org);
        p1_asub_p0 = __msa_asub_u_b(p1_org, p0_org);
        q1_asub_q0 = __msa_asub_u_b(q1_org, q0_org);

        is_less_than_alpha = (p0_asub_q0 < alpha);
        is_less_than_beta = (p1_asub_p0 < beta);
        is_less_than = is_less_than_beta & is_less_than_alpha;
        is_less_than_beta = (q1_asub_q0 < beta);
        is_less_than = is_less_than_beta & is_less_than;
    }

    if (!__msa_test_bz_v(is_less_than))
    {
        q2_org = LD_UB(data + (2 * img_width));
        p3_org = LD_UB(data - (img_width << 2));
        p2_org = LD_UB(data - (3 * img_width));

        UNPCK_UB_SH(p1_org, p1_org_r, p1_org_l);
        UNPCK_UB_SH(p0_org, p0_org_r, p0_org_l);
        UNPCK_UB_SH(q0_org, q0_org_r, q0_org_l);

        tmp_flag = alpha >> 2;
        tmp_flag = tmp_flag + 2;
        tmp_flag = (p0_asub_q0 < tmp_flag);

        p2_asub_p0 = __msa_asub_u_b(p2_org, p0_org);
        is_less_than_beta = (p2_asub_p0 < beta);
        is_less_than_beta = is_less_than_beta & tmp_flag;
        negate_is_less_than_beta = __msa_xori_b(is_less_than_beta, 0xff);
        is_less_than_beta = is_less_than_beta & is_less_than;
        negate_is_less_than_beta = negate_is_less_than_beta & is_less_than;
        {
            v8u16 is_less_than_beta_l, is_less_than_beta_r;

            q1_org_r = (v8i16) __msa_ilvr_b(zero, (v16i8) q1_org);

            is_less_than_beta_r =
                (v8u16) __msa_sldi_b((v16i8) is_less_than_beta, zero, 8);
            if (!__msa_test_bz_v((v16u8) is_less_than_beta_r))
            {
                v8i16 p3_org_r;

                ILVR_B2_SH(zero, p3_org, zero, p2_org, p3_org_r, p2_r);
                AVC_LPF_P0P1P2_OR_Q0Q1Q2(p3_org_r, p0_org_r, q0_org_r, p1_org_r,
                                         p2_r, q1_org_r, p0_r, p1_r, p2_r);
            }

            q1_org_l = (v8i16) __msa_ilvl_b(zero, (v16i8) q1_org);

            is_less_than_beta_l =
                (v8u16) __msa_sldi_b(zero, (v16i8) is_less_than_beta, 8);

            if (!__msa_test_bz_v((v16u8) is_less_than_beta_l))
            {
                v8i16 p3_org_l;

                ILVL_B2_SH(zero, p3_org, zero, p2_org, p3_org_l, p2_l);
                AVC_LPF_P0P1P2_OR_Q0Q1Q2(p3_org_l, p0_org_l, q0_org_l, p1_org_l,
                                         p2_l, q1_org_l, p0_l, p1_l, p2_l);
            }
        }
        /* combine and store */
        if (!__msa_test_bz_v(is_less_than_beta))
        {
            PCKEV_B3_UB(p0_l, p0_r, p1_l, p1_r, p2_l, p2_r, p0, p1, p2);

            p0_org = __msa_bmnz_v(p0_org, p0, is_less_than_beta);
            p1_org = __msa_bmnz_v(p1_org, p1, is_less_than_beta);
            p2_org = __msa_bmnz_v(p2_org, p2, is_less_than_beta);

            ST_UB(p1_org, data - (2 * img_width));
            ST_UB(p2_org, data - (3 * img_width));
        }
        {
            v8u16 negate_is_less_than_beta_r, negate_is_less_than_beta_l;

            negate_is_less_than_beta_r =
                (v8u16) __msa_sldi_b((v16i8) negate_is_less_than_beta, zero, 8);
            if (!__msa_test_bz_v((v16u8) negate_is_less_than_beta_r))
            {
                AVC_LPF_P0_OR_Q0(p0_org_r, q1_org_r, p1_org_r, p0_r);
            }

            negate_is_less_than_beta_l =
                (v8u16) __msa_sldi_b(zero, (v16i8) negate_is_less_than_beta, 8);
            if (!__msa_test_bz_v((v16u8) negate_is_less_than_beta_l))
            {
                AVC_LPF_P0_OR_Q0(p0_org_l, q1_org_l, p1_org_l, p0_l);
            }
        }
        /* combine */
        if (!__msa_test_bz_v(negate_is_less_than_beta))
        {
            p0 = (v16u8) __msa_pckev_b((v16i8) p0_l, (v16i8) p0_r);
            p0_org = __msa_bmnz_v(p0_org, p0, negate_is_less_than_beta);
        }

        ST_UB(p0_org, data - img_width);

        /* if (tmpFlag && (unsigned)ABS(q2-q0) < thresholds->beta_in) */
        q3_org = LD_UB(data + (3 * img_width));
        q2_asub_q0 = __msa_asub_u_b(q2_org, q0_org);
        is_less_than_beta = (q2_asub_q0 < beta);
        is_less_than_beta = is_less_than_beta & tmp_flag;
        negate_is_less_than_beta = __msa_xori_b(is_less_than_beta, 0xff);
        is_less_than_beta = is_less_than_beta & is_less_than;
        negate_is_less_than_beta = negate_is_less_than_beta & is_less_than;

        {
            v8u16 is_less_than_beta_l, is_less_than_beta_r;
            is_less_than_beta_r =
                (v8u16) __msa_sldi_b((v16i8) is_less_than_beta, zero, 8);
            if (!__msa_test_bz_v((v16u8) is_less_than_beta_r))
            {
                v8i16 q3_org_r;

                ILVR_B2_SH(zero, q3_org, zero, q2_org, q3_org_r, q2_r);
                AVC_LPF_P0P1P2_OR_Q0Q1Q2(q3_org_r, q0_org_r, p0_org_r, q1_org_r,
                                         q2_r, p1_org_r, q0_r, q1_r, q2_r);
            }
            is_less_than_beta_l =
                (v8u16) __msa_sldi_b(zero, (v16i8) is_less_than_beta, 8);
            if (!__msa_test_bz_v((v16u8) is_less_than_beta_l))
            {
                v8i16 q3_org_l;

                ILVL_B2_SH(zero, q3_org, zero, q2_org, q3_org_l, q2_l);
                AVC_LPF_P0P1P2_OR_Q0Q1Q2(q3_org_l, q0_org_l, p0_org_l, q1_org_l,
                                         q2_l, p1_org_l, q0_l, q1_l, q2_l);
            }
        }

        /* combine and store */
        if (!__msa_test_bz_v(is_less_than_beta))
        {
            PCKEV_B3_UB(q0_l, q0_r, q1_l, q1_r, q2_l, q2_r, q0, q1, q2);
            q0_org = __msa_bmnz_v(q0_org, q0, is_less_than_beta);
            q1_org = __msa_bmnz_v(q1_org, q1, is_less_than_beta);
            q2_org = __msa_bmnz_v(q2_org, q2, is_less_than_beta);

            ST_UB(q1_org, data + img_width);
            ST_UB(q2_org, data + 2 * img_width);
        }
        {
            v8u16 negate_is_less_than_beta_r, negate_is_less_than_beta_l;
            negate_is_less_than_beta_r =
                (v8u16) __msa_sldi_b((v16i8) negate_is_less_than_beta, zero, 8);
            if (!__msa_test_bz_v((v16u8) negate_is_less_than_beta_r))
            {
                AVC_LPF_P0_OR_Q0(q0_org_r, p1_org_r, q1_org_r, q0_r);
            }

            negate_is_less_than_beta_l =
                (v8u16) __msa_sldi_b(zero, (v16i8) negate_is_less_than_beta, 8);
            if (!__msa_test_bz_v((v16u8) negate_is_less_than_beta_l))
            {
                AVC_LPF_P0_OR_Q0(q0_org_l, p1_org_l, q1_org_l, q0_l);
            }
        }
        /* combine */
        if (!__msa_test_bz_v(negate_is_less_than_beta))
        {
            q0 = (v16u8) __msa_pckev_b((v16i8) q0_l, (v16i8) q0_r);
            q0_org = __msa_bmnz_v(q0_org, q0, negate_is_less_than_beta);
        }
        ST_UB(q0_org, data);
    }
}

void avc_loopfilter_luma_intra_edge_ver_msa(u8 *data, u8 alpha_in,
                                            u8 beta_in, u32 img_width)
{
    u8 *src;
    v16u8 alpha, beta, p0_asub_q0;
    v16u8 is_less_than_alpha, is_less_than;
    v16u8 is_less_than_beta, negate_is_less_than_beta;
    v16u8 p3_org, p2_org, p1_org, p0_org, q0_org, q1_org, q2_org, q3_org;
    v8i16 p1_org_r, p0_org_r, q0_org_r, q1_org_r;
    v8i16 p1_org_l, p0_org_l, q0_org_l, q1_org_l;
    v8i16 p2_r = { 0 };
    v8i16 p1_r = { 0 };
    v8i16 p0_r = { 0 };
    v8i16 q0_r = { 0 };
    v8i16 q1_r = { 0 };
    v8i16 q2_r = { 0 };
    v8i16 p2_l = { 0 };
    v8i16 p1_l = { 0 };
    v8i16 p0_l = { 0 };
    v8i16 q0_l = { 0 };
    v8i16 q1_l = { 0 };
    v8i16 q2_l = { 0 };
    v16i8 zero = { 0 };
    v16u8 tmp_flag;

    src = data - 4;
    {
        v16u8 row0, row1, row2, row3, row4, row5, row6, row7;
        v16u8 row8, row9, row10, row11, row12, row13, row14, row15;

        LD_UB8(src, img_width, row0, row1, row2, row3, row4, row5, row6, row7);
        LD_UB8(src + (8 * img_width), img_width,
               row8, row9, row10, row11, row12, row13, row14, row15);

        TRANSPOSE16x8_UB_UB(row0, row1, row2, row3,
                            row4, row5, row6, row7,
                            row8, row9, row10, row11,
                            row12, row13, row14, row15,
                            p3_org, p2_org, p1_org, p0_org,
                            q0_org, q1_org, q2_org, q3_org);
    }
    UNPCK_UB_SH(p1_org, p1_org_r, p1_org_l);
    UNPCK_UB_SH(p0_org, p0_org_r, p0_org_l);
    UNPCK_UB_SH(q0_org, q0_org_r, q0_org_l);
    UNPCK_UB_SH(q1_org, q1_org_r, q1_org_l);

    /*  if ( ((unsigned)ABS(p0-q0) < thresholds->alpha_in) &&
       ((unsigned)ABS(p1-p0) < thresholds->beta_in)  &&
       ((unsigned)ABS(q1-q0) < thresholds->beta_in) )   */
    {
        v16u8 p1_asub_p0, q1_asub_q0;

        p0_asub_q0 = __msa_asub_u_b(p0_org, q0_org);
        p1_asub_p0 = __msa_asub_u_b(p1_org, p0_org);
        q1_asub_q0 = __msa_asub_u_b(q1_org, q0_org);

        alpha = (v16u8) __msa_fill_b(alpha_in);
        beta = (v16u8) __msa_fill_b(beta_in);

        is_less_than_alpha = (p0_asub_q0 < alpha);
        is_less_than_beta = (p1_asub_p0 < beta);
        is_less_than = is_less_than_beta & is_less_than_alpha;
        is_less_than_beta = (q1_asub_q0 < beta);
        is_less_than = is_less_than_beta & is_less_than;
    }

    if (!__msa_test_bz_v(is_less_than))
    {
        tmp_flag = alpha >> 2;
        tmp_flag = tmp_flag + 2;
        tmp_flag = (p0_asub_q0 < tmp_flag);

        {
            v16u8 p2_asub_p0;

            p2_asub_p0 = __msa_asub_u_b(p2_org, p0_org);
            is_less_than_beta = (p2_asub_p0 < beta);
        }
        is_less_than_beta = tmp_flag & is_less_than_beta;
        negate_is_less_than_beta = __msa_xori_b(is_less_than_beta, 0xff);
        is_less_than_beta = is_less_than_beta & is_less_than;
        negate_is_less_than_beta = negate_is_less_than_beta & is_less_than;

        /* right */
        {
            v16u8 is_less_than_beta_r;

            is_less_than_beta_r =
                (v16u8) __msa_sldi_b((v16i8) is_less_than_beta, zero, 8);
            if (!__msa_test_bz_v(is_less_than_beta_r))
            {
                v8i16 p3_org_r;

                ILVR_B2_SH(zero, p3_org, zero, p2_org, p3_org_r, p2_r);
                AVC_LPF_P0P1P2_OR_Q0Q1Q2(p3_org_r, p0_org_r, q0_org_r, p1_org_r,
                                         p2_r, q1_org_r, p0_r, p1_r, p2_r);
            }
        }
        /* left */
        {
            v16u8 is_less_than_beta_l;

            is_less_than_beta_l =
                (v16u8) __msa_sldi_b(zero, (v16i8) is_less_than_beta, 8);
            if (!__msa_test_bz_v(is_less_than_beta_l))
            {
                v8i16 p3_org_l;

                ILVL_B2_SH(zero, p3_org, zero, p2_org, p3_org_l, p2_l);
                AVC_LPF_P0P1P2_OR_Q0Q1Q2(p3_org_l, p0_org_l, q0_org_l, p1_org_l,
                                         p2_l, q1_org_l, p0_l, p1_l, p2_l);
            }
        }
        /* combine and store */
        if (!__msa_test_bz_v(is_less_than_beta))
        {
            v16u8 p0, p2, p1;

            PCKEV_B3_UB(p0_l, p0_r, p1_l, p1_r, p2_l, p2_r, p0, p1, p2);
            p0_org = __msa_bmnz_v(p0_org, p0, is_less_than_beta);
            p1_org = __msa_bmnz_v(p1_org, p1, is_less_than_beta);
            p2_org = __msa_bmnz_v(p2_org, p2, is_less_than_beta);
        }
        /* right */
        {
            v16u8 negate_is_less_than_beta_r;

            negate_is_less_than_beta_r =
                (v16u8) __msa_sldi_b((v16i8) negate_is_less_than_beta, zero, 8);

            if (!__msa_test_bz_v(negate_is_less_than_beta_r))
            {
                AVC_LPF_P0_OR_Q0(p0_org_r, q1_org_r, p1_org_r, p0_r);
            }
        }
        /* left */
        {
            v16u8 negate_is_less_than_beta_l;

            negate_is_less_than_beta_l =
                (v16u8) __msa_sldi_b(zero, (v16i8) negate_is_less_than_beta, 8);
            if (!__msa_test_bz_v(negate_is_less_than_beta_l))
            {
                AVC_LPF_P0_OR_Q0(p0_org_l, q1_org_l, p1_org_l, p0_l);
            }
        }

        if (!__msa_test_bz_v(negate_is_less_than_beta))
        {
            v16u8 p0;

            p0 = (v16u8) __msa_pckev_b((v16i8) p0_l, (v16i8) p0_r);
            p0_org = __msa_bmnz_v(p0_org, p0, negate_is_less_than_beta);
        }

        {
            v16u8 q2_asub_q0;

            q2_asub_q0 = __msa_asub_u_b(q2_org, q0_org);
            is_less_than_beta = (q2_asub_q0 < beta);
        }

        is_less_than_beta = is_less_than_beta & tmp_flag;
        negate_is_less_than_beta = __msa_xori_b(is_less_than_beta, 0xff);

        is_less_than_beta = is_less_than_beta & is_less_than;
        negate_is_less_than_beta = negate_is_less_than_beta & is_less_than;

        /* right */
        {
            v16u8 is_less_than_beta_r;

            is_less_than_beta_r =
                (v16u8) __msa_sldi_b((v16i8) is_less_than_beta, zero, 8);
            if (!__msa_test_bz_v(is_less_than_beta_r))
            {
                v8i16 q3_org_r;

                ILVR_B2_SH(zero, q3_org, zero, q2_org, q3_org_r, q2_r);
                AVC_LPF_P0P1P2_OR_Q0Q1Q2(q3_org_r, q0_org_r, p0_org_r, q1_org_r,
                                         q2_r, p1_org_r, q0_r, q1_r, q2_r);
            }
        }
        /* left */
        {
            v16u8 is_less_than_beta_l;

            is_less_than_beta_l =
                (v16u8) __msa_sldi_b(zero, (v16i8) is_less_than_beta, 8);
            if (!__msa_test_bz_v(is_less_than_beta_l))
            {
                v8i16 q3_org_l;

                ILVL_B2_SH(zero, q3_org, zero, q2_org, q3_org_l, q2_l);
                AVC_LPF_P0P1P2_OR_Q0Q1Q2(q3_org_l, q0_org_l, p0_org_l, q1_org_l,
                                         q2_l, p1_org_l, q0_l, q1_l, q2_l);
            }
        }
        /* combine and store */
        if (!__msa_test_bz_v(is_less_than_beta))
        {
            v16u8 q0, q1, q2;

            PCKEV_B3_UB(q0_l, q0_r, q1_l, q1_r, q2_l, q2_r, q0, q1, q2);
            q0_org = __msa_bmnz_v(q0_org, q0, is_less_than_beta);
            q1_org = __msa_bmnz_v(q1_org, q1, is_less_than_beta);
            q2_org = __msa_bmnz_v(q2_org, q2, is_less_than_beta);
        }

        /* right */
        {
            v16u8 negate_is_less_than_beta_r;

            negate_is_less_than_beta_r =
                (v16u8) __msa_sldi_b((v16i8) negate_is_less_than_beta, zero, 8);
            if (!__msa_test_bz_v(negate_is_less_than_beta_r))
            {
                AVC_LPF_P0_OR_Q0(q0_org_r, p1_org_r, q1_org_r, q0_r);
            }
        }
        /* left */
        {
            v16u8 negate_is_less_than_beta_l;

            negate_is_less_than_beta_l =
                (v16u8) __msa_sldi_b(zero, (v16i8) negate_is_less_than_beta, 8);
            if (!__msa_test_bz_v(negate_is_less_than_beta_l))
            {
                AVC_LPF_P0_OR_Q0(q0_org_l, p1_org_l, q1_org_l, q0_l);
            }
        }
        if (!__msa_test_bz_v(negate_is_less_than_beta))
        {
            v16u8 q0;

            q0 = (v16u8) __msa_pckev_b((v16i8) q0_l, (v16i8) q0_r);
            q0_org = __msa_bmnz_v(q0_org, q0, negate_is_less_than_beta);
        }
    }
    {
        v8i16 tp0, tp1, tp2, tp3, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;

        ILVRL_B2_SH(p1_org, p2_org, tp0, tp2);
        ILVRL_B2_SH(q0_org, p0_org, tp1, tp3);
        ILVRL_B2_SH(q2_org, q1_org, tmp2, tmp5);

        ILVRL_H2_SH(tp1, tp0, tmp3, tmp4);
        ILVRL_H2_SH(tp3, tp2, tmp6, tmp7);

        src = data - 3;
        ST4x4_UB(tmp3, tmp3, 0, 1, 2, 3, src, img_width);
        ST2x4_UB(tmp2, 0, src + 4, img_width);
        src += 4 * img_width;
        ST4x4_UB(tmp4, tmp4, 0, 1, 2, 3, src, img_width);
        ST2x4_UB(tmp2, 4, src + 4, img_width);
        src += 4 * img_width;

        ST4x4_UB(tmp6, tmp6, 0, 1, 2, 3, src, img_width);
        ST2x4_UB(tmp5, 0, src + 4, img_width);
        src += 4 * img_width;
        ST4x4_UB(tmp7, tmp7, 0, 1, 2, 3, src, img_width);
        ST2x4_UB(tmp5, 4, src + 4, img_width);
    }
}

/*******************************************************************************
                              Intra Chroma
*******************************************************************************/
void avc_loopfilter_cbcr_intra_edge_hor_msa(u8 *data_cb, u8 *data_cr,
                                            u8 alpha_in, u8 beta_in,
                                            u32 img_width)
{
    v16u8 alpha, beta, is_less_than;
    v16u8 p0, q0, p1_org, p0_org, q0_org, q1_or_p1_org;
    v16u8 p0_cb_org, q0_cb_org, p0_cr_org, q0_cr_org;
    v8i16 p0_r = { 0 };
    v8i16 q0_r = { 0 };
    v8i16 p0_l = { 0 };
    v8i16 q0_l = { 0 };

    alpha = (v16u8) __msa_fill_b(alpha_in);
    beta = (v16u8) __msa_fill_b(beta_in);

    {
        v16u8 p1_cb_org, q1_cb_org, p1_cr_org, q1_cr_org;

        LD_UB4(data_cb - (img_width << 1), img_width,
               p1_cb_org, p0_cb_org, q0_cb_org, q1_cb_org);
        LD_UB4(data_cr - (img_width << 1), img_width,
               p1_cr_org, p0_cr_org, q0_cr_org, q1_cr_org);
        ILVR_D4_UB(p1_cr_org, p1_cb_org, p0_cr_org, p0_cb_org,
                   q0_cr_org, q0_cb_org, q1_cr_org, q1_cb_org,
                   p1_org, p0_org, q0_org, q1_or_p1_org);
    }
    {
        v16u8 p0_asub_q0, p1_asub_p0, q1_asub_q0;
        v16u8 is_less_than_alpha, is_less_than_beta;

        p0_asub_q0 = __msa_asub_u_b(p0_org, q0_org);
        p1_asub_p0 = __msa_asub_u_b(p1_org, p0_org);
        q1_asub_q0 = __msa_asub_u_b(q1_or_p1_org, q0_org);

        is_less_than_alpha = (p0_asub_q0 < alpha);
        is_less_than_beta = (p1_asub_p0 < beta);
        is_less_than = is_less_than_beta & is_less_than_alpha;
        is_less_than_beta = (q1_asub_q0 < beta);
        is_less_than = is_less_than_beta & is_less_than;
    }
    if (!__msa_test_bz_v(is_less_than))
    {
        v16i8 zero = { 0 };
        v16u8 is_less_than_r, is_less_than_l;

        is_less_than_r = (v16u8) __msa_sldi_b((v16i8) is_less_than, zero, 8);
        if (!__msa_test_bz_v(is_less_than_r))
        {
            v8i16 p1_org_r, p0_org_r, q0_org_r, q1_org_r;

            ILVR_B4_SH(zero, p1_org, zero, p0_org, zero, q0_org,
                       zero, q1_or_p1_org, p1_org_r, p0_org_r, q0_org_r,
                       q1_org_r);
            AVC_LPF_P0_OR_Q0(p0_org_r, q1_org_r, p1_org_r, p0_r);
            AVC_LPF_P0_OR_Q0(q0_org_r, p1_org_r, q1_org_r, q0_r);
        }

        is_less_than_l = (v16u8) __msa_sldi_b(zero, (v16i8) is_less_than, 8);
        if (!__msa_test_bz_v(is_less_than_l))
        {
            v8i16 p1_org_l, p0_org_l, q0_org_l, q1_org_l;

            ILVL_B4_SH(zero, p1_org, zero, p0_org, zero, q0_org,
                       zero, q1_or_p1_org, p1_org_l, p0_org_l, q0_org_l,
                       q1_org_l);
            AVC_LPF_P0_OR_Q0(p0_org_l, q1_org_l, p1_org_l, p0_l);
            AVC_LPF_P0_OR_Q0(q0_org_l, p1_org_l, q1_org_l, q0_l);
        }

        PCKEV_B2_UB(p0_l, p0_r, q0_l, q0_r, p0, q0);
        p0_org = __msa_bmnz_v(p0_org, p0, is_less_than);
        q0_org = __msa_bmnz_v(q0_org, q0, is_less_than);

        {
            v2u64 cb_coeff = { 0, 3 };
            p0_cb_org = (v16u8) __msa_vshf_d((v2i64) cb_coeff,
                                             (v2i64) p0_cb_org, (v2i64) p0_org);
            q0_cb_org = (v16u8) __msa_vshf_d((v2i64) cb_coeff,
                                             (v2i64) q0_cb_org, (v2i64) q0_org);
        }

        PCKOD_D2_UB(p0_cr_org, p0_org, q0_cr_org, q0_org, p0_cr_org, q0_cr_org);

        ST_UB(q0_cb_org, data_cb);
        ST_UB(p0_cb_org, data_cb - img_width);
        ST_UB(q0_cr_org, data_cr);
        ST_UB(p0_cr_org, data_cr - img_width);
    }
}

void avc_loopfilter_cbcr_intra_edge_ver_msa(u8 *data_cb, u8 *data_cr,
                                            u8 alpha_in, u8 beta_in,
                                            u32 img_width)
{
    u8 *src_cb, *src_cr;
    v16u8 is_less_than;
    v16u8 p0, q0, p1_org, p0_org, q0_org, q1_org;
    v8i16 p0_r = { 0 };
    v8i16 q0_r = { 0 };
    v8i16 p0_l = { 0 };
    v8i16 q0_l = { 0 };

    src_cb = data_cb - 2;
    src_cr = data_cr - 2;

    {
        v16u8 row0, row1, row2, row3, row4, row5, row6, row7;
        v16u8 row8, row9, row10, row11, row12, row13, row14, row15;

        LD_UB8(src_cb, img_width,
               row0, row1, row2, row3, row4, row5, row6, row7);
        LD_UB8(src_cr, img_width,
               row8, row9, row10, row11, row12, row13, row14, row15);
        TRANSPOSE16x4_UB_UB(row0, row1, row2, row3,
                            row4, row5, row6, row7,
                            row8, row9, row10, row11,
                            row12, row13, row14, row15,
                            p1_org, p0_org, q0_org, q1_org);
    }
    {
        v16u8 p0_asub_q0, p1_asub_p0, q1_asub_q0;
        v16u8 is_less_than_beta, is_less_than_alpha, alpha, beta;

        p0_asub_q0 = __msa_asub_u_b(p0_org, q0_org);
        p1_asub_p0 = __msa_asub_u_b(p1_org, p0_org);
        q1_asub_q0 = __msa_asub_u_b(q1_org, q0_org);
        alpha = (v16u8) __msa_fill_b(alpha_in);
        beta = (v16u8) __msa_fill_b(beta_in);

        is_less_than_alpha = (p0_asub_q0 < alpha);
        is_less_than_beta = (p1_asub_p0 < beta);
        is_less_than = is_less_than_beta & is_less_than_alpha;
        is_less_than_beta = (q1_asub_q0 < beta);
        is_less_than = is_less_than_beta & is_less_than;
    }

    if (!__msa_test_bz_v(is_less_than))
    {
        v16u8 is_less_than_r, is_less_than_l;
        v16i8 zero = { 0 };

        is_less_than_r = (v16u8) __msa_sldi_b((v16i8) is_less_than, zero, 8);
        if (!__msa_test_bz_v(is_less_than_r))
        {
            v8i16 p1_org_r, p0_org_r, q0_org_r, q1_org_r;

            ILVR_B4_SH(zero, p1_org, zero, p0_org, zero, q0_org,
                       zero, q1_org, p1_org_r, p0_org_r, q0_org_r,
                       q1_org_r);
            AVC_LPF_P0_OR_Q0(p0_org_r, q1_org_r, p1_org_r, p0_r);
            AVC_LPF_P0_OR_Q0(q0_org_r, p1_org_r, q1_org_r, q0_r);
        }

        is_less_than_l = (v16u8) __msa_sldi_b(zero, (v16i8) is_less_than, 8);
        if (!__msa_test_bz_v(is_less_than_l))
        {
            v8i16 p1_org_l, p0_org_l, q0_org_l, q1_org_l;

            ILVL_B4_SH(zero, p1_org, zero, p0_org, zero, q0_org,
                       zero, q1_org, p1_org_l, p0_org_l, q0_org_l,
                       q1_org_l);
            AVC_LPF_P0_OR_Q0(p0_org_l, q1_org_l, p1_org_l, p0_l);
            AVC_LPF_P0_OR_Q0(q0_org_l, p1_org_l, q1_org_l, q0_l);
        }

        /* combine left and right part */
        PCKEV_B2_UB(p0_l, p0_r, q0_l, q0_r, p0, q0);

        p0_org = __msa_bmnz_v(p0_org, p0, is_less_than);
        q0_org = __msa_bmnz_v(q0_org, q0, is_less_than);

        {
            v8i16 tmp0, tmp1;

            ILVRL_B2_SH(q0_org, p0_org, tmp1, tmp0);

            src_cb = data_cb - 1;
            src_cr = data_cr - 1;

            ST2x4_UB(tmp1, 0, src_cb, img_width);
            src_cb += 4 * img_width;

            ST2x4_UB(tmp1, 4, src_cb, img_width);
            src_cb += 4 * img_width;

            ST2x4_UB(tmp0, 0, src_cr, img_width);
            src_cr += 4 * img_width;

            ST2x4_UB(tmp0, 4, src_cr, img_width);
            src_cr += 4 * img_width;
        }
    }
}

/*******************************************************************************
                              Inter Luma
*******************************************************************************/
void avc_loopfilter_luma_inter_edge_ver_msa(u8 *data,
                                            u8 bs0, u8 bs1,
                                            u8 bs2, u8 bs3,
                                            u8 tc0, u8 tc1,
                                            u8 tc2, u8 tc3,
                                            u8 alpha_in, u8 beta_in,
                                            u32 img_width)
{
    u8 *src;
    v16u8 beta, tmp_vec, bs = { 0 };
    v16u8 tc = { 0 };
    v16u8 is_less_than, is_less_than_beta;
    v16u8 p1, p0, q0, q1;
    v8i16 p0_r, q0_r, p1_r = { 0 };
    v8i16 q1_r = { 0 };
    v8i16 p0_l, q0_l, p1_l = { 0 };
    v8i16 q1_l = { 0 };
    v16u8 p3_org, p2_org, p1_org, p0_org, q0_org, q1_org, q2_org, q3_org;
    v8i16 p2_org_r, p1_org_r, p0_org_r, q0_org_r, q1_org_r, q2_org_r;
    v8i16 p2_org_l, p1_org_l, p0_org_l, q0_org_l, q1_org_l, q2_org_l;
    v8i16 tc_r, tc_l;
    v16i8 zero = { 0 };
    v16u8 is_bs_greater_than0;

    tmp_vec = (v16u8) __msa_fill_b(bs0);
    bs = (v16u8) __msa_insve_w((v4i32) bs, 0, (v4i32) tmp_vec);
    tmp_vec = (v16u8) __msa_fill_b(bs1);
    bs = (v16u8) __msa_insve_w((v4i32) bs, 1, (v4i32) tmp_vec);
    tmp_vec = (v16u8) __msa_fill_b(bs2);
    bs = (v16u8) __msa_insve_w((v4i32) bs, 2, (v4i32) tmp_vec);
    tmp_vec = (v16u8) __msa_fill_b(bs3);
    bs = (v16u8) __msa_insve_w((v4i32) bs, 3, (v4i32) tmp_vec);

    if (!__msa_test_bz_v(bs))
    {
        tmp_vec = (v16u8) __msa_fill_b(tc0);
        tc = (v16u8) __msa_insve_w((v4i32) tc, 0, (v4i32) tmp_vec);
        tmp_vec = (v16u8) __msa_fill_b(tc1);
        tc = (v16u8) __msa_insve_w((v4i32) tc, 1, (v4i32) tmp_vec);
        tmp_vec = (v16u8) __msa_fill_b(tc2);
        tc = (v16u8) __msa_insve_w((v4i32) tc, 2, (v4i32) tmp_vec);
        tmp_vec = (v16u8) __msa_fill_b(tc3);
        tc = (v16u8) __msa_insve_w((v4i32) tc, 3, (v4i32) tmp_vec);

        is_bs_greater_than0 = (zero < bs);

        {
            v16u8 row0, row1, row2, row3, row4, row5, row6, row7;
            v16u8 row8, row9, row10, row11, row12, row13, row14, row15;

            src = data;
            src -= 4;

            LD_UB8(src, img_width,
                   row0, row1, row2, row3, row4, row5, row6, row7);
            src += (8 * img_width);
            LD_UB8(src, img_width,
                   row8, row9, row10, row11, row12, row13, row14, row15);

            TRANSPOSE16x8_UB_UB(row0, row1, row2, row3, row4, row5, row6, row7,
                                row8, row9, row10, row11,
                                row12, row13, row14, row15,
                                p3_org, p2_org, p1_org, p0_org,
                                q0_org, q1_org, q2_org, q3_org);
        }
        {
            v16u8 p0_asub_q0, p1_asub_p0, q1_asub_q0, alpha;
            v16u8 is_less_than_alpha;

            p0_asub_q0 = __msa_asub_u_b(p0_org, q0_org);
            p1_asub_p0 = __msa_asub_u_b(p1_org, p0_org);
            q1_asub_q0 = __msa_asub_u_b(q1_org, q0_org);

            alpha = (v16u8) __msa_fill_b(alpha_in);
            beta = (v16u8) __msa_fill_b(beta_in);

            is_less_than_alpha = (p0_asub_q0 < alpha);
            is_less_than_beta = (p1_asub_p0 < beta);
            is_less_than = is_less_than_beta & is_less_than_alpha;
            is_less_than_beta = (q1_asub_q0 < beta);
            is_less_than = is_less_than_beta & is_less_than;
            is_less_than = is_less_than & is_bs_greater_than0;
        }
        if (!__msa_test_bz_v(is_less_than))
        {
            v16i8 negate_tc, sign_negate_tc;
            v8i16 negate_tc_r, i16_negatetc_l;

            negate_tc = zero - (v16i8) tc;
            sign_negate_tc = __msa_clti_s_b(negate_tc, 0);

            ILVRL_B2_SH(sign_negate_tc, negate_tc, negate_tc_r,
                        i16_negatetc_l);

            UNPCK_UB_SH(tc, tc_r, tc_l);
            UNPCK_UB_SH(p1_org, p1_org_r, p1_org_l);
            UNPCK_UB_SH(p0_org, p0_org_r, p0_org_l);
            UNPCK_UB_SH(q0_org, q0_org_r, q0_org_l);

            {
                v16u8 p2_asub_p0;
                v16u8 is_less_than_beta_r, is_less_than_beta_l;

                p2_asub_p0 = __msa_asub_u_b(p2_org, p0_org);
                is_less_than_beta = (p2_asub_p0 < beta);
                is_less_than_beta = is_less_than_beta & is_less_than;

                is_less_than_beta_r =
                    (v16u8) __msa_sldi_b((v16i8) is_less_than_beta, zero, 8);
                if (!__msa_test_bz_v(is_less_than_beta_r))
                {
                    p2_org_r = (v8i16) __msa_ilvr_b(zero, (v16i8) p2_org);

                    AVC_LPF_P1_OR_Q1(p0_org_r, q0_org_r, p1_org_r, p2_org_r,
                                     negate_tc_r, tc_r, p1_r);
                }

                is_less_than_beta_l =
                    (v16u8) __msa_sldi_b(zero, (v16i8) is_less_than_beta, 8);
                if (!__msa_test_bz_v(is_less_than_beta_l))
                {
                    p2_org_l = (v8i16) __msa_ilvl_b(zero, (v16i8) p2_org);

                    AVC_LPF_P1_OR_Q1(p0_org_l, q0_org_l, p1_org_l, p2_org_l,
                                     i16_negatetc_l, tc_l, p1_l);
                }
            }

            if (!__msa_test_bz_v(is_less_than_beta))
            {
                p1 = (v16u8) __msa_pckev_b((v16i8) p1_l, (v16i8) p1_r);
                p1_org = __msa_bmnz_v(p1_org, p1, is_less_than_beta);

                is_less_than_beta = __msa_andi_b(is_less_than_beta, 1);
                tc = tc + is_less_than_beta;
            }

            {
                v16u8 u8_q2asub_q0;
                v16u8 is_less_than_beta_l, is_less_than_beta_r;

                u8_q2asub_q0 = __msa_asub_u_b(q2_org, q0_org);
                is_less_than_beta = (u8_q2asub_q0 < beta);
                is_less_than_beta = is_less_than_beta & is_less_than;

                q1_org_r = (v8i16) __msa_ilvr_b(zero, (v16i8) q1_org);

                is_less_than_beta_r =
                    (v16u8) __msa_sldi_b((v16i8) is_less_than_beta, zero, 8);
                if (!__msa_test_bz_v(is_less_than_beta_r))
                {
                    q2_org_r = (v8i16) __msa_ilvr_b(zero, (v16i8) q2_org);
                    AVC_LPF_P1_OR_Q1(p0_org_r, q0_org_r, q1_org_r, q2_org_r,
                                     negate_tc_r, tc_r, q1_r);
                }

                q1_org_l = (v8i16) __msa_ilvl_b(zero, (v16i8) q1_org);

                is_less_than_beta_l =
                    (v16u8) __msa_sldi_b(zero, (v16i8) is_less_than_beta, 8);
                if (!__msa_test_bz_v(is_less_than_beta_l))
                {
                    q2_org_l = (v8i16) __msa_ilvl_b(zero, (v16i8) q2_org);
                    AVC_LPF_P1_OR_Q1(p0_org_l, q0_org_l, q1_org_l, q2_org_l,
                                     i16_negatetc_l, tc_l, q1_l);
                }
            }

            if (!__msa_test_bz_v(is_less_than_beta))
            {
                q1 = (v16u8) __msa_pckev_b((v16i8) q1_l, (v16i8) q1_r);
                q1_org = __msa_bmnz_v(q1_org, q1, is_less_than_beta);

                is_less_than_beta = __msa_andi_b(is_less_than_beta, 1);
                tc = tc + is_less_than_beta;
            }

            {
                v8i16 threshold_r, negate_thresh_r;
                v8i16 threshold_l, negate_thresh_l;
                v16i8 negate_thresh, sign_negate_thresh;

                negate_thresh = zero - (v16i8) tc;
                sign_negate_thresh = __msa_clti_s_b(negate_thresh, 0);

                ILVR_B2_SH(zero, tc, sign_negate_thresh, negate_thresh,
                           threshold_r, negate_thresh_r);

                AVC_LPF_P0Q0(q0_org_r, p0_org_r, p1_org_r, q1_org_r,
                             negate_thresh_r, threshold_r, p0_r, q0_r);

                threshold_l = (v8i16) __msa_ilvl_b(zero, (v16i8) tc);
                negate_thresh_l = (v8i16) __msa_ilvl_b(sign_negate_thresh,
                                                       negate_thresh);

                AVC_LPF_P0Q0(q0_org_l, p0_org_l, p1_org_l, q1_org_l,
                             negate_thresh_l, threshold_l, p0_l, q0_l);
            }

            PCKEV_B2_UB(p0_l, p0_r, q0_l, q0_r, p0, q0);

            p0_org = __msa_bmnz_v(p0_org, p0, is_less_than);
            q0_org = __msa_bmnz_v(q0_org, q0, is_less_than);
        }
        {
            v16i8 tp0, tp1, tp2, tp3;
            v8i16 tmp2, tmp5;
            v4i32 tmp3, tmp4, tmp6, tmp7;
            u32 out0, out2;
            u16 out1, out3;

            src = data - 3;

            ILVRL_B2_SB(p1_org, p2_org, tp0, tp2);
            ILVRL_B2_SB(q0_org, p0_org, tp1, tp3);
            ILVRL_B2_SH(q2_org, q1_org, tmp2, tmp5);

            ILVRL_H2_SW(tp1, tp0, tmp3, tmp4);
            ILVRL_H2_SW(tp3, tp2, tmp6, tmp7);

            out0 = __msa_copy_u_w(tmp3, 0);
            out1 = __msa_copy_u_h(tmp2, 0);
            out2 = __msa_copy_u_w(tmp3, 1);
            out3 = __msa_copy_u_h(tmp2, 1);

            SW(out0, src);
            SH(out1, (src + 4));
            src += img_width;
            SW(out2, src);
            SH(out3, (src + 4));

            out0 = __msa_copy_u_w(tmp3, 2);
            out1 = __msa_copy_u_h(tmp2, 2);
            out2 = __msa_copy_u_w(tmp3, 3);
            out3 = __msa_copy_u_h(tmp2, 3);

            src += img_width;
            SW(out0, src);
            SH(out1, (src + 4));
            src += img_width;
            SW(out2, src);
            SH(out3, (src + 4));

            out0 = __msa_copy_u_w(tmp4, 0);
            out1 = __msa_copy_u_h(tmp2, 4);
            out2 = __msa_copy_u_w(tmp4, 1);
            out3 = __msa_copy_u_h(tmp2, 5);

            src += img_width;
            SW(out0, src);
            SH(out1, (src + 4));
            src += img_width;
            SW(out2, src);
            SH(out3, (src + 4));

            out0 = __msa_copy_u_w(tmp4, 2);
            out1 = __msa_copy_u_h(tmp2, 6);
            out2 = __msa_copy_u_w(tmp4, 3);
            out3 = __msa_copy_u_h(tmp2, 7);

            src += img_width;
            SW(out0, src);
            SH(out1, (src + 4));
            src += img_width;
            SW(out2, src);
            SH(out3, (src + 4));

            out0 = __msa_copy_u_w(tmp6, 0);
            out1 = __msa_copy_u_h(tmp5, 0);
            out2 = __msa_copy_u_w(tmp6, 1);
            out3 = __msa_copy_u_h(tmp5, 1);

            src += img_width;
            SW(out0, src);
            SH(out1, (src + 4));
            src += img_width;
            SW(out2, src);
            SH(out3, (src + 4));

            out0 = __msa_copy_u_w(tmp6, 2);
            out1 = __msa_copy_u_h(tmp5, 2);
            out2 = __msa_copy_u_w(tmp6, 3);
            out3 = __msa_copy_u_h(tmp5, 3);

            src += img_width;
            SW(out0, src);
            SH(out1, (src + 4));
            src += img_width;
            SW(out2, src);
            SH(out3, (src + 4));

            out0 = __msa_copy_u_w(tmp7, 0);
            out1 = __msa_copy_u_h(tmp5, 4);
            out2 = __msa_copy_u_w(tmp7, 1);
            out3 = __msa_copy_u_h(tmp5, 5);

            src += img_width;
            SW(out0, src);
            SH(out1, (src + 4));
            src += img_width;
            SW(out2, src);
            SH(out3, (src + 4));

            out0 = __msa_copy_u_w(tmp7, 2);
            out1 = __msa_copy_u_h(tmp5, 6);
            out2 = __msa_copy_u_w(tmp7, 3);
            out3 = __msa_copy_u_h(tmp5, 7);

            src += img_width;
            SW(out0, src);
            SH(out1, (src + 4));
            src += img_width;
            SW(out2, src);
            SH(out3, (src + 4));
        }
    }
}

void avc_loopfilter_luma_inter_edge_hor_msa(u8 *data,
                                            u8 bs0, u8 bs1,
                                            u8 bs2, u8 bs3,
                                            u8 tc0, u8 tc1,
                                            u8 tc2, u8 tc3,
                                            u8 alpha_in, u8 beta_in,
                                            u32 image_width)
{
    v16u8 p2_asub_p0, u8_q2asub_q0;
    v16u8 alpha, beta, is_less_than, is_less_than_beta;
    v16u8 p1, p0, q0, q1;
    v8i16 p1_r = { 0 };
    v8i16 p0_r, q0_r, q1_r = { 0 };
    v8i16 p1_l = { 0 };
    v8i16 p0_l, q0_l, q1_l = { 0 };
    v16u8 p2_org, p1_org, p0_org, q0_org, q1_org, q2_org;
    v8i16 p2_org_r, p1_org_r, p0_org_r, q0_org_r, q1_org_r, q2_org_r;
    v8i16 p2_org_l, p1_org_l, p0_org_l, q0_org_l, q1_org_l, q2_org_l;
    v16i8 zero = { 0 };
    v16u8 tmp_vec;
    v16u8 bs = { 0 };
    v16i8 tc = { 0 };

    tmp_vec = (v16u8) __msa_fill_b(bs0);
    bs = (v16u8) __msa_insve_w((v4i32) bs, 0, (v4i32) tmp_vec);
    tmp_vec = (v16u8) __msa_fill_b(bs1);
    bs = (v16u8) __msa_insve_w((v4i32) bs, 1, (v4i32) tmp_vec);
    tmp_vec = (v16u8) __msa_fill_b(bs2);
    bs = (v16u8) __msa_insve_w((v4i32) bs, 2, (v4i32) tmp_vec);
    tmp_vec = (v16u8) __msa_fill_b(bs3);
    bs = (v16u8) __msa_insve_w((v4i32) bs, 3, (v4i32) tmp_vec);

    if (!__msa_test_bz_v(bs))
    {
        tmp_vec = (v16u8) __msa_fill_b(tc0);
        tc = (v16i8) __msa_insve_w((v4i32) tc, 0, (v4i32) tmp_vec);
        tmp_vec = (v16u8) __msa_fill_b(tc1);
        tc = (v16i8) __msa_insve_w((v4i32) tc, 1, (v4i32) tmp_vec);
        tmp_vec = (v16u8) __msa_fill_b(tc2);
        tc = (v16i8) __msa_insve_w((v4i32) tc, 2, (v4i32) tmp_vec);
        tmp_vec = (v16u8) __msa_fill_b(tc3);
        tc = (v16i8) __msa_insve_w((v4i32) tc, 3, (v4i32) tmp_vec);

        alpha = (v16u8) __msa_fill_b(alpha_in);
        beta = (v16u8) __msa_fill_b(beta_in);

        LD_UB5(data - (3 * image_width), image_width,
               p2_org, p1_org, p0_org, q0_org, q1_org);

        {
            v16u8 p0_asub_q0, p1_asub_p0, q1_asub_q0;
            v16u8 is_less_than_alpha, is_bs_greater_than0;

            is_bs_greater_than0 = ((v16u8) zero < bs);
            p0_asub_q0 = __msa_asub_u_b(p0_org, q0_org);
            p1_asub_p0 = __msa_asub_u_b(p1_org, p0_org);
            q1_asub_q0 = __msa_asub_u_b(q1_org, q0_org);

            is_less_than_alpha = (p0_asub_q0 < alpha);
            is_less_than_beta = (p1_asub_p0 < beta);
            is_less_than = is_less_than_beta & is_less_than_alpha;
            is_less_than_beta = (q1_asub_q0 < beta);
            is_less_than = is_less_than_beta & is_less_than;
            is_less_than = is_less_than & is_bs_greater_than0;
        }

        if (!__msa_test_bz_v(is_less_than))
        {
            v16i8 sign_negate_tc, negate_tc;
            v8i16 negate_tc_r, i16_negatetc_l, tc_l, tc_r;

            q2_org = LD_UB(data + (2 * image_width));
            negate_tc = zero - tc;
            sign_negate_tc = __msa_clti_s_b(negate_tc, 0);

            ILVRL_B2_SH(sign_negate_tc, negate_tc, negate_tc_r, i16_negatetc_l);

            UNPCK_UB_SH(tc, tc_r, tc_l);
            UNPCK_UB_SH(p1_org, p1_org_r, p1_org_l);
            UNPCK_UB_SH(p0_org, p0_org_r, p0_org_l);
            UNPCK_UB_SH(q0_org, q0_org_r, q0_org_l);

            p2_asub_p0 = __msa_asub_u_b(p2_org, p0_org);
            is_less_than_beta = (p2_asub_p0 < beta);
            is_less_than_beta = is_less_than_beta & is_less_than;
            {
                v8u16 is_less_than_beta_r, is_less_than_beta_l;

                is_less_than_beta_r =
                    (v8u16) __msa_sldi_b((v16i8) is_less_than_beta, zero, 8);
                if (!__msa_test_bz_v((v16u8) is_less_than_beta_r))
                {
                    p2_org_r = (v8i16) __msa_ilvr_b(zero, (v16i8) p2_org);

                    AVC_LPF_P1_OR_Q1(p0_org_r, q0_org_r, p1_org_r, p2_org_r,
                                     negate_tc_r, tc_r, p1_r);
                }

                is_less_than_beta_l =
                    (v8u16) __msa_sldi_b(zero, (v16i8) is_less_than_beta, 8);
                if (!__msa_test_bz_v((v16u8) is_less_than_beta_l))
                {
                    p2_org_l = (v8i16) __msa_ilvl_b(zero, (v16i8) p2_org);

                    AVC_LPF_P1_OR_Q1(p0_org_l, q0_org_l, p1_org_l, p2_org_l,
                                     i16_negatetc_l, tc_l, p1_l);
                }
            }
            if (!__msa_test_bz_v(is_less_than_beta))
            {
                p1 = (v16u8) __msa_pckev_b((v16i8) p1_l, (v16i8) p1_r);
                p1_org = __msa_bmnz_v(p1_org, p1, is_less_than_beta);
                ST_UB(p1_org, data - (2 * image_width));

                is_less_than_beta = __msa_andi_b(is_less_than_beta, 1);
                tc = tc + (v16i8) is_less_than_beta;
            }

            u8_q2asub_q0 = __msa_asub_u_b(q2_org, q0_org);
            is_less_than_beta = (u8_q2asub_q0 < beta);
            is_less_than_beta = is_less_than_beta & is_less_than;

            {
                v8u16 is_less_than_beta_r, is_less_than_beta_l;
                is_less_than_beta_r =
                    (v8u16) __msa_sldi_b((v16i8) is_less_than_beta, zero, 8);

                q1_org_r = (v8i16) __msa_ilvr_b(zero, (v16i8) q1_org);
                if (!__msa_test_bz_v((v16u8) is_less_than_beta_r))
                {
                    q2_org_r = (v8i16) __msa_ilvr_b(zero, (v16i8) q2_org);

                    AVC_LPF_P1_OR_Q1(p0_org_r, q0_org_r, q1_org_r, q2_org_r,
                                     negate_tc_r, tc_r, q1_r);
                }
                is_less_than_beta_l =
                    (v8u16) __msa_sldi_b(zero, (v16i8) is_less_than_beta, 8);

                q1_org_l = (v8i16) __msa_ilvl_b(zero, (v16i8) q1_org);
                if (!__msa_test_bz_v((v16u8) is_less_than_beta_l))
                {
                    q2_org_l = (v8i16) __msa_ilvl_b(zero, (v16i8) q2_org);

                    AVC_LPF_P1_OR_Q1(p0_org_l, q0_org_l, q1_org_l, q2_org_l,
                                     i16_negatetc_l, tc_l, q1_l);
                }
            }
            if (!__msa_test_bz_v(is_less_than_beta))
            {
                q1 = (v16u8) __msa_pckev_b((v16i8) q1_l, (v16i8) q1_r);
                q1_org = __msa_bmnz_v(q1_org, q1, is_less_than_beta);
                ST_UB(q1_org, data + image_width);

                is_less_than_beta = __msa_andi_b(is_less_than_beta, 1);
                tc = tc + (v16i8) is_less_than_beta;
            }
            {
                v16i8 negate_thresh, sign_negate_thresh;
                v8i16 threshold_r, threshold_l;
                v8i16 negate_thresh_l, negate_thresh_r;

                negate_thresh = zero - tc;
                sign_negate_thresh = __msa_clti_s_b(negate_thresh, 0);

                ILVR_B2_SH(zero, tc, sign_negate_thresh, negate_thresh,
                           threshold_r, negate_thresh_r);
                AVC_LPF_P0Q0(q0_org_r, p0_org_r, p1_org_r, q1_org_r,
                             negate_thresh_r, threshold_r, p0_r, q0_r);

                threshold_l = (v8i16) __msa_ilvl_b(zero, tc);
                negate_thresh_l = (v8i16) __msa_ilvl_b(sign_negate_thresh,
                                                       negate_thresh);
                AVC_LPF_P0Q0(q0_org_l, p0_org_l, p1_org_l, q1_org_l,
                             negate_thresh_l, threshold_l, p0_l, q0_l);
            }

            PCKEV_B2_UB(p0_l, p0_r, q0_l, q0_r, p0, q0);

            p0_org = __msa_bmnz_v(p0_org, p0, is_less_than);
            q0_org = __msa_bmnz_v(q0_org, q0, is_less_than);

            ST_UB(p0_org, (data - image_width));
            ST_UB(q0_org, data);
        }
    }
}

/*******************************************************************************
                              Inter Chroma
*******************************************************************************/
void avc_loopfilter_cbcr_inter_edge_ver_msa(u8 *data_cb,
                                            u8 *data_cr,
                                            u8 bs0, u8 bs1,
                                            u8 bs2, u8 bs3,
                                            u8 tc0, u8 tc1,
                                            u8 tc2, u8 tc3,
                                            u8 alpha_in, u8 beta_in,
                                            u32 img_width)
{
    u8 *src_cb, *src_cr;
    v16u8 alpha, beta;
    v16u8 p0_asub_q0, p1_asub_p0, q1_asub_q0;
    v16u8 is_less_than, is_less_than1;
    v8i16 is_less_than_r, is_less_than_l;
    v16u8 is_less_than_beta, is_less_than_alpha;
    v16u8 p0, q0;
    v16u8 p1_org, p0_org, q0_org, q1_org;
    v8i16 p1_org_r, p0_org_r, q0_org_r, q1_org_r;
    v8i16 p1_org_l, p0_org_l, q0_org_l, q1_org_l;
    v16u8 is_bs_less_than4, is_bs_greater_than0;
    v8i16 tc_r, tc_l;
    v16i8 negate_tc, sign_negate_tc;
    v8i16 negate_tc_r, i16_negatetc_l;
    v16u8 const4;
    v8i16 p0_r = { 0 };
    v8i16 q0_r = { 0 };
    v8i16 p0_l = { 0 };
    v8i16 q0_l = { 0 };
    v16i8 zero = { 0 };
    v8i16 tmp0, tmp1;
    v16u8 tmp_vec;
    v8i16 bs = { 0 };
    v8i16 tc = { 0 };

    const4 = (v16u8) __msa_ldi_b(4);

    tmp_vec = (v16u8) __msa_fill_b(bs0);
    bs = __msa_insve_h(bs, 0, (v8i16) tmp_vec);
    bs = __msa_insve_h(bs, 4, (v8i16) tmp_vec);

    tmp_vec = (v16u8) __msa_fill_b(bs1);
    bs = __msa_insve_h(bs, 1, (v8i16) tmp_vec);
    bs = __msa_insve_h(bs, 5, (v8i16) tmp_vec);

    tmp_vec = (v16u8) __msa_fill_b(bs2);
    bs = __msa_insve_h(bs, 2, (v8i16) tmp_vec);
    bs = __msa_insve_h(bs, 6, (v8i16) tmp_vec);

    tmp_vec = (v16u8) __msa_fill_b(bs3);
    bs = __msa_insve_h(bs, 3, (v8i16) tmp_vec);
    bs = __msa_insve_h(bs, 7, (v8i16) tmp_vec);

    if (!__msa_test_bz_v((v16u8) bs))
    {
        tmp_vec = (v16u8) __msa_fill_b(tc0);
        tc = __msa_insve_h(tc, 0, (v8i16) tmp_vec);
        tc = __msa_insve_h(tc, 4, (v8i16) tmp_vec);

        tmp_vec = (v16u8) __msa_fill_b(tc1);
        tc = __msa_insve_h(tc, 1, (v8i16) tmp_vec);
        tc = __msa_insve_h(tc, 5, (v8i16) tmp_vec);

        tmp_vec = (v16u8) __msa_fill_b(tc2);
        tc = __msa_insve_h(tc, 2, (v8i16) tmp_vec);
        tc = __msa_insve_h(tc, 6, (v8i16) tmp_vec);

        tmp_vec = (v16u8) __msa_fill_b(tc3);
        tc = __msa_insve_h(tc, 3, (v8i16) tmp_vec);
        tc = __msa_insve_h(tc, 7, (v8i16) tmp_vec);

        is_bs_greater_than0 = (v16u8) (zero < (v16i8) bs);
        {
            v16u8 row0, row1, row2, row3, row4, row5, row6, row7;
            v16u8 row8, row9, row10, row11, row12, row13, row14, row15;

            LD_UB8((data_cb - 2), img_width,
                   row0, row1, row2, row3, row4, row5, row6, row7);
            LD_UB8((data_cr - 2), img_width,
                   row8, row9, row10, row11, row12, row13, row14, row15);
            TRANSPOSE16x4_UB_UB(row0, row1, row2, row3, row4, row5, row6, row7,
                                row8, row9, row10, row11,
                                row12, row13, row14, row15,
                                p1_org, p0_org, q0_org, q1_org);
        }
        p0_asub_q0 = __msa_asub_u_b(p0_org, q0_org);
        p1_asub_p0 = __msa_asub_u_b(p1_org, p0_org);
        q1_asub_q0 = __msa_asub_u_b(q1_org, q0_org);

        alpha = (v16u8) __msa_fill_b(alpha_in);
        beta = (v16u8) __msa_fill_b(beta_in);

        is_less_than_alpha = (p0_asub_q0 < alpha);
        is_less_than_beta = (p1_asub_p0 < beta);
        is_less_than = is_less_than_beta & is_less_than_alpha;
        is_less_than_beta = (q1_asub_q0 < beta);
        is_less_than = is_less_than_beta & is_less_than;
        is_less_than = is_bs_greater_than0 & is_less_than;

        if (!__msa_test_bz_v(is_less_than))
        {
            UNPCK_UB_SH(p1_org, p1_org_r, p1_org_l);
            UNPCK_UB_SH(p0_org, p0_org_r, p0_org_l);
            UNPCK_UB_SH(q0_org, q0_org_r, q0_org_l);
            UNPCK_UB_SH(q1_org, q1_org_r, q1_org_l);

            is_bs_less_than4 = (v16u8) ((v16i8) bs < const4);
            is_less_than1 = is_less_than & is_bs_less_than4;

            if (!__msa_test_bz_v(is_less_than1))
            {
                negate_tc = zero - (v16i8) tc;
                sign_negate_tc = __msa_clti_s_b(negate_tc, 0);

                ILVRL_B2_SH(sign_negate_tc, negate_tc, negate_tc_r,
                            i16_negatetc_l);

                UNPCK_UB_SH(tc, tc_r, tc_l);

                is_less_than_r =
                    (v8i16) __msa_sldi_b((v16i8) is_less_than1, zero, 8);
                if (!__msa_test_bz_v((v16u8) is_less_than_r))
                {
                    AVC_LPF_P0Q0(q0_org_r, p0_org_r, p1_org_r, q1_org_r,
                                 negate_tc_r, tc_r, p0_r, q0_r);
                }

                is_less_than_l =
                    (v8i16) __msa_sldi_b(zero, (v16i8) is_less_than1, 8);
                if (!__msa_test_bz_v((v16u8) is_less_than_l))
                {
                    AVC_LPF_P0Q0(q0_org_l, p0_org_l, p1_org_l, q1_org_l,
                                 i16_negatetc_l, tc_l, p0_l, q0_l);
                }

                /* combine */
                PCKEV_B2_UB(p0_l, p0_r, q0_l, q0_r, p0, q0);

                p0_org = __msa_bmnz_v(p0_org, p0, is_less_than1);
                q0_org = __msa_bmnz_v(q0_org, q0, is_less_than1);
            }

            ILVRL_B2_SH(q0_org, p0_org, tmp1, tmp0);

            src_cb = data_cb - 1;
            ST2x4_UB(tmp1, 0, src_cb, img_width);
            src_cb += 4 * img_width;
            ST2x4_UB(tmp1, 4, src_cb, img_width);
            src_cr = data_cr - 1;
            ST2x4_UB(tmp0, 0, src_cr, img_width);
            src_cr += 4 * img_width;
            ST2x4_UB(tmp0, 4, src_cr, img_width);
        }
    }
}

void avc_loopfilter_cbcr_inter_edge_hor_msa(u8 *data_cb,
                                            u8 *data_cr,
                                            u8 bs0, u8 bs1,
                                            u8 bs2, u8 bs3,
                                            u8 tc0, u8 tc1,
                                            u8 tc2, u8 tc3,
                                            u8 alpha_in, u8 beta_in,
                                            u32 img_width)
{
    v16u8 alpha, beta;
    v8i16 tmp_vec;
    v8i16 bs = { 0 };
    v8i16 tc = { 0 };
    v16u8 p0_asub_q0, p1_asub_p0, q1_asub_q0;
    v16u8 is_less_than;
    v8i16 is_less_than_r, is_less_than_l;
    v16u8 is_less_than_beta, is_less_than_alpha, is_bs_greater_than0;
    v16u8 p0, q0;
    v8i16 p0_r = { 0 };
    v8i16 q0_r = { 0 };
    v8i16 p0_l = { 0 };
    v8i16 q0_l = { 0 };
    v16u8 p1_org, p0_org, q0_org, q1_org;
    v16u8 p1_cb_org, u8_p0cb_org, u8_q0cb_org, q1_cb_org;
    v16u8 p1_cr_org, u8_p0cr_org, u8_q0cr_org, q1_cr_org;
    v8i16 p1_org_r, p0_org_r, q0_org_r, q1_org_r;
    v8i16 p1_org_l, p0_org_l, q0_org_l, q1_org_l;
    v16i8 negate_tc, sign_negate_tc;
    v8i16 negate_tc_r, i16_negatetc_l;
    v8i16 tc_r, tc_l;
    v16i8 zero = { 0 };

    tmp_vec = (v8i16) __msa_fill_b(bs0);
    bs = __msa_insve_h(bs, 0, tmp_vec);
    bs = __msa_insve_h(bs, 4, tmp_vec);

    tmp_vec = (v8i16) __msa_fill_b(bs1);
    bs = __msa_insve_h(bs, 1, tmp_vec);
    bs = __msa_insve_h(bs, 5, tmp_vec);

    tmp_vec = (v8i16) __msa_fill_b(bs2);
    bs = __msa_insve_h(bs, 2, tmp_vec);
    bs = __msa_insve_h(bs, 6, tmp_vec);

    tmp_vec = (v8i16) __msa_fill_b(bs3);
    bs = __msa_insve_h(bs, 3, tmp_vec);
    bs = __msa_insve_h(bs, 7, tmp_vec);

    if (!__msa_test_bz_v((v16u8) bs))
    {
        tmp_vec = (v8i16) __msa_fill_b(tc0);
        tc = __msa_insve_h(tc, 0, tmp_vec);
        tc = __msa_insve_h(tc, 4, tmp_vec);

        tmp_vec = (v8i16) __msa_fill_b(tc1);
        tc = __msa_insve_h(tc, 1,  tmp_vec);
        tc = __msa_insve_h(tc, 5,  tmp_vec);

        tmp_vec = (v8i16) __msa_fill_b(tc2);
        tc = __msa_insve_h(tc, 2, tmp_vec);
        tc = __msa_insve_h(tc, 6, tmp_vec);

        tmp_vec = (v8i16) __msa_fill_b(tc3);
        tc = __msa_insve_h(tc, 3, tmp_vec);
        tc = __msa_insve_h(tc, 7, tmp_vec);

        is_bs_greater_than0 = (v16u8) (zero < (v16i8) bs);

        alpha = (v16u8) __msa_fill_b(alpha_in);
        beta = (v16u8) __msa_fill_b(beta_in);

        LD_UB4(data_cb - (img_width << 1), img_width,
               p1_cb_org, u8_p0cb_org, u8_q0cb_org, q1_cb_org);

        LD_UB4(data_cr - (img_width << 1), img_width,
               p1_cr_org, u8_p0cr_org, u8_q0cr_org, q1_cr_org);

        ILVR_D4_UB(p1_cr_org, p1_cb_org, u8_p0cr_org, u8_p0cb_org,
                   u8_q0cr_org, u8_q0cb_org, q1_cr_org, q1_cb_org,
                   p1_org, p0_org, q0_org, q1_org);

        p0_asub_q0 = __msa_asub_u_b(p0_org, q0_org);
        p1_asub_p0 = __msa_asub_u_b(p1_org, p0_org);
        q1_asub_q0 = __msa_asub_u_b(q1_org, q0_org);

        is_less_than_alpha = (p0_asub_q0 < alpha);
        is_less_than_beta = (p1_asub_p0 < beta);
        is_less_than = is_less_than_beta & is_less_than_alpha;
        is_less_than_beta = (q1_asub_q0 < beta);
        is_less_than = is_less_than_beta & is_less_than;
        is_less_than = is_less_than & is_bs_greater_than0;

        if (!__msa_test_bz_v(is_less_than))
        {
            negate_tc = zero - (v16i8) tc;
            sign_negate_tc = __msa_clti_s_b(negate_tc, 0);

            ILVRL_B2_SH(sign_negate_tc, negate_tc, negate_tc_r,
                        i16_negatetc_l);

            UNPCK_UB_SH(tc, tc_r, tc_l);
            UNPCK_UB_SH(p1_org, p1_org_r, p1_org_l);
            UNPCK_UB_SH(p0_org, p0_org_r, p0_org_l);
            UNPCK_UB_SH(q0_org, q0_org_r, q0_org_l);
            UNPCK_UB_SH(q1_org, q1_org_r, q1_org_l);

            is_less_than_r =
                (v8i16) __msa_sldi_b((v16i8) is_less_than, zero, 8);
            if (!__msa_test_bz_v((v16u8) is_less_than_r))
            {
                AVC_LPF_P0Q0(q0_org_r, p0_org_r, p1_org_r, q1_org_r,
                             negate_tc_r, tc_r, p0_r, q0_r);
            }

            is_less_than_l =
                (v8i16) __msa_sldi_b(zero, (v16i8) is_less_than, 8);
            if (!__msa_test_bz_v((v16u8) is_less_than_l))
            {
                AVC_LPF_P0Q0(q0_org_l, p0_org_l, p1_org_l, q1_org_l,
                             i16_negatetc_l, tc_l, p0_l, q0_l);
            }

            /* combine */
            PCKEV_B2_UB(p0_l, p0_r, q0_l, q0_r, p0, q0);

            p0_org = __msa_bmnz_v(p0_org, p0, is_less_than);
            q0_org = __msa_bmnz_v(q0_org, q0, is_less_than);

            {
                v2i64 cb_coeff = { 0, 3 };

                u8_p0cb_org = (v16u8) __msa_vshf_d(cb_coeff,
                                                   (v2i64) u8_p0cb_org,
                                                   (v2i64) p0_org);
                u8_q0cb_org = (v16u8) __msa_vshf_d(cb_coeff,
                                                   (v2i64) u8_q0cb_org,
                                                   (v2i64) q0_org);
            }
            u8_p0cr_org =
                (v16u8) __msa_pckod_d((v2i64) u8_p0cr_org, (v2i64) p0_org);
            u8_q0cr_org =
                (v16u8) __msa_pckod_d((v2i64) u8_q0cr_org, (v2i64) q0_org);

            ST_UB(u8_q0cb_org, data_cb);
            ST_UB(u8_p0cb_org, (data_cb - img_width));
            ST_UB(u8_q0cr_org, data_cr);
            ST_UB(u8_p0cr_org, (data_cr - img_width));
        }
    }
}
#endif /* #ifdef H264DEC_MSA */
