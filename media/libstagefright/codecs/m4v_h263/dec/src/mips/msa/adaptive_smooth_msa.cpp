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
#include "post_proc.h"
#include "mp4def.h"
#include "common_macros_msa.h"
#include "prototypes_msa.h"

#define OSCL_DISABLE_WARNING_CONV_POSSIBLE_LOSS_OF_DATA

#ifdef PV_POSTPROC_ON

#ifdef M4VH263DEC_MSA
#define PCKEV_B_STORE_4_BYTES(in1, in2, pdest)            \
{                                                         \
    uint32 out_m;                                         \
    v16i8 tmp_m;                                          \
                                                          \
    tmp_m = __msa_pckev_b((v16i8) (in1), (v16i8) (in2));  \
    out_m = __msa_copy_u_w((v4i32) tmp_m, 0);             \
    SW(out_m, (pdest));                                   \
}

#define PCKEV_B_STORE_8_BYTES(in1, in2, pdest)            \
{                                                         \
    v16i8 tmp_m;                                          \
                                                          \
    tmp_m = __msa_pckev_b((v16i8) (in1), (v16i8) (in2));  \
    ST8x1_UB(tmp_m, pdest);                               \
}

void DeringAdaptiveSmooth4ColsMSA(uint8 *recon_y, int rows, int threshold,
                                  int width, int max_diff)
{
    int row_cnt;
    uint8 *recon_y_start;
    v16u8 pel_top, pel_cur, pel_bot;
    v16i8 zero = { 0 };
    v8i16 pel_sumr, pel_signr, pel_topr, pel_curr, pel_botr;
    v8i16 three_row_pel_sum, three_row_pel_sign, max_diff_vec;
    v8i16 threshold_vec, pel_diff, minus_max_diff;
    v8i16 pel_cur_updated, pel_out;
    v8i16 cur_minus_maxdiff, cur_plus_maxdiff;
    v8i16 diff_grt, diff_less, is_update_out_for0, is_update_out_for9;
    v8i16 pel_sum_shift1 = { 0 };
    v8i16 pel_sign_shift1 = { 0 };
    v8i16 pel_sum_shift2 = { 0 };
    v8i16 pel_sign_shift2 = { 0 };
    v8i16 pel_top_cmp, pel_cur_cmp, pel_bot_cmp;

    recon_y_start = recon_y;
    recon_y += width;

    max_diff_vec = __msa_fill_h(max_diff);
    threshold_vec = __msa_fill_h(threshold);

    minus_max_diff = (v8i16) zero - max_diff_vec;

    LD_UB3(recon_y_start - width, width, pel_top, pel_cur, pel_bot);

    ILVR_B3_SH(zero, pel_top, zero, pel_cur, zero, pel_bot,
               pel_topr, pel_curr, pel_botr);

    pel_out = (v8i16) __msa_sldi_b((v16i8) pel_curr, (v16i8) pel_curr, 2);

    pel_sumr = pel_topr + (pel_curr << 1) + pel_botr;

    pel_top_cmp = (threshold_vec <= pel_topr);
    pel_cur_cmp = (threshold_vec <= pel_curr);
    pel_bot_cmp = (threshold_vec <= pel_botr);
    pel_signr = pel_top_cmp + pel_cur_cmp + pel_bot_cmp;
    pel_signr = (v8i16) zero - pel_signr;

    pel_sum_shift1 = (v8i16) __msa_sldi_b(zero, (v16i8) pel_sumr, 2);
    pel_sign_shift1 = (v8i16) __msa_sldi_b(zero, (v16i8) pel_signr, 2);
    pel_sum_shift2 = (v8i16) __msa_sldi_b(zero, (v16i8) pel_sumr, 4);
    pel_sign_shift2 = (v8i16) __msa_sldi_b(zero, (v16i8) pel_signr, 4);

    three_row_pel_sign = pel_signr + pel_sign_shift1 + pel_sign_shift2;
    three_row_pel_sum = pel_sumr + (pel_sum_shift1 << 1) + pel_sum_shift2 + 8;
    three_row_pel_sum >>= 4;

    cur_minus_maxdiff = pel_out - max_diff_vec;
    cur_plus_maxdiff = pel_out + max_diff_vec;
    pel_diff = pel_out - three_row_pel_sum;

    diff_grt = (max_diff_vec <= pel_diff);
    diff_less = (pel_diff <= minus_max_diff);

    pel_cur_updated = three_row_pel_sum;
    pel_cur_updated = (v8i16) __msa_bmnz_v((v16u8) pel_cur_updated,
                                           (v16u8) cur_minus_maxdiff,
                                           (v16u8) diff_grt);
    pel_cur_updated = (v8i16) __msa_bmnz_v((v16u8) pel_cur_updated,
                                           (v16u8) cur_plus_maxdiff,
                                           (v16u8) diff_less);

    pel_cur_updated = CLIP_SH_0_255(pel_cur_updated);

    is_update_out_for0 = __msa_ceqi_h(three_row_pel_sign, 0);
    is_update_out_for9 = __msa_ceqi_h(three_row_pel_sign, 9);

    pel_out = (v8i16) __msa_bmnz_v((v16u8) pel_out,
                                   (v16u8) pel_cur_updated,
                                   (v16u8) is_update_out_for0);
    pel_out = (v8i16) __msa_bmnz_v((v16u8) pel_out,
                                   (v16u8) pel_cur_updated,
                                   (v16u8) is_update_out_for9);

    PCKEV_B_STORE_4_BYTES(pel_out, pel_out, &recon_y_start[1]);

    for (row_cnt = rows; row_cnt > 0; row_cnt--)
    {
        recon_y_start = recon_y;
        recon_y += width;

        pel_bot = LD_UB(recon_y_start + width);

        pel_topr = pel_curr;
        pel_curr = pel_botr;

        pel_out = (v8i16) __msa_sldi_b((v16i8) pel_curr, (v16i8) pel_curr, 2);
        pel_botr = (v8i16) __msa_ilvr_b(zero, (v16i8) pel_bot);
        pel_sumr = pel_topr + (pel_curr << 1) + pel_botr;

        pel_top_cmp = (threshold_vec <= pel_topr);
        pel_cur_cmp = (threshold_vec <= pel_curr);
        pel_bot_cmp = (threshold_vec <= pel_botr);
        pel_signr = pel_top_cmp + pel_cur_cmp + pel_bot_cmp;
        pel_signr = (v8i16) zero - pel_signr;

        pel_sum_shift1 = (v8i16) __msa_sldi_b(zero, (v16i8) pel_sumr, 2);
        pel_sum_shift2 = (v8i16) __msa_sldi_b(zero, (v16i8) pel_sumr, 4);
        pel_sign_shift1 = (v8i16) __msa_sldi_b(zero, (v16i8) pel_signr, 2);
        pel_sign_shift2 = (v8i16) __msa_sldi_b(zero, (v16i8) pel_signr, 4);

        three_row_pel_sign = pel_signr + pel_sign_shift1 + pel_sign_shift2;
        three_row_pel_sum = pel_sumr + (pel_sum_shift1 << 1) + pel_sum_shift2 + 8;
        three_row_pel_sum >>= 4;

        cur_minus_maxdiff = pel_out - max_diff_vec;
        cur_plus_maxdiff = pel_out + max_diff_vec;
        pel_diff = pel_out - three_row_pel_sum;

        diff_grt = (max_diff_vec <= pel_diff);
        diff_less = (pel_diff <= minus_max_diff);

        pel_cur_updated = three_row_pel_sum;
        pel_cur_updated = (v8i16) __msa_bmnz_v((v16u8) pel_cur_updated,
                                               (v16u8) cur_minus_maxdiff,
                                               (v16u8) diff_grt);
        pel_cur_updated = (v8i16) __msa_bmnz_v((v16u8) pel_cur_updated,
                                               (v16u8) cur_plus_maxdiff,
                                               (v16u8) diff_less);

        pel_cur_updated = CLIP_SH_0_255(pel_cur_updated);

        is_update_out_for0 = __msa_ceqi_h(three_row_pel_sign, 0);
        is_update_out_for9 = __msa_ceqi_h(three_row_pel_sign, 9);

        pel_out = (v8i16) __msa_bmnz_v((v16u8) pel_out,
                                       (v16u8) pel_cur_updated,
                                       (v16u8) is_update_out_for0);
        pel_out = (v8i16) __msa_bmnz_v((v16u8) pel_out,
                                       (v16u8) pel_cur_updated,
                                       (v16u8) is_update_out_for9);

        PCKEV_B_STORE_4_BYTES(pel_out, pel_out, &recon_y_start[1]);
    }

    return;
}

