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

#ifndef _COMMON_MACROS_MSA_H
#define _COMMON_MACROS_MSA_H

#include <stdint.h>
#include "types_msa.h"

#define LD_B(RTYPE, psrc) *((RTYPE *)(psrc))
#define LD_UB(...) LD_B(v16u8, __VA_ARGS__)

#define LD_H(RTYPE, psrc) *((RTYPE *)(psrc))

#define ST_B(RTYPE, in, pdst) *((RTYPE *)(pdst)) = (in)
#define ST_UB(...) ST_B(v16u8, __VA_ARGS__)

#define ST_H(RTYPE, in, pdst) *((RTYPE *)(pdst)) = (in)

#if (__mips_isa_rev >= 6)
    #define LH(psrc)                          \
    ( {                                       \
        uint8 *psrc_m = (uint8 *) (psrc);     \
        uint16 val_m;                         \
                                              \
        asm volatile (                        \
            "lh  %[val_m],  %[psrc_m]  \n\t"  \
                                              \
            : [val_m] "=r" (val_m)            \
            : [psrc_m] "m" (*psrc_m)          \
        );                                    \
                                              \
        val_m;                                \
    } )

    #define LW(psrc)                          \
    ( {                                       \
        uint8 *psrc_m = (uint8 *) (psrc);     \
        uint32 val_m;                         \
                                              \
        asm volatile (                        \
            "lw  %[val_m],  %[psrc_m]  \n\t"  \
                                              \
            : [val_m] "=r" (val_m)            \
            : [psrc_m] "m" (*psrc_m)          \
        );                                    \
                                              \
        val_m;                                \
    } )

    #if (__mips == 64)
        #define LD(psrc)                          \
        ( {                                       \
            uint8 *psrc_m = (uint8 *) (psrc);     \
            uint64_t val_m = 0;                   \
                                                  \
            asm volatile (                        \
                "ld  %[val_m],  %[psrc_m]  \n\t"  \
                                                  \
                : [val_m] "=r" (val_m)            \
                : [psrc_m] "m" (*psrc_m)          \
            );                                    \
                                                  \
            val_m;                                \
        } )
    #else  // !(__mips == 64)
        #define LD(psrc)                                              \
        ( {                                                           \
            uint8 *psrc_m = (uint8 *) (psrc);                         \
            uint32 val0_m, val1_m;                                    \
            uint64_t val_m = 0;                                       \
                                                                      \
            val0_m = LW(psrc_m);                                      \
            val1_m = LW(psrc_m + 4);                                  \
                                                                      \
            val_m = (uint64_t) (val1_m);                              \
            val_m = (uint64_t) ((val_m << 32) & 0xFFFFFFFF00000000);  \
            val_m = (uint64_t) (val_m | (uint64_t) val0_m);           \
                                                                      \
            val_m;                                                    \
        } )
    #endif  // (__mips == 64)

    #define SH(val, pdst)                     \
    {                                         \
        uint8 *pdst_m = (uint8 *) (pdst);     \
        uint16 val_m = (val);                 \
                                              \
        asm volatile (                        \
            "sh  %[val_m],  %[pdst_m]  \n\t"  \
                                              \
            : [pdst_m] "=m" (*pdst_m)         \
            : [val_m] "r" (val_m)             \
        );                                    \
    }

    #define SW(val, pdst)                     \
    {                                         \
        uint8 *pdst_m = (uint8 *) (pdst);     \
        uint32 val_m = (val);                 \
                                              \
        asm volatile (                        \
            "sw  %[val_m],  %[pdst_m]  \n\t"  \
                                              \
            : [pdst_m] "=m" (*pdst_m)         \
            : [val_m] "r" (val_m)             \
        );                                    \
    }

    #define SD(val, pdst)                     \
    {                                         \
        uint8 *pdst_m = (uint8 *) (pdst);     \
        uint64_t val_m = (val);               \
                                              \
        asm volatile (                        \
            "sd  %[val_m],  %[pdst_m]  \n\t"  \
                                              \
            : [pdst_m] "=m" (*pdst_m)         \
            : [val_m] "r" (val_m)             \
        );                                    \
    }

    #define SW_ZERO(pdst)                  \
    {                                      \
        uint8 *pdst_m = (uint8 *) (pdst);  \
                                           \
        asm volatile (                     \
            "sw  $0,  %[pdst_m]  \n\t"     \
                                           \
            : [pdst_m] "=m" (*pdst_m)      \
            :                              \
        );                                 \
    }
#else  // !(__mips_isa_rev >= 6)
    #define LH(psrc)                           \
    ( {                                        \
        uint8 *psrc_m = (uint8 *) (psrc);      \
        uint16 val_m;                          \
                                               \
        asm volatile (                         \
            "ulh  %[val_m],  %[psrc_m]  \n\t"  \
                                               \
            : [val_m] "=r" (val_m)             \
            : [psrc_m] "m" (*psrc_m)           \
        );                                     \
                                               \
        val_m;                                 \
    } )

    #define LW(psrc)                           \
    ( {                                        \
        uint8 *psrc_m = (uint8 *) (psrc);      \
        uint32 val_m;                          \
                                               \
        asm volatile (                         \
            "ulw  %[val_m],  %[psrc_m]  \n\t"  \
                                               \
            : [val_m] "=r" (val_m)             \
            : [psrc_m] "m" (*psrc_m)           \
        );                                     \
                                               \
        val_m;                                 \
    } )

    #if (__mips == 64)
        #define LD(psrc)                           \
        ( {                                        \
            uint8 *psrc_m = (uint8 *) (psrc);      \
            uint64_t val_m = 0;                    \
                                                   \
            asm volatile (                         \
                "uld  %[val_m],  %[psrc_m]  \n\t"  \
                                                   \
                : [val_m] "=r" (val_m)             \
                : [psrc_m] "m" (*psrc_m)           \
            );                                     \
                                                   \
            val_m;                                 \
        } )
    #else  // !(__mips == 64)
        #define LD(psrc)                                              \
        ( {                                                           \
            uint8 *psrc_m1 = (uint8 *) (psrc);                        \
            uint32 val0_m, val1_m;                                    \
            uint64_t val_m = 0;                                       \
                                                                      \
            val0_m = LW(psrc_m1);                                     \
            val1_m = LW(psrc_m1 + 4);                                 \
                                                                      \
            val_m = (uint64_t) (val1_m);                              \
            val_m = (uint64_t) ((val_m << 32) & 0xFFFFFFFF00000000);  \
            val_m = (uint64_t) (val_m | (uint64_t) val0_m);           \
                                                                      \
            val_m;                                                    \
        } )
    #endif  // (__mips == 64)

    #define SH(val, pdst)                      \
    {                                          \
        uint8 *pdst_m = (uint8 *) (pdst);      \
        uint16 val_m = (val);                  \
                                               \
        asm volatile (                         \
            "ush  %[val_m],  %[pdst_m]  \n\t"  \
                                               \
            : [pdst_m] "=m" (*pdst_m)          \
            : [val_m] "r" (val_m)              \
        );                                     \
    }

    #define SW(val, pdst)                      \
    {                                          \
        uint8 *pdst_m = (uint8 *) (pdst);      \
        uint32 val_m = (val);                  \
                                               \
        asm volatile (                         \
            "usw  %[val_m],  %[pdst_m]  \n\t"  \
                                               \
            : [pdst_m] "=m" (*pdst_m)          \
            : [val_m] "r" (val_m)              \
        );                                     \
    }

    #define SD(val, pdst)                                        \
    {                                                            \
        uint8 *pdst_m1 = (uint8 *) (pdst);                       \
        uint32 val0_m, val1_m;                                   \
                                                                 \
        val0_m = (uint32) ((val) & 0x00000000FFFFFFFF);          \
        val1_m = (uint32) (((val) >> 32) & 0x00000000FFFFFFFF);  \
                                                                 \
        SW(val0_m, pdst_m1);                                     \
        SW(val1_m, pdst_m1 + 4);                                 \
    }

    #define SW_ZERO(pdst)                  \
    {                                      \
        uint8 *pdst_m = (uint8 *) (pdst);  \
                                           \
        asm volatile (                     \
            "usw  $0,  %[pdst_m]  \n\t"    \
                                           \
            : [pdst_m] "=m" (*pdst_m)      \
            :                              \
        );                                 \
    }
