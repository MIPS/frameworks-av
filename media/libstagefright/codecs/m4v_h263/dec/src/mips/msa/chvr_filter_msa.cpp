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

#include "post_proc.h"
#include "common_macros_msa.h"
#include "prototypes_msa.h"
#include "mp4dec_lib.h"

#ifdef PV_POSTPROC_ON
#ifdef M4VH263DEC_MSA
void HardFiltHorMSA(uint8 *ptr, int qp, int width)
{
    v8u16 sum, delta;
    v8i16 k_th_h, qp_vec;
    v8i16 minus_k_th_h, minus_qp;
    v16i8 zero = { 0 };
    v8u16 v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11;
    v16u8 v0_org, v1_org, v2_org, v3_org, v4_org, v5_org;
    v16u8 v6_org, v7_org, v8_org, v9_org, v10_org, v11_org;
    v8i16 mask, mask0, mask1, mask2, mask3;
    v8i16 v6_sub_v5;

    LD_UB8(ptr - 6 * width, width, v0_org, v1_org, v2_org, v3_org,
           v4_org, v5_org, v6_org, v7_org);
    LD_UB4(ptr + 2 * width, width, v8_org, v9_org, v10_org, v11_org);

    v0 = (v8u16) __msa_ilvr_b(zero, (v16i8) v0_org);
    v1 = (v8u16) __msa_ilvr_b(zero, (v16i8) v1_org);
    v2 = (v8u16) __msa_ilvr_b(zero, (v16i8) v2_org);
    v3 = (v8u16) __msa_ilvr_b(zero, (v16i8) v3_org);
    v4 = (v8u16) __msa_ilvr_b(zero, (v16i8) v4_org);
    v5 = (v8u16) __msa_ilvr_b(zero, (v16i8) v5_org);
    v6 = (v8u16) __msa_ilvr_b(zero, (v16i8) v6_org);
    v7 = (v8u16) __msa_ilvr_b(zero, (v16i8) v7_org);
    v8 = (v8u16) __msa_ilvr_b(zero, (v16i8) v8_org);
    v9 = (v8u16) __msa_ilvr_b(zero, (v16i8) v9_org);
    v10 = (v8u16) __msa_ilvr_b(zero, (v16i8) v10_org);
    v11 = (v8u16) __msa_ilvr_b(zero, (v16i8) v11_org);

    /* a3_0 = *ptr - *(ptr - w1); */
    v6_sub_v5 = (v8i16) (v6 - v5);

    k_th_h = __msa_ldi_h(KThH);
    minus_k_th_h = __msa_ldi_h(-KThH);
    qp_vec = __msa_fill_h(qp);
    minus_qp = __msa_fill_h(-qp);

    /* if ((a3_0 > KThH || a3_0 < -KThH) && a3_0<qp && a3_0> -qp) */
    mask0 = __msa_clt_s_h(k_th_h, v6_sub_v5);
    mask1 = __msa_clt_s_h(v6_sub_v5, minus_k_th_h);
    mask2 = __msa_clt_s_h(v6_sub_v5, qp_vec);
    mask3 = __msa_clt_s_h(minus_qp, v6_sub_v5);

    mask = mask0 | mask1;
    mask = mask & mask2;
    mask = mask & mask3;
    mask = (v8i16) __msa_pckev_b(zero, (v16i8) mask);

    if (!__msa_test_bz_v((v16u8) mask))
    {
        sum = v0 + v1 + v2 + v3 + v4 + v5 + v6;

        ptr -= 3 * width;

        delta = sum + v3;
        delta = (v8u16) __msa_srari_h((v8i16) delta, 3);
        delta = (v8u16) __msa_pckev_b(zero, (v16i8) delta);
        v3_org = __msa_bmnz_v(v3_org, (v16u8) delta, (v16u8) mask);
        ST_UB(v3_org, ptr);
        ptr += width;

        sum = sum - v0 + v7;
        delta = sum + v4;
        delta = (v8u16) __msa_srari_h((v8i16) delta, 3);
        delta = (v8u16) __msa_pckev_b(zero, (v16i8) delta);
        v4_org = __msa_bmnz_v(v4_org, (v16u8) delta, (v16u8) mask);
        ST_UB(v4_org, ptr);
        ptr += width;

        sum = sum - v1 + v8;
        delta = sum + v5;
        delta = (v8u16) __msa_srari_h((v8i16) delta, 3);
        delta = (v8u16) __msa_pckev_b(zero, (v16i8) delta);
        v5_org = __msa_bmnz_v(v5_org, (v16u8) delta, (v16u8) mask);
        ST_UB(v5_org, ptr);
        ptr += width;

        sum = sum - v2 + v9;
        delta = sum + v6;
        delta = (v8u16) __msa_srari_h((v8i16) delta, 3);
        delta = (v8u16) __msa_pckev_b(zero, (v16i8) delta);
        v6_org = __msa_bmnz_v(v6_org, (v16u8) delta, (v16u8) mask);
        ST_UB(v6_org, ptr);
        ptr += width;

        sum = sum - v3 + v10;
        delta = sum + v7;
        delta = (v8u16) __msa_srari_h((v8i16) delta, 3);
        delta = (v8u16) __msa_pckev_b(zero, (v16i8) delta);
        v7_org = __msa_bmnz_v(v7_org, (v16u8) delta, (v16u8) mask);
        ST_UB(v7_org, ptr);
        ptr += width;

        sum = sum - v4 + v11;
        delta = sum + v8;
        delta = (v8u16) __msa_srari_h((v8i16) delta, 3);
        delta = (v8u16) __msa_pckev_b(zero, (v16i8) delta);
        v8_org = __msa_bmnz_v(v8_org, (v16u8) delta, (v16u8) mask);
        ST_UB(v8_org, ptr);
    }
}