void DeringAdaptiveSmooth6ColsMSA(uint8 *recon_y, int rows, int threshold,
                                  int width, int max_diff)
{
    int row_cnt;
    uint8 *recon_y_start;
    uint32 org_rec;
    v16u8 pel_top, pel_cur, pel_bot;
    v16i8 zero = { 0 };
    v8i16 pel_curl, pel_botl, pel_sumr, pel_signr;
    v8i16 pel_topr, pel_curr, pel_botr;
    v8i16 three_row_pel_sum, three_row_pel_sign, max_diff_vec;
    v8i16 threshold_vec, pel_diff, minus_max_diff;
    v8i16 pel_cur_updated, pel_out;
    v8i16 cur_minus_maxdiff, cur_plus_maxdiff;
    v8i16 diff_grt, diff_less, is_update_out_for0, is_update_out_for9;
    v8i16 pel_sum_shift1 = { 0 };
    v8i16 pel_sign_shift1 = { 0 };
    v8i16 pel_sum_shift2 = { 0 };
    v8i16 pel_sign_shift2 = { 0 };
    v8i16 pel_top_cmp, pel_cur_cmp, pel_bot_cmp;

    recon_y_start = recon_y;
    recon_y += width;

    max_diff_vec = __msa_fill_h(max_diff);
    threshold_vec = __msa_fill_h(threshold);

    minus_max_diff = (v8i16) zero - max_diff_vec;

    LD_UB3(recon_y_start - width, width, pel_top, pel_cur, pel_bot);

    pel_topr = (v8i16) __msa_ilvr_b(zero, (v16i8) pel_top);
    UNPCK_UB_SH(pel_cur, pel_curr, pel_curl);
    UNPCK_UB_SH(pel_bot, pel_botr, pel_botl);

    pel_out = (v8i16) __msa_sldi_b((v16i8) pel_curl, (v16i8) pel_curr, 2);

    org_rec = __msa_copy_u_w((v4i32) pel_out, 3);
    pel_sumr = pel_topr + (pel_curr << 1) + pel_botr;

    pel_top_cmp = (threshold_vec <= pel_topr);
    pel_cur_cmp = (threshold_vec <= pel_curr);
    pel_bot_cmp = (threshold_vec <= pel_botr);
    pel_signr = pel_top_cmp + pel_cur_cmp + pel_bot_cmp;
    pel_signr = (v8i16) zero - pel_signr;

    pel_sum_shift1 = (v8i16) __msa_sldi_b(zero, (v16i8) pel_sumr, 2);
    pel_sum_shift2 = (v8i16) __msa_sldi_b(zero, (v16i8) pel_sumr, 4);
    pel_sign_shift1 = (v8i16) __msa_sldi_b(zero, (v16i8) pel_signr, 2);
    pel_sign_shift2 = (v8i16) __msa_sldi_b(zero, (v16i8) pel_signr, 4);

    three_row_pel_sign = pel_signr + pel_sign_shift1 + pel_sign_shift2;
    three_row_pel_sum = pel_sumr + (pel_sum_shift1 << 1) + pel_sum_shift2 + 8;
    three_row_pel_sum >>= 4;

    cur_minus_maxdiff = pel_out - max_diff_vec;
    cur_plus_maxdiff = pel_out + max_diff_vec;
    pel_diff = pel_out - three_row_pel_sum;

    diff_grt = (max_diff_vec <= pel_diff);
    diff_less = (pel_diff <= minus_max_diff);

    pel_cur_updated = three_row_pel_sum;
    pel_cur_updated = (v8i16) __msa_bmnz_v((v16u8) pel_cur_updated,
                                           (v16u8) cur_minus_maxdiff,
                                           (v16u8) diff_grt);
    pel_cur_updated = (v8i16) __msa_bmnz_v((v16u8) pel_cur_updated,
                                           (v16u8) cur_plus_maxdiff,
                                           (v16u8) diff_less);

    pel_cur_updated = CLIP_SH_0_255(pel_cur_updated);

    is_update_out_for0 = __msa_ceqi_h(three_row_pel_sign, 0);
    is_update_out_for9 = __msa_ceqi_h(three_row_pel_sign, 9);

    pel_out = (v8i16) __msa_bmnz_v((v16u8) pel_out,
                                   (v16u8) pel_cur_updated,
                                   (v16u8) is_update_out_for0);
    pel_out = (v8i16) __msa_bmnz_v((v16u8) pel_out,
                                   (v16u8) pel_cur_updated,
                                   (v16u8) is_update_out_for9);
    pel_out = (v8i16) __msa_insert_w((v4i32) pel_out, 3, org_rec);

    PCKEV_B_STORE_8_BYTES(pel_out, pel_out, &recon_y_start[1]);

    for (row_cnt = rows; row_cnt > 0; row_cnt--)
    {
        recon_y_start = recon_y;
        recon_y += width;

        pel_bot = LD_UB(recon_y_start + width);

        pel_topr = pel_curr;
        pel_curl = pel_botl;
        pel_curr = pel_botr;

        pel_out = (v8i16) __msa_sldi_b((v16i8) pel_curl, (v16i8) pel_curr, 2);

        UNPCK_UB_SH(pel_bot, pel_botr, pel_botl);

        org_rec = __msa_copy_u_w((v4i32) pel_out, 3);
        pel_sumr = pel_topr + (pel_curr << 1) + pel_botr;

        pel_top_cmp = (threshold_vec <= pel_topr);
        pel_cur_cmp = (threshold_vec <= pel_curr);
        pel_bot_cmp = (threshold_vec <= pel_botr);
        pel_signr = pel_top_cmp + pel_cur_cmp + pel_bot_cmp;
        pel_signr = (v8i16) zero - pel_signr;

        pel_sum_shift1 = (v8i16) __msa_sldi_b(zero, (v16i8) pel_sumr, 2);
        pel_sum_shift2 = (v8i16) __msa_sldi_b(zero, (v16i8) pel_sumr, 4);
        pel_sign_shift1 = (v8i16) __msa_sldi_b(zero, (v16i8) pel_signr, 2);
        pel_sign_shift2 = (v8i16) __msa_sldi_b(zero, (v16i8) pel_signr, 4);

        three_row_pel_sign = pel_signr + pel_sign_shift1 + pel_sign_shift2;
        three_row_pel_sum = pel_sumr + (pel_sum_shift1 << 1) + pel_sum_shift2 + 8;
        three_row_pel_sum >>= 4;

        cur_minus_maxdiff = pel_out - max_diff_vec;
        cur_plus_maxdiff = pel_out + max_diff_vec;
        pel_diff = pel_out - three_row_pel_sum;

        diff_grt = (max_diff_vec <= pel_diff);
        diff_less = (pel_diff <= minus_max_diff);

        pel_cur_updated = three_row_pel_sum;
        pel_cur_updated = (v8i16) __msa_bmnz_v((v16u8) pel_cur_updated,
                                               (v16u8) cur_minus_maxdiff,
                                               (v16u8) diff_grt);
        pel_cur_updated = (v8i16) __msa_bmnz_v((v16u8) pel_cur_updated,
                                               (v16u8) cur_plus_maxdiff,
                                               (v16u8) diff_less);

        pel_cur_updated = CLIP_SH_0_255(pel_cur_updated);

        is_update_out_for0 = __msa_ceqi_h(three_row_pel_sign, 0);
        is_update_out_for9 = __msa_ceqi_h(three_row_pel_sign, 9);

        pel_out = (v8i16) __msa_bmnz_v((v16u8) pel_out,
                                       (v16u8) pel_cur_updated,
                                       (v16u8) is_update_out_for0);
        pel_out = (v8i16) __msa_bmnz_v((v16u8) pel_out,
                                       (v16u8) pel_cur_updated,
                                       (v16u8) is_update_out_for9);
        pel_out = (v8i16) __msa_insert_w((v4i32) pel_out, 3, org_rec);

        PCKEV_B_STORE_8_BYTES(pel_out, pel_out, &recon_y_start[1]);
    }

    return;
}

