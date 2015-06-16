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
#include "idct.h"
#include "common_macros_msa.h"
#include "prototypes_msa.h"

#ifdef M4VH263DEC_MSA
void m4v_h263_idct_msa(int16 *coeff_blk, uint8 *dst, int32 dst_stride)
{
    v4i32 const181, const128, const8192;
    v4i32 const_w3, const_w6, const_w7;
    v4i32 const_w1m_w7, const_w1p_w7, const_w5m_w3;
    v4i32 constm_w3m_w5, constm_w2m_w6, const_w2m_w6;
    v8i16 in_vec0, in_vec1, in_vec2, in_vec3;
    v8i16 in_vec4, in_vec5, in_vec6, in_vec7;
    v4i32 in_vec0_r, in_vec1_r, in_vec2_r, in_vec3_r;
    v4i32 in_vec4_r, in_vec5_r, in_vec6_r, in_vec7_r;
    v4i32 in_vec0_l, in_vec1_l, in_vec2_l, in_vec3_l;
    v4i32 in_vec4_l, in_vec5_l, in_vec6_l, in_vec7_l;
    v4i32 out_vec0_r, out_vec1_r, out_vec2_r, out_vec3_r;
    v4i32 out_vec4_r, out_vec6_r, out_vec7_r;
    v4i32 in_vec8_r, in_vec8_l, out_vec8_r, out_vec8_l;
    v4i32 out_vec0_l, out_vec1_l, out_vec2_l, out_vec3_l;
    v4i32 out_vec4_l, out_vec6_l, out_vec7_l;
    v16i8 tmp0, tmp1;
    v8i16 out0, out1, out2, out3, out4, out5, out6, out7;
    v4i32 const_w3_w6_w7_w1m_w7 = { W3, W6, W7, W1mW7 };
    v4i32 const_w1m_w7_w7_w6_w3 = { W1mW7, W7, W6, W3 };
    v4i32 const_w1pw7_w5mw3m_w3mw5m_w2mw6 = { W1pW7, W5mW3, mW3mW5, mW2mW6 };
    v4i32 constm_w2mw6m_w3mw5_w5mw3_w1pw7 = { mW2mW6, mW3mW5, W5mW3, W1pW7 };

    const8192 = __msa_fill_w(8192);
    const181 = __msa_ldi_w(181);
    const128 = __msa_ldi_w(128);

    LD_SH4(coeff_blk, 8, in_vec0, in_vec4, in_vec3, in_vec7);
    LD_SH4(coeff_blk + 32, 8, in_vec1, in_vec6, in_vec2, in_vec5);
    UNPCK_SH_SW(in_vec0, in_vec0_r, in_vec0_l);
    UNPCK_SH_SW(in_vec1, in_vec1_r, in_vec1_l);
    UNPCK_SH_SW(in_vec2, in_vec2_r, in_vec2_l);
    UNPCK_SH_SW(in_vec3, in_vec3_r, in_vec3_l);
    UNPCK_SH_SW(in_vec4, in_vec4_r, in_vec4_l);
    UNPCK_SH_SW(in_vec5, in_vec5_r, in_vec5_l);
    UNPCK_SH_SW(in_vec6, in_vec6_r, in_vec6_l);
    UNPCK_SH_SW(in_vec7, in_vec7_r, in_vec7_l);
    in_vec1_r <<= 11;
    in_vec0_r <<= 11;
    in_vec0_r += const128;
    in_vec1_l <<= 11;
    in_vec0_l <<= 11;
    in_vec0_l += const128;
    SPLATI_W4_SW(const_w3_w6_w7_w1m_w7, const_w3, const_w6, const_w7, const_w1m_w7);
    SPLATI_W4_SW(const_w1pw7_w5mw3m_w3mw5m_w2mw6, const_w1p_w7, const_w5m_w3,
                 constm_w3m_w5, constm_w2m_w6);
    const_w2m_w6 = __msa_fill_w(W2mW6);
    in_vec8_r = const_w7 * (in_vec4_r + in_vec5_r);
    in_vec4_r = in_vec8_r + (const_w1m_w7) * in_vec4_r;
    in_vec5_r = in_vec8_r - (const_w1p_w7) * in_vec5_r;
    in_vec8_r = const_w3 * (in_vec6_r + in_vec7_r);
    in_vec6_r = in_vec8_r + (const_w5m_w3) * in_vec6_r;
    in_vec7_r = in_vec8_r + (constm_w3m_w5) * in_vec7_r;
    in_vec8_l = const_w7 * (in_vec4_l + in_vec5_l);
    in_vec4_l = in_vec8_l + (const_w1m_w7) * in_vec4_l;
    in_vec5_l = in_vec8_l - (const_w1p_w7) * in_vec5_l;
    in_vec8_l = const_w3 * (in_vec6_l + in_vec7_l);
    in_vec6_l = in_vec8_l + (const_w5m_w3) * in_vec6_l;
    in_vec7_l = in_vec8_l + (constm_w3m_w5) * in_vec7_l;

    /* second stage */
    BUTTERFLY_4(in_vec0_r, in_vec0_l, in_vec1_l, in_vec1_r,
                in_vec8_r, in_vec8_l, in_vec0_l, in_vec0_r);
    in_vec1_r = const_w6 * (in_vec3_r + in_vec2_r);
    in_vec2_r = in_vec1_r + (constm_w2m_w6) * in_vec2_r;
    in_vec3_r = in_vec1_r + (const_w2m_w6) * in_vec3_r;
    in_vec1_l = const_w6 * (in_vec3_l + in_vec2_l);
    in_vec2_l = in_vec1_l + (constm_w2m_w6) * in_vec2_l;
    in_vec3_l = in_vec1_l + (const_w2m_w6) * in_vec3_l;
    /* butterfly */
    in_vec1_r = in_vec4_r + in_vec6_r;
    in_vec4_r = in_vec4_r - in_vec6_r;
    in_vec6_r = in_vec5_r + in_vec7_r;
    in_vec5_r = in_vec5_r - in_vec7_r;
    in_vec1_l = in_vec4_l + in_vec6_l;
    in_vec4_l = in_vec4_l - in_vec6_l;
    in_vec6_l = in_vec5_l + in_vec7_l;
    in_vec5_l = in_vec5_l - in_vec7_l;

    /* third stage butterfly */
    in_vec7_r = in_vec8_r + in_vec3_r;
    in_vec8_r = in_vec8_r - in_vec3_r;
    in_vec3_r = in_vec0_r + in_vec2_r;
    in_vec0_r = in_vec0_r - in_vec2_r;
    in_vec7_l = in_vec8_l + in_vec3_l;
    in_vec8_l = in_vec8_l - in_vec3_l;
    in_vec3_l = in_vec0_l + in_vec2_l;
    in_vec0_l = in_vec0_l - in_vec2_l;
    in_vec2_r = const181 * (in_vec4_r + in_vec5_r) + const128;
    in_vec2_r >>= 8;
    in_vec4_r = const181 * (in_vec4_r - in_vec5_r) + const128;
    in_vec4_r >>= 8;
    in_vec2_l = const181 * (in_vec4_l + in_vec5_l) + const128;
    in_vec2_l >>= 8;
    in_vec4_l = const181 * (in_vec4_l - in_vec5_l) + const128;
    in_vec4_l >>= 8;
    BUTTERFLY_8(in_vec7_r, in_vec3_r, in_vec0_r, in_vec8_r,
                in_vec6_r, in_vec4_r, in_vec2_r, in_vec1_r,
                out_vec0_r, out_vec1_r, out_vec2_r, out_vec3_r,
                out_vec4_r, out_vec6_r, out_vec7_r, out_vec8_r);
    SRA_4V(out_vec7_r, out_vec1_r, out_vec3_r, out_vec2_r, 8);
    SRA_4V(out_vec0_r, out_vec4_r, out_vec8_r, out_vec6_r, 8);
    BUTTERFLY_8(in_vec7_l, in_vec3_l, in_vec0_l, in_vec8_l,
                in_vec6_l, in_vec4_l, in_vec2_l, in_vec1_l,
                out_vec0_l, out_vec1_l, out_vec2_l, out_vec3_l,
                out_vec4_l, out_vec6_l, out_vec7_l, out_vec8_l);
    SRA_4V(out_vec7_l, out_vec1_l, out_vec3_l, out_vec2_l, 8);
    SRA_4V(out_vec0_l, out_vec4_l, out_vec8_l, out_vec6_l, 8);
    TRANSPOSE4x4_SW_SW(out_vec0_r, out_vec1_r, out_vec2_r, out_vec3_r,
                       in_vec0_r, in_vec4_r, in_vec3_r, in_vec7_r);
    TRANSPOSE4x4_SW_SW(out_vec0_l, out_vec1_l, out_vec2_l, out_vec3_l,
                       in_vec1_r, in_vec6_r, in_vec2_r, in_vec5_r);
    TRANSPOSE4x4_SW_SW(out_vec4_r, out_vec6_r, out_vec7_r, out_vec8_r,
                       in_vec0_l, in_vec4_l, in_vec3_l, in_vec7_l);
    TRANSPOSE4x4_SW_SW(out_vec4_l, out_vec6_l, out_vec7_l, out_vec8_l,
                       in_vec1_l, in_vec6_l, in_vec2_l, in_vec5_l);

    in_vec1_r <<= 8;
    in_vec0_r <<= 8;
    in_vec0_r += const8192;
    in_vec1_l <<= 8;
    in_vec0_l <<= 8;
    in_vec0_l += const8192;
    SPLATI_W4_SW(const_w1m_w7_w7_w6_w3, const_w1m_w7, const_w7, const_w6, const_w3);
    SPLATI_W4_SW(constm_w2mw6m_w3mw5_w5mw3_w1pw7, constm_w2m_w6, constm_w3m_w5,
                 const_w5m_w3, const_w1p_w7);
    const_w2m_w6 = __msa_fill_w(W2mW6);
    in_vec8_r = const_w7 * (in_vec4_r + in_vec5_r);
    in_vec8_r += 4;
    in_vec4_r = in_vec8_r + (const_w1m_w7) * in_vec4_r;
    in_vec4_r >>= 3;
    in_vec5_r = in_vec8_r - (const_w1p_w7) * in_vec5_r;
    in_vec5_r >>= 3;
    in_vec8_r = const_w3 * (in_vec6_r + in_vec7_r);
    in_vec8_r += 4;
    in_vec6_r = in_vec8_r + (const_w5m_w3) * in_vec6_r;
    in_vec6_r >>= 3;
    in_vec7_r = in_vec8_r + (constm_w3m_w5) * in_vec7_r;
    in_vec7_r >>= 3;
    in_vec8_l = const_w7 * (in_vec4_l + in_vec5_l);
    in_vec8_l += 4;
    in_vec4_l = in_vec8_l + (const_w1m_w7) * in_vec4_l;
    in_vec4_l >>= 3;
    in_vec5_l = in_vec8_l - (const_w1p_w7) * in_vec5_l;
    in_vec5_l >>= 3;
    in_vec8_l = const_w3 * (in_vec6_l + in_vec7_l);
    in_vec8_l += 4;
    in_vec6_l = in_vec8_l + (const_w5m_w3) * in_vec6_l;
    in_vec6_l >>= 3;
    in_vec7_l = in_vec8_l + (constm_w3m_w5) * in_vec7_l;
    in_vec7_l >>= 3;

    /* second stage */
    BUTTERFLY_4(in_vec0_r, in_vec0_l, in_vec1_l, in_vec1_r,
                in_vec8_r, in_vec8_l, in_vec0_l, in_vec0_r);
    in_vec1_r = const_w6 * (in_vec3_r + in_vec2_r);
    in_vec1_r += 4;
    in_vec2_r = in_vec1_r + (constm_w2m_w6) * in_vec2_r;
    in_vec2_r >>= 3;
    in_vec3_r = in_vec1_r + (const_w2m_w6) * in_vec3_r;
    in_vec3_r >>= 3;
    in_vec1_l = const_w6 * (in_vec3_l + in_vec2_l);
    in_vec1_l += 4;
    in_vec2_l = in_vec1_l + (constm_w2m_w6) * in_vec2_l;
    in_vec2_l >>= 3;
    in_vec3_l = in_vec1_l + (const_w2m_w6) * in_vec3_l;
    in_vec3_l >>= 3;
    /* butterfly */
    in_vec1_r = in_vec4_r + in_vec6_r;
    in_vec4_r = in_vec4_r - in_vec6_r;
    in_vec6_r = in_vec5_r + in_vec7_r;
    in_vec5_r = in_vec5_r - in_vec7_r;
    in_vec1_l = in_vec4_l + in_vec6_l;
    in_vec4_l = in_vec4_l - in_vec6_l;
    in_vec6_l = in_vec5_l + in_vec7_l;
    in_vec5_l = in_vec5_l - in_vec7_l;

    /* third stage butterfly */
    in_vec7_r = in_vec8_r + in_vec3_r;
    in_vec8_r = in_vec8_r - in_vec3_r;
    in_vec3_r = in_vec0_r + in_vec2_r;
    in_vec0_r = in_vec0_r - in_vec2_r;
    in_vec7_l = in_vec8_l + in_vec3_l;
    in_vec8_l = in_vec8_l - in_vec3_l;
    in_vec3_l = in_vec0_l + in_vec2_l;
    in_vec0_l = in_vec0_l - in_vec2_l;
    in_vec2_r = const181 * (in_vec4_r + in_vec5_r) + const128;
    in_vec2_r >>= 8;
    in_vec4_r = const181 * (in_vec4_r - in_vec5_r) + const128;
    in_vec4_r >>= 8;
    in_vec2_l = const181 * (in_vec4_l + in_vec5_l) + const128;
    in_vec2_l >>= 8;
    in_vec4_l = const181 * (in_vec4_l - in_vec5_l) + const128;
    in_vec4_l >>= 8;
    BUTTERFLY_8(in_vec7_r, in_vec3_r, in_vec0_r, in_vec8_r,
                in_vec6_r, in_vec4_r, in_vec2_r, in_vec1_r,
                out_vec0_r, out_vec1_r, out_vec2_r, out_vec3_r,
                out_vec4_r, out_vec6_r, out_vec7_r, out_vec8_r);
    SRA_4V(out_vec7_r, out_vec1_r, out_vec3_r, out_vec2_r, 14);
    SRA_4V(out_vec0_r, out_vec4_r, out_vec8_r, out_vec6_r, 14);
    BUTTERFLY_8(in_vec7_l, in_vec3_l, in_vec0_l, in_vec8_l,
                in_vec6_l, in_vec4_l, in_vec2_l, in_vec1_l,
                out_vec0_l, out_vec1_l, out_vec2_l, out_vec3_l,
                out_vec4_l, out_vec6_l, out_vec7_l, out_vec8_l);
    SRA_4V(out_vec7_l, out_vec1_l, out_vec3_l, out_vec2_l, 14);
    SRA_4V(out_vec0_l, out_vec4_l, out_vec8_l, out_vec6_l, 14);
    PCKEV_H4_SH(out_vec0_l, out_vec0_r, out_vec1_l, out_vec1_r, out_vec2_l,
                out_vec2_r, out_vec3_l, out_vec3_r, out0, out1, out2, out3);
    PCKEV_H4_SH(out_vec4_l, out_vec4_r, out_vec6_l, out_vec6_r, out_vec7_l,
                out_vec7_r, out_vec8_l, out_vec8_r, out4, out5, out6, out7);
    TRANSPOSE8x8_SH_SH(out0, out1, out2, out3, out4, out5, out6, out7,
                       out0, out1, out2, out3, out4, out5, out6, out7);
    CLIP_SH4_0_255(out0, out1, out2, out3);
    PCKEV_B2_SB(out1, out0, out3, out2, tmp0, tmp1);
    ST8x4_UB(tmp0, tmp1, dst, dst_stride);
    dst += 4 * dst_stride;

    CLIP_SH4_0_255(out4, out5, out6, out7);
    PCKEV_B2_SB(out5, out4, out7, out6, tmp0, tmp1);
    ST8x4_UB(tmp0, tmp1, dst, dst_stride);
}

