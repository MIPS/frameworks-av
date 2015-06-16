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

#include "mp4dec_lib.h"
#include "motion_comp.h"
#include "mbtype_mode.h"
#include "common_macros_msa.h"
#include "prototypes_msa.h"

#ifdef M4VH263DEC_MSA
const uint8 h263_loop_filter_strength_msa[32] =
{
    0, 1, 1, 2, 2, 3, 3, 4, 4, 4, 5, 5, 6, 6, 7, 7,
    7, 8, 8, 8, 9, 9, 9, 10, 10, 10, 11, 11, 11, 12, 12, 12
};

#define H263_TRANSPOSE16X4(in0, in1, in2, in3, in4, in5, in6, in7, in8,  \
                           in9, in10, in11, in12, in13, in14, in15,      \
                           out0, out1, out2, out3)                       \
{                                                                        \
    v16i8 tmp0, tmp1, tmp2, tmp3, tmp4, tmp5;                            \
                                                                         \
    ILVR_B2_SB(in1, in0, in3, in2, tmp0, tmp1);                          \
    tmp0 = (v16i8) __msa_ilvr_h((v8i16) tmp1, (v8i16) tmp0);             \
    ILVR_B2_SB(in5, in4, in7, in6, tmp1, tmp2)                           \
    tmp1 = (v16i8) __msa_ilvr_h((v8i16) tmp2, (v8i16) tmp1);             \
    tmp2 = (v16i8) __msa_ilvr_w((v4i32) tmp1, (v4i32) tmp0);             \
    tmp3 = (v16i8) __msa_ilvl_w((v4i32) tmp1, (v4i32) tmp0);             \
    ILVR_B2_SB(in9, in8, in11, in10, tmp0, tmp1);                        \
    tmp0 = (v16i8) __msa_ilvr_h((v8i16) tmp1, (v8i16) tmp0);             \
    ILVR_B2_SB(in13, in12, in15, in14, tmp1, tmp4);                      \
    tmp4 = (v16i8) __msa_ilvr_h((v8i16) tmp4, (v8i16) tmp1);             \
    tmp5 = (v16i8) __msa_ilvl_w((v4i32) tmp4, (v4i32) tmp0);             \
    tmp4 = (v16i8) __msa_ilvr_w((v4i32) tmp4, (v4i32) tmp0);             \
    ILVR_D2_UB(tmp4, tmp2, tmp5, tmp3, out0, out2);                      \
    out1 = (v16u8) __msa_ilvl_d((v2i64) tmp4, (v2i64) tmp2);             \
    out3 = (v16u8) __msa_ilvl_d((v2i64) tmp5, (v2i64) tmp3);             \
}

