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

#ifdef H264ENC_MSA
void intra_predict_hor_vert_dc_16x16_msa(uint8 *src_top,
                                         uint8 *src_left,
                                         int32 src_stride_left,
                                         uint8 *dst_vert,
                                         uint8 *dst_horiz,
                                         uint8 *dst_dc,
                                         int32 dst_stride,
                                         uint8 is_above,
                                         uint8 is_left)
{
    uint32 row;
    uint32 addition = 0;
    uint8 inp0, inp1;
    v16u8 store, src0, src1, src_above = { 0 };
    v8u16 sum_above;
    v4u32 add;
    v2u64 sum;

    if (is_above)
    {
        src_above = LD_UB(src_top);

        for (row = 16; row--;)
        {
            ST_UB(src_above, dst_vert);
            dst_vert += dst_stride;
        }
    }
    if (is_left)
    {
        for (row = 0; row < 8; row++)
        {
            inp0 = src_left[(row * 2) * src_stride_left];
            inp1 = src_left[(row * 2 + 1) * src_stride_left];
            addition += inp0;
            addition += inp1;
            src0 = (v16u8) __msa_fill_b(inp0);
            src1 = (v16u8) __msa_fill_b(inp1);
            ST_UB2(src0, src1, dst_horiz, dst_stride);
            dst_horiz += (2 * dst_stride);
        }
    }
    if (is_left && is_above)
    {
        sum_above = __msa_hadd_u_h(src_above, src_above);
        add = __msa_hadd_u_w(sum_above, sum_above);
        sum = __msa_hadd_u_d(add, add);
        add = (v4u32) __msa_pckev_w((v4i32) sum, (v4i32) sum);
        sum = __msa_hadd_u_d(add, add);
        addition += __msa_copy_u_w((v4i32) sum, 0);
        addition = (addition + 16) >> 5;
        store = (v16u8) __msa_fill_b(addition);
    }
    else if (is_left)
    {
        addition = (addition + 8) >> 4;
        store = (v16u8) __msa_fill_b(addition);
    }
    else if (is_above)
    {
        sum_above = __msa_hadd_u_h(src_above, src_above);
        add = __msa_hadd_u_w(sum_above, sum_above);
        sum = __msa_hadd_u_d(add, add);
        add = (v4u32) __msa_pckev_w((v4i32) sum, (v4i32) sum);
        sum = __msa_hadd_u_d(add, add);
        sum = (v2u64) __msa_srari_d((v2i64) sum, 4);
        store = (v16u8) __msa_splati_b((v16i8) sum, 0);
    }
    else
    {
        store = (v16u8) __msa_ldi_b(128);
    }

    for (row = 16; row--;)
    {
        ST_UB(store, dst_dc);
        dst_dc += dst_stride;
    }
}
#endif /* #ifdef H264ENC_MSA */
