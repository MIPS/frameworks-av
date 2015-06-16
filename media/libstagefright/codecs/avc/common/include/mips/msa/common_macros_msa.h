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

#ifndef AVC_COMMON_MACROS_MSA_H
#define AVC_COMMON_MACROS_MSA_H

#include <stdint.h>
#include "types_msa.h"
#include "avc_types.h"

#define ALIGNMENT           16
#define ALIGNMENT_MINUS_1   (ALIGNMENT - 1)
#define ALLOC_ALIGNED(align) __attribute__ ((aligned((align) << 1)))

#define LD_B(RTYPE, psrc) *((RTYPE *)(psrc))
#define LD_UB(...) LD_B(v16u8, __VA_ARGS__)
#define LD_SB(...) LD_B(v16i8, __VA_ARGS__)

#define LD_H(RTYPE, psrc) *((RTYPE *)(psrc))

#define ST_B(RTYPE, in, pdst) *((RTYPE *)(pdst)) = (in)
#define ST_UB(...) ST_B(v16u8, __VA_ARGS__)

#define ST_H(RTYPE, in, pdst) *((RTYPE *)(pdst)) = (in)

#if (__mips_isa_rev >= 6)
    #define LH(psrc)                           \
    ( {                                        \
        uint8 *psrc_m = (uint8 *) (psrc);      \
        uint16 val_m;                          \
                                               \
        asm volatile (                         \
            "lh  %[val_m],  %[psrc_m]  \n\t"   \
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
            "lw  %[val_m],  %[psrc_m]  \n\t"   \
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
                "ld  %[val_m],  %[psrc_m]  \n\t"   \
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

    #define SH(val, pdst)                      \
    {                                          \
        uint8 *pdst_m = (uint8 *) (pdst);      \
        uint16 val_m = (val);                  \
                                               \
        asm volatile (                         \
            "sh  %[val_m],  %[pdst_m]  \n\t"   \
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
            "sw  %[val_m],  %[pdst_m]  \n\t"   \
                                               \
            : [pdst_m] "=m" (*pdst_m)          \
            : [val_m] "r" (val_m)              \
        );                                     \
    }

    #define SD(val, pdst)                      \
    {                                          \
        uint8 *pdst_m = (uint8 *) (pdst);      \
        uint64_t val_m = (val);                \
                                               \
        asm volatile (                         \
            "sd  %[val_m],  %[pdst_m]  \n\t"   \
                                               \
            : [pdst_m] "=m" (*pdst_m)          \
            : [val_m] "r" (val_m)              \
        );                                     \
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
#endif // (__mips_isa_rev >= 6)

/* Description : Load 4 words with stride
   Arguments   : Inputs  - psrc    (source pointer to load from)
                         - stride
                 Outputs - out0, out1, out2, out3
   Details     : Loads word in 'out0' from (psrc)
                 Loads word in 'out1' from (psrc + stride)
                 Loads word in 'out2' from (psrc + 2 * stride)
                 Loads word in 'out3' from (psrc + 3 * stride)
*/
#define LW4(psrc, stride, out0, out1, out2, out3)  \
{                                                  \
    out0 = LW((psrc));                             \
    out1 = LW((psrc) + stride);                    \
    out2 = LW((psrc) + 2 * stride);                \
    out3 = LW((psrc) + 3 * stride);                \
}

/* Description : Load double words with stride
   Arguments   : Inputs  - psrc    (source pointer to load from)
                         - stride
                 Outputs - out0, out1
   Details     : Loads double word in 'out0' from (psrc)
                 Loads double word in 'out1' from (psrc + stride)
*/
#define LD2(psrc, stride, out0, out1)  \
{                                      \
    out0 = LD((psrc));                 \
    out1 = LD((psrc) + stride);        \
}
#define LD4(psrc, stride, out0, out1, out2, out3)  \
{                                                  \
    LD2((psrc), stride, out0, out1);               \
    LD2((psrc) + 2 * stride, stride, out2, out3);  \
}

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
#define LD_UB2(...) LD_B2(v16u8, __VA_ARGS__)
#define LD_SB2(...) LD_B2(v16i8, __VA_ARGS__)

#define LD_B3(RTYPE, psrc, stride, out0, out1, out2)  \
{                                                     \
    LD_B2(RTYPE, (psrc), stride, out0, out1);         \
    out2 = LD_B(RTYPE, (psrc) + 2 * stride);          \
}
#define LD_UB3(...) LD_B3(v16u8, __VA_ARGS__)
#define LD_SB3(...) LD_B3(v16i8, __VA_ARGS__)

#define LD_B4(RTYPE, psrc, stride, out0, out1, out2, out3)   \
{                                                            \
    LD_B2(RTYPE, (psrc), stride, out0, out1);                \
    LD_B2(RTYPE, (psrc) + 2 * stride , stride, out2, out3);  \
}
#define LD_UB4(...) LD_B4(v16u8, __VA_ARGS__)
#define LD_SB4(...) LD_B4(v16i8, __VA_ARGS__)

#define LD_B5(RTYPE, psrc, stride, out0, out1, out2, out3, out4)  \
{                                                                 \
    LD_B4(RTYPE, (psrc), stride, out0, out1, out2, out3);         \
    out4 = LD_B(RTYPE, (psrc) + 4 * stride);                      \
}
#define LD_UB5(...) LD_B5(v16u8, __VA_ARGS__)
#define LD_SB5(...) LD_B5(v16i8, __VA_ARGS__)

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
#define LD_SH2(...) LD_H2(v8i16, __VA_ARGS__)

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
#define ST_SB2(...) ST_B2(v16i8, __VA_ARGS__)

#define ST_B4(RTYPE, in0, in1, in2, in3, pdst, stride)    \
{                                                         \
    ST_B2(RTYPE, in0, in1, (pdst), stride);               \
    ST_B2(RTYPE, in2, in3, (pdst) + 2 * stride, stride);  \
}
#define ST_UB4(...) ST_B4(v16u8, __VA_ARGS__)
#define ST_SB4(...) ST_B4(v16i8, __VA_ARGS__)

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
#define ST_SH2(...) ST_H2(v8i16, __VA_ARGS__)

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

/* Description : Store as 4x2 byte block to destination memory from input vector
   Arguments   : Inputs  - in, pdst, stride
                 Return Type - unsigned byte
   Details     : Index 0 word element from input vector is copied and stored
                 on first line
                 Index 1 word element from input vector is copied and stored
                 on second line
*/
#define ST4x2_UB(in, pdst, stride)             \
{                                              \
    uint32 out0_m, out1_m;                     \
    uint8 *pblk_4x2_m = (uint8 *) (pdst);      \
                                               \
    out0_m = __msa_copy_u_w((v4i32) in, 0);    \
    out1_m = __msa_copy_u_w((v4i32) in, 1);    \
                                               \
    SW(out0_m, pblk_4x2_m);                    \
    SW(out1_m, pblk_4x2_m + stride);           \
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

/* Description : average with rounding (in0 + in1 + 1) / 2.
   Arguments   : Inputs  - in0, in1, in2, in3,
                 Outputs - out0, out1
                 Return Type - signed byte
   Details     : Each byte element from 'in0' vector is added with each byte
                 element from 'in1' vector. The addition of the elements plus 1
                (for rounding) is done unsigned with full precision,
                i.e. the result has one extra bit. Unsigned division by 2
                (or logical shift right by one bit) is performed before writing
                the result to vector 'out0'
                Similar for the pair of 'in2' and 'in3'
*/
#define AVER_UB2(RTYPE, in0, in1, in2, in3, out0, out1)       \
{                                                             \
    out0 = (RTYPE) __msa_aver_u_b((v16u8) in0, (v16u8) in1);  \
    out1 = (RTYPE) __msa_aver_u_b((v16u8) in2, (v16u8) in3);  \
}

#define AVER_UB4(RTYPE, in0, in1, in2, in3, in4, in5, in6, in7, \
                 out0, out1, out2, out3)                        \
{                                                               \
    AVER_UB2(RTYPE, in0, in1, in2, in3, out0, out1)             \
    AVER_UB2(RTYPE, in4, in5, in6, in7, out2, out3)             \
}
#define AVER_UB4_UB(...) AVER_UB4(v16u8, __VA_ARGS__)

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
#define VSHF_B2_SB(...) VSHF_B2(v16i8, __VA_ARGS__)

/* Description : Shuffle halfword vector elements as per mask vector
   Arguments   : Inputs  - in0, in1, in2, in3, mask0, mask1
                 Outputs - out0, out1
                 Return Type - as per RTYPE
   Details     : Selective halfword elements from in0 & in1 are copied to out0
                 as per control vector mask0
                 Selective halfword elements from in2 & in3 are copied to out1
                 as per control vector mask1
*/
#define VSHF_H2(RTYPE, in0, in1, in2, in3, mask0, mask1, out0, out1)       \
{                                                                          \
    out0 = (RTYPE) __msa_vshf_h((v8i16) mask0, (v8i16) in1, (v8i16) in0);  \
    out1 = (RTYPE) __msa_vshf_h((v8i16) mask1, (v8i16) in3, (v8i16) in2);  \
}
#define VSHF_H2_SH(...) VSHF_H2(v8i16, __VA_ARGS__)

#define VSHF_H3(RTYPE, in0, in1, in2, in3, in4, in5, mask0, mask1, mask2,  \
                out0, out1, out2)                                          \
{                                                                          \
    VSHF_H2(RTYPE, in0, in1, in2, in3, mask0, mask1, out0, out1);          \
    out2 = (RTYPE) __msa_vshf_h((v8i16) mask2, (v8i16) in5, (v8i16) in4);  \
}
#define VSHF_H3_SH(...) VSHF_H3(v8i16, __VA_ARGS__)

/* Description : Dot product of byte vector elements
   Arguments   : Inputs  - mult0, mult1
                           cnst0, cnst1
                 Outputs - out0, out1
                 Return Type - unsigned halfword
   Details     : Unsigned byte elements from mult0 are multiplied with
                 unsigned byte elements from cnst0 producing a result
                 twice the size of input i.e. unsigned halfword.
                 Then this multiplication results of adjacent odd-even elements
                 are added together and stored to the out vector
                 (2 unsigned halfword results)
*/
#define DOTP_UB2(RTYPE, mult0, mult1, cnst0, cnst1, out0, out1)   \
{                                                                 \
    out0 = (RTYPE) __msa_dotp_u_h((v16u8) mult0, (v16u8) cnst0);  \
    out1 = (RTYPE) __msa_dotp_u_h((v16u8) mult1, (v16u8) cnst1);  \
}
#define DOTP_UB2_UH(...) DOTP_UB2(v8u16, __VA_ARGS__)

#define DOTP_UB4(RTYPE, mult0, mult1, mult2, mult3,           \
                 cnst0, cnst1, cnst2, cnst3,                  \
                 out0, out1, out2, out3)                      \
{                                                             \
    DOTP_UB2(RTYPE, mult0, mult1, cnst0, cnst1, out0, out1);  \
    DOTP_UB2(RTYPE, mult2, mult3, cnst2, cnst3, out2, out3);  \
}
#define DOTP_UB4_UH(...) DOTP_UB4(v8u16, __VA_ARGS__)

/* Description : Dot product of byte vector elements
   Arguments   : Inputs  - mult0, mult1
                           cnst0, cnst1
                 Outputs - out0, out1
                 Return Type - signed halfword
   Details     : Signed byte elements from mult0 are multiplied with
                 signed byte elements from cnst0 producing a result
                 twice the size of input i.e. signed halfword.
                 Then this multiplication results of adjacent odd-even elements
                 are added together and stored to the out vector
                 (2 signed halfword results)
*/
#define DOTP_SB2(RTYPE, mult0, mult1, cnst0, cnst1, out0, out1)   \
{                                                                 \
    out0 = (RTYPE) __msa_dotp_s_h((v16i8) mult0, (v16i8) cnst0);  \
    out1 = (RTYPE) __msa_dotp_s_h((v16i8) mult1, (v16i8) cnst1);  \
}
#define DOTP_SB2_SH(...) DOTP_SB2(v8i16, __VA_ARGS__)

/* Description : Dot product & addition of byte vector elements
   Arguments   : Inputs  - mult0, mult1
                           cnst0, cnst1
                 Outputs - out0, out1
                 Return Type - signed halfword
   Details     : Signed byte elements from mult0 are multiplied with
                 signed byte elements from cnst0 producing a result
                 twice the size of input i.e. signed halfword.
                 Then this multiplication results of adjacent odd-even elements
                 are added to the out vector
                 (2 signed halfword results)
*/
#define DPADD_SB2(RTYPE, mult0, mult1, cnst0, cnst1, out0, out1)   \
{                                                                  \
    out0 = (RTYPE) __msa_dpadd_s_h((v8i16) out0,                   \
                                   (v16i8) mult0, (v16i8) cnst0);  \
    out1 = (RTYPE) __msa_dpadd_s_h((v8i16) out1,                   \
                                   (v16i8) mult1, (v16i8) cnst1);  \
}
#define DPADD_SB2_SH(...) DPADD_SB2(v8i16, __VA_ARGS__)

#define DPADD_SB4(RTYPE, mult0, mult1, mult2, mult3,                   \
                  cnst0, cnst1, cnst2, cnst3, out0, out1, out2, out3)  \
{                                                                      \
    DPADD_SB2(RTYPE, mult0, mult1, cnst0, cnst1, out0, out1);          \
    DPADD_SB2(RTYPE, mult2, mult3, cnst2, cnst3, out2, out3);          \
}
#define DPADD_SB4_SH(...) DPADD_SB4(v8i16, __VA_ARGS__)

/* Description : Dot product & addition of halfword vector elements
   Arguments   : Inputs  - mult0, mult1
                           cnst0, cnst1
                 Outputs - out0, out1
                 Return Type - signed word
   Details     : Signed halfword elements from mult0 are multiplied with
                 signed halfword elements from cnst0 producing a result
                 twice the size of input i.e. signed word.
                 Then this multiplication results of adjacent odd-even elements
                 are added to the out vector
                 (2 signed word results)
*/
#define DPADD_SH2(RTYPE, mult0, mult1, cnst0, cnst1, out0, out1)   \
{                                                                  \
    out0 = (RTYPE) __msa_dpadd_s_w((v4i32) out0,                   \
                                   (v8i16) mult0, (v8i16) cnst0);  \
    out1 = (RTYPE) __msa_dpadd_s_w((v4i32) out1,                   \
                                   (v8i16) mult1, (v8i16) cnst1);  \
}
#define DPADD_SH2_SW(...) DPADD_SH2(v4i32, __VA_ARGS__)

/* Description : Clips all halfword elements of input vector between min & max
                 out = ((in) < (min)) ? (min) : (((in) > (max)) ? (max) : (in))
   Arguments   : Inputs  - in       (input vector)
                         - min      (min threshold)
                         - max      (max threshold)
                 Outputs - out_m    (output vector with clipped elements)
                 Return Type - signed halfword
*/
#define CLIP_SH(in, min, max)                           \
( {                                                     \
    v8i16 out_m;                                        \
                                                        \
    out_m = __msa_max_s_h((v8i16) min, (v8i16) in);     \
    out_m = __msa_min_s_h((v8i16) max, (v8i16) out_m);  \
    out_m;                                              \
} )

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

/* Description : Addition of 8 unsigned halfword elements
                 8 unsigned halfword elements of input vector are added
                 together and resulted integer sum is returned
   Arguments   : Inputs  - in       (unsigned halfword vector)
                 Outputs - sum_m    (u32 sum)
                 Return Type - unsigned word
*/
#define HADD_UH_U32(in)                                  \
( {                                                      \
    v4u32 res_m;                                         \
    v2u64 res0_m, res1_m;                                \
    uint32 sum_m;                                        \
                                                         \
    res_m = __msa_hadd_u_w((v8u16) in, (v8u16) in);      \
    res0_m = __msa_hadd_u_d(res_m, res_m);               \
    res1_m = (v2u64) __msa_splati_d((v2i64) res0_m, 1);  \
    res0_m = res0_m + res1_m;                            \
    sum_m = __msa_copy_u_w((v4i32) res0_m, 0);           \
    sum_m;                                               \
} )

/* Description : Horizontal addition of signed byte vector elements
   Arguments   : Inputs  - in0, in1
                 Outputs - out0, out1
                 Return Type - as per RTYPE
   Details     : Each signed odd byte element from 'in0' is added to
                 even signed byte element from 'in0' (pairwise) and the
                 halfword result is stored in 'out0'
*/
#define HADD_SB2(RTYPE, in0, in1, out0, out1)                 \
{                                                             \
    out0 = (RTYPE) __msa_hadd_s_h((v16i8) in0, (v16i8) in0);  \
    out1 = (RTYPE) __msa_hadd_s_h((v16i8) in1, (v16i8) in1);  \
}
#define HADD_SB2_SH(...) HADD_SB2(v8i16, __VA_ARGS__)

#define HADD_SB4(RTYPE, in0, in1, in2, in3, out0, out1, out2, out3)  \
{                                                                    \
    HADD_SB2(RTYPE, in0, in1, out0, out1);                           \
    HADD_SB2(RTYPE, in2, in3, out2, out3);                           \
}
#define HADD_SB4_SH(...) HADD_SB4(v8i16, __VA_ARGS__)

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

/* Description : SAD (Sum of Absolute Difference)
   Arguments   : Inputs  - in0, in1, ref0, ref1  (unsigned byte src & ref)
                 Outputs - sad_m                 (halfword vector with sad)
                 Return Type - unsigned halfword
   Details     : Absolute difference of all the byte elements from 'in0' with
                 'ref0' is calculated and preserved in 'diff0'. From the 16
                 unsigned absolute diff values, even-odd pairs are added
                 together to generate 8 halfword results.
*/
#define SAD_UB2_UH(in0, in1, ref0, ref1)                        \
( {                                                             \
    v16u8 diff0_m, diff1_m;                                     \
    v8u16 sad_m = { 0 };                                        \
                                                                \
    diff0_m = __msa_asub_u_b((v16u8) in0, (v16u8) ref0);        \
    diff1_m = __msa_asub_u_b((v16u8) in1, (v16u8) ref1);        \
                                                                \
    sad_m += __msa_hadd_u_h((v16u8) diff0_m, (v16u8) diff0_m);  \
    sad_m += __msa_hadd_u_h((v16u8) diff1_m, (v16u8) diff1_m);  \
                                                                \
    sad_m;                                                      \
} )

/* Description : Insert specified word elements from input vectors to 1
                 destination vector
   Arguments   : Inputs  - in0, in1, in2, in3 (4 input vectors)
                 Outputs - out                (output vector)
                 Return Type - as per RTYPE
*/
#define INSERT_W4(RTYPE, in0, in1, in2, in3, out)       \
{                                                       \
    out = (RTYPE) __msa_insert_w((v4i32) out, 0, in0);  \
    out = (RTYPE) __msa_insert_w((v4i32) out, 1, in1);  \
    out = (RTYPE) __msa_insert_w((v4i32) out, 2, in2);  \
    out = (RTYPE) __msa_insert_w((v4i32) out, 3, in3);  \
}
#define INSERT_W4_UB(...) INSERT_W4(v16u8, __VA_ARGS__)
#define INSERT_W4_SB(...) INSERT_W4(v16i8, __VA_ARGS__)

/* Description : Interleave even byte elements from vectors
   Arguments   : Inputs  - in0, in1, in2, in3
                 Outputs - out0, out1
                 Return Type - as per RTYPE
   Details     : Even byte elements of 'in0' and even byte
                 elements of 'in1' are interleaved and copied to 'out0'
                 Even byte elements of 'in2' and even byte
                 elements of 'in3' are interleaved and copied to 'out1'
*/
#define ILVEV_B2(RTYPE, in0, in1, in2, in3, out0, out1)      \
{                                                            \
    out0 = (RTYPE) __msa_ilvev_b((v16i8) in1, (v16i8) in0);  \
    out1 = (RTYPE) __msa_ilvev_b((v16i8) in3, (v16i8) in2);  \
}
#define ILVEV_B2_SD(...) ILVEV_B2(v2i64, __VA_ARGS__)

/* Description : Interleave even halfword elements from vectors
   Arguments   : Inputs  - in0, in1, in2, in3
                 Outputs - out0, out1
                 Return Type - as per RTYPE
   Details     : Even halfword elements of 'in0' and even halfword
                 elements of 'in1' are interleaved and copied to 'out0'
                 Even halfword elements of 'in2' and even halfword
                 elements of 'in3' are interleaved and copied to 'out1'
*/
#define ILVEV_H2(RTYPE, in0, in1, in2, in3, out0, out1)      \
{                                                            \
    out0 = (RTYPE) __msa_ilvev_h((v8i16) in1, (v8i16) in0);  \
    out1 = (RTYPE) __msa_ilvev_h((v8i16) in3, (v8i16) in2);  \
}
#define ILVEV_H2_UB(...) ILVEV_H2(v16u8, __VA_ARGS__)
#define ILVEV_H2_SH(...) ILVEV_H2(v8i16, __VA_ARGS__)

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
#define ILVEV_W2_SD(...) ILVEV_W2(v2i64, __VA_ARGS__)

/* Description : Interleave even double word elements from vectors
   Arguments   : Inputs  - in0, in1, in2, in3
                 Outputs - out0, out1
                 Return Type - as per RTYPE
   Details     : Even double word elements of 'in0' and even double word
                 elements of 'in1' are interleaved and copied to 'out0'
                 Even double word elements of 'in2' and even double word
                 elements of 'in3' are interleaved and copied to 'out1'
*/
#define ILVEV_D2(RTYPE, in0, in1, in2, in3, out0, out1)      \
{                                                            \
    out0 = (RTYPE) __msa_ilvev_d((v2i64) in1, (v2i64) in0);  \
    out1 = (RTYPE) __msa_ilvev_d((v2i64) in3, (v2i64) in2);  \
}
#define ILVEV_D2_UB(...) ILVEV_D2(v16u8, __VA_ARGS__)

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
#define ILVL_B4_SH(...) ILVL_B4(v8i16, __VA_ARGS__)

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
#define ILVR_B2_UB(...) ILVR_B2(v16u8, __VA_ARGS__)
#define ILVR_B2_SB(...) ILVR_B2(v16i8, __VA_ARGS__)
#define ILVR_B2_SH(...) ILVR_B2(v8i16, __VA_ARGS__)

#define ILVR_B4(RTYPE, in0, in1, in2, in3, in4, in5, in6, in7,  \
                out0, out1, out2, out3)                         \
{                                                               \
    ILVR_B2(RTYPE, in0, in1, in2, in3, out0, out1);             \
    ILVR_B2(RTYPE, in4, in5, in6, in7, out2, out3);             \
}
#define ILVR_B4_UB(...) ILVR_B4(v16u8, __VA_ARGS__)
#define ILVR_B4_SB(...) ILVR_B4(v16i8, __VA_ARGS__)
#define ILVR_B4_SH(...) ILVR_B4(v8i16, __VA_ARGS__)

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

