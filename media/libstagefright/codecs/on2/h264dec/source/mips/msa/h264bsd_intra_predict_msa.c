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

#include "h264bsd_intra_prediction.h"
#include "h264bsd_macros_msa.h"

#ifdef H264DEC_MSA
static void intra_predict_vert_4x4_msa(u8 *src, u8 *dst,
                                       i32 dst_stride)
{
    u32 src_data;

    src_data = LW(src);

    SW4(src_data, src_data, src_data, src_data, dst, dst_stride);
}

static void intra_predict_vert_8x8_msa(u8 *src, u8 *dst,
                                       i32 dst_stride)
{
    u32 row;
    u32 src_data1, src_data2;

    src_data1 = LW(src);
    src_data2 = LW(src + 4);

    for (row = 8; row--;)
    {
        SW(src_data1, dst);
        SW(src_data2, (dst + 4));
        dst += dst_stride;
    }
}

static void intra_predict_vert_16x16_msa(u8 *src, u8 *dst,
                                         i32 dst_stride)
{
    u32 row;
    v16u8 src0;

    src0 = LD_UB(src);

    for (row = 16; row--;)
    {
        ST_UB(src0, dst);
        dst += dst_stride;
    }
}

static void intra_predict_horiz_4x4_msa(u8 *src, u8 *dst,
                                        i32 dst_stride)
{
    u32 out0, out1, out2, out3;

    out0 = src[0] * 0x01010101;
    out1 = src[1] * 0x01010101;
    out2 = src[2] * 0x01010101;
    out3 = src[3] * 0x01010101;

    SW4(out0, out1, out2, out3, dst, dst_stride);
}

static void intra_predict_horiz_8x8_msa(u8 *src, u8 *dst,
                                        i32 dst_stride)
{
    uint64_t out0, out1, out2, out3, out4, out5, out6, out7;

    out0 = src[0] * 0x0101010101010101ull;
    out1 = src[1] * 0x0101010101010101ull;
    out2 = src[2] * 0x0101010101010101ull;
    out3 = src[3] * 0x0101010101010101ull;
    out4 = src[4] * 0x0101010101010101ull;
    out5 = src[5] * 0x0101010101010101ull;
    out6 = src[6] * 0x0101010101010101ull;
    out7 = src[7] * 0x0101010101010101ull;

    SD4(out0, out1, out2, out3, dst, dst_stride);
    dst += (4 * dst_stride);
    SD4(out4, out5, out6, out7, dst, dst_stride);
}

static void intra_predict_horiz_16x16_msa(u8 *src, u8 *dst,
                                          i32 dst_stride)
{
    u32 row;
    u8 inp0, inp1, inp2, inp3;
    v16u8 src0, src1, src2, src3;

    for (row = 4; row--;)
    {
        inp0 = src[0];
        inp1 = src[1];
        inp2 = src[2];
        inp3 = src[3];
        src += 4;

        src0 = (v16u8) __msa_fill_b(inp0);
        src1 = (v16u8) __msa_fill_b(inp1);
        src2 = (v16u8) __msa_fill_b(inp2);
        src3 = (v16u8) __msa_fill_b(inp3);

        ST_UB4(src0, src1, src2, src3, dst, dst_stride);
        dst += (4 * dst_stride);
    }
}

static void intra_predict_dc_4x4_msa(u8 *src_top, u8 *src_left,
                                     u8 *dst, i32 dst_stride)
{
    u32 val0, val1;
    v16i8 store, src = { 0 };
    v8u16 sum_h;
    v4u32 sum_w;
    v2u64 sum_d;

    val0 = LW(src_top);
    val1 = LW(src_left);
    INSERT_W2_SB(val0, val1, src);
    sum_h = __msa_hadd_u_h((v16u8) src, (v16u8) src);
    sum_w = __msa_hadd_u_w(sum_h, sum_h);
    sum_d = __msa_hadd_u_d(sum_w, sum_w);
    sum_w = (v4u32) __msa_srari_w((v4i32) sum_d, 3);
    store = __msa_splati_b((v16i8) sum_w, 0);
    val0 = __msa_copy_u_w((v4i32) store, 0);

    SW4(val0, val0, val0, val0, dst, dst_stride);
}

static void intra_predict_dc_tl_4x4_msa(u8 *src, u8 *dst,
                                        i32 dst_stride)
{
    u32 val0;
    v16i8 store, data = { 0 };
    v8u16 sum_h;
    v4u32 sum_w;

    val0 = LW(src);
    data = (v16i8) __msa_insert_w((v4i32) data, 0, val0);
    sum_h = __msa_hadd_u_h((v16u8) data, (v16u8) data);
    sum_w = __msa_hadd_u_w(sum_h, sum_h);
    sum_w = (v4u32) __msa_srari_w((v4i32) sum_w, 2);
    store = __msa_splati_b((v16i8) sum_w, 0);
    val0 = __msa_copy_u_w((v4i32) store, 0);

    SW4(val0, val0, val0, val0, dst, dst_stride);
}