#endif  // (__mips_isa_rev >= 6)

/* Description : Store 4 words with stride
   Arguments   : Inputs  - in0, in1, in2, in3, pdst, stride
   Details     : Stores word from 'in0' to (pdst)
                 Stores word from 'in1' to (pdst + stride)
                 Stores word from 'in2' to (pdst + 2 * stride)
                 Stores word from 'in3' to (pdst + 3 * stride)
*/
#define SW4(in0, in1, in2, in3, pdst, stride)  \
{                                              \
    SW(in0, (pdst))                            \
    SW(in1, (pdst) + stride);                  \
    SW(in2, (pdst) + 2 * stride);              \
    SW(in3, (pdst) + 3 * stride);              \
}

/* Description : Store 4 double words with stride
   Arguments   : Inputs  - in0, in1, in2, in3, pdst, stride
   Details     : Stores double word from 'in0' to (pdst)
                 Stores double word from 'in1' to (pdst + stride)
                 Stores double word from 'in2' to (pdst + 2 * stride)
                 Stores double word from 'in3' to (pdst + 3 * stride)
*/
#define SD4(in0, in1, in2, in3, pdst, stride)  \
{                                              \
    SD(in0, (pdst))                            \
    SD(in1, (pdst) + stride);                  \
    SD(in2, (pdst) + 2 * stride);              \
    SD(in3, (pdst) + 3 * stride);              \
}

/* Description : Load vectors with 16 byte elements with stride
   Arguments   : Inputs  - psrc    (source pointer to load from)
                         - stride
                 Outputs - out0, out1
                 Return Type - as per RTYPE
   Details     : Loads 16 byte elements in 'out0' from (psrc)
                 Loads 16 byte elements in 'out1' from (psrc + stride)
*/
#define LD_B2(RTYPE, psrc, stride, out0, out1)  \
{                                               \
    out0 = LD_B(RTYPE, (psrc));                 \
    out1 = LD_B(RTYPE, (psrc) + stride);        \
}

#define LD_B3(RTYPE, psrc, stride, out0, out1, out2)  \
{                                                     \
    LD_B2(RTYPE, (psrc), stride, out0, out1);         \
    out2 = LD_B(RTYPE, (psrc) + 2 * stride);          \
}
#define LD_UB3(...) LD_B3(v16u8, __VA_ARGS__)

#define LD_B4(RTYPE, psrc, stride, out0, out1, out2, out3)   \
{                                                            \
    LD_B2(RTYPE, (psrc), stride, out0, out1);                \
    LD_B2(RTYPE, (psrc) + 2 * stride , stride, out2, out3);  \
}
#define LD_UB4(...) LD_B4(v16u8, __VA_ARGS__)

#define LD_B6(RTYPE, psrc, stride, out0, out1, out2, out3, out4, out5)  \
{                                                                       \
    LD_B4(RTYPE, (psrc), stride, out0, out1, out2, out3);               \
    LD_B2(RTYPE, (psrc) + 4 * stride, stride, out4, out5);              \
}
#define LD_UB6(...) LD_B6(v16u8, __VA_ARGS__)

#define LD_B8(RTYPE, psrc, stride,                                      \
              out0, out1, out2, out3, out4, out5, out6, out7)           \
{                                                                       \
    LD_B4(RTYPE, (psrc), stride, out0, out1, out2, out3);               \
    LD_B4(RTYPE, (psrc) + 4 * stride, stride, out4, out5, out6, out7);  \
}
#define LD_UB8(...) LD_B8(v16u8, __VA_ARGS__)

/* Description : Load vectors with 8 halfword elements with stride
   Arguments   : Inputs  - psrc    (source pointer to load from)
                         - stride
                 Outputs - out0, out1
   Details     : Loads 8 halfword elements in 'out0' from (psrc)
                 Loads 8 halfword elements in 'out1' from (psrc + stride)
*/
#define LD_H2(RTYPE, psrc, stride, out0, out1)  \
{                                               \
    out0 = LD_H(RTYPE, (psrc));                 \
    out1 = LD_H(RTYPE, (psrc) + (stride));      \
}

#define LD_H4(RTYPE, psrc, stride, out0, out1, out2, out3)  \
{                                                           \
    LD_H2(RTYPE, (psrc), stride, out0, out1);               \
    LD_H2(RTYPE, (psrc) + 2 * stride, stride, out2, out3);  \
}
#define LD_SH4(...) LD_H4(v8i16, __VA_ARGS__)