void m4v_h263_idct_addblk_msa(int16 *coeff_blk, uint8 *pred,
                              uint8 *dst, int32 dst_stride)
{
    v4i32 const181, const128, const8192;
    v4i32 const_w3, const_w6, const_w7;
    v4i32 const_w1m_w7, const_w1p_w7, const_w5m_w3;
    v4i32 constm_w3m_w5, constm_w2m_w6, const_w2m_w6;
    v8i16 in_vec0, in_vec1, in_vec2, in_vec3;
    v8i16 in_vec4, in_vec5, in_vec6, in_vec7;
    v4i32 in_vec0_r, in_vec1_r, in_vec2_r, in_vec3_r;
    v4i32 in_vec4_r, in_vec5_r, in_vec6_r, in_vec7_r, in_vec8_r;
    v4i32 in_vec0_l, in_vec1_l, in_vec2_l, in_vec3_l;
    v4i32 in_vec4_l, in_vec5_l, in_vec6_l, in_vec7_l, in_vec8_l;
    v4i32 out_vec0_r, out_vec1_r, out_vec2_r, out_vec3_r;
    v4i32 out_vec4_r, out_vec6_r, out_vec7_r, out_vec8_r;
    v4i32 out_vec0_l, out_vec1_l, out_vec2_l, out_vec3_l;
    v4i32 out_vec4_l, out_vec6_l, out_vec7_l, out_vec8_l;
    v16u8 pred_data0, pred_data1, pred_data2, pred_data3;
    v8i16 pred0, pred1, pred2, pred3;
    v8i16 out0, out1, out2, out3, out4, out5, out6, out7;
    v8i16 res0, res1, res2, res3;
    v16i8 zero = { 0 };
    v16i8 tmp0, tmp1;
    v4i32 const_w3_w6_w7_w1m_w7 = { W3, W6, W7, W1mW7 };
    v4i32 const_w1m_w7_w7_w6_w3 = { W1mW7, W7, W6, W3 };
    v4i32 const_w1pw7_w5mw3m_w3mw5m_w2mw6 = { W1pW7, W5mW3, mW3mW5, mW2mW6 };
    v4i32 constm_w2mw6m_w3mw5_w5mw3_w1pw7 = { mW2mW6, mW3mW5, W5mW3, W1pW7 };

    const8192 = __msa_fill_w(8192);
    const181 = __msa_ldi_w(181);
    const128 = __msa_ldi_w(128);

    LD_SH4(coeff_blk, 8, in_vec0, in_vec4, in_vec3, in_vec7);
    LD_SH4(coeff_blk + 32, 8, in_vec1, in_vec6, in_vec2, in_vec5);

    UNPCK_SH_SW(in_vec0, in_vec0_r, in_vec0_l);
    UNPCK_SH_SW(in_vec1, in_vec1_r, in_vec1_l);
    UNPCK_SH_SW(in_vec2, in_vec2_r, in_vec2_l);
    UNPCK_SH_SW(in_vec3, in_vec3_r, in_vec3_l);
    UNPCK_SH_SW(in_vec4, in_vec4_r, in_vec4_l);
    UNPCK_SH_SW(in_vec5, in_vec5_r, in_vec5_l);
    UNPCK_SH_SW(in_vec6, in_vec6_r, in_vec6_l);
    UNPCK_SH_SW(in_vec7, in_vec7_r, in_vec7_l);
    SPLATI_W4_SW(const_w3_w6_w7_w1m_w7, const_w3, const_w6, const_w7, const_w1m_w7);
    SPLATI_W4_SW(const_w1pw7_w5mw3m_w3mw5m_w2mw6, const_w1p_w7, const_w5m_w3,
                 constm_w3m_w5, constm_w2m_w6);
    const_w2m_w6 = __msa_fill_w(W2mW6);
    in_vec1_r <<= 11;
    in_vec0_r <<= 11;
    in_vec0_r += const128;
    in_vec1_l <<= 11;
    in_vec0_l <<= 11;
    in_vec0_l += const128;
    in_vec8_r = const_w7 * (in_vec4_r + in_vec5_r);
    in_vec4_r = in_vec8_r + (const_w1m_w7) * in_vec4_r;
    in_vec5_r = in_vec8_r - (const_w1p_w7) * in_vec5_r;
    in_vec8_r = const_w3 * (in_vec6_r + in_vec7_r);
    in_vec6_r = in_vec8_r + (const_w5m_w3) * in_vec6_r;
    in_vec7_r = in_vec8_r + (constm_w3m_w5) * in_vec7_r;
    in_vec8_l = const_w7 * (in_vec4_l + in_vec5_l);
    in_vec4_l = in_vec8_l + (const_w1m_w7) * in_vec4_l;
    in_vec5_l = in_vec8_l - (const_w1p_w7) * in_vec5_l;
    in_vec8_l = const_w3 * (in_vec6_l + in_vec7_l);
    in_vec6_l = in_vec8_l + (const_w5m_w3) * in_vec6_l;
    in_vec7_l = in_vec8_l + (constm_w3m_w5) * in_vec7_l;

    /* second stage */
    BUTTERFLY_4(in_vec0_r, in_vec0_l, in_vec1_l, in_vec1_r,
                in_vec8_r, in_vec8_l, in_vec0_l, in_vec0_r);
    in_vec1_r = const_w6 * (in_vec3_r + in_vec2_r);
    in_vec2_r = in_vec1_r + (constm_w2m_w6) * in_vec2_r;
    in_vec3_r = in_vec1_r + (const_w2m_w6) * in_vec3_r;
    in_vec1_l = const_w6 * (in_vec3_l + in_vec2_l);
    in_vec2_l = in_vec1_l + (constm_w2m_w6) * in_vec2_l;
    in_vec3_l = in_vec1_l + (const_w2m_w6) * in_vec3_l;
    /* butterfly */
    in_vec1_r = in_vec4_r + in_vec6_r;
    in_vec4_r = in_vec4_r - in_vec6_r;
    in_vec6_r = in_vec5_r + in_vec7_r;
    in_vec5_r = in_vec5_r - in_vec7_r;
    in_vec1_l = in_vec4_l + in_vec6_l;
    in_vec4_l = in_vec4_l - in_vec6_l;
    in_vec6_l = in_vec5_l + in_vec7_l;
    in_vec5_l = in_vec5_l - in_vec7_l;

    /* third stage butterfly */
    in_vec7_r = in_vec8_r + in_vec3_r;
    in_vec8_r = in_vec8_r - in_vec3_r;
    in_vec3_r = in_vec0_r + in_vec2_r;
    in_vec0_r = in_vec0_r - in_vec2_r;
    in_vec7_l = in_vec8_l + in_vec3_l;
    in_vec8_l = in_vec8_l - in_vec3_l;
    in_vec3_l = in_vec0_l + in_vec2_l;
    in_vec0_l = in_vec0_l - in_vec2_l;

    in_vec2_r = const181 * (in_vec4_r + in_vec5_r) + const128;
    in_vec2_r >>= 8;
    in_vec4_r = const181 * (in_vec4_r - in_vec5_r) + const128;
    in_vec4_r >>= 8;
    in_vec2_l = const181 * (in_vec4_l + in_vec5_l) + const128;
    in_vec2_l >>= 8;
    in_vec4_l = const181 * (in_vec4_l - in_vec5_l) + const128;
    in_vec4_l >>= 8;
    BUTTERFLY_8(in_vec7_r, in_vec3_r, in_vec0_r, in_vec8_r,
                in_vec6_r, in_vec4_r, in_vec2_r, in_vec1_r,
                out_vec0_r, out_vec1_r, out_vec2_r, out_vec3_r,
                out_vec4_r, out_vec6_r, out_vec7_r, out_vec8_r);
    SRA_4V(out_vec7_r, out_vec1_r, out_vec3_r, out_vec2_r, 8);
    SRA_4V(out_vec0_r, out_vec4_r, out_vec8_r, out_vec6_r, 8);
    BUTTERFLY_8(in_vec7_l, in_vec3_l, in_vec0_l, in_vec8_l,
                in_vec6_l, in_vec4_l, in_vec2_l, in_vec1_l,
                out_vec0_l, out_vec1_l, out_vec2_l, out_vec3_l,
                out_vec4_l, out_vec6_l, out_vec7_l, out_vec8_l);
    SRA_4V(out_vec7_l, out_vec1_l, out_vec3_l, out_vec2_l, 8);
    SRA_4V(out_vec0_l, out_vec4_l, out_vec8_l, out_vec6_l, 8);
    TRANSPOSE4x4_SW_SW(out_vec0_r, out_vec1_r, out_vec2_r, out_vec3_r,
                       in_vec0_r, in_vec4_r, in_vec3_r, in_vec7_r);
    TRANSPOSE4x4_SW_SW(out_vec0_l, out_vec1_l, out_vec2_l, out_vec3_l,
                       in_vec1_r, in_vec6_r, in_vec2_r, in_vec5_r);
    TRANSPOSE4x4_SW_SW(out_vec4_r, out_vec6_r, out_vec7_r, out_vec8_r,
                       in_vec0_l, in_vec4_l, in_vec3_l, in_vec7_l);
    TRANSPOSE4x4_SW_SW(out_vec4_l, out_vec6_l, out_vec7_l, out_vec8_l,
                       in_vec1_l, in_vec6_l, in_vec2_l, in_vec5_l);

    in_vec1_r <<= 8;
    in_vec0_r <<= 8;
    in_vec0_r += const8192;
    in_vec1_l <<= 8;
    in_vec0_l <<= 8;
    in_vec0_l += const8192;
    SPLATI_W4_SW(const_w1m_w7_w7_w6_w3, const_w1m_w7, const_w7, const_w6, const_w3);
    SPLATI_W4_SW(constm_w2mw6m_w3mw5_w5mw3_w1pw7, constm_w2m_w6, constm_w3m_w5,
                 const_w5m_w3, const_w1p_w7);
    const_w2m_w6 = __msa_fill_w(W2mW6);
    in_vec8_r = const_w7 * (in_vec4_r + in_vec5_r);
    in_vec8_r += 4;
    in_vec4_r = in_vec8_r + (const_w1m_w7) * in_vec4_r;
    in_vec4_r >>= 3;
    in_vec5_r = in_vec8_r - (const_w1p_w7) * in_vec5_r;
    in_vec5_r >>= 3;
    in_vec8_r = const_w3 * (in_vec6_r + in_vec7_r);
    in_vec8_r += 4;
    in_vec6_r = in_vec8_r + (const_w5m_w3) * in_vec6_r;
    in_vec6_r >>= 3;
    in_vec7_r = in_vec8_r + (constm_w3m_w5) * in_vec7_r;
    in_vec7_r >>= 3;
    in_vec8_l = const_w7 * (in_vec4_l + in_vec5_l);
    in_vec8_l += 4;
    in_vec4_l = in_vec8_l + (const_w1m_w7) * in_vec4_l;
    in_vec4_l >>= 3;
    in_vec5_l = in_vec8_l - (const_w1p_w7) * in_vec5_l;
    in_vec5_l >>= 3;
    in_vec8_l = const_w3 * (in_vec6_l + in_vec7_l);
    in_vec8_l += 4;
    in_vec6_l = in_vec8_l + (const_w5m_w3) * in_vec6_l;
    in_vec6_l >>= 3;
    in_vec7_l = in_vec8_l + (constm_w3m_w5) * in_vec7_l;
    in_vec7_l >>= 3;

    /* second stage */
    BUTTERFLY_4(in_vec0_r, in_vec0_l, in_vec1_l, in_vec1_r,
                in_vec8_r, in_vec8_l, in_vec0_l, in_vec0_r);
    in_vec1_r = const_w6 * (in_vec3_r + in_vec2_r);
    in_vec1_r += 4;
    in_vec2_r = in_vec1_r + (constm_w2m_w6) * in_vec2_r;
    in_vec2_r >>= 3;
    in_vec3_r = in_vec1_r + (const_w2m_w6) * in_vec3_r;
    in_vec3_r >>= 3;
    in_vec1_l = const_w6 * (in_vec3_l + in_vec2_l);
    in_vec1_l += 4;
    in_vec2_l = in_vec1_l + (constm_w2m_w6) * in_vec2_l;
    in_vec2_l >>= 3;
    in_vec3_l = in_vec1_l + (const_w2m_w6) * in_vec3_l;
    in_vec3_l >>= 3;
    /* butterfly */
    in_vec1_r = in_vec4_r + in_vec6_r;
    in_vec4_r = in_vec4_r - in_vec6_r;
    in_vec6_r = in_vec5_r + in_vec7_r;
    in_vec5_r = in_vec5_r - in_vec7_r;
    in_vec1_l = in_vec4_l + in_vec6_l;
    in_vec4_l = in_vec4_l - in_vec6_l;
    in_vec6_l = in_vec5_l + in_vec7_l;
    in_vec5_l = in_vec5_l - in_vec7_l;

    /* third stage butterfly */
    in_vec7_r = in_vec8_r + in_vec3_r;
    in_vec8_r = in_vec8_r - in_vec3_r;
    in_vec3_r = in_vec0_r + in_vec2_r;
    in_vec0_r = in_vec0_r - in_vec2_r;
    in_vec7_l = in_vec8_l + in_vec3_l;
    in_vec8_l = in_vec8_l - in_vec3_l;
    in_vec3_l = in_vec0_l + in_vec2_l;
    in_vec0_l = in_vec0_l - in_vec2_l;
    in_vec2_r = const181 * (in_vec4_r + in_vec5_r) + const128;
    in_vec2_r >>= 8;
    in_vec4_r = const181 * (in_vec4_r - in_vec5_r) + const128;
    in_vec4_r >>= 8;
    in_vec2_l = const181 * (in_vec4_l + in_vec5_l) + const128;
    in_vec2_l >>= 8;
    in_vec4_l = const181 * (in_vec4_l - in_vec5_l) + const128;
    in_vec4_l >>= 8;
    BUTTERFLY_8(in_vec7_r, in_vec3_r, in_vec0_r, in_vec8_r,
                in_vec6_r, in_vec4_r, in_vec2_r, in_vec1_r,
                out_vec0_r, out_vec1_r, out_vec2_r, out_vec3_r,
                out_vec4_r, out_vec6_r, out_vec7_r, out_vec8_r);
    SRA_4V(out_vec7_r, out_vec1_r, out_vec3_r, out_vec2_r, 14);
    SRA_4V(out_vec0_r, out_vec4_r, out_vec8_r, out_vec6_r, 14);
    BUTTERFLY_8(in_vec7_l, in_vec3_l, in_vec0_l, in_vec8_l,
                in_vec6_l, in_vec4_l, in_vec2_l, in_vec1_l,
                out_vec0_l, out_vec1_l, out_vec2_l, out_vec3_l,
                out_vec4_l, out_vec6_l, out_vec7_l, out_vec8_l);
    SRA_4V(out_vec7_l, out_vec1_l, out_vec3_l, out_vec2_l, 14);
    SRA_4V(out_vec0_l, out_vec4_l, out_vec8_l, out_vec6_l, 14);
    PCKEV_H4_SH(out_vec0_l, out_vec0_r, out_vec1_l, out_vec1_r, out_vec2_l,
                out_vec2_r, out_vec3_l, out_vec3_r, out0, out1, out2, out3);
    PCKEV_H4_SH(out_vec4_l, out_vec4_r, out_vec6_l, out_vec6_r, out_vec7_l,
                out_vec7_r, out_vec8_l, out_vec8_r, out4, out5, out6, out7);
    TRANSPOSE8x8_SH_SH(out0, out1, out2, out3, out4, out5, out6, out7,
                       out0, out1, out2, out3, out4, out5, out6, out7);

    LD_UB4(pred, 16, pred_data0, pred_data1, pred_data2, pred_data3);
    ST_SH4(out4, out5, out6, out7, coeff_blk + 32, 8);
    ILVR_B4_SH(zero, pred_data0, zero, pred_data1, zero, pred_data2, zero, pred_data3,
               pred0, pred1, pred2, pred3);
    ADD4(out7, pred0, out6, pred1, out5, pred2, out4, pred3, res0, res1, res2, res3);
    CLIP_SH4_0_255(res0, res1, res2, res3);
    PCKEV_B2_SB(res1, res0, res3, res2, tmp0, tmp1);
    ST8x4_UB(tmp0, tmp1, dst, dst_stride);
    LD_UB4(pred + 16 * 4, 16, pred_data0, pred_data1, pred_data2, pred_data3);
    ST_SH4(out0, out1, out2, out3, coeff_blk, 8);
    ILVR_B4_SH(zero, pred_data0, zero, pred_data1, zero, pred_data2, zero, pred_data3,
               pred0, pred1, pred2, pred3);
    ADD4(out3, pred0, out2, pred1, out1, pred2, out0, pred3, res0, res1, res2, res3);
    CLIP_SH4_0_255(res0, res1, res2, res3);
    PCKEV_B2_SB(res1, res0, res3, res2, tmp0, tmp1);
    ST8x4_UB(tmp0, tmp1, dst + 4 * dst_stride, dst_stride);
}