void h263_deblock_luma_msa(uint8 *rec, int32 width,
                           int32 height, int16 *qp_store,
                           uint8 *mode, int32 is_chroma,
                           int32 annex_t)
{
    int32 col, row;
    uint8 *rec_y;
    int32 mb_num, strength, b_size;
    int32 offset, num_mb_per_row, num_mb_per_col;
    int32 width2 = (width << 1);

    mb_num = 0;
    num_mb_per_row = width >> 4;
    num_mb_per_col = height >> 4;
    b_size = 16;

    /* vertical filtering */
    rec_y = rec + (width << 3);
    for (col = (height >> 4); col--;)
    {
        for (row = (width >> 4); row--;)
        {
            if (MODE_SKIPPED != mode[mb_num])
            {
                int32 temp;
                v16u8 in0, in1, in2, in3;
                v8i16 temp0, temp1, temp2, temp3;
                v8i16 diff0, diff1, diff2, diff3, diff4;
                v8i16 diff5, diff6, diff7, diff8, diff9;
                v8i16 d0, d1, str_x2, str, a_d0, a_d1;

                LD_UB4(rec_y - width2, width, in0, in3, in2, in1);
                strength = h263_loop_filter_strength_msa[qp_store[mb_num]];
                ILVRL_B2_SH(in0, in1, temp0, temp1);
                HSUB_UB2_SH(temp0, temp1, a_d0, a_d1);
                ILVRL_B2_SH(in2, in3, temp2, temp3);
                HSUB_UB2_SH(temp2, temp3, temp2, temp3);
                temp2 <<= 2;
                temp3 <<= 2;
                ADD2(a_d0, temp2, a_d1, temp3, diff0, diff1);
                temp = - (strength << 1);
                diff2 = - (- diff0 >> 3);
                diff3 = - (- diff1 >> 3);
                str_x2 = __msa_fill_h(temp);
                temp0 = str_x2 <= diff2;
                temp1 = str_x2 <= diff3;
                diff2 = (v8i16) __msa_bmz_v((v16u8) diff2, (v16u8)temp0, (v16u8) temp0);
                diff3 = (v8i16) __msa_bmz_v((v16u8) diff3, (v16u8) temp1, (v16u8) temp1);
                temp = -strength;
                SUB2(str_x2, diff2, str_x2, diff3, temp2, temp3);
                str = __msa_fill_h(temp);
                temp0 = diff2 < str;
                temp1 = diff3 < str;
                diff2 = (v8i16) __msa_bmnz_v((v16u8) diff2, (v16u8) temp2, (v16u8) temp0);
                diff3 = (v8i16) __msa_bmnz_v((v16u8) diff3, (v16u8) temp3, (v16u8) temp1);
                temp = strength << 1;
                diff4 = diff0 >> 3;
                diff5 = diff1 >> 3;
                str_x2 = __msa_fill_h(temp);
                temp0 = diff4 <= str_x2;
                temp1 = diff5 <= str_x2;
                diff4 = (v8i16) __msa_bmz_v((v16u8) diff4, (v16u8) temp0, (v16u8) temp0);
                diff5 = (v8i16) __msa_bmz_v((v16u8) diff5, (v16u8) temp1, (v16u8) temp1);
                SUB2(str_x2, diff4, str_x2, diff5, temp2, temp3);
                str = __msa_fill_h(strength);
                temp0 = (str < diff4);
                temp1 = (str < diff5);
                diff4 = (v8i16) __msa_bmnz_v((v16u8) diff4, (v16u8) temp2, (v16u8) temp0);
                diff5 = (v8i16) __msa_bmnz_v((v16u8) diff5, (v16u8) temp3, (v16u8) temp1);
                temp0 = __msa_clti_s_h(diff0, 0);
                temp1 = __msa_clti_s_h(diff1, 0);
                d0 = (v8i16) __msa_bmnz_v((v16u8) diff4, (v16u8) diff2, (v16u8) temp0);
                d1 = (v8i16) __msa_bmnz_v((v16u8) diff5, (v16u8) diff3, (v16u8) temp1);
                diff2 = - diff2 >> 1;
                diff3 = - diff3 >> 1;
                diff4 >>= 1;
                diff5 >>= 1;
                diff8 = (v8i16) __msa_bmnz_v((v16u8) diff4, (v16u8) diff2, (v16u8) temp0);
                diff9 = (v8i16) __msa_bmnz_v((v16u8) diff5, (v16u8) diff3, (v16u8) temp1);
                diff6 = (- a_d0) >> 2;
                diff7 = (- a_d1) >> 2;
                diff6 = - diff6;
                diff7 = - diff7;
                temp2 = - diff8;
                temp3 = - diff9;
                temp0 = diff6 < temp2;
                temp1 = diff7 < temp3;
                diff6 = (v8i16) __msa_bmnz_v((v16u8) diff6, (v16u8) temp2, (v16u8) temp0);
                diff7 = (v8i16) __msa_bmnz_v((v16u8) diff7, (v16u8) temp3, (v16u8) temp1);
                diff2 = a_d0 >> 2;
                diff3 = a_d1 >> 2;
                temp0 = diff2 <= diff8;
                temp1 = diff3 <= diff9;
                diff2 = (v8i16) __msa_bmz_v((v16u8) diff2, (v16u8) diff8, (v16u8) temp0);
                diff3 = (v8i16) __msa_bmz_v((v16u8) diff3, (v16u8) diff9, (v16u8) temp1);
                temp0 = __msa_clti_s_h(a_d0, 0);
                temp1 = __msa_clti_s_h(a_d1, 0);
                diff6 = (v8i16) __msa_bmz_v((v16u8) diff6, (v16u8) diff2, (v16u8) temp0);
                diff7 = (v8i16) __msa_bmz_v((v16u8) diff7, (v16u8) diff3, (v16u8) temp1);
                PCKEV_B2_SH(diff7, diff6, d1, d0, diff6, d0);
                in0 = (v16u8) ((v16i8) in0 - (v16i8) diff6);
                in1 = (v16u8) ((v16i8) in1 + (v16i8) diff6);
                in3 = __msa_xori_b(in3, 128);
                in3 = (v16u8) __msa_adds_s_b((v16i8) in3, (v16i8) d0);
                in3 = __msa_xori_b(in3, 128);
                in2 = __msa_subsus_u_b(in2, (v16i8) d0);
                ST_UB4(in0, in3, in2, in1, rec_y - width2, width);
            }

            rec_y += b_size;
            mb_num++;
        }

        rec_y += (15 * width);
    }

    /* vertical boundary blocks */
    rec_y = rec + width * b_size;
    mb_num = num_mb_per_row;
    for (col = num_mb_per_col - 1; col--;)
    {
        for (row = num_mb_per_row; row--;)
        {
            if (MODE_SKIPPED != mode[mb_num] ||
                MODE_SKIPPED != mode[mb_num - num_mb_per_row])
            {
                v16u8 in0, in1, in2, in3;
                v8i16 temp0, temp1, temp2, temp3;
                v8i16 diff0, diff1, diff2, diff3, diff4;
                v8i16 diff5, diff6, diff7, diff8, diff9;
                v8i16 d0, d1, str_x2, str, a_d0, a_d1;
                uint32 strength_tab_idx;

                if (MODE_SKIPPED != mode[mb_num])
                {
                    strength_tab_idx =
                        annex_t ? MQ_chroma_QP_table[qp_store[mb_num]] :
                        qp_store[mb_num];
                }
                else
                {
                    strength_tab_idx =
                        annex_t ?
                        MQ_chroma_QP_table[qp_store[mb_num - num_mb_per_row]]
                        : qp_store[mb_num - num_mb_per_row];
                }

                strength = h263_loop_filter_strength_msa[strength_tab_idx];
                LD_UB4(rec_y - width2, width, in0, in3, in2, in1);
                ILVRL_B2_SH(in0, in1, temp0, temp1);
                HSUB_UB2_SH(temp0, temp1, a_d0, a_d1);
                ILVRL_B2_SH(in2, in3, temp2, temp3);
                HSUB_UB2_SH(temp2, temp3, temp2, temp3);
                temp2 <<= 2;
                temp3 <<= 2;
                ADD2(a_d0, temp2, a_d1, temp3, diff0, diff1);
                diff2 = - (- diff0 >> 3);
                diff3 = - (- diff1 >> 3);
                str_x2 = __msa_fill_h(-(strength << 1));
                temp0 = str_x2 <= diff2;
                temp1 = str_x2 <= diff3;
                diff2 = (v8i16) __msa_bmz_v((v16u8) diff2, (v16u8) temp0, (v16u8) temp0);
                diff3 = (v8i16) __msa_bmz_v((v16u8) diff3, (v16u8) temp1, (v16u8) temp1);
                SUB2(str_x2, diff2, str_x2, diff3, temp2, temp3);
                str = __msa_fill_h(- strength);
                temp0 = diff2 < str;
                temp1 = diff3 < str;
                diff2 = (v8i16) __msa_bmnz_v((v16u8) diff2, (v16u8) temp2, (v16u8) temp0);
                diff3 = (v8i16) __msa_bmnz_v((v16u8) diff3, (v16u8) temp3, (v16u8) temp1);
                diff4 = diff0 >> 3;
                diff5 = diff1 >> 3;
                str_x2 = __msa_fill_h(strength << 1);
                temp0 = diff4 <= str_x2;
                temp1 = diff5 <= str_x2;
                diff4 = (v8i16) __msa_bmz_v((v16u8) diff4, (v16u8) temp0, (v16u8) temp0);
                diff5 = (v8i16) __msa_bmz_v((v16u8) diff5, (v16u8) temp1, (v16u8) temp1);
                SUB2(str_x2, diff4, str_x2, diff5, temp2, temp3);
                str = __msa_fill_h(strength);
                temp0 = str < diff4;
                temp1 = str < diff5;
                diff4 = (v8i16) __msa_bmnz_v((v16u8) diff4, (v16u8) temp2, (v16u8) temp0);
                diff5 = (v8i16) __msa_bmnz_v((v16u8) diff5, (v16u8) temp3, (v16u8) temp1);
                temp0 = __msa_clti_s_h(diff0, 0);
                temp1 = __msa_clti_s_h(diff1, 0);
                d0 = (v8i16) __msa_bmnz_v((v16u8) diff4, (v16u8) diff2, (v16u8) temp0);
                d1 = (v8i16) __msa_bmnz_v((v16u8) diff5, (v16u8) diff3, (v16u8) temp1);
                diff2 = - diff2 >> 1;
                diff3 = - diff3 >> 1;
                diff4 >>= 1;
                diff5 >>= 1;
                diff8 = (v8i16) __msa_bmnz_v((v16u8) diff4, (v16u8) diff2, (v16u8) temp0);
                diff9 = (v8i16) __msa_bmnz_v((v16u8) diff5, (v16u8) diff3, (v16u8) temp1);
                diff6 = (- a_d0) >> 2;
                diff7 = (- a_d1) >> 2;
                diff6 = - diff6;
                diff7 = - diff7;
                temp2 = - diff8;
                temp3 = - diff9;
                temp0 = diff6 < temp2;
                temp1 = diff7 < temp3;
                diff6 = (v8i16) __msa_bmnz_v((v16u8) diff6, (v16u8) temp2, (v16u8) temp0);
                diff7 = (v8i16) __msa_bmnz_v((v16u8) diff7, (v16u8) temp3, (v16u8) temp1);
                diff2 = a_d0 >> 2;
                diff3 = a_d1 >> 2;
                temp0 = diff2 <= diff8;
                temp1 = diff3 <= diff9;
                diff2 = (v8i16) __msa_bmz_v((v16u8) diff2, (v16u8) diff8, (v16u8) temp0);
                diff3 = (v8i16) __msa_bmz_v((v16u8) diff3, (v16u8) diff9, (v16u8) temp1);
                temp0 = __msa_clti_s_h(a_d0, 0);
                temp1 = __msa_clti_s_h(a_d1, 0);
                diff6 = (v8i16) __msa_bmz_v((v16u8) diff6, (v16u8) diff2, (v16u8) temp0);
                diff7 = (v8i16) __msa_bmz_v((v16u8) diff7, (v16u8) diff3, (v16u8) temp1);
                PCKEV_B2_SH(diff7, diff6, d1, d0, diff6, d0);
                in0 = (v16u8) ((v16i8) in0 - (v16i8) diff6);
                in1 = (v16u8) ((v16i8) in1 + (v16i8) diff6);
                in3 = __msa_xori_b(in3, 128);
                ST_UB(in0, rec_y - width2);
                in3 = (v16u8) __msa_adds_s_b((v16i8) in3, (v16i8) d0);
                in3 = __msa_xori_b(in3, 128);
                ST_UB(in3, rec_y - width);
                in2 = __msa_subsus_u_b(in2, (v16i8) d0);
                ST_UB2(in2, in1, rec_y, width);
            }

            rec_y += b_size;
            mb_num++;
        }

        rec_y += ((b_size - 1) * width);
    }

    /* horizontal filtering */
    mb_num = 0;
    rec_y = rec + 8;
    offset = width * b_size - b_size;
    for (col = num_mb_per_col; col--;)
    {
        for (row = num_mb_per_row; row--;)
        {
            if (MODE_SKIPPED != mode[mb_num])
            {
                uint8 *rec_local = rec_y - 2;
                v16u8 in0, in1, in2, in3, in4, in5, in6, in7;
                v16u8 in8, in9, in10, in11, in12, in13, in14, in15;
                v8i16 temp0, temp1, temp2, temp3;
                v8i16 diff0, diff1, diff2, diff3, diff4;
                v8i16 diff5, diff6, diff7, diff8, diff9;
                v8i16 d0, d1, str_x2, str, a_d0, a_d1;

                LD_UB8(rec_local, width, in0, in1, in2, in3, in4, in5, in6, in7);
                LD_UB8((rec_local + 8 * width), width,
                       in8, in9, in10, in11, in12, in13, in14, in15);
                strength = h263_loop_filter_strength_msa[qp_store[mb_num]];
                H263_TRANSPOSE16X4(in0, in1, in2, in3, in4, in5, in6, in7,
                                   in8, in9, in10, in11, in12, in13, in14, in15,
                                   in0, in3, in2, in1);
                ILVRL_B2_SH(in0, in1, temp0, temp1);
                HSUB_UB2_SH(temp0, temp1, a_d0, a_d1);
                ILVRL_B2_SH(in2, in3, temp2, temp3);
                HSUB_UB2_SH(temp2, temp3, temp2, temp3);
                temp2 <<= 2;
                temp3 <<= 2;
                ADD2(a_d0, temp2, a_d1, temp3, diff0, diff1);
                diff2 = - (- diff0 >> 3);
                diff3 = - (- diff1 >> 3);
                str_x2 = __msa_fill_h(- (strength << 1));
                temp0 = (str_x2 <= diff2);
                temp1 = (str_x2 <= diff3);
                diff2 = (v8i16) __msa_bmz_v((v16u8) diff2, (v16u8) temp0, (v16u8) temp0);
                diff3 = (v8i16) __msa_bmz_v((v16u8) diff3, (v16u8) temp1, (v16u8) temp1);
                SUB2(str_x2, diff2, str_x2, diff3, temp2, temp3);
                str = __msa_fill_h(-strength);
                temp0 = (diff2 < str);
                temp1 = (diff3 < str);
                diff2 = (v8i16) __msa_bmnz_v((v16u8) diff2, (v16u8) temp2, (v16u8) temp0);
                diff3 = (v8i16) __msa_bmnz_v((v16u8) diff3, (v16u8) temp3, (v16u8) temp1);
                diff4 = diff0 >> 3;
                diff5 = diff1 >> 3;
                str_x2 = __msa_fill_h(strength << 1);
                temp0 = (diff4 <= str_x2);
                temp1 = (diff5 <= str_x2);
                diff4 = (v8i16) __msa_bmz_v((v16u8) diff4, (v16u8) temp0, (v16u8) temp0);
                diff5 = (v8i16) __msa_bmz_v((v16u8) diff5, (v16u8) temp1, (v16u8) temp1);
                SUB2(str_x2, diff4, str_x2, diff5, temp2, temp3);
                str = __msa_fill_h(strength);
                temp0 = (str < diff4);
                temp1 = (str < diff5);
                diff4 = (v8i16) __msa_bmnz_v((v16u8) diff4, (v16u8) temp2, (v16u8) temp0);
                diff5 = (v8i16) __msa_bmnz_v((v16u8) diff5, (v16u8) temp3, (v16u8) temp1);
                temp0 = __msa_clti_s_h(diff0, 0);
                temp1 = __msa_clti_s_h(diff1, 0);
                d0 = (v8i16) __msa_bmnz_v((v16u8) diff4, (v16u8) diff2, (v16u8) temp0);
                d1 = (v8i16) __msa_bmnz_v((v16u8) diff5, (v16u8) diff3, (v16u8) temp1);
                diff2 = (- diff2) >> 1;
                diff3 = (- diff3) >> 1;
                diff4 >>= 1;
                diff5 >>= 1;
                diff8 = (v8i16) __msa_bmnz_v((v16u8) diff4, (v16u8) diff2, (v16u8) temp0);
                diff9 = (v8i16) __msa_bmnz_v((v16u8) diff5, (v16u8) diff3, (v16u8) temp1);
                diff6 = (- a_d0) >> 2;
                diff7 = (- a_d1) >> 2;
                diff6 = - diff6;
                diff7 = - diff7;
                temp2 = - diff8;
                temp3 = - diff9;
                temp0 = (diff6 < temp2);
                temp1 = (diff7 < temp3);
                diff6 = (v8i16) __msa_bmnz_v((v16u8) diff6, (v16u8) temp2, (v16u8) temp0);
                diff7 = (v8i16) __msa_bmnz_v((v16u8) diff7, (v16u8) temp3, (v16u8) temp1);
                diff2 = a_d0 >> 2;
                diff3 = a_d1 >> 2;
                temp0 = (diff2 <= diff8);
                temp1 = (diff3 <= diff9);
                diff2 = (v8i16) __msa_bmz_v((v16u8) diff2, (v16u8) diff8, (v16u8) temp0);
                diff3 = (v8i16) __msa_bmz_v((v16u8) diff3, (v16u8) diff9, (v16u8) temp1);
                temp0 = __msa_clti_s_h(a_d0, 0);
                temp1 = __msa_clti_s_h(a_d1, 0);
                diff6 = (v8i16) __msa_bmz_v((v16u8) diff6, (v16u8) diff2, (v16u8) temp0);
                diff7 = (v8i16) __msa_bmz_v((v16u8) diff7, (v16u8) diff3, (v16u8) temp1);
                PCKEV_B2_SH(diff7, diff6, d1, d0, diff6, d0);
                in0 = (v16u8) ((v16i8) in0 - (v16i8) diff6);
                in1 = (v16u8) ((v16i8) in1 + (v16i8) diff6);
                in3 = __msa_xori_b(in3, 128);
                in3 = (v16u8) __msa_adds_s_b((v16i8) in3, (v16i8) d0);
                in3 = __msa_xori_b(in3, 128);
                in2 = __msa_subsus_u_b(in2, (v16i8) d0);
                ILVR_B2_SH(in3, in0, in1, in2, temp0, temp1);
                ILVL_B2_SH(in3, in0, in1, in2, temp2, temp3);
                in0 = (v16u8) __msa_ilvr_h(temp1, temp0);
                in3 = (v16u8) __msa_ilvl_h(temp1, temp0);
                in2 = (v16u8) __msa_ilvr_h(temp3, temp2);
                in1 = (v16u8) __msa_ilvl_h(temp3, temp2);
                ST4x4_UB(in0, in0, 0, 1, 2, 3, rec_local, width);
                rec_local += 4 * width;
                ST4x4_UB(in3, in3,  0, 1, 2, 3, rec_local, width);
                rec_local += 4 * width;
                ST4x4_UB(in2, in2,  0, 1, 2, 3, rec_local, width);
                rec_local += 4 * width;
                ST4x4_UB(in1, in1,  0, 1, 2, 3, rec_local, width);
                rec_local += 4 * width;

                rec_y += 16 * width;
                rec_y -= offset;
            }
            else
            {
                rec_y += b_size;
            }

            mb_num++;
        }

        rec_y += (15 * width);
    }

    /* horizontal edge */
    rec_y = rec + b_size;
    offset = width * b_size - b_size;
    mb_num = 1;
    for (col = num_mb_per_col; col--;)
    {
        for (row = num_mb_per_row - 1; row--;)
        {
            if (MODE_SKIPPED != mode[mb_num] ||
                MODE_SKIPPED != mode[mb_num - 1])
            {
                uint8 *rec_local = rec_y - 2;
                v16u8 in0, in1, in2, in3, in4, in5, in6, in7;
                v16u8 in8, in9, in10, in11, in12, in13, in14, in15;
                v8i16 temp0, temp1, temp2, temp3;
                v8i16 diff0, diff1, diff2, diff3, diff4;
                v8i16 diff5, diff6, diff7, diff8, diff9;
                v8i16 d0, d1, str_x2, str, a_d0, a_d1;

                uint32 strength_tab_idx;
                if (MODE_SKIPPED != mode[mb_num])
                {
                    strength_tab_idx =
                        annex_t ? MQ_chroma_QP_table[qp_store[mb_num]] :
                        qp_store[mb_num];
                }
                else
                {
                    strength_tab_idx =
                        annex_t ? MQ_chroma_QP_table[qp_store[mb_num - 1]] :
                        qp_store[mb_num - 1];
                }

                strength = h263_loop_filter_strength_msa[strength_tab_idx];
                LD_UB8(rec_local, width, in0, in1, in2, in3, in4, in5, in6, in7);
                LD_UB8((rec_local + 8 * width), width,
                       in8, in9, in10, in11, in12, in13, in14, in15);
                H263_TRANSPOSE16X4(in0, in1, in2, in3, in4, in5, in6, in7,
                                   in8, in9, in10, in11, in12, in13, in14, in15,
                                   in0, in3, in2, in1);
                ILVRL_B2_SH(in0, in1, temp0, temp1);
                HSUB_UB2_SH(temp0, temp1, a_d0, a_d1);
                ILVRL_B2_SH(in2, in3, temp2, temp3);
                HSUB_UB2_SH(temp2, temp3, temp2, temp3);
                temp2 <<= 2;
                temp3 <<= 2;
                ADD2(a_d0, temp2, a_d1, temp3, diff0, diff1);
                diff2 = -(-diff0 >> 3);
                diff3 = -(-diff1 >> 3);
                str_x2 = __msa_fill_h(-(strength << 1));
                temp0 = (str_x2 <= diff2);
                temp1 = (str_x2 <= diff3);
                diff2 = (v8i16) __msa_bmz_v((v16u8) diff2, (v16u8) temp0, (v16u8) temp0);
                diff3 = (v8i16) __msa_bmz_v((v16u8) diff3, (v16u8) temp1, (v16u8) temp1);
                SUB2(str_x2, diff2, str_x2, diff3, temp2, temp3);
                str = __msa_fill_h(-strength);
                temp0 = (diff2 < str);
                temp1 = (diff3 < str);
                diff2 = (v8i16) __msa_bmnz_v((v16u8) diff2, (v16u8) temp2, (v16u8) temp0);
                diff3 = (v8i16) __msa_bmnz_v((v16u8) diff3, (v16u8) temp3, (v16u8) temp1);
                diff4 = diff0 >> 3;
                diff5 = diff1 >> 3;
                str_x2 = __msa_fill_h(strength << 1);
                temp0 = (diff4 <= str_x2);
                temp1 = (diff5 <= str_x2);
                diff4 = (v8i16) __msa_bmz_v((v16u8) diff4, (v16u8) temp0, (v16u8) temp0);
                diff5 = (v8i16) __msa_bmz_v((v16u8) diff5, (v16u8) temp1, (v16u8) temp1);
                SUB2(str_x2, diff4, str_x2, diff5, temp2, temp3);
                str = __msa_fill_h(strength);
                temp0 = (str < diff4);
                temp1 = (str < diff5);
                diff4 = (v8i16) __msa_bmnz_v((v16u8) diff4, (v16u8) temp2, (v16u8) temp0);
                diff5 = (v8i16) __msa_bmnz_v((v16u8) diff5, (v16u8) temp3, (v16u8) temp1);
                temp0 = __msa_clti_s_h(diff0, 0);
                temp1 = __msa_clti_s_h(diff1, 0);
                d0 = (v8i16) __msa_bmnz_v((v16u8) diff4, (v16u8) diff2, (v16u8) temp0);
                d1 = (v8i16) __msa_bmnz_v((v16u8) diff5, (v16u8) diff3, (v16u8) temp1);
                diff2 = -diff2 >> 1;
                diff3 = -diff3 >> 1;
                diff4 >>= 1;
                diff5 >>= 1;
                diff8 = (v8i16) __msa_bmnz_v((v16u8) diff4, (v16u8) diff2, (v16u8) temp0);
                diff9 = (v8i16) __msa_bmnz_v((v16u8) diff5, (v16u8) diff3, (v16u8) temp1);
                diff6 = (-a_d0) >> 2;
                diff7 = (-a_d1) >> 2;
                diff6 = -(diff6);
                diff7 = -(diff7);
                temp2 = -diff8;
                temp3 = -diff9;
                temp0 = (diff6 < temp2);
                temp1 = (diff7 < temp3);
                diff6 = (v8i16) __msa_bmnz_v((v16u8) diff6, (v16u8) temp2, (v16u8) temp0);
                diff7 = (v8i16) __msa_bmnz_v((v16u8) diff7, (v16u8) temp3, (v16u8) temp1);
                diff2 = a_d0 >> 2;
                diff3 = a_d1 >> 2;
                temp0 = (diff2 <= diff8);
                temp1 = (diff3 <= diff9);
                diff2 = (v8i16) __msa_bmz_v((v16u8) diff2, (v16u8) diff8, (v16u8) temp0);
                diff3 = (v8i16) __msa_bmz_v((v16u8) diff3, (v16u8) diff9, (v16u8) temp1);
                temp0 = __msa_clti_s_h(a_d0, 0);
                temp1 = __msa_clti_s_h(a_d1, 0);
                diff6 = (v8i16) __msa_bmz_v((v16u8) diff6, (v16u8) diff2, (v16u8) temp0);
                diff7 = (v8i16) __msa_bmz_v((v16u8) diff7, (v16u8) diff3, (v16u8) temp1);
                PCKEV_B2_SH(diff7, diff6, d1, d0, diff6, d0);
                in0 = (v16u8) ((v16i8) in0 - (v16i8) diff6);
                in1 = (v16u8) ((v16i8) in1 + (v16i8) diff6);
                in3 = __msa_xori_b(in3, 128);
                in3 = (v16u8) __msa_adds_s_b((v16i8) in3, (v16i8) d0);
                in3 = __msa_xori_b(in3, 128);
                in2 = __msa_subsus_u_b(in2, (v16i8) d0);
                ILVR_B2_SH(in3, in0, in1, in2, temp0, temp1);
                ILVL_B2_SH(in3, in0, in1, in2, temp2, temp3);
                in0 = (v16u8) __msa_ilvr_h(temp1, temp0);
                in3 = (v16u8) __msa_ilvl_h(temp1, temp0);
                in2 = (v16u8) __msa_ilvr_h(temp3, temp2);
                in1 = (v16u8) __msa_ilvl_h(temp3, temp2);
                ST4x4_UB(in0, in0,  0, 1, 2, 3, rec_local, width);
                rec_local += 4 * width;
                ST4x4_UB(in3, in3,  0, 1, 2, 3,  rec_local, width);
                rec_local += 4 * width;
                ST4x4_UB(in2, in2,  0, 1, 2, 3, rec_local, width);
                rec_local += 4 * width;
                ST4x4_UB(in1, in1,  0, 1, 2, 3, rec_local, width);
                rec_local += 4 * width;

                rec_y += 16 * width;
                rec_y -= offset;
            }
            else  // if !(mode[mb_num] != MODE_SKIPPED ||
                  // mode[mb_num-1] != MODE_SKIPPED)
            {
                rec_y += b_size;
            }

            mb_num++;
        }

        rec_y += ((width * (b_size - 1)) + b_size);
        mb_num++;
    }

    return;
}