#define ILVR_W2(RTYPE, in0, in1, in2, in3, out0, out1)      \
{                                                           \
    out0 = (RTYPE) __msa_ilvr_w((v4i32) in0, (v4i32) in1);  \
    out1 = (RTYPE) __msa_ilvr_w((v4i32) in2, (v4i32) in3);  \
}
#define ILVR_W2_UB(...) ILVR_W2(v16u8, __VA_ARGS__)

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
#define ILVR_D2_SB(...) ILVR_D2(v16i8, __VA_ARGS__)

#define ILVR_D4(RTYPE, in0, in1, in2, in3, in4, in5, in6, in7,  \
                out0, out1, out2, out3)                         \
{                                                               \
    ILVR_D2(RTYPE, in0, in1, in2, in3, out0, out1);             \
    ILVR_D2(RTYPE, in4, in5, in6, in7, out2, out3);             \
}
#define ILVR_D4_UB(...) ILVR_D4(v16u8, __VA_ARGS__)

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
#define ILVRL_H2_SH(...) ILVRL_H2(v8i16, __VA_ARGS__)
#define ILVRL_H2_SW(...) ILVRL_H2(v4i32, __VA_ARGS__)

#define ILVRL_W2(RTYPE, in0, in1, out0, out1)               \
{                                                           \
    out0 = (RTYPE) __msa_ilvr_w((v4i32) in0, (v4i32) in1);  \
    out1 = (RTYPE) __msa_ilvl_w((v4i32) in0, (v4i32) in1);  \
}
#define ILVRL_W2_SH(...) ILVRL_W2(v8i16, __VA_ARGS__)