void DeringAdaptiveSmooth8ColsMSA(uint8 *recon_y, int rows, int threshold,
                                  int width, int max_diff)
{
    int row_cnt;
    uint8 *recon_y_start;
    v16u8 pel_top, pel_cur, pel_bot;
    v16u8 zero = { 0 };
    v8i16 pel_topl, pel_curl, pel_botl, pel_sumr, pel_signr;
    v8i16 pel_topr, pel_curr, pel_botr, pel_suml, pel_signl;
    v8i16 three_row_pel_sum, three_row_pel_sign, max_diff_vec;
    v8i16 threshold_vec, pel_diff, minus_max_diff;
    v8i16 pel_cur_updated, pel_out;
    v8i16 cur_minus_maxdiff, cur_plus_maxdiff;
    v8i16 diff_grt, diff_less, is_update_out_for0, is_update_out_for9;
    v8i16 pel_sum_shift1 = { 0 };
    v8i16 pel_sign_shift1 = { 0 };
    v8i16 pel_sum_shift2 = { 0 };
    v8i16 pel_sign_shift2 = { 0 };

    v8i16 pel_top_cmp, pel_cur_cmp, pel_bot_cmp;

    recon_y_start = recon_y;
    recon_y += width;

    max_diff_vec = __msa_fill_h(max_diff);
    threshold_vec = __msa_fill_h(threshold);

    minus_max_diff = (v8i16) zero - max_diff_vec;

    LD_UB3(recon_y_start - width, width, pel_top, pel_cur, pel_bot);

    UNPCK_UB_SH(pel_top, pel_topr, pel_topl);
    UNPCK_UB_SH(pel_cur, pel_curr, pel_curl);
    UNPCK_UB_SH(pel_bot, pel_botr, pel_botl);

    pel_out = (v8i16) __msa_sldi_b((v16i8) pel_curl, (v16i8) pel_curr, 2);

    pel_sumr = pel_topr + (pel_curr << 1) + pel_botr;
    pel_suml = pel_topl + (pel_curl << 1) + pel_botl;

    pel_top_cmp = (threshold_vec <= pel_topr);
    pel_cur_cmp = (threshold_vec <= pel_curr);
    pel_bot_cmp = (threshold_vec <= pel_botr);
    pel_signr = pel_top_cmp + pel_cur_cmp + pel_bot_cmp;
    pel_top_cmp = (threshold_vec <= pel_topl);
    pel_cur_cmp = (threshold_vec <= pel_curl);
    pel_bot_cmp = (threshold_vec <= pel_botl);
    pel_signl = pel_top_cmp + pel_cur_cmp + pel_bot_cmp;
    pel_signr = (v8i16) zero - pel_signr;
    pel_signl = (v8i16) zero - pel_signl;

    pel_sum_shift1 = (v8i16) __msa_sldi_b((v16i8) pel_suml,
                                          (v16i8) pel_sumr, 2);
    pel_sum_shift2 = (v8i16) __msa_sldi_b((v16i8) pel_suml,
                                          (v16i8) pel_sumr, 4);
    pel_sign_shift1 = (v8i16) __msa_sldi_b((v16i8) pel_signl,
                                           (v16i8) pel_signr, 2);
    pel_sign_shift2 = (v8i16) __msa_sldi_b((v16i8) pel_signl,
                                           (v16i8) pel_signr, 4);

    three_row_pel_sign = pel_signr + pel_sign_shift1 + pel_sign_shift2;
    three_row_pel_sum = pel_sumr + (pel_sum_shift1 << 1) + pel_sum_shift2 + 8;
    three_row_pel_sum >>= 4;

    cur_minus_maxdiff = pel_out - max_diff_vec;
    cur_plus_maxdiff = pel_out + max_diff_vec;
    pel_diff = pel_out - three_row_pel_sum;

    diff_grt = (max_diff_vec <= pel_diff);
    diff_less = (pel_diff <= minus_max_diff);

    pel_cur_updated = three_row_pel_sum;
    pel_cur_updated = (v8i16) __msa_bmnz_v((v16u8) pel_cur_updated,
                                           (v16u8) cur_minus_maxdiff,
                                           (v16u8) diff_grt);
    pel_cur_updated = (v8i16) __msa_bmnz_v((v16u8) pel_cur_updated,
                                           (v16u8) cur_plus_maxdiff,
                                           (v16u8) diff_less);

    pel_cur_updated = CLIP_SH_0_255(pel_cur_updated);

    is_update_out_for0 = __msa_ceqi_h(three_row_pel_sign, 0);
    is_update_out_for9 = __msa_ceqi_h(three_row_pel_sign, 9);

    pel_out = (v8i16) __msa_bmnz_v((v16u8) pel_out,
                               (v16u8) pel_cur_updated,
                               (v16u8) is_update_out_for0);
    pel_out = (v8i16) __msa_bmnz_v((v16u8) pel_out,
                               (v16u8) pel_cur_updated,
                               (v16u8) is_update_out_for9);

    PCKEV_B_STORE_8_BYTES(pel_out, pel_out, &recon_y_start[1]);

    for (row_cnt = rows; row_cnt--;)
    {
        recon_y_start = recon_y;
        recon_y += width;

        pel_bot = LD_UB(recon_y_start + width);

        pel_topl = pel_curl;
        pel_topr = pel_curr;
        pel_curl = pel_botl;
        pel_curr = pel_botr;

        pel_out = (v8i16) __msa_sldi_b((v16i8) pel_curl, (v16i8) pel_curr, 2);

        UNPCK_UB_SH(pel_bot, pel_botr, pel_botl);

        pel_sumr = pel_topr + (pel_curr << 1) + pel_botr;
        pel_suml = pel_topl + (pel_curl << 1) + pel_botl;

        pel_top_cmp = (threshold_vec <= pel_topr);
        pel_cur_cmp = (threshold_vec <= pel_curr);
        pel_bot_cmp = (threshold_vec <= pel_botr);
        pel_signr = pel_top_cmp + pel_cur_cmp + pel_bot_cmp;
        pel_top_cmp = (threshold_vec <= pel_topl);
        pel_cur_cmp = (threshold_vec <= pel_curl);
        pel_bot_cmp = (threshold_vec <= pel_botl);
        pel_signl = pel_top_cmp + pel_cur_cmp + pel_bot_cmp;
        pel_signr = (v8i16) zero - pel_signr;
        pel_signl = (v8i16) zero - pel_signl;

        pel_sum_shift1 = (v8i16) __msa_sldi_b((v16i8) pel_suml,
                                              (v16i8) pel_sumr, 2);
        pel_sum_shift2 = (v8i16) __msa_sldi_b((v16i8) pel_suml,
                                              (v16i8) pel_sumr, 4);
        pel_sign_shift1 = (v8i16) __msa_sldi_b((v16i8) pel_signl,
                                               (v16i8) pel_signr, 2);
        pel_sign_shift2 = (v8i16) __msa_sldi_b((v16i8) pel_signl,
                                               (v16i8) pel_signr, 4);

        three_row_pel_sign = pel_signr + pel_sign_shift1 + pel_sign_shift2;
        three_row_pel_sum = pel_sumr + (pel_sum_shift1 << 1) + pel_sum_shift2 + 8;
        three_row_pel_sum >>= 4;

        cur_minus_maxdiff = pel_out - max_diff_vec;
        cur_plus_maxdiff = pel_out + max_diff_vec;
        pel_diff = pel_out - three_row_pel_sum;

        diff_grt = (max_diff_vec <= pel_diff);
        diff_less = (pel_diff <= minus_max_diff);

        pel_cur_updated = three_row_pel_sum;
        pel_cur_updated = (v8i16) __msa_bmnz_v((v16u8) pel_cur_updated,
                                               (v16u8) cur_minus_maxdiff,
                                               (v16u8) diff_grt);
        pel_cur_updated = (v8i16) __msa_bmnz_v((v16u8) pel_cur_updated,
                                               (v16u8) cur_plus_maxdiff,
                                               (v16u8) diff_less);

        pel_cur_updated = CLIP_SH_0_255(pel_cur_updated);

        is_update_out_for0 = __msa_ceqi_h(three_row_pel_sign, 0);
        is_update_out_for9 = __msa_ceqi_h(three_row_pel_sign, 9);

        pel_out = (v8i16) __msa_bmnz_v((v16u8) pel_out,
                                       (v16u8) pel_cur_updated,
                                       (v16u8) is_update_out_for0);
        pel_out = (v8i16) __msa_bmnz_v((v16u8) pel_out,
                                       (v16u8) pel_cur_updated,
                                       (v16u8) is_update_out_for9);

        PCKEV_B_STORE_8_BYTES(pel_out, pel_out, &recon_y_start[1]);
    }

    return;
}