void m4v_h263_idct_4rows_msa(int16 *coeff_blk, uint8 *dst,
                             int32 dst_stride)
{
    v4i32 const_w1, const_w2, const_w3, const_w5, const_w6, const_w7;
    v4i32 const181, const128, const8192;
    v8i16 in_vec0, in_vec1, in_vec2, in_vec3;
    v8i16 in_vec4, in_vec5, in_vec6, in_vec7;
    v4i32 in_vec0_r, in_vec1_r, in_vec2_r, in_vec3_r;
    v4i32 in_vec4_r, in_vec5_r, in_vec6_r, in_vec7_r, in_vec8_r;
    v4i32 in_vec0_l, in_vec1_l, in_vec2_l, in_vec3_l;
    v4i32 in_vec4_l, in_vec5_l, in_vec6_l, in_vec7_l, in_vec8_l;
    v8i16 out0, out1, out2, out3, out4, out5, out6, out7;
    v4i32 out_vec0, out_vec1, out_vec2, out_vec3;
    v4i32 out_vec4, out_vec5, out_vec6, out_vec7;
    v16i8 tmp0, tmp1;

    const_w1 = __msa_fill_w(W1);
    const_w2 = __msa_fill_w(W2);
    const_w3 = __msa_fill_w(W3);
    const_w5 = __msa_fill_w(W5);
    const_w6 = __msa_fill_w(W6);
    const_w7 = __msa_fill_w(W7);
    const8192 = __msa_fill_w(8192);
    const181 = __msa_ldi_w(181);
    const128 = __msa_ldi_w(128);

    LD_SH4(coeff_blk, 8, in_vec0, in_vec1, in_vec2, in_vec3);
    LD_SH4(coeff_blk + 32, 8, in_vec4, in_vec5, in_vec6, in_vec7);
    SW_ZERO(coeff_blk);
    SW_ZERO(coeff_blk + 2);
    coeff_blk += 8;
    SW_ZERO(coeff_blk);
    SW_ZERO(coeff_blk + 2);
    coeff_blk += 8;
    SW_ZERO(coeff_blk);
    SW_ZERO(coeff_blk + 2);
    coeff_blk += 8;
    SW_ZERO(coeff_blk);
    SW_ZERO(coeff_blk + 2);
    coeff_blk += 8;
    SW_ZERO(coeff_blk);
    SW_ZERO(coeff_blk + 2);
    coeff_blk += 8;
    SW_ZERO(coeff_blk);
    SW_ZERO(coeff_blk + 2);
    coeff_blk += 8;
    SW_ZERO(coeff_blk);
    SW_ZERO(coeff_blk + 2);
    coeff_blk += 8;
    SW_ZERO(coeff_blk);
    SW_ZERO(coeff_blk + 2);
    TRANSPOSE8x8_SH_SH(in_vec0, in_vec1, in_vec2, in_vec3,
                       in_vec4, in_vec5, in_vec6, in_vec7,
                       in_vec0, in_vec1, in_vec2, in_vec3,
                       in_vec4, in_vec5, in_vec6, in_vec7);

    UNPCK_SH_SW(in_vec0, in_vec0_r, in_vec0_l);
    UNPCK_SH_SW(in_vec1, in_vec1_r, in_vec1_l);
    UNPCK_SH_SW(in_vec2, in_vec2_r, in_vec2_l);
    UNPCK_SH_SW(in_vec3, in_vec3_r, in_vec3_l);
    in_vec0_r <<= 8;
    in_vec0_r += const8192;
    in_vec4_r = in_vec0_r;
    in_vec6_r = const_w6 * in_vec2_r;
    in_vec6_r += 4;
    in_vec6_r >>= 3;
    in_vec2_r *= const_w2;
    in_vec2_r += 4;
    in_vec2_r >>= 3;
    in_vec8_r = in_vec0_r - in_vec2_r;
    in_vec0_r += in_vec2_r;
    in_vec2_r = in_vec8_r;
    in_vec8_r = in_vec4_r - in_vec6_r;
    in_vec4_r += in_vec6_r;
    in_vec6_r = in_vec8_r;
    in_vec7_r = const_w7 * in_vec1_r;
    in_vec7_r += 4;
    in_vec7_r >>= 3;
    in_vec1_r *= const_w1;
    in_vec1_r += 4;
    in_vec1_r >>= 3;
    in_vec5_r = const_w3 * in_vec3_r;
    in_vec5_r += 4;
    in_vec5_r >>= 3;
    in_vec3_r *= -const_w5;
    in_vec3_r += 4;
    in_vec3_r >>= 3;
    in_vec8_r = in_vec1_r - in_vec5_r;
    in_vec1_r += in_vec5_r;
    in_vec5_r = in_vec8_r;
    in_vec8_r = in_vec7_r - in_vec3_r;
    in_vec3_r += in_vec7_r;
    in_vec7_r = const181 * (in_vec5_r + in_vec8_r) + const128;
    in_vec7_r >>= 8;
    in_vec5_r = const181 * (in_vec5_r - in_vec8_r) + const128;
    in_vec5_r >>= 8;
    BUTTERFLY_8(in_vec0_r, in_vec4_r, in_vec6_r, in_vec2_r,
                in_vec3_r, in_vec5_r, in_vec7_r, in_vec1_r,
                out_vec0, out_vec1, out_vec2, out_vec3,
                out_vec4, out_vec5, out_vec6, out_vec7);

    in_vec0_r = out_vec0 >> 14;
    in_vec1_r = out_vec1 >> 14;
    in_vec2_r = out_vec2 >> 14;
    in_vec3_r = out_vec3 >> 14;
    in_vec4_r = out_vec4 >> 14;
    in_vec5_r = out_vec5 >> 14;
    in_vec6_r = out_vec6 >> 14;
    in_vec7_r = out_vec7 >> 14;
    in_vec0_l <<= 8;
    in_vec0_l += const8192;
    in_vec4_l = in_vec0_l;
    in_vec6_l = const_w6 * in_vec2_l;
    in_vec6_l += 4;
    in_vec6_l >>= 3;
    in_vec2_l *= const_w2;
    in_vec2_l += 4;
    in_vec2_l >>= 3;
    in_vec8_l = in_vec0_l - in_vec2_l;
    in_vec0_l += in_vec2_l;
    in_vec2_l = in_vec8_l;
    in_vec8_l = in_vec4_l - in_vec6_l;
    in_vec4_l += in_vec6_l;
    in_vec6_l = in_vec8_l;
    in_vec7_l = const_w7 * in_vec1_l;
    in_vec7_l += 4;
    in_vec7_l >>= 3;
    in_vec1_l *= const_w1;
    in_vec1_l += 4;
    in_vec1_l >>= 3;
    in_vec5_l = const_w3 * in_vec3_l;
    in_vec5_l += 4;
    in_vec5_l >>= 3;
    in_vec3_l *= -const_w5;
    in_vec3_l += 4;
    in_vec3_l >>= 3;
    in_vec8_l = in_vec1_l - in_vec5_l;
    in_vec1_l += in_vec5_l;
    in_vec5_l = in_vec8_l;
    in_vec8_l = in_vec7_l - in_vec3_l;
    in_vec3_l += in_vec7_l;
    in_vec7_l = const181 * (in_vec5_l + in_vec8_l) + const128;
    in_vec7_l >>= 8;
    in_vec5_l = const181 * (in_vec5_l - in_vec8_l) + const128;
    in_vec5_l >>= 8;
    BUTTERFLY_8(in_vec0_l, in_vec4_l, in_vec6_l, in_vec2_l,
                in_vec3_l, in_vec5_l, in_vec7_l, in_vec1_l,
                out_vec0, out_vec1, out_vec2, out_vec3,
                out_vec4, out_vec5, out_vec6, out_vec7);
    in_vec0_l = out_vec0 >> 14;
    in_vec1_l = out_vec1 >> 14;
    in_vec2_l = out_vec2 >> 14;
    in_vec3_l = out_vec3 >> 14;
    in_vec4_l = out_vec4 >> 14;
    in_vec5_l = out_vec5 >> 14;
    in_vec6_l = out_vec6 >> 14;
    in_vec7_l = out_vec7 >> 14;
    PCKEV_H4_SH(in_vec0_l, in_vec0_r, in_vec1_l, in_vec1_r, in_vec2_l, in_vec2_r,
                in_vec3_l, in_vec3_r, out0, out1, out2, out3);
    PCKEV_H4_SH(in_vec4_l, in_vec4_r, in_vec5_l, in_vec5_r, in_vec6_l, in_vec6_r,
                in_vec7_l, in_vec7_r, out4, out5, out6, out7);
    TRANSPOSE8x8_SH_SH(out0, out1, out2, out3, out4, out5, out6, out7,
                       out0, out1, out2, out3, out4, out5, out6, out7);
    CLIP_SH4_0_255(out0, out1, out2, out3);
    PCKEV_B2_SB(out1, out0, out3, out2, tmp0, tmp1);
    ST8x4_UB(tmp0, tmp1, dst, dst_stride);
    CLIP_SH4_0_255(out4, out5, out6, out7);
    PCKEV_B2_SB(out5, out4, out7, out6, tmp0, tmp1);
    ST8x4_UB(tmp0, tmp1, dst + 4 * dst_stride, dst_stride);
}