/* Description : Saturate the halfword element values to the max
                 unsigned value of (sat_val+1 bits)
                 The element data width remains unchanged
   Arguments   : Inputs  - in0, in1, in2, in3, sat_val
                 Outputs - in0, in1, in2, in3 (in place)
                 Return Type - unsigned halfword
   Details     : Each unsigned halfword element from 'in0' is saturated to the
                 value generated with (sat_val+1) bit range
                 Results are in placed to original vectors
*/
#define SAT_UH2(RTYPE, in0, in1, sat_val)               \
{                                                       \
    in0 = (RTYPE) __msa_sat_u_h((v8u16) in0, sat_val);  \
    in1 = (RTYPE) __msa_sat_u_h((v8u16) in1, sat_val);  \
}
#define SAT_UH2_UH(...) SAT_UH2(v8u16, __VA_ARGS__)

#define SAT_UH4(RTYPE, in0, in1, in2, in3, sat_val)  \
{                                                    \
    SAT_UH2(RTYPE, in0, in1, sat_val);               \
    SAT_UH2(RTYPE, in2, in3, sat_val)                \
}
#define SAT_UH4_UH(...) SAT_UH4(v8u16, __VA_ARGS__)

/* Description : Saturate the halfword element values to the max
                 unsigned value of (sat_val+1 bits)
                 The element data width remains unchanged
   Arguments   : Inputs  - in0, in1, in2, in3, sat_val
                 Outputs - in0, in1, in2, in3 (in place)
                 Return Type - unsigned halfword
   Details     : Each unsigned halfword element from 'in0' is saturated to the
                 value generated with (sat_val+1) bit range
                 Results are in placed to original vectors
*/
#define SAT_SH2(RTYPE, in0, in1, sat_val)               \
{                                                       \
    in0 = (RTYPE) __msa_sat_s_h((v8i16) in0, sat_val);  \
    in1 = (RTYPE) __msa_sat_s_h((v8i16) in1, sat_val);  \
}
#define SAT_SH2_SH(...) SAT_SH2(v8i16, __VA_ARGS__)