void HardFiltVerMSA(uint8 *ptr, int qp, int width)
{
    v8u16 sum, k_th_h, qp_vec, delta;
    v8i16 minus_k_th_h, minus_qp;
    v16i8 zero = { 0 };
    v16u8 vec0, vec1, vec2, vec3, vec4;
    v8u16 v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11;
    v16u8 v0_org, v1_org, v2_org, v3_org, v4_org, v5_org;
    v16u8 v6_org, v7_org, v8_org, v9_org, v10_org, v11_org;
    v8i16 mask, mask0, mask1, mask2, mask3;
    v8i16 v6_sub_v5;
    v16u8 row0, row1, row2, row3, row4, row5, row6, row7;

    /* transpose 8x12 matrix into 12x8 */
    LD_UB8(ptr - 6, width, row0, row1, row2, row3, row4, row5, row6, row7);

    /* v0 to v7_org */
    TRANSPOSE8x8_UB_UB(row0, row1, row2, row3, row4, row5, row6, row7,
                       v0_org, v1_org, v2_org, v3_org, v4_org, v5_org, v6_org, v7_org);

    /* now get v8_org v9 v10 v11 */
    /* shift by 8 byte right */
    row0 = (v16u8) __msa_shf_w((v4i32) row0, 0xAA);
    row1 = (v16u8) __msa_shf_w((v4i32) row1, 0xAA);
    row2 = (v16u8) __msa_shf_w((v4i32) row2, 0xAA);
    row3 = (v16u8) __msa_shf_w((v4i32) row3, 0xAA);
    row4 = (v16u8) __msa_shf_w((v4i32) row4, 0xAA);
    row5 = (v16u8) __msa_shf_w((v4i32) row5, 0xAA);
    row6 = (v16u8) __msa_shf_w((v4i32) row6, 0xAA);
    row7 = (v16u8) __msa_shf_w((v4i32) row7, 0xAA);

    TRANSPOSE8x4_UB_UB(row0, row1, row2, row3, row4, row5, row6, row7,
                       v8_org, v9_org, v10_org, v11_org);

    v0 = (v8u16) __msa_ilvr_b(zero, (v16i8) v0_org);
    v1 = (v8u16) __msa_ilvr_b(zero, (v16i8) v1_org);
    v2 = (v8u16) __msa_ilvr_b(zero, (v16i8) v2_org);
    v3 = (v8u16) __msa_ilvr_b(zero, (v16i8) v3_org);
    v4 = (v8u16) __msa_ilvr_b(zero, (v16i8) v4_org);
    v5 = (v8u16) __msa_ilvr_b(zero, (v16i8) v5_org);
    v6 = (v8u16) __msa_ilvr_b(zero, (v16i8) v6_org);
    v7 = (v8u16) __msa_ilvr_b(zero, (v16i8) v7_org);
    v8 = (v8u16) __msa_ilvr_b(zero, (v16i8) v8_org);
    v9 = (v8u16) __msa_ilvr_b(zero, (v16i8) v9_org);
    v10 = (v8u16) __msa_ilvr_b(zero, (v16i8) v10_org);
    v11 = (v8u16) __msa_ilvr_b(zero, (v16i8) v11_org);

    v6_sub_v5 = (v8i16) (v6 - v5);
    k_th_h = (v8u16) __msa_ldi_h(KThH);
    minus_k_th_h = __msa_ldi_h(-KThH);
    qp_vec = (v8u16) __msa_fill_h(qp);
    minus_qp = __msa_fill_h(-qp);

    mask0 = __msa_clt_s_h((v8i16) k_th_h, v6_sub_v5);
    mask1 = __msa_clt_s_h(v6_sub_v5, minus_k_th_h);
    mask2 = __msa_clt_s_h(v6_sub_v5, (v8i16) qp_vec);
    mask3 = __msa_clt_s_h(minus_qp, v6_sub_v5);

    mask = mask0 | mask1;
    mask = mask & mask2;
    mask = mask & mask3;
    mask = (v8i16) __msa_pckev_b(zero, (v16i8) mask);

    if (!__msa_test_bz_v((v16u8) mask))
    {
        sum = v0 + v1 + v2 + v3 + v4 + v5 + v6;

        delta = sum + v3;
        delta = (v8u16) __msa_srari_h((v8i16) delta, 3);
        delta = (v8u16) __msa_pckev_b(zero, (v16i8) delta);
        v3_org = __msa_bmnz_v(v3_org, (v16u8) delta, (v16u8) mask);

        sum = sum - v0 + v7;
        delta = sum + v4;
        delta = (v8u16) __msa_srari_h((v8i16) delta, 3);
        delta = (v8u16) __msa_pckev_b(zero, (v16i8) delta);
        v4_org = __msa_bmnz_v(v4_org, (v16u8) delta, (v16u8) mask);

        sum = sum - v1 + v8;
        delta = sum + v5;
        delta = (v8u16) __msa_srari_h((v8i16) delta, 3);
        delta = (v8u16) __msa_pckev_b(zero, (v16i8) delta);
        v5_org = __msa_bmnz_v(v5_org, (v16u8) delta, (v16u8) mask);

        sum = sum - v2 + v9;
        delta = sum + v6;
        delta = (v8u16) __msa_srari_h((v8i16) delta, 3);
        delta = (v8u16) __msa_pckev_b(zero, (v16i8) delta);
        v6_org = __msa_bmnz_v(v6_org, (v16u8) delta, (v16u8) mask);

        sum = sum - v3 + v10;
        delta = sum + v7;
        delta = (v8u16) __msa_srari_h((v8i16) delta, 3);
        delta = (v8u16) __msa_pckev_b(zero, (v16i8) delta);
        v7_org = __msa_bmnz_v(v7_org, (v16u8) delta, (v16u8) mask);

        sum = sum - v4 + v11;
        delta = sum + v8;
        delta = (v8u16) __msa_srari_h((v8i16) delta, 3);
        delta = (v8u16) __msa_pckev_b(zero, (v16i8) delta);
        v8_org = __msa_bmnz_v(v8_org, (v16u8) delta, (v16u8) mask);

        vec0 = (v16u8) __msa_ilvr_b((v16i8) v4_org, (v16i8) v3_org);
        vec1 = (v16u8) __msa_ilvr_b((v16i8) v6_org, (v16i8) v5_org);
        vec2 = (v16u8) __msa_ilvr_h((v8i16) vec1, (v8i16) vec0);
        vec3 = (v16u8) __msa_ilvl_h((v8i16) vec1, (v8i16) vec0);
        vec4 = (v16u8) __msa_ilvr_b((v16i8) v8_org, (v16i8) v7_org);

        ptr -= 3;

        ST4x4_UB(vec2, vec2, 0, 1, 2, 3, ptr, width);
        ST2x4_UB(vec4, 0, ptr + 4, width);
        ptr += (4 * width);
        ST4x4_UB(vec3, vec3, 0, 1, 2, 3, ptr, width);
        ST2x4_UB(vec4, 4, ptr + 4, width);
    }
}