void m4v_h263_idct_addblk_4rows_msa(int16 *coeff_blk, uint8 *pred,
                                    uint8 *dst, int32 dst_stride)
{
    v4i32 const_w1, const_w2, const_w3, const_w5, const_w6, const_w7;
    v4i32 const181, const128, const8192;
    v8i16 in_vec0, in_vec1, in_vec2, in_vec3;
    v8i16 in_vec4, in_vec5, in_vec6, in_vec7;
    v4i32 in_vec0_r, in_vec1_r, in_vec2_r, in_vec3_r;
    v4i32 in_vec4_r, in_vec5_r, in_vec6_r, in_vec7_r, in_vec8_r;
    v4i32 in_vec0_l, in_vec1_l, in_vec2_l, in_vec3_l;
    v4i32 in_vec4_l, in_vec5_l, in_vec6_l, in_vec7_l, in_vec8_l;
    v16u8 pred_data0, pred_data1, pred_data2, pred_data3;
    v8i16 pred0, pred1, pred2, pred3;
    v8i16 out0, out1, out2, out3, out4, out5, out6, out7;
    v8i16 res0, res1, res2, res3;
    v16i8 zero = { 0 };
    v16i8 tmp0, tmp1;
    v4i32 out_vec0, out_vec1, out_vec2, out_vec3;
    v4i32 out_vec4, out_vec5, out_vec6, out_vec7;

    LD_SH4(coeff_blk, 8, in_vec0, in_vec1, in_vec2, in_vec3);
    LD_SH4(coeff_blk + 32, 8, in_vec4, in_vec5, in_vec6, in_vec7);
    SW_ZERO(coeff_blk);
    SW_ZERO(coeff_blk + 2);
    coeff_blk += 8;
    SW_ZERO(coeff_blk);
    SW_ZERO(coeff_blk + 2);
    coeff_blk += 8;
    SW_ZERO(coeff_blk);
    SW_ZERO(coeff_blk + 2);
    coeff_blk += 8;
    SW_ZERO(coeff_blk);
    SW_ZERO(coeff_blk + 2);
    coeff_blk += 8;
    SW_ZERO(coeff_blk);
    SW_ZERO(coeff_blk + 2);
    coeff_blk += 8;
    SW_ZERO(coeff_blk);
    SW_ZERO(coeff_blk + 2);
    coeff_blk += 8;
    SW_ZERO(coeff_blk);
    SW_ZERO(coeff_blk + 2);
    coeff_blk += 8;
    SW_ZERO(coeff_blk);
    SW_ZERO(coeff_blk + 2);
    TRANSPOSE8x8_SH_SH(in_vec0, in_vec1, in_vec2, in_vec3,
                       in_vec4, in_vec5, in_vec6, in_vec7,
                       in_vec0, in_vec1, in_vec2, in_vec3,
                       in_vec4, in_vec5, in_vec6, in_vec7);

    UNPCK_SH_SW(in_vec0, in_vec0_r, in_vec0_l);
    UNPCK_SH_SW(in_vec1, in_vec1_r, in_vec1_l);
    UNPCK_SH_SW(in_vec2, in_vec2_r, in_vec2_l);
    UNPCK_SH_SW(in_vec3, in_vec3_r, in_vec3_l);
    const8192 = __msa_fill_w(8192);
    in_vec0_r <<= 8;
    in_vec0_r += const8192;
    in_vec0_l <<= 8;
    in_vec0_l += const8192;
    const_w2 = __msa_fill_w(W2);
    const_w6 = __msa_fill_w(W6);
    in_vec4_r = in_vec0_r;
    in_vec6_r = const_w6 * in_vec2_r;
    in_vec6_r += 4;
    in_vec6_r >>= 3;
    in_vec2_r *= const_w2;
    in_vec2_r += 4;
    in_vec2_r >>= 3;
    in_vec4_l = in_vec0_l;
    in_vec6_l = const_w6 * in_vec2_l;
    in_vec6_l += 4;
    in_vec6_l >>= 3;
    in_vec2_l *= const_w2;
    in_vec2_l += 4;
    in_vec2_l >>= 3;
    in_vec8_r = in_vec0_r - in_vec2_r;
    in_vec0_r += in_vec2_r;
    in_vec2_r = in_vec8_r;
    in_vec8_r = in_vec4_r - in_vec6_r;
    in_vec4_r += in_vec6_r;
    in_vec6_r = in_vec8_r;
    in_vec8_l = in_vec0_l - in_vec2_l;
    in_vec0_l += in_vec2_l;
    in_vec2_l = in_vec8_l;
    in_vec8_l = in_vec4_l - in_vec6_l;
    in_vec4_l += in_vec6_l;
    in_vec6_l = in_vec8_l;
    const_w1 = __msa_fill_w(W1);
    const_w3 = __msa_fill_w(W3);
    const_w5 = __msa_fill_w(W5);
    const_w7 = __msa_fill_w(W7);
    in_vec7_r = const_w7 * in_vec1_r;
    in_vec7_r += 4;
    in_vec7_r >>= 3;
    in_vec1_r *= const_w1;
    in_vec1_r += 4;
    in_vec1_r >>= 3;
    in_vec5_r = const_w3 * in_vec3_r;
    in_vec5_r += 4;
    in_vec5_r >>= 3;
    in_vec3_r *= -const_w5;
    in_vec3_r += 4;
    in_vec3_r >>= 3;
    in_vec7_l = const_w7 * in_vec1_l;
    in_vec7_l += 4;
    in_vec7_l >>= 3;
    in_vec1_l *= const_w1;
    in_vec1_l += 4;
    in_vec1_l >>= 3;
    in_vec5_l = const_w3 * in_vec3_l;
    in_vec5_l += 4;
    in_vec5_l >>= 3;
    in_vec3_l *= -const_w5;
    in_vec3_l += 4;
    in_vec3_l >>= 3;
    in_vec8_r = in_vec1_r - in_vec5_r;
    in_vec1_r += in_vec5_r;
    in_vec5_r = in_vec8_r;
    in_vec8_r = in_vec7_r - in_vec3_r;
    in_vec3_r += in_vec7_r;
    in_vec8_l = in_vec1_l - in_vec5_l;
    in_vec1_l += in_vec5_l;
    in_vec5_l = in_vec8_l;
    in_vec8_l = in_vec7_l - in_vec3_l;
    in_vec3_l += in_vec7_l;
    const181 = __msa_ldi_w(181);
    const128 = __msa_ldi_w(128);
    in_vec7_r = const181 * (in_vec5_r + in_vec8_r) + const128;
    in_vec7_r >>= 8;
    in_vec5_r = const181 * (in_vec5_r - in_vec8_r) + const128;
    in_vec5_r >>= 8;
    in_vec7_l = const181 * (in_vec5_l + in_vec8_l) + const128;
    in_vec7_l >>= 8;
    in_vec5_l = const181 * (in_vec5_l - in_vec8_l) + const128;
    in_vec5_l >>= 8;
    BUTTERFLY_8(in_vec0_r, in_vec4_r, in_vec6_r, in_vec2_r,
                in_vec3_r, in_vec5_r, in_vec7_r, in_vec1_r,
                out_vec0, out_vec1, out_vec2, out_vec3,
                out_vec4, out_vec5, out_vec6, out_vec7);
    in_vec0_r = out_vec0 >> 14;
    in_vec1_r = out_vec1 >> 14;
    in_vec2_r = out_vec2 >> 14;
    in_vec3_r = out_vec3 >> 14;
    in_vec4_r = out_vec4 >> 14;
    in_vec5_r = out_vec5 >> 14;
    in_vec6_r = out_vec6 >> 14;
    in_vec7_r = out_vec7 >> 14;
    BUTTERFLY_8(in_vec0_l, in_vec4_l, in_vec6_l, in_vec2_l,
                in_vec3_l, in_vec5_l, in_vec7_l, in_vec1_l,
                out_vec0, out_vec1, out_vec2, out_vec3,
                out_vec4, out_vec5, out_vec6, out_vec7);
    in_vec0_l = out_vec0 >> 14;
    in_vec1_l = out_vec1 >> 14;
    in_vec2_l = out_vec2 >> 14;
    in_vec3_l = out_vec3 >> 14;
    in_vec4_l = out_vec4 >> 14;
    in_vec5_l = out_vec5 >> 14;
    in_vec6_l = out_vec6 >> 14;
    in_vec7_l = out_vec7 >> 14;
    PCKEV_H4_SH(in_vec0_l, in_vec0_r, in_vec1_l, in_vec1_r, in_vec2_l, in_vec2_r,
                in_vec3_l, in_vec3_r, out0, out1, out2, out3);
    PCKEV_H4_SH(in_vec4_l, in_vec4_r, in_vec5_l, in_vec5_r, in_vec6_l, in_vec6_r,
                in_vec7_l, in_vec7_r, out4, out5, out6, out7);
    TRANSPOSE8x8_SH_SH(out0, out1, out2, out3, out4, out5, out6, out7,
                       out0, out1, out2, out3, out4, out5, out6, out7);

    LD_UB4(pred, 16, pred_data0, pred_data1, pred_data2, pred_data3);
    ILVR_B4_SH(zero, pred_data0, zero, pred_data1, zero, pred_data2, zero, pred_data3,
               pred0, pred1, pred2, pred3);
    ADD4(out0, pred0, out1, pred1, out2, pred2, out3, pred3, res0, res1, res2, res3);
    CLIP_SH4_0_255(res0, res1, res2, res3);
    PCKEV_B2_SB(res1, res0, res3, res2, tmp0, tmp1);
    ST8x4_UB(tmp0, tmp1, dst, dst_stride);

    LD_UB4(pred + 16 * 4, 16, pred_data0, pred_data1, pred_data2, pred_data3);
    ILVR_B4_SH(zero, pred_data0, zero, pred_data1, zero, pred_data2, zero, pred_data3,
               pred0, pred1, pred2, pred3);
    ADD4(out4, pred0, out5, pred1, out6, pred2, out7, pred3, res0, res1, res2, res3);
    CLIP_SH4_0_255(res0, res1, res2, res3);
    PCKEV_B2_SB(res1, res0, res3, res2, tmp0, tmp1);
    ST8x4_UB(tmp0, tmp1, dst + 4 * dst_stride, dst_stride);
}
#endif /* M4VH263DEC_MSA */