#define SAT_SH4(RTYPE, in0, in1, in2, in3, sat_val)  \
{                                                    \
    SAT_SH2(RTYPE, in0, in1, sat_val);               \
    SAT_SH2(RTYPE, in2, in3, sat_val);               \
}
#define SAT_SH4_SH(...) SAT_SH4(v8i16, __VA_ARGS__)

/* Description : Saturate the word element values to the max
                 unsigned value of (sat_val+1 bits)
                 The element data width remains unchanged
   Arguments   : Inputs  - in0, in1, in2, in3, sat_val
                 Outputs - in0, in1, in2, in3 (in place)
                 Return Type - unsigned word
   Details     : Each unsigned word element from 'in0' is saturated to the
                 value generated with (sat_val+1) bit range
                 Results are in placed to original vectors
*/
#define SAT_SW2(RTYPE, in0, in1, sat_val)               \
{                                                       \
    in0 = (RTYPE) __msa_sat_s_w((v4i32) in0, sat_val);  \
    in1 = (RTYPE) __msa_sat_s_w((v4i32) in1, sat_val);  \
}
#define SAT_SW2_SW(...) SAT_SW2(v4i32, __VA_ARGS__)

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
#define PCKEV_B2_UB(...) PCKEV_B2(v16u8, __VA_ARGS__)
#define PCKEV_B2_SH(...) PCKEV_B2(v8i16, __VA_ARGS__)
#define PCKEV_B2_SW(...) PCKEV_B2(v4i32, __VA_ARGS__)