static void intra_predict_dc_16x16_msa(u8 *src_top, u8 *src_left,
                                       u8 *dst, i32 dst_stride)
{
    v16u8 top, left, out;
    v8u16 sum_h, sum_top, sum_left;
    v4u32 sum_w;
    v2u64 sum_d;

    top = LD_UB(src_top);
    left = LD_UB(src_left);
    HADD_UB2_UH(top, left, sum_top, sum_left);
    sum_h = sum_top + sum_left;
    sum_w = __msa_hadd_u_w(sum_h, sum_h);
    sum_d = __msa_hadd_u_d(sum_w, sum_w);
    sum_w = (v4u32) __msa_pckev_w((v4i32) sum_d, (v4i32) sum_d);
    sum_d = __msa_hadd_u_d(sum_w, sum_w);
    sum_w = (v4u32) __msa_srari_w((v4i32) sum_d, 5);
    out = (v16u8) __msa_splati_b((v16i8) sum_w, 0);

    ST_UB8(out, out, out, out, out, out, out, out, dst, dst_stride);
    dst += (8 * dst_stride);
    ST_UB8(out, out, out, out, out, out, out, out, dst, dst_stride);
}

static void intra_predict_dc_tl_16x16_msa(u8 *src, u8 *dst,
                                          i32 dst_stride)
{
    v16u8 data, out;
    v8u16 sum_h;
    v4u32 sum_w;
    v2u64 sum_d;

    data = LD_UB(src);
    sum_h = __msa_hadd_u_h(data, data);
    sum_w = __msa_hadd_u_w(sum_h, sum_h);
    sum_d = __msa_hadd_u_d(sum_w, sum_w);
    sum_w = (v4u32) __msa_pckev_w((v4i32) sum_d, (v4i32) sum_d);
    sum_d = __msa_hadd_u_d(sum_w, sum_w);
    sum_w = (v4u32) __msa_srari_w((v4i32) sum_d, 4);
    out = (v16u8) __msa_splati_b((v16i8) sum_w, 0);

    ST_UB8(out, out, out, out, out, out, out, out, dst, dst_stride);
    dst += (8 * dst_stride);
    ST_UB8(out, out, out, out, out, out, out, out, dst, dst_stride);
}

#define INTRA_PREDICT_VALDC_4X4_MSA(val)                                       \
static void intra_predict_##val##dc_4x4_msa(uint8_t *dst, int32_t dst_stride)  \
{                                                                              \
    uint32_t out;                                                              \
    v16i8 store = __msa_ldi_b(val);                                            \
                                                                               \
    out = __msa_copy_u_w((v4i32) store, 0);                                    \
                                                                               \
    SW4(out, out, out, out, dst, dst_stride);                                  \
}

INTRA_PREDICT_VALDC_4X4_MSA(128);

#define INTRA_PREDICT_VALDC_16X16_MSA(val)                                       \
static void intra_predict_##val##dc_16x16_msa(uint8_t *dst, int32_t dst_stride)  \
{                                                                                \
    v16u8 out = (v16u8) __msa_ldi_b(val);                                        \
                                                                                 \
    ST_UB8(out, out, out, out, out, out, out, out, dst, dst_stride);             \
    dst += (8 * dst_stride);                                                     \
    ST_UB8(out, out, out, out, out, out, out, out, dst, dst_stride);             \
}

INTRA_PREDICT_VALDC_16X16_MSA(128);

