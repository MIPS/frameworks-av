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
#include <stdio.h>
#include <string.h>

#include "h264bsd_image.h"
#include "h264bsd_util.h"
#include "h264bsd_neighbour.h"

#ifdef H264DEC_MSA
#include "h264bsd_types_msa.h"
#include "h264bsd_macros_msa.h"

extern u32 h264bsdBlockX[];
extern u32 h264bsdBlockY[];

static void copy_width8_msa(u8 *src, i32 src_stride,
                            u8 *dst, i32 dst_stride,
                            i32 height)
{
    i32 cnt;
    uint64_t out0, out1, out2, out3, out4, out5, out6, out7;
    v16u8 src0, src1, src2, src3, src4, src5, src6, src7;

    if (0 == height % 12)
    {
        for (cnt = (height / 12); cnt--;)
        {
            LD_UB8(src, src_stride,
                   src0, src1, src2, src3, src4, src5, src6, src7);
            src += (8 * src_stride);

            out0 = __msa_copy_u_d((v2i64) src0, 0);
            out1 = __msa_copy_u_d((v2i64) src1, 0);
            out2 = __msa_copy_u_d((v2i64) src2, 0);
            out3 = __msa_copy_u_d((v2i64) src3, 0);
            out4 = __msa_copy_u_d((v2i64) src4, 0);
            out5 = __msa_copy_u_d((v2i64) src5, 0);
            out6 = __msa_copy_u_d((v2i64) src6, 0);
            out7 = __msa_copy_u_d((v2i64) src7, 0);

            SD4(out0, out1, out2, out3, dst, dst_stride);
            dst += (4 * dst_stride);
            SD4(out4, out5, out6, out7, dst, dst_stride);
            dst += (4 * dst_stride);

            LD_UB4(src, src_stride, src0, src1, src2, src3);
            src += (4 * src_stride);

            out0 = __msa_copy_u_d((v2i64) src0, 0);
            out1 = __msa_copy_u_d((v2i64) src1, 0);
            out2 = __msa_copy_u_d((v2i64) src2, 0);
            out3 = __msa_copy_u_d((v2i64) src3, 0);

            SD4(out0, out1, out2, out3, dst, dst_stride);
            dst += (4 * dst_stride);
        }
    }
    else if (0 == height % 8)
    {
        for (cnt = height >> 3; cnt--;)
        {
            LD_UB8(src, src_stride,
                   src0, src1, src2, src3, src4, src5, src6, src7);
            src += (8 * src_stride);

            out0 = __msa_copy_u_d((v2i64) src0, 0);
            out1 = __msa_copy_u_d((v2i64) src1, 0);
            out2 = __msa_copy_u_d((v2i64) src2, 0);
            out3 = __msa_copy_u_d((v2i64) src3, 0);
            out4 = __msa_copy_u_d((v2i64) src4, 0);
            out5 = __msa_copy_u_d((v2i64) src5, 0);
            out6 = __msa_copy_u_d((v2i64) src6, 0);
            out7 = __msa_copy_u_d((v2i64) src7, 0);

            SD4(out0, out1, out2, out3, dst, dst_stride);
            dst += (4 * dst_stride);
            SD4(out4, out5, out6, out7, dst, dst_stride);
            dst += (4 * dst_stride);
        }
    }
    else if (0 == height % 4)
    {
        for (cnt = (height / 4); cnt--;)
        {
            LD_UB4(src, src_stride, src0, src1, src2, src3);
            src += (4 * src_stride);
            out0 = __msa_copy_u_d((v2i64) src0, 0);
            out1 = __msa_copy_u_d((v2i64) src1, 0);
            out2 = __msa_copy_u_d((v2i64) src2, 0);
            out3 = __msa_copy_u_d((v2i64) src3, 0);

            SD4(out0, out1, out2, out3, dst, dst_stride);
            dst += (4 * dst_stride);
        }
    }
    else if (0 == height % 2)
    {
        for (cnt = (height / 2); cnt--;)
        {
            LD_UB2(src, src_stride, src0, src1);
            src += (2 * src_stride);
            out0 = __msa_copy_u_d((v2i64) src0, 0);
            out1 = __msa_copy_u_d((v2i64) src1, 0);

            SD(out0, dst);
            dst += dst_stride;
            SD(out1, dst);
            dst += dst_stride;
        }
    }
}