void PostProcCr6ColsMSA(uint8 *recon_c, int rows, int threshold,
                        int width, int max_diff)
{
    int row_cnt;
    uint8 *recon_c_start;
    uint32 org_rec;
    v16u8 pel_top, pel_cur, pel_bot;
    v16i8 zero = { 0 };
    v8i16 pel_curl, pel_sumr, pel_signr;
    v8i16 pel_topr, pel_curr, pel_botr;
    v8i16 three_row_pel_sum, three_row_pel_sign, max_diff_vec;
    v8i16 threshold_vec, abspel_minus_sum;
    v8i16 pel_cur_updated, pel_cur_updated_with_sum, pel_out;
    v8i16 sum_grt;
    v8i16 cur_plus_maxdiff;
    v8i16 diff_grt, is_update_out_for0, is_update_out_for9;
    v8i16 pel_sum_shift1 = { 0 };
    v8i16 pel_sign_shift1 = { 0 };
    v8i16 pel_sum_shift2 = { 0 };
    v8i16 pel_sign_shift2 = { 0 };
    v8i16 pel_top_cmp, pel_cur_cmp, pel_bot_cmp;

    max_diff_vec = __msa_fill_h(max_diff);
    threshold_vec = __msa_fill_h(threshold);

    recon_c_start = recon_c;

    for (row_cnt = rows; row_cnt--;)
    {
        LD_UB3(recon_c_start - width, width, pel_top, pel_cur, pel_bot);

        UNPCK_UB_SH(pel_cur, pel_curr, pel_curl);
        ILVR_B2_SH(zero, pel_top, zero, pel_bot, pel_topr, pel_botr);

        pel_out = (v8i16) __msa_sldi_b((v16i8) pel_curl, (v16i8) pel_curr, 2);

        org_rec = __msa_copy_u_w((v4i32) pel_out, 3);
        pel_sumr = pel_topr + (pel_curr << 1) + pel_botr;

        pel_top_cmp = (threshold_vec <= pel_topr);
        pel_cur_cmp = (threshold_vec <= pel_curr);
        pel_bot_cmp = (threshold_vec <= pel_botr);
        pel_signr = pel_top_cmp + pel_cur_cmp + pel_bot_cmp;
        pel_signr = (v8i16) zero - pel_signr;

        pel_sum_shift1 = (v8i16) __msa_sldi_b(zero, (v16i8) pel_sumr, 2);
        pel_sum_shift2 = (v8i16) __msa_sldi_b(zero, (v16i8) pel_sumr, 4);
        pel_sign_shift1 = (v8i16) __msa_sldi_b(zero, (v16i8) pel_signr, 2);
        pel_sign_shift2 = (v8i16) __msa_sldi_b(zero, (v16i8) pel_signr, 4);

        three_row_pel_sign = pel_signr + pel_sign_shift1 + pel_sign_shift2;
        three_row_pel_sum = pel_sumr + (pel_sum_shift1 << 1) + pel_sum_shift2 + 8;
        three_row_pel_sum >>= 4;

        abspel_minus_sum = __msa_asub_s_h(pel_out, three_row_pel_sum);
        diff_grt = __msa_clt_s_h(max_diff_vec, abspel_minus_sum);
        sum_grt = (pel_out <= three_row_pel_sum);

        pel_cur_updated_with_sum = pel_out - max_diff_vec;
        cur_plus_maxdiff = pel_out + max_diff_vec;

        pel_cur_updated = three_row_pel_sum;
        pel_cur_updated_with_sum = (v8i16) __msa_bmnz_v((v16u8) pel_cur_updated_with_sum,
                                                        (v16u8) cur_plus_maxdiff,
                                                        (v16u8) sum_grt);
        pel_cur_updated = (v8i16) __msa_bmnz_v((v16u8) pel_cur_updated,
                                               (v16u8) pel_cur_updated_with_sum,
                                               (v16u8) diff_grt);

        pel_cur_updated = CLIP_SH_0_255(pel_cur_updated);

        is_update_out_for0 = __msa_ceqi_h(three_row_pel_sign, 0);
        is_update_out_for9 = __msa_ceqi_h(three_row_pel_sign, 9);

        pel_out = (v8i16) __msa_bmnz_v((v16u8) pel_out,
                                       (v16u8) pel_cur_updated,
                                       (v16u8) is_update_out_for0);
        pel_out = (v8i16) __msa_bmnz_v((v16u8) pel_out,
                                       (v16u8) pel_cur_updated,
                                       (v16u8) is_update_out_for9);
        pel_out = (v8i16) __msa_insert_w((v4i32) pel_out, 3, org_rec);

        PCKEV_B_STORE_8_BYTES(pel_out, pel_out, &recon_c_start[1]);

        recon_c_start += width;
    }

    return;
}