/* Description : Store vectors of 16 byte elements with stride
   Arguments   : Inputs  - in0, in1, stride
                 Outputs - pdst    (destination pointer to store to)
   Details     : Stores 16 byte elements from 'in0' to (pdst)
                 Stores 16 byte elements from 'in1' to (pdst + stride)
*/
#define ST_B2(RTYPE, in0, in1, pdst, stride)  \
{                                             \
    ST_B(RTYPE, in0, (pdst));                 \
    ST_B(RTYPE, in1, (pdst) + stride);        \
}
#define ST_UB2(...) ST_B2(v16u8, __VA_ARGS__)

#define ST_B4(RTYPE, in0, in1, in2, in3, pdst, stride)    \
{                                                         \
    ST_B2(RTYPE, in0, in1, (pdst), stride);               \
    ST_B2(RTYPE, in2, in3, (pdst) + 2 * stride, stride);  \
}
#define ST_UB4(...) ST_B4(v16u8, __VA_ARGS__)

#define ST_B8(RTYPE, in0, in1, in2, in3, in4, in5, in6, in7,        \
              pdst, stride)                                         \
{                                                                   \
    ST_B4(RTYPE, in0, in1, in2, in3, pdst, stride);                 \
    ST_B4(RTYPE, in4, in5, in6, in7, (pdst) + 4 * stride, stride);  \
}
#define ST_UB8(...) ST_B8(v16u8, __VA_ARGS__)

/* Description : Store vectors of 8 halfword elements with stride
   Arguments   : Inputs  - in0, in1, stride
                 Outputs - pdst    (destination pointer to store to)
   Details     : Stores 8 halfword elements from 'in0' to (pdst)
                 Stores 8 halfword elements from 'in1' to (pdst + stride)
*/
#define ST_H2(RTYPE, in0, in1, pdst, stride)  \
{                                             \
    ST_H(RTYPE, in0, (pdst));                 \
    ST_H(RTYPE, in1, (pdst) + stride);        \
}

#define ST_H4(RTYPE, in0, in1, in2, in3, pdst, stride)    \
{                                                         \
    ST_H2(RTYPE, in0, in1, (pdst), stride);               \
    ST_H2(RTYPE, in2, in3, (pdst) + 2 * stride, stride);  \
}
#define ST_SH4(...) ST_H4(v8i16, __VA_ARGS__)

/* Description : Store as 2x4 byte block to destination memory from input vector
   Arguments   : Inputs  - in, stidx, pdst, stride
                 Return Type - unsigned byte
   Details     : Index stidx halfword element from 'in' vector is copied and
                 stored on first line
                 Index stidx+1 halfword element from 'in' vector is copied and
                 stored on second line
                 Index stidx+2 halfword element from 'in' vector is copied and
                 stored on third line
                 Index stidx+3 halfword element from 'in' vector is copied and
                 stored on fourth line
*/
#define ST2x4_UB(in, stidx, pdst, stride)              \
{                                                      \
    uint16 out0_m, out1_m, out2_m, out3_m;             \
    uint8 *pblk_2x4_m = (uint8 *) (pdst);              \
                                                       \
    out0_m = __msa_copy_u_h((v8i16) in, (stidx));      \
    out1_m = __msa_copy_u_h((v8i16) in, (stidx + 1));  \
    out2_m = __msa_copy_u_h((v8i16) in, (stidx + 2));  \
    out3_m = __msa_copy_u_h((v8i16) in, (stidx + 3));  \
                                                       \
    SH(out0_m, pblk_2x4_m);                            \
    SH(out1_m, pblk_2x4_m + stride);                   \
    SH(out2_m, pblk_2x4_m + 2 * stride);               \
    SH(out3_m, pblk_2x4_m + 3 * stride);               \
}

/* Description : Store as 4x4 byte block to destination memory from input vector
   Arguments   : Inputs  - in0, in1, pdst, stride
                 Return Type - unsigned byte
   Details     : Idx0 word element from input vector 'in0' is copied and stored
                 on first line
                 Idx1 word element from input vector 'in0' is copied and stored
                 on second line
                 Idx2 word element from input vector 'in1' is copied and stored
                 on third line
                 Idx3 word element from input vector 'in1' is copied and stored
                 on fourth line
*/
#define ST4x4_UB(in0, in1, idx0, idx1, idx2, idx3, pdst, stride)  \
{                                                                 \
    uint32 out0_m, out1_m, out2_m, out3_m;                        \
    uint8 *pblk_4x4_m = (uint8 *) (pdst);                         \
                                                                  \
    out0_m = __msa_copy_u_w((v4i32) in0, idx0);                   \
    out1_m = __msa_copy_u_w((v4i32) in0, idx1);                   \
    out2_m = __msa_copy_u_w((v4i32) in1, idx2);                   \
    out3_m = __msa_copy_u_w((v4i32) in1, idx3);                   \
                                                                  \
    SW4(out0_m, out1_m, out2_m, out3_m, pblk_4x4_m, stride);      \
}

/* Description : Store as 8x1 byte block to destination memory from input vector
   Arguments   : Inputs  - in, pdst
   Details     : Index 0 double word element from input vector 'in' is copied
                 and stored to destination memory at (pdst)
*/
#define ST8x1_UB(in, pdst)                   \
{                                            \
    uint64_t out0_m;                         \
    out0_m = __msa_copy_u_d((v2i64) in, 0);  \
    SD(out0_m, pdst);                        \
}

/* Description : Store as 8x4 byte block to destination memory from input
                 vectors
   Arguments   : Inputs  - in0, in1, pdst, stride
   Details     : Index 0 double word element from input vector 'in0' is copied
                 and stored to destination memory at (pblk_8x4_m)
                 Index 1 double word element from input vector 'in0' is copied
                 and stored to destination memory at (pblk_8x4_m + stride)
                 Index 0 double word element from input vector 'in1' is copied
                 and stored to destination memory at (pblk_8x4_m + 2 * stride)
                 Index 1 double word element from input vector 'in1' is copied
                 and stored to destination memory at (pblk_8x4_m + 3 * stride)
*/
#define ST8x4_UB(in0, in1, pdst, stride)                      \
{                                                             \
    uint64_t out0_m, out1_m, out2_m, out3_m;                  \
    uint8 *pblk_8x4_m = (uint8 *) (pdst);                     \
                                                              \
    out0_m = __msa_copy_u_d((v2i64) in0, 0);                  \
    out1_m = __msa_copy_u_d((v2i64) in0, 1);                  \
    out2_m = __msa_copy_u_d((v2i64) in1, 0);                  \
    out3_m = __msa_copy_u_d((v2i64) in1, 1);                  \
                                                              \
    SD4(out0_m, out1_m, out2_m, out3_m, pblk_8x4_m, stride);  \
}