#define PCKEV_B3(RTYPE, in0, in1, in2, in3, in4, in5, out0, out1, out2)  \
{                                                                        \
    PCKEV_B2(RTYPE, in0, in1, in2, in3, out0, out1);                     \
    out2 = (RTYPE) __msa_pckev_b((v16i8) in4, (v16i8) in5);              \
}
#define PCKEV_B3_UB(...) PCKEV_B3(v16u8, __VA_ARGS__)

#define PCKEV_B4(RTYPE, in0, in1, in2, in3, in4, in5, in6, in7,  \
                 out0, out1, out2, out3)                         \
{                                                                \
    PCKEV_B2(RTYPE, in0, in1, in2, in3, out0, out1);             \
    PCKEV_B2(RTYPE, in4, in5, in6, in7, out2, out3);             \
}
#define PCKEV_B4_SB(...) PCKEV_B4(v16i8, __VA_ARGS__)
#define PCKEV_B4_UB(...) PCKEV_B4(v16u8, __VA_ARGS__)

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
#define PCKEV_D2_SB(...) PCKEV_D2(v16i8, __VA_ARGS__)
#define PCKEV_D2_SH(...) PCKEV_D2(v8i16, __VA_ARGS__)

/* Description : Pack odd double word elements of vector pairs
   Arguments   : Inputs  - in0, in1
                 Outputs - out0, out1
                 Return Type - as per RTYPE
   Details     : As operation is on same input 'in0' vector, index 1 double word
                 element is overwritten to index 0 and result is written to out0
                 As operation is on same input 'in1' vector, index 1 double word
                 element is overwritten to index 0 and result is written to out1
*/
#define PCKOD_D2(RTYPE, in0, in1, in2, in3, out0, out1)      \
{                                                            \
    out0 = (RTYPE) __msa_pckod_d((v2i64) in0, (v2i64) in1);  \
    out1 = (RTYPE) __msa_pckod_d((v2i64) in2, (v2i64) in3);  \
}
#define PCKOD_D2_UB(...) PCKOD_D2(v16u8, __VA_ARGS__)
#define PCKOD_D2_SH(...) PCKOD_D2(v8i16, __VA_ARGS__)

