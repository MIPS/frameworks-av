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
#include "common_macros_msa.h"
#include "prototypes_msa.h"

#ifdef PV_POSTPROC_ON
#ifdef M4VH263DEC_MSA
void FindMaxMin2BlockMSA(uint8 *in, int *min, int *max, int width)
{
    v16i8 first_shuffle = { 4, 5, 6, 7, 4, 5, 6, 7, 12, 13, 14, 15, 12, 13, 14, 15 };
    v16i8 second_shuffle = { 2, 3, 2, 3, 4, 5, 6, 7, 10, 11, 10, 11, 12, 13, 14, 15 };
    v16i8 third_shuffle = { 1, 1, 2, 3, 4, 5, 6, 7, 9, 9, 10, 11, 12, 13, 14, 15 };
    v16u8 in_r0, in_r1, in_r2, in_r3, in_r4, in_r5, in_r6, in_r7;
    v16u8 in_max, in_max_temp, in_min, in_min_temp;

    LD_UB8(in, width, in_r0, in_r1, in_r2, in_r3, in_r4, in_r5, in_r6, in_r7);

    in_max = __msa_max_u_b(in_r0, in_r1);
    in_min = __msa_min_u_b(in_r0, in_r1);
    in_max = __msa_max_u_b(in_max, in_r2);
    in_min = __msa_min_u_b(in_min, in_r2);
    in_max = __msa_max_u_b(in_max, in_r3);
    in_min = __msa_min_u_b(in_min, in_r3);
    in_max = __msa_max_u_b(in_max, in_r4);
    in_min = __msa_min_u_b(in_min, in_r4);
    in_max = __msa_max_u_b(in_max, in_r5);
    in_min = __msa_min_u_b(in_min, in_r5);
    in_max = __msa_max_u_b(in_max, in_r6);
    in_min = __msa_min_u_b(in_min, in_r6);
    in_max = __msa_max_u_b(in_max, in_r7);
    in_min = __msa_min_u_b(in_min, in_r7);

    VSHF_B2_UB(in_max, in_max, in_min, in_min, first_shuffle, first_shuffle, 
               in_max_temp, in_min_temp);

    in_max = __msa_max_u_b(in_max, in_max_temp);
    in_min = __msa_min_u_b(in_min, in_min_temp);
    VSHF_B2_UB(in_max, in_max, in_min, in_min, second_shuffle, second_shuffle, 
               in_max_temp, in_min_temp);

    in_max = __msa_max_u_b(in_max, in_max_temp);
    in_min = __msa_min_u_b(in_min, in_min_temp);
    VSHF_B2_UB(in_max, in_max, in_min, in_min, third_shuffle, third_shuffle, 
               in_max_temp, in_min_temp);

    in_max = __msa_max_u_b(in_max, in_max_temp);
    in_min = __msa_min_u_b(in_min, in_min_temp);

    *max++ = __msa_copy_u_b((v16i8) in_max, 0);
    *min++ = __msa_copy_u_b((v16i8) in_min, 0);
    *max = __msa_copy_u_b((v16i8) in_max, 8);
    *min = __msa_copy_u_b((v16i8) in_min, 8);

    return;
}

void FindMaxMin1BlockMSA(uint8 *in, int *min, int *max, int width)
{
    v16u8 in_r0, in_r1, in_r2, in_r3, in_r4, in_r5, in_r6, in_r7;
    v16u8 in_max, in_max_temp, in_min, in_min_temp;

    LD_UB8(in, width, in_r0, in_r1, in_r2, in_r3, in_r4, in_r5, in_r6, in_r7);

    in_max = __msa_max_u_b(in_r0, in_r1);
    in_min = __msa_min_u_b(in_r0, in_r1);
    in_max = __msa_max_u_b(in_max, in_r2);
    in_min = __msa_min_u_b(in_min, in_r2);
    in_max = __msa_max_u_b(in_max, in_r3);
    in_min = __msa_min_u_b(in_min, in_r3);
    in_max = __msa_max_u_b(in_max, in_r4);
    in_min = __msa_min_u_b(in_min, in_r4);
    in_max = __msa_max_u_b(in_max, in_r5);
    in_min = __msa_min_u_b(in_min, in_r5);
    in_max = __msa_max_u_b(in_max, in_r6);
    in_min = __msa_min_u_b(in_min, in_r6);
    in_max = __msa_max_u_b(in_max, in_r7);
    in_min = __msa_min_u_b(in_min, in_r7);

    in_max_temp = (v16u8) __msa_splati_w((v4i32) in_max, 1);
    in_min_temp = (v16u8) __msa_splati_w((v4i32) in_min, 1);

    in_max = __msa_max_u_b(in_max, in_max_temp);
    in_min = __msa_min_u_b(in_min, in_min_temp);

    in_max_temp = (v16u8) __msa_splati_h((v8i16) in_max, 1);
    in_min_temp = (v16u8) __msa_splati_h((v8i16) in_min, 1);

    in_max = __msa_max_u_b(in_max, in_max_temp);
    in_min = __msa_min_u_b(in_min, in_min_temp);

    in_max_temp = (v16u8) __msa_splati_b((v16i8) in_max, 1);
    in_min_temp = (v16u8) __msa_splati_b((v16i8) in_min, 1);

    in_max = __msa_max_u_b(in_max, in_max_temp);
    in_min = __msa_min_u_b(in_min, in_min_temp);

    *max = __msa_copy_u_b((v16i8) in_max, 0);
    *min = __msa_copy_u_b((v16i8) in_min, 0);

    return;
}
#endif /* M4VH263DEC_MSA */
#endif /* PV_POSTPROC_ON */