void h263_deblock_chroma_msa(uint8 *rec, int32 width,
                             int32 height, int16 *qp_store,
                             uint8 *mode, int32 is_chroma,
                             int32 annex_t)
{
    int32 col, row;
    uint8 *rec_y;
    int32 mb_num, strength, b_size;
    int32 offset, num_mb_per_row, num_mb_per_col;
    int32 width2 = (width << 1);

    mb_num = 0;
    num_mb_per_row = width >> 3;
    num_mb_per_col = height >> 3;
    b_size = 8;

    /* vertical boundary blocks */
    rec_y = rec + width * b_size;
    mb_num = num_mb_per_row;
    for (col = num_mb_per_col - 1; col--;)
    {
        for (row = num_mb_per_row; row--;)
        {
            if (MODE_SKIPPED != mode[mb_num] ||
                MODE_SKIPPED != mode[mb_num - num_mb_per_row])
            {
                int64_t temp;
                int64_t res0, res1, res2, res3;
                v16u8 in0, in1, in2, in3;
                v8i16 temp0, temp1, temp2, temp3, temp4, temp5;
                v8i16 diff0, diff1, diff2, diff3, diff4;
                v8i16 d0, str_x2, str, a_d0;
                uint32 strength_tab_idx;

                if (MODE_SKIPPED != mode[mb_num])
                {
                    strength_tab_idx =
                        annex_t ? MQ_chroma_QP_table[qp_store[mb_num]] :
                        qp_store[mb_num];
                }
                else
                {
                    strength_tab_idx =
                        annex_t ?
                        MQ_chroma_QP_table[qp_store[mb_num - num_mb_per_row]]
                        : qp_store[mb_num - num_mb_per_row];
                }

                strength = h263_loop_filter_strength_msa[strength_tab_idx];
                temp = LD(rec_y - width2);
                in0 = (v16u8) __msa_fill_d(temp);
                temp = LD(rec_y + width);
                in1 = (v16u8) __msa_fill_d(temp);
                temp = LD(rec_y);
                in2 = (v16u8) __msa_fill_d(temp);
                temp = LD(rec_y - width);
                in3 = (v16u8) __msa_fill_d(temp);
                temp0 = (v8i16) __msa_ilvr_b((v16i8) in0, (v16i8) in1);
                a_d0 = __msa_hsub_u_h((v16u8) temp0, (v16u8) temp0);
                temp1 = (v8i16) __msa_ilvr_b((v16i8) in2, (v16i8) in3);
                temp1 = __msa_hsub_u_h((v16u8) temp1, (v16u8) temp1);
                temp1 <<= 2;
                diff0 = a_d0 + temp1;
                diff1 = -(-diff0 >> 3);
                str_x2 = __msa_fill_h(-(strength << 1));
                temp0 = (str_x2 <= diff1);
                diff1 = (v8i16) __msa_bmz_v((v16u8) diff1, (v16u8) temp0, (v16u8) temp0);
                temp1 = str_x2 - diff1;
                str = __msa_fill_h(-strength);
                temp0 = (diff1 < str);
                diff1 = (v8i16) __msa_bmnz_v((v16u8) diff1, (v16u8) temp1, (v16u8) temp0);
                diff2 = diff0 >> 3;
                str_x2 = __msa_fill_h(strength << 1);
                temp0 = (diff2 <= str_x2);
                diff2 = (v8i16) __msa_bmz_v((v16u8) diff2, (v16u8) temp0, (v16u8) temp0);
                temp1 = str_x2 - diff2;
                str = __msa_fill_h(strength);
                temp0 = (str < diff2);
                diff2 = (v8i16) __msa_bmnz_v((v16u8) diff2, (v16u8) temp1, (v16u8) temp0);
                temp0 = __msa_clti_s_h(diff0, 0);
                d0 = (v8i16) __msa_bmnz_v((v16u8) diff2, (v16u8) diff1, (v16u8) temp0);
                diff1 = - diff1 >> 1;
                diff2 = diff2 >> 1;
                diff4 = (v8i16) __msa_bmnz_v((v16u8) diff2, (v16u8) diff1, (v16u8) temp0);
                diff3 = (- a_d0) >> 2;
                diff3 = - (diff3);
                temp1 = - diff4;
                temp0 = (diff3 < temp1);
                diff3 = (v8i16) __msa_bmnz_v((v16u8) diff3, (v16u8) temp1, (v16u8) temp0);
                diff1 = a_d0 >> 2;
                temp0 = (diff1 <= diff4);
                diff1 = (v8i16) __msa_bmz_v((v16u8) diff1, (v16u8) diff4, (v16u8) temp0);
                temp0 = __msa_clti_s_h(a_d0, 0);
                diff3 = (v8i16) __msa_bmz_v((v16u8) diff3, (v16u8) diff1, (v16u8) temp0);
                PCKEV_B2_SH(diff3, diff3, d0, d0, diff3, d0);
                PCKEV_B2_SH(diff3, diff3, d0, d0, diff3, d0);
                temp2 = (v8i16) ((v16i8) in0 - (v16i8) diff3);
                temp3 = (v8i16) ((v16i8) in1 + (v16i8) diff3);
                temp5 = (v8i16) __msa_xori_b(in3, 128);
                temp5 = (v8i16) __msa_adds_s_b((v16i8) temp5, (v16i8) d0);
                temp5 = (v8i16) __msa_xori_b((v16u8) temp5, 128);
                temp4 = (v8i16) __msa_subsus_u_b(in2, (v16i8) d0);
                res0 = __msa_copy_u_d((v2i64) temp2, 0);
                res1 = __msa_copy_u_d((v2i64) temp5, 0);
                res2 = __msa_copy_u_d((v2i64) temp4, 0);
                res3 = __msa_copy_u_d((v2i64) temp3, 0);
                SD4(res0, res1, res2, res3, rec_y - width2, width);
            }

            rec_y += b_size;
            mb_num++;
        }

        rec_y += ((b_size - 1) * width);
    }

    /* horizontal edge */
    rec_y = rec + b_size;
    offset = width * b_size - b_size;
    mb_num = 1;
    for (col = num_mb_per_col; col--;)
    {
        for (row = num_mb_per_row - 1; row--;)
        {
            if (MODE_SKIPPED != mode[mb_num] ||
                MODE_SKIPPED != mode[mb_num - 1])
            {
                uint8 *rec_store = rec_y - 2;
                uint8 *rec_load = rec_y - 2;
                int64_t temp;
                v16u8 in0, in1, in2, in3, in4, in5, in6, in7;
                v8i16 temp0, temp1, temp2;
                v8i16 diff0, diff2, diff4, diff6, diff8;
                v8i16 d0, str_x2, str, a_d0;
                uint32 strength_tab_idx;

                if (MODE_SKIPPED != mode[mb_num])
                {
                    strength_tab_idx =
                        annex_t ? MQ_chroma_QP_table[qp_store[mb_num]] :
                        qp_store[mb_num];
                }
                else
                {
                    strength_tab_idx =
                        annex_t ? MQ_chroma_QP_table[qp_store[mb_num - 1]] :
                        qp_store[mb_num - 1];
                }

                strength = h263_loop_filter_strength_msa[strength_tab_idx];
                temp = LD(rec_load);
                rec_load += width;
                in0 = (v16u8) __msa_fill_d(temp);
                temp = LD(rec_load);
                rec_load += width;
                in1 = (v16u8) __msa_fill_d(temp);
                temp = LD(rec_load);
                rec_load += width;
                in2 = (v16u8) __msa_fill_d(temp);
                temp = LD(rec_load);
                rec_load += width;
                in3 = (v16u8) __msa_fill_d(temp);
                temp = LD(rec_load);
                rec_load += width;
                in4 = (v16u8) __msa_fill_d(temp);
                temp = LD(rec_load);
                rec_load += width;
                in5 = (v16u8) __msa_fill_d(temp);
                temp = LD(rec_load);
                rec_load += width;
                in6 = (v16u8) __msa_fill_d(temp);
                temp = LD(rec_load);
                rec_load += width;
                in7 = (v16u8) __msa_fill_d(temp);
                TRANSPOSE8x4_UB_UB(in0, in1, in2, in3, in4, in5, in6, in7,
                                   in0, in3, in2, in1);

                temp0 = (v8i16) __msa_ilvr_b((v16i8) in0, (v16i8) in1);
                a_d0 = __msa_hsub_u_h((v16u8) temp0, (v16u8) temp0);
                temp2 = (v8i16) __msa_ilvr_b((v16i8) in2, (v16i8) in3);
                temp2 = __msa_hsub_u_h((v16u8) temp2, (v16u8) temp2);
                temp2 <<= 2;
                diff0 = a_d0 + temp2;
                diff2 = -(-diff0 >> 3);
                str_x2 = __msa_fill_h(-(strength << 1));
                temp0 = (str_x2 <= diff2);
                diff2 = (v8i16) __msa_bmz_v((v16u8) diff2, (v16u8) temp0, (v16u8) temp0);
                temp2 = str_x2 - diff2;
                str = __msa_fill_h(-strength);
                temp0 = (diff2 < str);
                diff2 = (v8i16) __msa_bmnz_v((v16u8) diff2, (v16u8) temp2, (v16u8) temp0);
                diff4 = diff0 >> 3;
                str_x2 = __msa_fill_h(strength << 1);
                temp0 = (diff4 <= str_x2);
                diff4 = (v8i16) __msa_bmz_v((v16u8) diff4, (v16u8) temp0, (v16u8) temp0);
                temp2 = str_x2 - diff4;
                str = __msa_fill_h(strength);
                temp0 = (str < diff4);
                diff4 = (v8i16) __msa_bmnz_v((v16u8) diff4, (v16u8) temp2, (v16u8) temp0);
                temp0 = __msa_clti_s_h(diff0, 0);
                d0 = (v8i16) __msa_bmnz_v((v16u8) diff4, (v16u8) diff2, (v16u8) temp0);
                diff2 = -diff2 >> 1;
                diff4 >>= 1;
                diff8 = (v8i16) __msa_bmnz_v((v16u8) diff4, (v16u8) diff2, (v16u8) temp0);
                diff6 = (-a_d0) >> 2;
                diff6 = -(diff6);
                temp2 = -diff8;
                temp0 = (diff6 < temp2);
                diff6 = (v8i16) __msa_bmnz_v((v16u8) diff6, (v16u8) temp2, (v16u8) temp0);
                diff2 = a_d0 >> 2;
                temp0 = (diff2 <= diff8);
                diff2 = (v8i16) __msa_bmz_v((v16u8) diff2, (v16u8) diff8, (v16u8) temp0);
                temp0 = __msa_clti_s_h(a_d0, 0);
                diff6 = (v8i16) __msa_bmz_v((v16u8) diff6, (v16u8) diff2, (v16u8) temp0);
                PCKEV_B2_SH(diff6, diff6, d0, d0, diff6, d0);
                in0 = (v16u8) ((v16i8) in0 - (v16i8) diff6);
                in1 = (v16u8) ((v16i8) in1 + (v16i8) diff6);
                in3 = __msa_xori_b(in3, 128);
                in3 = (v16u8) __msa_adds_s_b((v16i8) in3, (v16i8) d0);
                in3 = __msa_xori_b(in3, 128);
                in2 = __msa_subsus_u_b(in2, (v16i8) d0);
                ILVR_B2_SH(in3, in0, in1, in2, temp0, temp1);
                in0 = (v16u8) __msa_ilvr_h(temp1, temp0);
                in3 = (v16u8) __msa_ilvl_h(temp1, temp0);
                ST4x4_UB(in0, in0,  0, 1, 2, 3, rec_store, width);
                rec_store += 4 * width;
                ST4x4_UB(in3, in3,  0, 1, 2, 3, rec_store, width);
                rec_store += 4 * width;

                rec_y += 8 * width;
                rec_y -= offset;
            }
            else  // if !(mode[mb_num] != MODE_SKIPPED ||
                  // mode[mb_num-1] != MODE_SKIPPED)
            {
                rec_y += b_size;
            }

            mb_num++;
        }

        rec_y += ((width * (b_size - 1)) + b_size);
        mb_num++;
    }

    return;
}
#endif /* M4VH263DEC_MSA */