/* Description : Each byte element is logically xor'ed with immediate 128
   Arguments   : Inputs  - in0, in1
                 Outputs - in0, in1 (in-place)
                 Return Type - as per RTYPE
   Details     : Each unsigned byte element from input vector 'in0' is
                 logically xor'ed with 128 and result is in-place stored in
                 'in0' vector
                 Each unsigned byte element from input vector 'in1' is
                 logically xor'ed with 128 and result is in-place stored in
                 'in1' vector
                 Similar for other pairs
*/
#define XORI_B2_128(RTYPE, in0, in1)               \
{                                                  \
    in0 = (RTYPE) __msa_xori_b((v16u8) in0, 128);  \
    in1 = (RTYPE) __msa_xori_b((v16u8) in1, 128);  \
}
#define XORI_B2_128_SB(...) XORI_B2_128(v16i8, __VA_ARGS__)
#define XORI_B2_128_SH(...) XORI_B2_128(v8i16, __VA_ARGS__)

#define XORI_B3_128(RTYPE, in0, in1, in2)          \
{                                                  \
    XORI_B2_128(RTYPE, in0, in1);                  \
    in2 = (RTYPE) __msa_xori_b((v16u8) in2, 128);  \
}

#define XORI_B4_128(RTYPE, in0, in1, in2, in3)  \
{                                               \
    XORI_B2_128(RTYPE, in0, in1);               \
    XORI_B2_128(RTYPE, in2, in3);               \
}
#define XORI_B4_128_UB(...) XORI_B4_128(v16u8, __VA_ARGS__)
#define XORI_B4_128_SB(...) XORI_B4_128(v16i8, __VA_ARGS__)

#define XORI_B5_128(RTYPE, in0, in1, in2, in3, in4)  \
{                                                    \
    XORI_B3_128(RTYPE, in0, in1, in2);               \
    XORI_B2_128(RTYPE, in3, in4);                    \
}
#define XORI_B5_128_SB(...) XORI_B5_128(v16i8, __VA_ARGS__)

/* Description : Shift left all elements of vector (generic for all data types)
   Arguments   : Inputs  - in0, in1, in2, in3, shift
                 Outputs - in0, in1, in2, in3 (in place)
                 Return Type - as per input vector RTYPE
   Details     : Each element of vector 'in0' is left shifted by 'shift' and
                 result is in place written to 'in0'
                 Similar for other pairs
*/
#define SLLI_4V(in0, in1, in2, in3, shift)  \
{                                           \
    in0 = in0 << shift;                     \
    in1 = in1 << shift;                     \
    in2 = in2 << shift;                     \
    in3 = in3 << shift;                     \
}

/* Description : Arithmetic shift right all elements of vector
                 (generic for all data types)
   Arguments   : Inputs  - in0, in1, in2, in3, shift
                 Outputs - in0, in1, in2, in3 (in place)
                 Return Type - as per input vector RTYPE
   Details     : Each element of vector 'in0' is right shifted by 'shift' and
                 result is in place written to 'in0'
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
#define SRARI_H2_UH(...) SRARI_H2(v8u16, __VA_ARGS__)
#define SRARI_H2_SH(...) SRARI_H2(v8i16, __VA_ARGS__)

#define SRARI_H4(RTYPE, in0, in1, in2, in3, shift)    \
{                                                     \
    SRARI_H2(RTYPE, in0, in1, shift);                 \
    SRARI_H2(RTYPE, in2, in3, shift);                 \
}
#define SRARI_H4_UH(...) SRARI_H4(v8u16, __VA_ARGS__)
#define SRARI_H4_SH(...) SRARI_H4(v8i16, __VA_ARGS__)

/* Description : Shift right arithmetic rounded (immediate)
   Arguments   : Inputs  - in0, in1, shift
                 Outputs - in0, in1     (in place)
                 Return Type - as per RTYPE
   Details     : Each element of vector 'in0' is shifted right arithmetic by
                 value in 'shift'.
                 The last discarded bit is added to shifted value for rounding
                 and the result is in place written to 'in0'
                 Similar for other pairs
*/
#define SRARI_W2(RTYPE, in0, in1, shift)              \
{                                                     \
    in0 = (RTYPE) __msa_srari_w((v4i32) in0, shift);  \
    in1 = (RTYPE) __msa_srari_w((v4i32) in1, shift);  \
}
#define SRARI_W2_SW(...) SRARI_W2(v4i32, __VA_ARGS__)

#define SRARI_W4(RTYPE, in0, in1, in2, in3, shift)  \
{                                                   \
    SRARI_W2(RTYPE, in0, in1, shift);               \
    SRARI_W2(RTYPE, in2, in3, shift);               \
}
#define SRARI_W4_SW(...) SRARI_W4(v4i32, __VA_ARGS__)