void SoftFiltHorMSA(uint8 *ptr, int qp, int width)
{
    v8u16 k_th_h, qp_vec;
    v8i16 minus_k_th_h;
    v16i8 zero = { 0 };
    v8u16 v0, v1, v2, v3, v4, v5, v6, v7;
    v16u8 v0_org, v1_org, v2_org, v3_org, v4_org, v5_org, v6_org, v7_org;
    v8i16 mask, mask0, mask1, mask2, mask3;
    v8i16 a3_0, a3_1, a3_2, temp;
    v8u16 absa3_0, absa3_1, absa3_2;
    v8i16 aa3_0, minus_absaa3_0, delta;

    LD_UB8(ptr - 4 * width, width, v0_org, v1_org, v2_org, v3_org,
           v4_org, v5_org, v6_org, v7_org);

    v0 = (v8u16) __msa_ilvr_b(zero, (v16i8) v0_org);
    v1 = (v8u16) __msa_ilvr_b(zero, (v16i8) v1_org);
    v2 = (v8u16) __msa_ilvr_b(zero, (v16i8) v2_org);
    v3 = (v8u16) __msa_ilvr_b(zero, (v16i8) v3_org);
    v4 = (v8u16) __msa_ilvr_b(zero, (v16i8) v4_org);
    v5 = (v8u16) __msa_ilvr_b(zero, (v16i8) v5_org);
    v6 = (v8u16) __msa_ilvr_b(zero, (v16i8) v6_org);
    v7 = (v8u16) __msa_ilvr_b(zero, (v16i8) v7_org);

    /* a3_0 = *(ptr) - *(ptr - w1); */
    a3_0 = (v8i16) (v4 - v3);

    k_th_h = (v8u16) __msa_ldi_h(KThH);
    minus_k_th_h = __msa_ldi_h(-KThH);
    qp_vec = (v8u16) __msa_fill_h(qp);

    /* if ((a3_0 > KTh || a3_0 < -KTh)) */
    mask0 = __msa_clt_s_h((v8i16) k_th_h, a3_0);
    mask1 = __msa_clt_s_h(a3_0, minus_k_th_h);
    mask = mask0 | mask1;

    /* if ((a3_0 > KTh || a3_0 < -KTh)) */
    if (!__msa_test_bz_v((v16u8) mask))
    {
        /* a3_0 += ((*(ptr - w2) - *(ptr + w1)) << 1) + (a3_0 << 2); */
        temp = a3_0 << 2;
        a3_0 += temp;
        temp = (v8i16) (v2 - v5);
        temp <<= 1;
        a3_0 += temp;

        /* if (PV_ABS(a3_0) < (qp << 3)) */
        absa3_0 = (v8u16) __msa_asub_s_h(a3_0, (v8i16) zero);
        qp_vec = qp_vec << 3;
        mask0 = __msa_clt_s_h((v8i16) absa3_0, (v8i16) qp_vec);

        /* if (PV_ABS(a3_0) < (qp << 3)) */
        if (!__msa_test_bz_v((v16u8) mask0))
        {
            a3_1 = (v8i16) (v2 - v1);
            temp = a3_1 << 2;
            a3_1 += temp;
            temp = (v8i16) (v0 - v3);
            temp <<= 1;
            a3_1 += temp;
            absa3_1 = (v8u16) __msa_asub_s_h(a3_1, (v8i16) zero);

            a3_2 = (v8i16) (v6 - v5);
            temp = a3_2 << 2;
            a3_2 += temp;
            temp = (v8i16) (v4 - v7);
            temp <<= 1;
            a3_2 += temp;
            absa3_2 = (v8u16) __msa_asub_s_h(a3_2, (v8i16) zero);

            aa3_0 = (v8i16) __msa_min_u_h(absa3_1, absa3_2);
            aa3_0 = (v8i16) absa3_0 - aa3_0;

            /* if (A3_0 > 0) */
            mask1 = __msa_clt_s_h((v8i16) zero, aa3_0);

            /* if (A3_0 > 0) */
            if (!__msa_test_bz_v((v16u8) mask1))
            {
                temp = aa3_0 << 2;
                aa3_0 += temp;
                aa3_0 = __msa_srari_h(aa3_0, 6);
                minus_absaa3_0 = (v8i16) zero - aa3_0;

                /* if (a3_0 > 0) */
                a3_0 = __msa_clt_s_h((v8i16) zero, a3_0);
                aa3_0 = (v8i16) __msa_bmnz_v((v16u8) aa3_0,
                                             (v16u8) minus_absaa3_0,
                                             (v16u8) a3_0);

                /* delta = (*(ptr - w1) - *(ptr)) >> 1; */
                delta = (v8i16) (v3 - v4);
                delta >>= 1;

                /* if (delta >= 0) */
                mask2 = ((v8i16) zero <= delta);

                /* if (delta >= A3_0) */
                mask3 = (aa3_0 <= delta);
                mask3 = mask2 & mask3;
                temp = __msa_max_s_h(aa3_0, (v8i16) zero);
                delta = (v8i16) __msa_bmnz_v((v16u8) delta,
                                             (v16u8) temp, (v16u8) mask3);

                /* if (!(delta >= 0)) */
                mask2 = (v8i16) __msa_xori_b((v16u8) mask2, 0xFF);
                /* if (A3_0 > 0) */
                mask3 = __msa_clt_s_h((v8i16) zero, aa3_0);
                mask3 = mask3 & mask2;
                delta = (v8i16) __msa_bmnz_v((v16u8) delta,
                                             (v16u8) zero, (v16u8) mask3);

                temp = __msa_max_s_h(aa3_0, delta);
                /* if (!(A3_0 > 0)) */
                mask3 = (v8i16) __msa_xori_b((v16u8) mask3, 0xFF);
                mask3 = mask3 & mask2;
                delta = (v8i16) __msa_bmnz_v((v16u8) delta,
                                             (v16u8) temp, (v16u8) mask3);

                v3 = (v8u16) ((v8i16) v3 - delta);
                v4 = (v8u16) ((v8i16) v4 + delta);

                v3 = (v8u16) __msa_pckev_b(zero, (v16i8) v3);
                v4 = (v8u16) __msa_pckev_b(zero, (v16i8) v4);
                mask2 = mask & mask0;
                mask2 = mask2 & mask1;
                mask2 = (v8i16) __msa_pckev_b(zero, (v16i8) mask2);
                v3_org = __msa_bmnz_v(v3_org, (v16u8) v3, (v16u8) mask2);
                v4_org = __msa_bmnz_v(v4_org, (v16u8) v4, (v16u8) mask2);

                ST_UB(v3_org, ptr - width);
                ST_UB(v4_org, ptr);
            }
        }
    }
}