static void intra_predict_dc_chroma8x8_msa(u8 *src_above,
                                           u8 *src_left,
                                           i32 src_left_stride,
                                           u8 *dst,
                                           i32 dst_stride,
                                           u8 is_above,
                                           u8 is_left)
{
    u32 row;
    u32 addition1 = 0;
    u32 addition2 = 0;
    u32 addition3 = 0;
    u32 addition4 = 0;
    uint64_t out1, out2;
    v16u8 src_above_data;
    v16u8 store1, store2;
    v4i32 tmp1, tmp2;
    v8u16 sum_above;
    v4i32 sum;

    if (is_left && is_above)
    {
        src_above_data = LD_UB(src_above);

        sum_above = __msa_hadd_u_h(src_above_data, src_above_data);
        sum = (v4i32) __msa_hadd_u_w(sum_above, sum_above);
        addition1 = __msa_copy_u_w(sum, 0);
        addition2 = __msa_copy_u_w(sum, 1);
        addition4 = addition2;

        for (row = 0; row < 4; row++)
        {
            addition1 += src_left[row * src_left_stride];
            addition3 += src_left[(4 + row) * src_left_stride];
            addition4 += src_left[(4 + row) * src_left_stride];
        }
        addition1 = (addition1 + 4) >> 3;
        addition2 = (addition2 + 2) >> 2;
        addition3 = (addition3 + 2) >> 2;
        addition4 = (addition4 + 4) >> 3;
        tmp1 = (v4i32) __msa_fill_b(addition1);
        tmp2 = (v4i32) __msa_fill_b(addition2);
        store1 = (v16u8) __msa_ilvev_w(tmp2, tmp1);
        tmp1 = (v4i32) __msa_fill_b(addition3);
        tmp2 = (v4i32) __msa_fill_b(addition4);
        store2 = (v16u8) __msa_ilvev_w(tmp2, tmp1);
    }
    else if (is_left)
    {
        for (row = 0; row < 4; row++)
        {
            addition1 += src_left[row * src_left_stride];
            addition3 += src_left[(4 + row) * src_left_stride];
        }
        addition1 = (addition1 + 2) >> 2;
        addition3 = (addition3 + 2) >> 2;
        store1 = (v16u8) __msa_fill_b(addition1);
        store2 = (v16u8) __msa_fill_b(addition3);
    }
    else if (is_above)
    {
        src_above_data = LD_UB(src_above);

        sum_above = __msa_hadd_u_h(src_above_data, src_above_data);
        sum = (v4i32) __msa_hadd_u_w(sum_above, sum_above);
        sum = __msa_srari_w(sum, 2);
        addition1 = __msa_copy_u_w(sum, 0);
        addition2 = __msa_copy_u_w(sum, 1);
        tmp1 = (v4i32) __msa_fill_b(addition1);
        tmp2 = (v4i32) __msa_fill_b(addition2);
        store1 = (v16u8) __msa_ilvev_w(tmp2, tmp1);
        store2 = store1;
    }
    else
    {
        store1 = (v16u8) __msa_ldi_b(128);
        store2 = store1;
    }
    out1 = __msa_copy_u_d((v2i64) store1, 0);
    out2 = __msa_copy_u_d((v2i64) store2, 0);

    for (row = 4; row--;)
    {
        SD(out1, dst);
        SD(out2, (dst + 4 * dst_stride));
        dst += dst_stride;
    }
}

void Intra16x16VerticalPrediction(u8 *data, u8 *above)
{
    intra_predict_vert_16x16_msa(above, data, 16);
}

void Intra16x16HorizontalPrediction(u8 *data, u8 *left)
{
    intra_predict_horiz_16x16_msa(left, data, 16);
}

void Intra16x16DcPrediction(u8 *data, u8 *above, u8 *left,
                            u32 availableA, u32 availableB)
{
    if (availableA && availableB)
    {
        intra_predict_dc_16x16_msa(above, left, data, 16);
    }
    else if (availableA)
    {
        intra_predict_dc_tl_16x16_msa(left, data, 16);
    }
    else if (availableB)
    {
        intra_predict_dc_tl_16x16_msa(above, data, 16);
    }
    else
    {
        intra_predict_128dc_16x16_msa(data, 16);
    }
}

void Intra4x4VerticalPrediction(u8 *data, u8 *above)
{
    intra_predict_vert_4x4_msa(above, data, 4);
}

void Intra4x4HorizontalPrediction(u8 *data, u8 *left)
{
    intra_predict_horiz_4x4_msa(left, data, 4);
}

void Intra4x4DcPrediction(u8 *data, u8 *above, u8 *left,
                          u32 availableA, u32 availableB)
{
    if (availableA && availableB)
    {
        intra_predict_dc_4x4_msa(above, left, data, 4);
    }
    else if (availableA)
    {
        intra_predict_dc_tl_4x4_msa(left, data, 4);
    }
    else if (availableB)
    {
        intra_predict_dc_tl_4x4_msa(above, data, 4);
    }
    else
    {
        intra_predict_128dc_4x4_msa(data, 4);
    }
    
}

void IntraChromaHorizontalPrediction(u8 *data, u8 *left)
{
    intra_predict_horiz_8x8_msa(left, data, 8);
}

void IntraChromaVerticalPrediction(u8 *data, u8 *above)
{
    intra_predict_vert_8x8_msa(above, data, 8);
}

void IntraChromaDcPrediction(u8 *data, u8 *above, u8 *left,
                             u32 availableA, u32 availableB)
{
    intra_predict_dc_chroma8x8_msa(above, left, 1, data, 8,
                                   (u8)availableB, (u8)availableA);
}
#endif /* #ifdef H264DEC_MSA */