/* Description : Multiplication of pairs of vectors
   Arguments   : Inputs  - in0, in1, in2, in3
                 Outputs - out0, out1
   Details     : Each element from 'in0' is multiplied with elements from 'in1'
                 and result is written to 'out0'
                 Similar for other pairs
*/
#define MUL2(in0, in1, in2, in3, out0, out1)  \
{                                             \
    out0 = in0 * in1;                         \
    out1 = in2 * in3;                         \
}
#define MUL4(in0, in1, in2, in3, in4, in5, in6, in7, out0, out1, out2, out3)  \
{                                                                             \
    MUL2(in0, in1, in2, in3, out0, out1);                                     \
    MUL2(in4, in5, in6, in7, out2, out3);                                     \
}

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

/* Description : Sign extend halfword elements from right half of the vector
   Arguments   : Inputs  - in    (input halfword vector)
                 Outputs - out   (sign extended word vectors)
                 Return Type - signed word
   Details     : Sign bit of halfword elements from input vector 'in' is
                 extracted and interleaved with same vector 'in0' to generate
                 4 word elements keeping sign intact
*/
#define UNPCK_R_SH_SW(in, out)                       \
{                                                    \
    v8i16 sign_m;                                    \
                                                     \
    sign_m = __msa_clti_s_h((v8i16) in, 0);          \
    out = (v4i32) __msa_ilvr_h(sign_m, (v8i16) in);  \
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

/* Description : Transposes 16x4 block into 4x16 with byte elements in vectors
   Arguments   : Inputs  - in0, in1, in2, in3, in4, in5, in6, in7,
                           in8, in9, in10, in11, in12, in13, in14, in15
                 Outputs - out0, out1, out2, out3
                 Return Type - unsigned byte
   Details     :
*/
#define TRANSPOSE16x4_UB_UB(in0, in1, in2, in3, in4, in5, in6, in7,        \
                            in8, in9, in10, in11, in12, in13, in14, in15,  \
                            out0, out1, out2, out3)                        \
{                                                                          \
    v2i64 tmp0_m, tmp1_m, tmp2_m, tmp3_m;                                  \
                                                                           \
    ILVEV_W2_SD(in0, in4, in8, in12, tmp0_m, tmp1_m);                      \
    out1 = (v16u8) __msa_ilvev_d(tmp1_m, tmp0_m);                          \
                                                                           \
    ILVEV_W2_SD(in1, in5, in9, in13, tmp0_m, tmp1_m);                      \
    out3 = (v16u8) __msa_ilvev_d(tmp1_m, tmp0_m);                          \
                                                                           \
    ILVEV_W2_SD(in2, in6, in10, in14, tmp0_m, tmp1_m);                     \
                                                                           \
    tmp2_m = __msa_ilvev_d(tmp1_m, tmp0_m);                                \
    ILVEV_W2_SD(in3, in7, in11, in15, tmp0_m, tmp1_m);                     \
                                                                           \
    tmp3_m = __msa_ilvev_d(tmp1_m, tmp0_m);                                \
    ILVEV_B2_SD(out1, out3, tmp2_m, tmp3_m, tmp0_m, tmp1_m);               \
    out0 = (v16u8) __msa_ilvev_h((v8i16) tmp1_m, (v8i16) tmp0_m);          \
    out2 = (v16u8) __msa_ilvod_h((v8i16) tmp1_m, (v8i16) tmp0_m);          \
                                                                           \
    tmp0_m = (v2i64) __msa_ilvod_b((v16i8) out3, (v16i8) out1);            \
    tmp1_m = (v2i64) __msa_ilvod_b((v16i8) tmp3_m, (v16i8) tmp2_m);        \
    out1 = (v16u8) __msa_ilvev_h((v8i16) tmp1_m, (v8i16) tmp0_m);          \
    out3 = (v16u8) __msa_ilvod_h((v8i16) tmp1_m, (v8i16) tmp0_m);          \
}

/* Description : Transposes 16x8 block into 8x16 with byte elements in vectors
   Arguments   : Inputs  - in0, in1, in2, in3, in4, in5, in6, in7,
                           in8, in9, in10, in11, in12, in13, in14, in15
                 Outputs - out0, out1, out2, out3, out4, out5, out6, out7
                 Return Type - unsigned byte
   Details     :
*/
#define TRANSPOSE16x8_UB_UB(in0, in1, in2, in3, in4, in5, in6, in7,          \
                            in8, in9, in10, in11, in12, in13, in14, in15,    \
                            out0, out1, out2, out3, out4, out5, out6, out7)  \
{                                                                            \
    v16u8 tmp0_m, tmp1_m, tmp2_m, tmp3_m;                                    \
    v16u8 tmp4_m, tmp5_m, tmp6_m, tmp7_m;                                    \
                                                                             \
    ILVEV_D2_UB(in0, in8, in1, in9, out7, out6);                             \
    ILVEV_D2_UB(in2, in10, in3, in11, out5, out4);                           \
    ILVEV_D2_UB(in4, in12, in5, in13, out3, out2);                           \
    ILVEV_D2_UB(in6, in14, in7, in15, out1, out0);                           \
                                                                             \
    tmp0_m = (v16u8) __msa_ilvev_b((v16i8) out6, (v16i8) out7);              \
    tmp4_m = (v16u8) __msa_ilvod_b((v16i8) out6, (v16i8) out7);              \
    tmp1_m = (v16u8) __msa_ilvev_b((v16i8) out4, (v16i8) out5);              \
    tmp5_m = (v16u8) __msa_ilvod_b((v16i8) out4, (v16i8) out5);              \
    out5 = (v16u8) __msa_ilvev_b((v16i8) out2, (v16i8) out3);                \
    tmp6_m = (v16u8) __msa_ilvod_b((v16i8) out2, (v16i8) out3);              \
    out7 = (v16u8) __msa_ilvev_b((v16i8) out0, (v16i8) out1);                \
    tmp7_m = (v16u8) __msa_ilvod_b((v16i8) out0, (v16i8) out1);              \
                                                                             \
    ILVEV_H2_UB(tmp0_m, tmp1_m, out5, out7, tmp2_m, tmp3_m);                 \
    out0 = (v16u8) __msa_ilvev_w((v4i32) tmp3_m, (v4i32) tmp2_m);            \
    out4 = (v16u8) __msa_ilvod_w((v4i32) tmp3_m, (v4i32) tmp2_m);            \
                                                                             \
    tmp2_m = (v16u8) __msa_ilvod_h((v8i16) tmp1_m, (v8i16) tmp0_m);          \
    tmp3_m = (v16u8) __msa_ilvod_h((v8i16) out7, (v8i16) out5);              \
    out2 = (v16u8) __msa_ilvev_w((v4i32) tmp3_m, (v4i32) tmp2_m);            \
    out6 = (v16u8) __msa_ilvod_w((v4i32) tmp3_m, (v4i32) tmp2_m);            \
                                                                             \
    ILVEV_H2_UB(tmp4_m, tmp5_m, tmp6_m, tmp7_m, tmp2_m, tmp3_m);             \
    out1 = (v16u8) __msa_ilvev_w((v4i32) tmp3_m, (v4i32) tmp2_m);            \
    out5 = (v16u8) __msa_ilvod_w((v4i32) tmp3_m, (v4i32) tmp2_m);            \
                                                                             \
    tmp2_m = (v16u8) __msa_ilvod_h((v8i16) tmp5_m, (v8i16) tmp4_m);          \
    tmp2_m = (v16u8) __msa_ilvod_h((v8i16) tmp5_m, (v8i16) tmp4_m);          \
    tmp3_m = (v16u8) __msa_ilvod_h((v8i16) tmp7_m, (v8i16) tmp6_m);          \
    tmp3_m = (v16u8) __msa_ilvod_h((v8i16) tmp7_m, (v8i16) tmp6_m);          \
    out3 = (v16u8) __msa_ilvev_w((v4i32) tmp3_m, (v4i32) tmp2_m);            \
    out7 = (v16u8) __msa_ilvod_w((v4i32) tmp3_m, (v4i32) tmp2_m);            \
}