/* Description : Immediate number of columns to slide with zero
   Arguments   : Inputs  - in0, in1, slide_val
                 Outputs - out0, out1
                 Return Type - as per RTYPE
   Details     : Byte elements from 'zero_m' vector are slide into 'in0' by
                 number of elements specified by 'slide_val'
*/
#define SLDI_B2_0(RTYPE, in0, in1, out0, out1, slide_val)                 \
{                                                                         \
    v16i8 zero_m = { 0 };                                                 \
    out0 = (RTYPE) __msa_sldi_b((v16i8) zero_m, (v16i8) in0, slide_val);  \
    out1 = (RTYPE) __msa_sldi_b((v16i8) zero_m, (v16i8) in1, slide_val);  \
}

#define SLDI_B4_0(RTYPE, in0, in1, in2, in3,            \
                  out0, out1, out2, out3, slide_val)    \
{                                                       \
    SLDI_B2_0(RTYPE, in0, in1, out0, out1, slide_val);  \
    SLDI_B2_0(RTYPE, in2, in3, out2, out3, slide_val);  \
}
#define SLDI_B4_0_UB(...) SLDI_B4_0(v16u8, __VA_ARGS__)

/* Description : Shuffle byte vector elements as per mask vector
   Arguments   : Inputs  - in0, in1, in2, in3, mask0, mask1
                 Outputs - out0, out1
                 Return Type - as per RTYPE
   Details     : Selective byte elements from in0 & in1 are copied to out0 as
                 per control vector mask0
                 Selective byte elements from in2 & in3 are copied to out1 as
                 per control vector mask1
*/
#define VSHF_B2(RTYPE, in0, in1, in2, in3, mask0, mask1, out0, out1)       \
{                                                                          \
    out0 = (RTYPE) __msa_vshf_b((v16i8) mask0, (v16i8) in1, (v16i8) in0);  \
    out1 = (RTYPE) __msa_vshf_b((v16i8) mask1, (v16i8) in3, (v16i8) in2);  \
}
#define VSHF_B2_UB(...) VSHF_B2(v16u8, __VA_ARGS__)

/* Description : Clips all signed halfword elements of input vector
                 between 0 & 255
   Arguments   : Inputs  - in       (input vector)
                 Outputs - out_m    (output vector with clipped elements)
                 Return Type - signed halfword
*/
#define CLIP_SH_0_255(in)                                 \
( {                                                       \
    v8i16 max_m = __msa_ldi_h(255);                       \
    v8i16 out_m;                                          \
                                                          \
    out_m = __msa_maxi_s_h((v8i16) in, 0);                \
    out_m = __msa_min_s_h((v8i16) max_m, (v8i16) out_m);  \
    out_m;                                                \
} )
#define CLIP_SH2_0_255(in0, in1)  \
{                                 \
    in0 = CLIP_SH_0_255(in0);     \
    in1 = CLIP_SH_0_255(in1);     \
}
#define CLIP_SH4_0_255(in0, in1, in2, in3)  \
{                                           \
    CLIP_SH2_0_255(in0, in1);               \
    CLIP_SH2_0_255(in2, in3);               \
}

/* Description : Horizontal addition of unsigned byte vector elements
   Arguments   : Inputs  - in0, in1
                 Outputs - out0, out1
                 Return Type - as per RTYPE
   Details     : Each unsigned odd byte element from 'in0' is added to
                 even unsigned byte element from 'in0' (pairwise) and the
                 halfword result is stored in 'out0'
*/
#define HADD_UB2(RTYPE, in0, in1, out0, out1)                 \
{                                                             \
    out0 = (RTYPE) __msa_hadd_u_h((v16u8) in0, (v16u8) in0);  \
    out1 = (RTYPE) __msa_hadd_u_h((v16u8) in1, (v16u8) in1);  \
}
#define HADD_UB2_SH(...) HADD_UB2(v8i16, __VA_ARGS__)

#define HADD_UB4(RTYPE, in0, in1, in2, in3, out0, out1, out2, out3)  \
{                                                                    \
    HADD_UB2(RTYPE, in0, in1, out0, out1);                           \
    HADD_UB2(RTYPE, in2, in3, out2, out3);                           \
}
#define HADD_UB4_UH(...) HADD_UB4(v8u16, __VA_ARGS__)
#define HADD_UB4_SH(...) HADD_UB4(v8i16, __VA_ARGS__)

/* Description : Horizontal subtraction of unsigned byte vector elements
   Arguments   : Inputs  - in0, in1
                 Outputs - out0, out1
                 Return Type - as per RTYPE
   Details     : Each unsigned odd byte element from 'in0' is subtracted from
                 even unsigned byte element from 'in0' (pairwise) and the
                 halfword result is stored in 'out0'
*/
#define HSUB_UB2(RTYPE, in0, in1, out0, out1)                 \
{                                                             \
    out0 = (RTYPE) __msa_hsub_u_h((v16u8) in0, (v16u8) in0);  \
    out1 = (RTYPE) __msa_hsub_u_h((v16u8) in1, (v16u8) in1);  \
}
#define HSUB_UB2_SH(...) HSUB_UB2(v8i16, __VA_ARGS__)

/* Description : Interleave even word elements from vectors
   Arguments   : Inputs  - in0, in1, in2, in3
                 Outputs - out0, out1
                 Return Type - as per RTYPE
   Details     : Even word elements of 'in0' and even word
                 elements of 'in1' are interleaved and copied to 'out0'
                 Even word elements of 'in2' and even word
                 elements of 'in3' are interleaved and copied to 'out1'
*/
#define ILVEV_W2(RTYPE, in0, in1, in2, in3, out0, out1)      \
{                                                            \
    out0 = (RTYPE) __msa_ilvev_w((v4i32) in1, (v4i32) in0);  \
    out1 = (RTYPE) __msa_ilvev_w((v4i32) in3, (v4i32) in2);  \
}
#define ILVEV_W2_SB(...) ILVEV_W2(v16i8, __VA_ARGS__)