void SoftFiltVerMSA(uint8 *ptr, int qp, int width)
{
    v8u16 k_th_h, qp_vec;
    v8i16 minus_k_th_h;
    v16i8 zero = { 0 };
    v16u8 v0_org, v1_org, v2_org, v3_org, v4_org, v5_org, v6_org, v7_org;
    v8u16 v0, v1, v2, v3, v4, v5, v6, v7;
    v8i16 mask, mask0, mask1, mask2, mask3;
    v8i16 a3_0, a3_1, a3_2, temp;
    v8u16 absa3_0, absa3_1, absa3_2;
    v8i16 aa3_0, minus_absaa3_0, delta;

    LD_UB8(ptr - 4, width, v0_org, v1_org, v2_org, v3_org,
           v4_org, v5_org, v6_org, v7_org);

    TRANSPOSE8x8_UB_UB(v0_org, v1_org, v2_org, v3_org, v4_org, v5_org, v6_org, v7_org,
                       v0_org, v1_org, v2_org, v3_org, v4_org, v5_org, v6_org, v7_org);

    v0 = (v8u16) __msa_ilvr_b(zero, (v16i8) v0_org);
    v1 = (v8u16) __msa_ilvr_b(zero, (v16i8) v1_org);
    v2 = (v8u16) __msa_ilvr_b(zero, (v16i8) v2_org);
    v3 = (v8u16) __msa_ilvr_b(zero, (v16i8) v3_org);
    v4 = (v8u16) __msa_ilvr_b(zero, (v16i8) v4_org);
    v5 = (v8u16) __msa_ilvr_b(zero, (v16i8) v5_org);
    v6 = (v8u16) __msa_ilvr_b(zero, (v16i8) v6_org);
    v7 = (v8u16) __msa_ilvr_b(zero, (v16i8) v7_org);

    a3_0 = (v8i16) (v4 - v3);

    k_th_h = (v8u16) __msa_ldi_h(KThH);
    minus_k_th_h = __msa_ldi_h(-KThH);
    qp_vec = (v8u16) __msa_fill_h(qp);

    /* if ((a3_0 > KTh || a3_0 < -KTh)) */
    mask0 = __msa_clt_s_h((v8i16) k_th_h, a3_0);
    mask1 = __msa_clt_s_h(a3_0, minus_k_th_h);
    mask = mask0 | mask1;

    if (!__msa_test_bz_v((v16u8) mask))
    {
        temp = a3_0 << 2;
        a3_0 = a3_0 + temp;
        temp = (v8i16) (v2 - v5);
        temp = temp << 1;
        a3_0 = a3_0 + temp;

        /* if (PV_ABS(a3_0) < (qp << 3)) */
        absa3_0 = (v8u16) __msa_asub_s_h(a3_0, (v8i16) zero);
        qp_vec = qp_vec << 3;
        mask0 = __msa_clt_s_h((v8i16) absa3_0, (v8i16) qp_vec);

        if (!__msa_test_bz_v((v16u8) mask0))
        {
            a3_1 = (v8i16) (v2 - v1);
            temp = a3_1 << 2;
            a3_1 += temp;
            temp = (v8i16) (v0 - v3);
            temp <<= 1;
            a3_1 += temp;
            absa3_1 = (v8u16) __msa_asub_s_h(a3_1, (v8i16) zero);

            a3_2 = (v8i16) (v6 - v5);
            temp = a3_2 << 2;
            a3_2 += temp;
            temp = (v8i16) (v4 - v7);
            temp <<= 1;
            a3_2 += temp;
            absa3_2 = (v8u16) __msa_asub_s_h(a3_2, (v8i16) zero);

            aa3_0 = (v8i16) __msa_min_u_h(absa3_1, absa3_2);
            aa3_0 = (v8i16) absa3_0 - aa3_0;

            /* if (A3_0 > 0) */
            mask1 = __msa_clt_s_h((v8i16) zero, aa3_0);

            if (!__msa_test_bz_v((v16u8) mask1))
            {
                temp = aa3_0 << 2;
                aa3_0 += temp;
                aa3_0 = __msa_srari_h(aa3_0, 6);
                minus_absaa3_0 = (v8i16) zero - aa3_0;
                /* if (a3_0 > 0) */
                a3_0 = __msa_clt_s_h((v8i16) zero, a3_0);
                aa3_0 = (v8i16) __msa_bmnz_v((v16u8) aa3_0,
                                             (v16u8) minus_absaa3_0,
                                             (v16u8) a3_0);

                delta = (v8i16) (v3 - v4);
                delta >>= 1;

                /* if (delta >= 0) */
                mask2 = ((v8i16) zero <= delta);
                /* if (delta >= A3_0) */
                mask3 = (aa3_0 <= delta);
                mask3 = mask2 & mask3;
                temp = __msa_max_s_h(aa3_0, (v8i16) zero);
                delta = (v8i16) __msa_bmnz_v((v16u8) delta,
                                             (v16u8) temp, (v16u8) mask3);

                /* if (!(delta >= 0)) */
                mask2 = (v8i16) __msa_xori_b((v16u8) mask2, 0xFF);
                /* if (A3_0 > 0) */
                mask3 = __msa_clt_s_h((v8i16) zero, aa3_0);
                mask3 = mask3 & mask2;
                delta = (v8i16) __msa_bmnz_v((v16u8) delta,
                                             (v16u8) zero, (v16u8) mask3);

                temp = __msa_max_s_h(aa3_0, delta);
                /* if (!(A3_0 > 0)) */
                mask3 = (v8i16) __msa_xori_b((v16u8) mask3, 0xFF);
                mask3 = mask3 & mask2;
                delta = (v8i16) __msa_bmnz_v((v16u8) delta,
                                             (v16u8) temp, (v16u8) mask3);

                v3 = (v8u16) ((v8i16) v3 - delta);
                v4 += delta;

                v3 = (v8u16) __msa_pckev_b(zero, (v16i8) v3);
                v4 = (v8u16) __msa_pckev_b(zero, (v16i8) v4);
                mask2 = mask & mask0;
                mask2 = mask2 & mask1;
                mask2 = (v8i16) __msa_pckev_b(zero, (v16i8) mask2);
                v3_org = __msa_bmnz_v(v3_org, (v16u8) v3, (v16u8) mask2);
                v4_org = __msa_bmnz_v(v4_org, (v16u8) v4, (v16u8) mask2);

                ptr -= 1;
                qp_vec = (v8u16) __msa_ilvr_b((v16i8) v4_org, (v16i8) v3_org);

                ST2x4_UB(qp_vec, 0, ptr, width);
                ptr += (4 * width);
                ST2x4_UB(qp_vec, 4, ptr, width);
            }
        }
    }
}
#endif /* M4VH263DEC_MSA */
#endif /* PV_POSTPROC_ON */