/* Description : Transposes 4x4 block with half word elements in vectors
   Arguments   : Inputs  - in0, in1, in2, in3
                 Outputs - out0, out1, out2, out3
                 Return Type - signed halfword
   Details     :
*/
#define TRANSPOSE4x4_SH_SH(in0, in1, in2, in3, out0, out1, out2, out3)  \
{                                                                       \
    v8i16 s0_m, s1_m;                                                   \
                                                                        \
    ILVR_H2_SH(in1, in0, in3, in2, s0_m, s1_m);                         \
    ILVRL_W2_SH(s1_m, s0_m, out0, out2);                                \
    out1 = (v8i16) __msa_ilvl_d((v2i64) out0, (v2i64) out0);            \
    out3 = (v8i16) __msa_ilvl_d((v2i64) out0, (v2i64) out2);            \
}

/* Description : Dot product and addition of 3 signed halfword input vectors
   Arguments   : Inputs  - in0, in1, in2, coeff0, coeff1, coeff2
                 Outputs - out0_m
                 Return Type - signed halfword
   Details     : Dot product of 'in0' with 'coeff0'
                 Dot product of 'in1' with 'coeff1'
                 Dot product of 'in2' with 'coeff2'
                 Addition of all the 3 vector results

                 out0_m = (in0 * coeff0) + (in1 * coeff1) + (in2 * coeff2)
*/
#define DPADD_SH3_SH(in0, in1, in2, coeff0, coeff1, coeff2)         \
( {                                                                 \
    v8i16 tmp1_m;                                                   \
    v8i16 out0_m;                                                   \
                                                                    \
    out0_m = __msa_dotp_s_h((v16i8) in0, (v16i8) coeff0);           \
    out0_m = __msa_dpadd_s_h(out0_m, (v16i8) in1, (v16i8) coeff1);  \
    tmp1_m = __msa_dotp_s_h((v16i8) in2, (v16i8) coeff2);           \
    out0_m = __msa_adds_s_h(out0_m, tmp1_m);                        \
                                                                    \
    out0_m;                                                         \
} )

/* Description : Pack even elements of input vectors & xor with 128
   Arguments   : Inputs  - in0, in1
                 Outputs - out_m
                 Return Type - unsigned byte
   Details     : Signed byte even elements from 'in0' and 'in1' are packed
                 together in one vector and the resulted vector is xor'ed with
                 128 to shift the range from signed to unsigned byte
*/
#define PCKEV_XORI128_UB(in0, in1)                            \
( {                                                           \
    v16u8 out_m;                                              \
    out_m = (v16u8) __msa_pckev_b((v16i8) in1, (v16i8) in0);  \
    out_m = (v16u8) __msa_xori_b((v16u8) out_m, 128);         \
    out_m;                                                    \
} )

/* Description : Pack even byte elements, extract 0 & 2 index words from pair
                 of results and store 4 words in destination memory as per
                 stride
   Arguments   : Inputs  - in0, in1, in2, in3, pdst, stride
*/
#define PCKEV_ST4x4_UB(in0, in1, in2, in3, pdst, stride)  \
{                                                         \
    uint32 out0_m, out1_m, out2_m, out3_m;                \
    v16i8 tmp0_m, tmp1_m;                                 \
                                                          \
    PCKEV_B2_SB(in1, in0, in3, in2, tmp0_m, tmp1_m);      \
                                                          \
    out0_m = __msa_copy_u_w((v4i32) tmp0_m, 0);           \
    out1_m = __msa_copy_u_w((v4i32) tmp0_m, 2);           \
    out2_m = __msa_copy_u_w((v4i32) tmp1_m, 0);           \
    out3_m = __msa_copy_u_w((v4i32) tmp1_m, 2);           \
                                                          \
    SW4(out0_m, out1_m, out2_m, out3_m, pdst, stride);    \
}
#endif  /* #ifndef AVC_COMMON_MACROS_MSA_H */