/* Description : Interleave left half of byte elements from vectors
   Arguments   : Inputs  - in0, in1, in2, in3
                 Outputs - out0, out1
                 Return Type - as per RTYPE
   Details     : Left half of byte elements of in0 and left half of byte
                 elements of in1 are interleaved and copied to out0.
                 Left half of byte elements of in2 and left half of byte
                 elements of in3 are interleaved and copied to out1.
*/
#define ILVL_B2(RTYPE, in0, in1, in2, in3, out0, out1)      \
{                                                           \
    out0 = (RTYPE) __msa_ilvl_b((v16i8) in0, (v16i8) in1);  \
    out1 = (RTYPE) __msa_ilvl_b((v16i8) in2, (v16i8) in3);  \
}
#define ILVL_B2_SH(...) ILVL_B2(v8i16, __VA_ARGS__)

#define ILVL_B4(RTYPE, in0, in1, in2, in3, in4, in5, in6, in7,  \
                out0, out1, out2, out3)                         \
{                                                               \
    ILVL_B2(RTYPE, in0, in1, in2, in3, out0, out1);             \
    ILVL_B2(RTYPE, in4, in5, in6, in7, out2, out3);             \
}
#define ILVL_B4_SB(...) ILVL_B4(v16i8, __VA_ARGS__)

/* Description : Interleave left half of halfword elements from vectors
   Arguments   : Inputs  - in0, in1, in2, in3
                 Outputs - out0, out1
                 Return Type - as per RTYPE
   Details     : Left half of halfword elements of in0 and left half of halfword
                 elements of in1 are interleaved and copied to out0.
                 Left half of halfword elements of in2 and left half of halfword
                 elements of in3 are interleaved and copied to out1.
*/
#define ILVL_H2(RTYPE, in0, in1, in2, in3, out0, out1)      \
{                                                           \
    out0 = (RTYPE) __msa_ilvl_h((v8i16) in0, (v8i16) in1);  \
    out1 = (RTYPE) __msa_ilvl_h((v8i16) in2, (v8i16) in3);  \
}
#define ILVL_H2_SH(...) ILVL_H2(v8i16, __VA_ARGS__)

/* Description : Interleave right half of byte elements from vectors
   Arguments   : Inputs  - in0, in1, in2, in3, in4, in5, in6, in7
                 Outputs - out0, out1, out2, out3
                 Return Type - as per RTYPE
   Details     : Right half of byte elements of in0 and right half of byte
                 elements of in1 are interleaved and copied to out0.
                 Right half of byte elements of in2 and right half of byte
                 elements of in3 are interleaved and copied to out1.
                 Similar for other pairs
*/
#define ILVR_B2(RTYPE, in0, in1, in2, in3, out0, out1)      \
{                                                           \
    out0 = (RTYPE) __msa_ilvr_b((v16i8) in0, (v16i8) in1);  \
    out1 = (RTYPE) __msa_ilvr_b((v16i8) in2, (v16i8) in3);  \
}
#define ILVR_B2_SB(...) ILVR_B2(v16i8, __VA_ARGS__)
#define ILVR_B2_SH(...) ILVR_B2(v8i16, __VA_ARGS__)

#define ILVR_B3(RTYPE, in0, in1, in2, in3, in4, in5, out0, out1, out2)  \
{                                                                       \
    ILVR_B2(RTYPE, in0, in1, in2, in3, out0, out1);                     \
    out2 = (RTYPE) __msa_ilvr_b((v16i8) in4, (v16i8) in5);              \
}
#define ILVR_B3_SH(...) ILVR_B3(v8i16, __VA_ARGS__)

#define ILVR_B4(RTYPE, in0, in1, in2, in3, in4, in5, in6, in7,  \
                out0, out1, out2, out3)                         \
{                                                               \
    ILVR_B2(RTYPE, in0, in1, in2, in3, out0, out1);             \
    ILVR_B2(RTYPE, in4, in5, in6, in7, out2, out3);             \
}
#define ILVR_B4_SB(...) ILVR_B4(v16i8, __VA_ARGS__)
#define ILVR_B4_SH(...) ILVR_B4(v8i16, __VA_ARGS__)

#define ILVR_B8(RTYPE, in0, in1, in2, in3, in4, in5, in6, in7,    \
                in8, in9, in10, in11, in12, in13, in14, in15,     \
                out0, out1, out2, out3, out4, out5, out6, out7)   \
{                                                                 \
    ILVR_B4(RTYPE, in0, in1, in2, in3, in4, in5, in6, in7,        \
            out0, out1, out2, out3);                              \
    ILVR_B4(RTYPE, in8, in9, in10, in11, in12, in13, in14, in15,  \
            out4, out5, out6, out7);                              \
}
#define ILVR_B8_UH(...) ILVR_B8(v8u16, __VA_ARGS__)

/* Description : Interleave right half of halfword elements from vectors
   Arguments   : Inputs  - in0, in1, in2, in3, in4, in5, in6, in7
                 Outputs - out0, out1, out2, out3
                 Return Type - signed halfword
   Details     : Right half of halfword elements of in0 and right half of
                 halfword elements of in1 are interleaved and copied to out0.
                 Right half of halfword elements of in2 and right half of
                 halfword elements of in3 are interleaved and copied to out1.
                 Similar for other pairs
*/
#define ILVR_H2(RTYPE, in0, in1, in2, in3, out0, out1)      \
{                                                           \
    out0 = (RTYPE) __msa_ilvr_h((v8i16) in0, (v8i16) in1);  \
    out1 = (RTYPE) __msa_ilvr_h((v8i16) in2, (v8i16) in3);  \
}
#define ILVR_H2_SH(...) ILVR_H2(v8i16, __VA_ARGS__)

/* Description : Interleave right half of double word elements from vectors
   Arguments   : Inputs  - in0, in1, in2, in3, in4, in5, in6, in7
                 Outputs - out0, out1, out2, out3
                 Return Type - unsigned double word
   Details     : Right half of double word elements of in0 and right half of
                 double word elements of in1 are interleaved and copied to out0.
                 Right half of double word elements of in2 and right half of
                 double word elements of in3 are interleaved and copied to out1.
*/
#define ILVR_D2(RTYPE, in0, in1, in2, in3, out0, out1)          \
{                                                               \
    out0 = (RTYPE) __msa_ilvr_d((v2i64) (in0), (v2i64) (in1));  \
    out1 = (RTYPE) __msa_ilvr_d((v2i64) (in2), (v2i64) (in3));  \
}
#define ILVR_D2_UB(...) ILVR_D2(v16u8, __VA_ARGS__)