static void copy_16multx8mult_msa(u8 *src, i32 src_stride,
                                  u8 *dst, i32 dst_stride,
                                  i32 height, i32 width)
{
    i32 cnt, loop_cnt;
    u8 *src_tmp, *dst_tmp;
    v16u8 src0, src1, src2, src3, src4, src5, src6, src7;

    for (cnt = (width >> 4); cnt--;)
    {
        src_tmp = src;
        dst_tmp = dst;

        for (loop_cnt = (height >> 3); loop_cnt--;)
        {
            LD_UB8(src_tmp, src_stride,
                   src0, src1, src2, src3, src4, src5, src6, src7);
            src_tmp += (8 * src_stride);

            ST_UB8(src0, src1, src2, src3, src4, src5, src6, src7,
                   dst_tmp, dst_stride);
            dst_tmp += (8 * dst_stride);
        }

        src += 16;
        dst += 16;
    }
}

static void copy_width16_msa(u8 *src, i32 src_stride,
                             u8 *dst, i32 dst_stride,
                             i32 height)
{
    i32 cnt;
    v16u8 src0, src1, src2, src3, src4, src5, src6, src7;

    if (0 == height % 12)
    {
        for (cnt = (height / 12); cnt--;)
        {
            LD_UB8(src, src_stride,
                   src0, src1, src2, src3, src4, src5, src6, src7);
            src += (8 * src_stride);
            ST_UB8(src0, src1, src2, src3, src4, src5, src6, src7,
                   dst, dst_stride);
            dst += (8 * dst_stride);

            LD_UB4(src, src_stride, src0, src1, src2, src3);
            src += (4 * src_stride);
            ST_UB4(src0, src1, src2, src3, dst, dst_stride);
            dst += (4 * dst_stride);
        }
    }
    else if (0 == height % 8)
    {
        copy_16multx8mult_msa(src, src_stride, dst, dst_stride, height, 16);
    }
    else if (0 == height % 4)
    {
        for (cnt = (height >> 2); cnt--;)
        {
            LD_UB4(src, src_stride, src0, src1, src2, src3);
            src += (4 * src_stride);

            ST_UB4(src0, src1, src2, src3, dst, dst_stride);
            dst += (4 * dst_stride);
        }
    }
}

static void avc_addblock_clip_luma_msa(u8 *src, i16 *residual,
                                       u8 *dst, u32 dst_stride)
{
    u8 *dst_ptr, *tmp;
    i16 *res_ptr;
    i32 block, tmp1, tmp2, tmp3, tmp4;
    u32 hdev, vdev;
    v16u8 data0, data1, data2, data3, tp;
    v8i16 res0, res1, temp0, temp1;
    v4i32 out;

    for (block = 0; block < 16; block++)
    {
        hdev = h264bsdBlockX[block];
        vdev = h264bsdBlockY[block];

        res_ptr = residual + (block * 16);
        tmp = src + vdev * 16 + hdev;
        dst_ptr = dst + vdev * dst_stride + hdev;

        if (IS_RESIDUAL_EMPTY(res_ptr))
        {
            i32 *in32 = (i32 *) tmp;
            i32 *out32 = (i32 *) dst_ptr;

            LW4(in32, 4, tmp1, tmp2, tmp3, tmp4);
            SW4(tmp1, tmp2, tmp3, tmp4, out32, (dst_stride / 4));
        }
        else
        {
            LD_SH2(res_ptr, 8, res0, res1);
            LD_UB4(tmp, 16, data0, data1, data2, data3);

            temp0 = (v8i16) __msa_pckev_w((v4i32) data1, (v4i32) data0);
            temp1 = (v8i16) __msa_pckev_w((v4i32) data3, (v4i32) data2);
            tp = (v16u8) __msa_pckev_w((v4i32) temp1, (v4i32) temp0);

            UNPCK_UB_SH(tp, temp0, temp1);

            res0 += temp0;
            res1 += temp1;

            CLIP_SH2_0_255(res0, res1);
            out = (v4i32) __msa_pckev_b((v16i8) res1, (v16i8) res0);
            ST4x4_UB(out, out, 0, 1, 2, 3, dst_ptr, dst_stride)
        }
    }
}

