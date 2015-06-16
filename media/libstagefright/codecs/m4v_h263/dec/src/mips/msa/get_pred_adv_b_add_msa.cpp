/*
 * Copyright (C) 2015 The Android Open Source Project
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

#ifdef M4VH263DEC_MSA
int32 m4v_h263_horiz_filter_msa(uint8 *src, uint8 *dst, int32 src_stride,
                                int32 pred_width_rnd)
{
    int32 dst_stride;
    uint32 rnd_type;
    v16u8 inp0, inp1, inp2, inp3, inp4, inp5, inp6, inp7;
    v16u8 inp0_sld1, inp1_sld1, inp2_sld1, inp3_sld1;
    v16u8 inp4_sld1, inp5_sld1, inp6_sld1, inp7_sld1;
    v8u16 src0, src1, src2, src3, src4, src5, src6, src7;
    v8i16 sum0, sum1, sum2, sum3, sum4, sum5, sum6, sum7;
    v8i16 rnd_type_vec;
    v16i8 tmp0, tmp1;

    dst_stride = (pred_width_rnd >> 1);
    rnd_type = 1 - (pred_width_rnd & 1);

    LD_UB8(src, src_stride, inp0, inp1, inp2, inp3, inp4, inp5, inp6, inp7);

    rnd_type_vec = __msa_fill_h(rnd_type);

    SLDI_B4_0_UB(inp0, inp1, inp2, inp3, inp0_sld1, inp1_sld1, inp2_sld1, inp3_sld1, 1);
    SLDI_B4_0_UB(inp4, inp5, inp6, inp7, inp4_sld1, inp5_sld1, inp6_sld1, inp7_sld1, 1);
    ILVR_B8_UH(inp0_sld1, inp0, inp1_sld1, inp1, inp2_sld1, inp2, inp3_sld1, inp3,
               inp4_sld1, inp4, inp5_sld1, inp5, inp6_sld1, inp6, inp7_sld1, inp7,
               src0, src1, src2, src3, src4, src5, src6, src7);
    HADD_UB4_SH(src0, src1, src2, src3, sum0, sum1, sum2, sum3);
    HADD_UB4_SH(src4, src5, src6, src7, sum4, sum5, sum6, sum7);
    SUB4(sum0, rnd_type_vec, sum1, rnd_type_vec, sum2, rnd_type_vec, sum3, rnd_type_vec,
         sum0, sum1, sum2, sum3);
    SUB4(sum4, rnd_type_vec, sum5, rnd_type_vec, sum6, rnd_type_vec, sum7, rnd_type_vec,
         sum4, sum5, sum6, sum7);
    SRARI_H4_SH(sum0, sum1, sum2, sum3, 1);
    SRARI_H4_SH(sum4, sum5, sum6, sum7, 1);
    PCKEV_B2_SB(sum1, sum0, sum3, sum2, tmp0, tmp1);
    ST8x4_UB(tmp0, tmp1, dst, dst_stride);
    PCKEV_B2_SB(sum5, sum4, sum7, sum6, tmp0, tmp1);
    ST8x4_UB(tmp0, tmp1, dst + 4 * dst_stride, dst_stride);

    return 1;
}

int32 m4v_h263_vert_filter_msa(uint8 *src, uint8 *dst, int32 src_stride,
                               int32 pred_width_rnd)
{
    int32 dst_stride;
    uint32 rnd_type;
    v16u8 inp0, inp1, inp2, inp3, inp4, inp5, inp6, inp7, inp8;
    v8u16 src0, src1, src2, src3, src4, src5, src6, src7;
    v8i16 sum0, sum1, sum2, sum3, sum4, sum5, sum6, sum7;
    v8i16 rnd_type_vec;
    v16i8 tmp0, tmp1;

    dst_stride = (pred_width_rnd >> 1);
    rnd_type = 1 - (pred_width_rnd & 1);

    LD_UB8(src, src_stride, inp0, inp1, inp2, inp3, inp4, inp5, inp6, inp7);
    src += (8 * src_stride);
    inp8 = LD_UB(src);

    rnd_type_vec = __msa_fill_h(rnd_type);

    ILVR_B8_UH(inp1, inp0, inp2, inp1, inp3, inp2, inp4, inp3,
               inp5, inp4, inp6, inp5, inp7, inp6, inp8, inp7,
               src0, src1, src2, src3, src4, src5, src6, src7);
    HADD_UB4_SH(src0, src1, src2, src3, sum0, sum1, sum2, sum3);
    HADD_UB4_SH(src4, src5, src6, src7, sum4, sum5, sum6, sum7);
    SUB4(sum0, rnd_type_vec, sum1, rnd_type_vec, sum2, rnd_type_vec, sum3, rnd_type_vec,
         sum0, sum1, sum2, sum3);
    SUB4(sum4, rnd_type_vec, sum5, rnd_type_vec, sum6, rnd_type_vec, sum7, rnd_type_vec,
         sum4, sum5, sum6, sum7);
    SRARI_H4_SH(sum0, sum1, sum2, sum3, 1);
    SRARI_H4_SH(sum4, sum5, sum6, sum7, 1);
    PCKEV_B2_SB(sum1, sum0, sum3, sum2, tmp0, tmp1);
    ST8x4_UB(tmp0, tmp1, dst, dst_stride);
    PCKEV_B2_SB(sum5, sum4, sum7, sum6, tmp0, tmp1);
    ST8x4_UB(tmp0, tmp1, dst + 4 * dst_stride, dst_stride);

    return 1;
}

int32 m4v_h263_hv_filter_msa(uint8 *src, uint8 *dst, int32 src_stride,
                             int32 pred_width_rnd)
{
    int32 dst_stride;
    uint32 rnd_type;
    v16u8 inp0, inp1, inp2, inp3, inp4, inp5, inp6, inp7, inp8;
    v16u8 inp0_sld2, inp1_sld2, inp2_sld2, inp3_sld2;
    v16u8 inp4_sld2, inp5_sld2, inp6_sld2, inp7_sld2, inp8_sld2;
    v8u16 src0, src1, src2, src3, src4, src5, src6, src7, src8;
    v8u16 src0_sld2, src1_sld2, src2_sld2, src3_sld2;
    v8u16 src4_sld2, src5_sld2, src6_sld2, src7_sld2, src8_sld2;
    v8u16 sum0, sum1, sum2, sum3, sum4, sum5, sum6, sum7;
    v8i16 rnd_type_vec;
    v16i8 tmp0, tmp1, zero = { 0 };

    dst_stride = (pred_width_rnd >> 1);
    rnd_type = 1 - (pred_width_rnd & 1);

    LD_UB8(src, src_stride, inp0, inp1, inp2, inp3, inp4, inp5, inp6, inp7);
    src += (8 * src_stride);
    inp8 = LD_UB(src);

    rnd_type_vec = __msa_fill_h(rnd_type);

    SLDI_B4_0_UB(inp0, inp1, inp2, inp3, inp0_sld2, inp1_sld2, inp2_sld2, inp3_sld2, 1);
    SLDI_B4_0_UB(inp4, inp5, inp6, inp7, inp4_sld2, inp5_sld2, inp6_sld2, inp7_sld2, 1);

    inp8_sld2 = (v16u8) __msa_sldi_b(zero, (v16i8) inp8, 1);

    ILVR_B8_UH(inp0_sld2, inp0, inp1_sld2, inp1, inp2_sld2, inp2,
               inp3_sld2, inp3, inp4_sld2, inp4, inp5_sld2, inp5,
               inp6_sld2, inp6, inp7_sld2, inp7,
               src0, src1, src2, src3, src4, src5, src6, src7);

    src8 = (v8u16) __msa_ilvr_b((v16i8) inp8_sld2, (v16i8) inp8);

    HADD_UB4_UH(src0, src1, src2, src3, src0_sld2, src1_sld2, src2_sld2, src3_sld2);
    HADD_UB4_UH(src4, src5, src6, src7, src4_sld2, src5_sld2, src6_sld2, src7_sld2);

    src8_sld2 = __msa_hadd_u_h((v16u8) src8, (v16u8) src8);
    sum0 = src0_sld2 + src1_sld2 - rnd_type_vec;
    sum1 = src1_sld2 + src2_sld2 - rnd_type_vec;
    sum2 = src2_sld2 + src3_sld2 - rnd_type_vec;
    sum3 = src3_sld2 + src4_sld2 - rnd_type_vec;
    sum4 = src4_sld2 + src5_sld2 - rnd_type_vec;
    sum5 = src5_sld2 + src6_sld2 - rnd_type_vec;
    sum6 = src6_sld2 + src7_sld2 - rnd_type_vec;
    sum7 = src7_sld2 + src8_sld2 - rnd_type_vec;

    SRARI_H4_UH(sum0, sum1, sum2, sum3, 2);
    SRARI_H4_UH(sum4, sum5, sum6, sum7, 2);
    PCKEV_B2_SB(sum1, sum0, sum3, sum2, tmp0, tmp1);
    ST8x4_UB(tmp0, tmp1, dst, dst_stride);
    PCKEV_B2_SB(sum5, sum4, sum7, sum6, tmp0, tmp1);
    ST8x4_UB(tmp0, tmp1, dst + 4 * dst_stride, dst_stride);

    return 1;
}

static void copy_8x8_msa(uint8 *src, int32 src_stride,
                         uint8 *dst, int32 dst_stride)
{
    uint64_t src0, src1;
    int32 loop_cnt;

    for (loop_cnt = 4; loop_cnt--;)
    {
        src0 = LD(src);
        src += src_stride;
        src1 = LD(src);
        src += src_stride;

        SD(src0, dst);
        dst += dst_stride;
        SD(src1, dst);
        dst += dst_stride;
    }
}

int32 m4v_h263_copy_msa(uint8 *src, uint8 *dst, int32 src_stride,
                        int32 pred_width_rnd)
{
    copy_8x8_msa(src, src_stride, dst, (pred_width_rnd >> 1));

    return 1;
}
#endif /* M4VH263DEC_MSA */