/* Description : Interleave both left and right half of input vectors
   Arguments   : Inputs  - in0, in1
                 Outputs - out0, out1
                 Return Type - as per RTYPE
   Details     : Right half of byte elements from 'in0' and 'in1' are
                 interleaved and stored to 'out0'
                 Left half of byte elements from 'in0' and 'in1' are
                 interleaved and stored to 'out1'
*/
#define ILVRL_B2(RTYPE, in0, in1, out0, out1)               \
{                                                           \
    out0 = (RTYPE) __msa_ilvr_b((v16i8) in0, (v16i8) in1);  \
    out1 = (RTYPE) __msa_ilvl_b((v16i8) in0, (v16i8) in1);  \
}
#define ILVRL_B2_UB(...) ILVRL_B2(v16u8, __VA_ARGS__)
#define ILVRL_B2_SB(...) ILVRL_B2(v16i8, __VA_ARGS__)
#define ILVRL_B2_SH(...) ILVRL_B2(v8i16, __VA_ARGS__)

#define ILVRL_H2(RTYPE, in0, in1, out0, out1)               \
{                                                           \
    out0 = (RTYPE) __msa_ilvr_h((v8i16) in0, (v8i16) in1);  \
    out1 = (RTYPE) __msa_ilvl_h((v8i16) in0, (v8i16) in1);  \
}
#define ILVRL_H2_SB(...) ILVRL_H2(v16i8, __VA_ARGS__)
#define ILVRL_H2_SH(...) ILVRL_H2(v8i16, __VA_ARGS__)
#define ILVRL_H2_SW(...) ILVRL_H2(v4i32, __VA_ARGS__)

#define ILVRL_W2(RTYPE, in0, in1, out0, out1)               \
{                                                           \
    out0 = (RTYPE) __msa_ilvr_w((v4i32) in0, (v4i32) in1);  \
    out1 = (RTYPE) __msa_ilvl_w((v4i32) in0, (v4i32) in1);  \
}
#define ILVRL_W2_SW(...) ILVRL_W2(v4i32, __VA_ARGS__)
#define ILVRL_W2_SD(...) ILVRL_W2(v2i64, __VA_ARGS__)

#define ILVRL_D2(RTYPE, in0, in1, out0, out1)               \
{                                                           \
    out0 = (RTYPE) __msa_ilvr_d((v2i64) in0, (v2i64) in1);  \
    out1 = (RTYPE) __msa_ilvl_d((v2i64) in0, (v2i64) in1);  \
}
#define ILVRL_D2_UB(...) ILVRL_D2(v16u8, __VA_ARGS__)

/* Description : Indexed word element values are replicated to all
                 elements in output vector
   Arguments   : Inputs  - in, stidx
                 Outputs - out0, out1
                 Return Type - as per RTYPE
   Details     : 'stidx' element value from 'in' vector is replicated to all
                  elements in 'out0' vector
                 'stidx + 1' element value from 'in' vector is replicated to all
                  elements in 'out1' vector
                  Valid index range for halfword operation is 0-3
*/
#define SPLATI_W2(RTYPE, in, stidx, out0, out1)            \
{                                                          \
    out0 = (RTYPE) __msa_splati_w((v4i32) in, stidx);      \
    out1 = (RTYPE) __msa_splati_w((v4i32) in, (stidx+1));  \
}

#define SPLATI_W4(RTYPE, in, out0, out1, out2, out3)  \
{                                                     \
    SPLATI_W2(RTYPE, in, 0, out0, out1);              \
    SPLATI_W2(RTYPE, in, 2, out2, out3);              \
}
#define SPLATI_W4_SW(...) SPLATI_W4(v4i32, __VA_ARGS__)

/* Description : Pack even byte elements of vector pairs
   Arguments   : Inputs  - in0, in1, in2, in3
                 Outputs - out0, out1
                 Return Type - as per RTYPE
   Details     : Even byte elements of in0 are copied to the left half of
                 out0 & even byte elements of in1 are copied to the right
                 half of out0.
                 Even byte elements of in2 are copied to the left half of
                 out1 & even byte elements of in3 are copied to the right
                 half of out1.
*/
#define PCKEV_B2(RTYPE, in0, in1, in2, in3, out0, out1)      \
{                                                            \
    out0 = (RTYPE) __msa_pckev_b((v16i8) in0, (v16i8) in1);  \
    out1 = (RTYPE) __msa_pckev_b((v16i8) in2, (v16i8) in3);  \
}
#define PCKEV_B2_SB(...) PCKEV_B2(v16i8, __VA_ARGS__)
#define PCKEV_B2_SH(...) PCKEV_B2(v8i16, __VA_ARGS__)

/* Description : Pack even halfword elements of vector pairs
   Arguments   : Inputs  - in0, in1, in2, in3
                 Outputs - out0, out1
                 Return Type - as per RTYPE
   Details     : Even halfword elements of in0 are copied to the left half of
                 out0 & even halfword elements of in1 are copied to the right
                 half of out0.
                 Even halfword elements of in2 are copied to the left half of
                 out1 & even halfword elements of in3 are copied to the right
                 half of out1.
*/
#define PCKEV_H2(RTYPE, in0, in1, in2, in3, out0, out1)      \
{                                                            \
    out0 = (RTYPE) __msa_pckev_h((v8i16) in0, (v8i16) in1);  \
    out1 = (RTYPE) __msa_pckev_h((v8i16) in2, (v8i16) in3);  \
}

#define PCKEV_H4(RTYPE, in0, in1, in2, in3, in4, in5, in6, in7,  \
                 out0, out1, out2, out3)                         \
{                                                                \
    PCKEV_H2(RTYPE, in0, in1, in2, in3, out0, out1);             \
    PCKEV_H2(RTYPE, in4, in5, in6, in7, out2, out3);             \
}
#define PCKEV_H4_SH(...) PCKEV_H4(v8i16, __VA_ARGS__)

/* Description : Pack even double word elements of vector pairs
   Arguments   : Inputs  - in0, in1, in2, in3
                 Outputs - out0, out1
                 Return Type - unsigned byte
   Details     : Even double elements of in0 are copied to the left half of
                 out0 & even double elements of in1 are copied to the right
                 half of out0.
                 Even double elements of in2 are copied to the left half of
                 out1 & even double elements of in3 are copied to the right
                 half of out1.
*/
#define PCKEV_D2(RTYPE, in0, in1, in2, in3, out0, out1)      \
{                                                            \
    out0 = (RTYPE) __msa_pckev_d((v2i64) in0, (v2i64) in1);  \
    out1 = (RTYPE) __msa_pckev_d((v2i64) in2, (v2i64) in3);  \
}