void PostProcCr8ColsMSA(uint8 *recon_c, int rows, int threshold,
                        int width, int max_diff)
{
    int row_cnt;
    uint8 *recon_c_start;
    v16u8 pel_top, pel_cur, pel_bot;
    v16i8 zero = { 0 };
    v8i16 pel_topl, pel_curl, pel_botl, pel_sumr, pel_signr;
    v8i16 pel_topr, pel_curr, pel_botr, pel_suml, pel_signl;
    v8i16 three_row_pel_sum, three_row_pel_sign, max_diff_vec;
    v8i16 threshold_vec, abspel_minus_sum;
    v8i16 pel_cur_updated, pel_cur_updated_with_sum, pel_out;
    v8i16 sum_grt, cur_plus_maxdiff;
    v8i16 diff_grt, is_update_out_for0, is_update_out_for9;
    v8i16 pel_sum_shift1 = { 0 };
    v8i16 pel_sign_shift1 = { 0 };
    v8i16 pel_sum_shift2 = { 0 };
    v8i16 pel_sign_shift2 = { 0 };
    v8i16 pel_top_cmp, pel_cur_cmp, pel_bot_cmp;

    max_diff_vec = __msa_fill_h(max_diff);
    threshold_vec = __msa_fill_h(threshold);

    recon_c_start = recon_c;

    for (row_cnt = rows; row_cnt > 0; row_cnt--)
    {
        LD_UB3(recon_c_start - width, width, pel_top, pel_cur, pel_bot);

        UNPCK_UB_SH(pel_top, pel_topr, pel_topl);
        UNPCK_UB_SH(pel_cur, pel_curr, pel_curl);
        UNPCK_UB_SH(pel_bot, pel_botr, pel_botl);

        pel_out = (v8i16) __msa_sldi_b((v16i8) pel_curl, (v16i8) pel_curr, 2);

        pel_sumr = pel_topr + (pel_curr << 1) + pel_botr;
        pel_suml = pel_topl + (pel_curl << 1) + pel_botl;

        pel_top_cmp = (threshold_vec <= pel_topr);
        pel_cur_cmp = (threshold_vec <= pel_curr);
        pel_bot_cmp = (threshold_vec <= pel_botr);
        pel_signr = pel_top_cmp + pel_cur_cmp + pel_bot_cmp;
        pel_top_cmp = (threshold_vec <= pel_topl);
        pel_cur_cmp = (threshold_vec <= pel_curl);
        pel_bot_cmp = (threshold_vec <= pel_botl);
        pel_signl = pel_top_cmp + pel_cur_cmp + pel_bot_cmp;

        pel_signr = (v8i16) zero - pel_signr;
        pel_signl = (v8i16) zero - pel_signl;

        pel_sum_shift1 = (v8i16) __msa_sldi_b((v16i8) pel_suml,
                                              (v16i8) pel_sumr, 2);
        pel_sum_shift2 = (v8i16) __msa_sldi_b((v16i8) pel_suml,
                                              (v16i8) pel_sumr, 4);
        pel_sign_shift1 = (v8i16) __msa_sldi_b((v16i8) pel_signl,
                                               (v16i8) pel_signr, 2);
        pel_sign_shift2 = (v8i16) __msa_sldi_b((v16i8) pel_signl,
                                               (v16i8) pel_signr, 4);

        three_row_pel_sign = pel_signr + pel_sign_shift1 + pel_sign_shift2;
        three_row_pel_sum = pel_sumr + (pel_sum_shift1 << 1) + pel_sum_shift2 + 8;
        three_row_pel_sum >>= 4;

        abspel_minus_sum = __msa_asub_s_h(pel_out, three_row_pel_sum);
        diff_grt = __msa_clt_s_h(max_diff_vec, abspel_minus_sum);
        sum_grt = (pel_out <= three_row_pel_sum);

        pel_cur_updated_with_sum = pel_out - max_diff_vec;
        cur_plus_maxdiff = pel_out + max_diff_vec;

        pel_cur_updated = three_row_pel_sum;
        pel_cur_updated_with_sum = (v8i16) __msa_bmnz_v((v16u8) pel_cur_updated_with_sum,
                                                        (v16u8) cur_plus_maxdiff,
                                                        (v16u8) sum_grt);
        pel_cur_updated = (v8i16) __msa_bmnz_v((v16u8) pel_cur_updated,
                                               (v16u8) pel_cur_updated_with_sum,
                                               (v16u8) diff_grt);

        pel_cur_updated = CLIP_SH_0_255(pel_cur_updated);

        is_update_out_for0 = __msa_ceqi_h(three_row_pel_sign, 0);
        is_update_out_for9 = __msa_ceqi_h(three_row_pel_sign, 9);

        pel_out = (v8i16) __msa_bmnz_v((v16u8) pel_out,
                                       (v16u8) pel_cur_updated,
                                       (v16u8) is_update_out_for0);
        pel_out = (v8i16) __msa_bmnz_v((v16u8) pel_out,
                                       (v16u8) pel_cur_updated,
                                       (v16u8) is_update_out_for9);

        PCKEV_B_STORE_8_BYTES(pel_out, pel_out, &recon_c_start[1]);

        recon_c_start += width;
    }

    return;
}
#endif /* M4VH263DEC_MSA */
#endif /* PV_POSTPROC_ON */