static void avc_addblock_clip_chroma_msa(u8 *src, i16 *residual,
                                         u8 *dst_cb, u8 *dst_cr,
                                         u32 dst_stride)
{
    u8 *dst_ptr, *tmp;
    i16 *res_ptr;
    u32 block, hdev, vdev;
    i32 tmp1, tmp2, tmp3, tmp4;
    v16u8 data0, data1, tp;
    v8i16 res0, res1, temp0, temp1;
    v4i32 out;

    for (block = 16; block <= 23; block++)
    {
        hdev = h264bsdBlockX[block & 0x3];
        vdev = h264bsdBlockY[block & 0x3];

        res_ptr = residual + (block * 16);

        tmp = src + 256;
        dst_ptr = dst_cb;

        if (block >= 20)
        {
            dst_ptr = dst_cr;
            tmp += 64;
        }

        tmp += vdev * 8 + hdev;
        dst_ptr += vdev * dst_stride + hdev;

        if (IS_RESIDUAL_EMPTY(res_ptr))
        {
            i32 *in32 = (i32 *) tmp;
            i32 *out32 = (i32 *) dst_ptr;

            LW4(in32, 2, tmp1, tmp2, tmp3, tmp4);
            SW4(tmp1, tmp2, tmp3, tmp4, out32, (dst_stride / 4));
        }
        else
        {
            LD_SH2(res_ptr, 8, res0, res1);
            LD_UB2(tmp, 16, data0, data1);

            tp = (v16u8) __msa_pckev_w((v4i32) data1, (v4i32) data0);

            UNPCK_UB_SH(tp, temp0, temp1);

            res0 += temp0;
            res1 += temp1;

            CLIP_SH2_0_255(res0, res1);
            out = (v4i32) __msa_pckev_b((v16i8) res1, (v16i8) res0);
            ST4x4_UB(out, out, 0, 1, 2, 3, dst_ptr, dst_stride)
        }
    }
}

/*------------------------------------------------------------------------------

    Function: h264bsdWriteMacroblock

        Functional description:
            Write one macroblock into the image. Both luma and chroma
            components will be written at the same time.

        Inputs:
            data    pointer to macroblock data to be written, 256 values for
                    luma followed by 64 values for both chroma components

        Outputs:
            image   pointer to the image where the macroblock will be written

        Returns:
            none

------------------------------------------------------------------------------*/
void h264bsdWriteMacroblock(image_t *image, u8 *data)
{
    ASSERT(image);
    ASSERT(data);
    ASSERT(!((u32)data&0x3));

    copy_width16_msa(data, 16, image->luma, (image->width * 16), 16);
    copy_width8_msa(data + 256, 8, image->cb, (image->width * 8), 8);
    copy_width8_msa(data + 320, 8, image->cr, (image->width * 8), 8);
}

#ifndef H264DEC_OMXDL
/*------------------------------------------------------------------------------

    Function: h264bsdWriteOutputBlocks

        Functional description:
            Write one macroblock into the image. Prediction for the macroblock
            and the residual are given separately and will be combined while
            writing the data to the image

        Inputs:
            data        pointer to macroblock prediction data, 256 values for
                        luma followed by 64 values for both chroma components
            mbNum       number of the macroblock
            residual    pointer to residual data, 16 16-element arrays for luma
                        followed by 4 16-element arrays for both chroma
                        components

        Outputs:
            image       pointer to the image where the data will be written

        Returns:
            none

------------------------------------------------------------------------------*/
void h264bsdWriteOutputBlocks(image_t *image, u32 mbNum, u8 *data,
                              i16 residual[][16])
{
    u32 picWidth, picSize;
    u8 *lum, *cb, *cr;
    u8 *imageBlock, *tmp;
    u32 row, col, block;
    i16 *residue = &residual[0][0];

    /* Image size in macroblocks */
    picWidth = image->width;
    picSize = picWidth * image->height;
    row = mbNum / picWidth;
    col = mbNum % picWidth;

    /* Output macroblock position in output picture */
    lum = (image->data + row * picWidth * 256 + col * 16);
    cb = (image->data + picSize * 256 + row * picWidth * 64 + col * 8);
    cr = (cb + picSize * 64);

    picWidth *= 16;
    avc_addblock_clip_luma_msa(data, residue, lum, picWidth);
    picWidth /= 2;
    avc_addblock_clip_chroma_msa(data, residue, cb, cr, picWidth);
}
#endif /* #ifndef H264DEC_OMXDL */
#endif /* #ifdef H264DEC_MSA */