#define PCKEV_D4(RTYPE, in0, in1, in2, in3, in4, in5, in6, in7,  \
                 out0, out1, out2, out3)                         \
{                                                                \
    PCKEV_D2(RTYPE, in0, in1, in2, in3, out0, out1);             \
    PCKEV_D2(RTYPE, in4, in5, in6, in7, out2, out3);             \
}

/* Description : Arithmetic shift right all elements of vector
                 (generic for all data types)
   Arguments   : Inputs  - in0, in1, in2, in3, shift
                 Outputs - in0, in1, in2, in3 (in place)
                 Return Type - as per input vector RTYPE
   Details     : Each element of vector 'in0' is right shifted by 'shift' and
                 the result is in place written to 'in0'
                 Here, 'shift' is GP variable passed in
                 Similar for other pairs
*/
#define SRA_4V(in0, in1, in2, in3, shift)  \
{                                          \
    in0 = in0 >> shift;                    \
    in1 = in1 >> shift;                    \
    in2 = in2 >> shift;                    \
    in3 = in3 >> shift;                    \
}

/* Description : Shift right arithmetic rounded (immediate)
   Arguments   : Inputs  - in0, in1, in2, in3, shift
                 Outputs - in0, in1, in2, in3 (in place)
                 Return Type - as per RTYPE
   Details     : Each element of vector 'in0' is shifted right arithmetic by
                 value in 'shift'.
                 The last discarded bit is added to shifted value for rounding
                 and the result is in place written to 'in0'
                 Similar for other pairs
*/
#define SRARI_H2(RTYPE, in0, in1, shift)              \
{                                                     \
    in0 = (RTYPE) __msa_srari_h((v8i16) in0, shift);  \
    in1 = (RTYPE) __msa_srari_h((v8i16) in1, shift);  \
}

#define SRARI_H4(RTYPE, in0, in1, in2, in3, shift)    \
{                                                     \
    SRARI_H2(RTYPE, in0, in1, shift);                 \
    SRARI_H2(RTYPE, in2, in3, shift);                 \
}
#define SRARI_H4_UH(...) SRARI_H4(v8u16, __VA_ARGS__)
#define SRARI_H4_SH(...) SRARI_H4(v8i16, __VA_ARGS__)

/* Description : Addition of 2 pairs of vectors
   Arguments   : Inputs  - in0, in1, in2, in3
                 Outputs - out0, out1
   Details     : Each element from 2 pairs vectors is added and 2 results are
                 produced
*/
#define ADD2(in0, in1, in2, in3, out0, out1)  \
{                                             \
    out0 = in0 + in1;                         \
    out1 = in2 + in3;                         \
}
#define ADD4(in0, in1, in2, in3, in4, in5, in6, in7, out0, out1, out2, out3)  \
{                                                                             \
    ADD2(in0, in1, in2, in3, out0, out1);                                     \
    ADD2(in4, in5, in6, in7, out2, out3);                                     \
}

/* Description : Subtraction of 2 pairs of vectors
   Arguments   : Inputs  - in0, in1, in2, in3
                 Outputs - out0, out1
   Details     : Each element from 2 pairs vectors is subtracted and 2 results
                 are produced
*/
#define SUB2(in0, in1, in2, in3, out0, out1)  \
{                                             \
    out0 = in0 - in1;                         \
    out1 = in2 - in3;                         \
}
#define SUB4(in0, in1, in2, in3, in4, in5, in6, in7, out0, out1, out2, out3)  \
{                                                                             \
    out0 = in0 - in1;                                                         \
    out1 = in2 - in3;                                                         \
    out2 = in4 - in5;                                                         \
    out3 = in6 - in7;                                                         \
}

/* Description : Zero extend unsigned byte elements to halfword elements
   Arguments   : Inputs  - in           (1 input unsigned byte vector)
                 Outputs - out0, out1   (unsigned 2 halfword vectors)
                 Return Type - signed halfword
   Details     : Zero extended right half of vector is returned in 'out0'
                 Zero extended left half of vector is returned in 'out1'
*/
#define UNPCK_UB_SH(in, out0, out1)       \
{                                         \
    v16i8 zero_m = { 0 };                 \
                                          \
    ILVRL_B2_SH(zero_m, in, out0, out1);  \
}

/* Description : Sign extend halfword elements from input vector and return
                 the result in pair of vectors
   Arguments   : Inputs  - in           (1 input halfword vector)
                 Outputs - out0, out1   (sign extended 2 word vectors)
                 Return Type - signed word
   Details     : Sign bit of halfword elements from input vector 'in' is
                 extracted and interleaved right with same vector 'in0' to
                 generate 4 signed word elements in 'out0'
                 Then interleaved left with same vector 'in0' to
                 generate 4 signed word elements in 'out1'
*/
#define UNPCK_SH_SW(in, out0, out1)         \
{                                           \
    v8i16 tmp_m;                            \
                                            \
    tmp_m = __msa_clti_s_h((v8i16) in, 0);  \
    ILVRL_H2_SW(tmp_m, in, out0, out1);     \
}

/* Description : Butterfly of 4 input vectors
   Arguments   : Inputs  - in0, in1, in2, in3
                 Outputs - out0, out1, out2, out3
   Details     : Butterfly operation
*/
#define BUTTERFLY_4(in0, in1, in2, in3, out0, out1, out2, out3)  \
{                                                                \
    out0 = in0 + in3;                                            \
    out1 = in1 + in2;                                            \
                                                                 \
    out2 = in1 - in2;                                            \
    out3 = in0 - in3;                                            \
}

/* Description : Butterfly of 8 input vectors
   Arguments   : Inputs  - in0 ...  in7
                 Outputs - out0 .. out7
   Details     : Butterfly operation
*/
#define BUTTERFLY_8(in0, in1, in2, in3, in4, in5, in6, in7,          \
                    out0, out1, out2, out3, out4, out5, out6, out7)  \
{                                                                    \
    out0 = in0 + in7;                                                \
    out1 = in1 + in6;                                                \
    out2 = in2 + in5;                                                \
    out3 = in3 + in4;                                                \
                                                                     \
    out4 = in3 - in4;                                                \
    out5 = in2 - in5;                                                \
    out6 = in1 - in6;                                                \
    out7 = in0 - in7;                                                \
}

/* Description : Transposes input 8x4 byte block into 4x8
   Arguments   : Inputs  - in0, in1, in2, in3      (input 8x4 byte block)
                 Outputs - out0, out1, out2, out3  (output 4x8 byte block)
                 Return Type - unsigned byte
   Details     :
*/
#define TRANSPOSE8x4_UB(RTYPE, in0, in1, in2, in3, in4, in5, in6, in7,  \
                        out0, out1, out2, out3)                         \
{                                                                       \
    v16i8 tmp0_m, tmp1_m, tmp2_m, tmp3_m;                               \
                                                                        \
    ILVEV_W2_SB(in0, in4, in1, in5, tmp0_m, tmp1_m);                    \
    tmp2_m = __msa_ilvr_b(tmp1_m, tmp0_m);                              \
    ILVEV_W2_SB(in2, in6, in3, in7, tmp0_m, tmp1_m);                    \
                                                                        \
    tmp3_m = __msa_ilvr_b(tmp1_m, tmp0_m);                              \
    ILVRL_H2_SB(tmp3_m, tmp2_m, tmp0_m, tmp1_m);                        \
                                                                        \
    ILVRL_W2(RTYPE, tmp1_m, tmp0_m, out0, out2);                        \
    out1 = (RTYPE) __msa_ilvl_d((v2i64) out2, (v2i64) out0);            \
    out3 = (RTYPE) __msa_ilvl_d((v2i64) out0, (v2i64) out2);            \
}
#define TRANSPOSE8x4_UB_UB(...) TRANSPOSE8x4_UB(v16u8, __VA_ARGS__)

/* Description : Transposes input 8x8 byte block
   Arguments   : Inputs  - in0, in1, in2, in3, in4, in5, in6, in7
                           (input 8x8 byte block)
                 Outputs - out0, out1, out2, out3, out4, out5, out6, out7
                           (output 8x8 byte block)
                 Return Type - unsigned byte
   Details     :
*/
#define TRANSPOSE8x8_UB(RTYPE, in0, in1, in2, in3, in4, in5, in6, in7,   \
                        out0, out1, out2, out3, out4, out5, out6, out7)  \
{                                                                        \
    v16i8 tmp0_m, tmp1_m, tmp2_m, tmp3_m;                                \
    v16i8 tmp4_m, tmp5_m, tmp6_m, tmp7_m;                                \
                                                                         \
    ILVR_B4_SB(in2, in0, in3, in1, in6, in4, in7, in5,                   \
               tmp0_m, tmp1_m, tmp2_m, tmp3_m);                          \
    ILVRL_B2_SB(tmp1_m, tmp0_m, tmp4_m, tmp5_m);                         \
    ILVRL_B2_SB(tmp3_m, tmp2_m, tmp6_m, tmp7_m);                         \
    ILVRL_W2(RTYPE, tmp6_m, tmp4_m, out0, out2);                         \
    ILVRL_W2(RTYPE, tmp7_m, tmp5_m, out4, out6);                         \
    SLDI_B2_0(RTYPE, out0, out2, out1, out3, 8);                         \
    SLDI_B2_0(RTYPE, out4, out6, out5, out7, 8);                         \
}
#define TRANSPOSE8x8_UB_UB(...) TRANSPOSE8x8_UB(v16u8, __VA_ARGS__)

/* Description : Transposes 8x8 block with half word elements in vectors
   Arguments   : Inputs  - in0, in1, in2, in3, in4, in5, in6, in7
                 Outputs - out0, out1, out2, out3, out4, out5, out6, out7
                 Return Type - signed halfword
   Details     :
*/
#define TRANSPOSE8x8_H(RTYPE, in0, in1, in2, in3, in4, in5, in6, in7,   \
                       out0, out1, out2, out3, out4, out5, out6, out7)  \
{                                                                       \
    v8i16 s0_m, s1_m;                                                   \
    v8i16 tmp0_m, tmp1_m, tmp2_m, tmp3_m;                               \
    v8i16 tmp4_m, tmp5_m, tmp6_m, tmp7_m;                               \
                                                                        \
    ILVR_H2_SH(in6, in4, in7, in5, s0_m, s1_m);                         \
    ILVRL_H2_SH(s1_m, s0_m, tmp0_m, tmp1_m);                            \
    ILVL_H2_SH(in6, in4, in7, in5, s0_m, s1_m);                         \
    ILVRL_H2_SH(s1_m, s0_m, tmp2_m, tmp3_m);                            \
    ILVR_H2_SH(in2, in0, in3, in1, s0_m, s1_m);                         \
    ILVRL_H2_SH(s1_m, s0_m, tmp4_m, tmp5_m);                            \
    ILVL_H2_SH(in2, in0, in3, in1, s0_m, s1_m);                         \
    ILVRL_H2_SH(s1_m, s0_m, tmp6_m, tmp7_m);                            \
    PCKEV_D4(RTYPE, tmp0_m, tmp4_m, tmp1_m, tmp5_m, tmp2_m, tmp6_m,     \
             tmp3_m, tmp7_m, out0, out2, out4, out6);                   \
    out1 = (RTYPE) __msa_pckod_d((v2i64) tmp0_m, (v2i64) tmp4_m);       \
    out3 = (RTYPE) __msa_pckod_d((v2i64) tmp1_m, (v2i64) tmp5_m);       \
    out5 = (RTYPE) __msa_pckod_d((v2i64) tmp2_m, (v2i64) tmp6_m);       \
    out7 = (RTYPE) __msa_pckod_d((v2i64) tmp3_m, (v2i64) tmp7_m);       \
}
#define TRANSPOSE8x8_SH_SH(...) TRANSPOSE8x8_H(v8i16, __VA_ARGS__)

/* Description : Transposes 4x4 block with word elements in vectors
   Arguments   : Inputs  - in0, in1, in2, in3
                 Outputs - out0, out1, out2, out3
                 Return Type - signed word
   Details     :
*/
#define TRANSPOSE4x4_SW_SW(in0, in1, in2, in3, out0, out1, out2, out3)  \
{                                                                       \
    v4i32 s0_m, s1_m, s2_m, s3_m;                                       \
                                                                        \
    ILVRL_W2_SW(in1, in0, s0_m, s1_m);                                  \
    ILVRL_W2_SW(in3, in2, s2_m, s3_m);                                  \
                                                                        \
    out0 = (v4i32) __msa_ilvr_d((v2i64) s2_m, (v2i64) s0_m);            \
    out1 = (v4i32) __msa_ilvl_d((v2i64) s2_m, (v2i64) s0_m);            \
    out2 = (v4i32) __msa_ilvr_d((v2i64) s3_m, (v2i64) s1_m);            \
    out3 = (v4i32) __msa_ilvl_d((v2i64) s3_m, (v2i64) s1_m);            \
}
#endif  /* _COMMON_MACROS_MSA_H */
