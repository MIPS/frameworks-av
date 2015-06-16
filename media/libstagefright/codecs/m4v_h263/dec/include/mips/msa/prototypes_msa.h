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

#ifndef _PROTOTYPES_MSA_H
#define _PROTOTYPES_MSA_H

#include "m4vh263_decoder_pv_types.h"

#ifdef M4VH263DEC_MSA
void h263_deblock_luma_msa(uint8 *rec, int32 width, int32 height,
                           int16 *QP_store, uint8 *mode,
                           int32 chr, int32 annex_T);
void h263_deblock_chroma_msa(uint8 *rec, int32 width, int32 height,
                             int16 *QP_store, uint8 *mode,
                             int32 chr, int32 annex_T);

void m4v_h263_idct_msa(int16 *coeff_blk, uint8 *dst, int32 dst_stride);
void m4v_h263_idct_addblk_msa(int16 *coeff_blk, uint8 *pred,
                              uint8 *dst, int32 dst_stride);
void m4v_h263_idct_4rows_msa(int16 *coeff_blk, uint8 *dst, int32 dst_stride);
void m4v_h263_idct_addblk_4rows_msa(int16 *coeff_blk, uint8 *pred,
                                    uint8 *dst, int32 dst_stride);

int32 m4v_h263_horiz_filter_msa(uint8 *src, uint8 *dst, int32 src_stride,
                                int32 pred_width_rnd);
int32 m4v_h263_vert_filter_msa(uint8 *src, uint8 *dst, int32 src_stride,
                               int32 pred_width_rnd);
int32 m4v_h263_hv_filter_msa(uint8 *src, uint8 *dst, int32 src_stride,
                             int32 pred_width_rnd);
int32 m4v_h263_copy_msa(uint8 *pu8Src, uint8 *pu8Dst, int32 i32SrcStride,
                        int32 i32PredWidthRnd);

void DeringAdaptiveSmooth4ColsMSA(uint8 *recon_y,   /* i/o */
                                  int rows,         /* i   */
                                  int threshold,    /* i   */
                                  int width,        /* i   */
                                  int max_diff      /* i   */);

void DeringAdaptiveSmooth6ColsMSA(uint8 *recon_y,   /* i/o */
                                  int rows,         /* i   */
                                  int threshold,    /* i   */
                                  int width,        /* i   */
                                  int max_diff      /* i   */);

void DeringAdaptiveSmooth8ColsMSA(uint8 *recon_y,   /* i/o  */
                                  int rows,         /* i    */
                                  int threshold,    /* i    */
                                  int width,        /* i    */
                                  int max_diff      /* i    */);

void PostProcCr6ColsMSA(uint8 *recon_c,   /* i/o  */
                        int rows,         /* i    */
                        int threshold,    /* i    */
                        int width,        /* i    */
                        int max_diff      /* i    */);

void PostProcCr8ColsMSA(uint8 *recon_c,  /* i/o  */
                        int rows,        /* i    */
                        int threshold,   /* i    */
                        int width,       /* i    */
                        int max_diff     /* i    */);

void FindMaxMin2BlockMSA(uint8 *in, int *min, int *max, int width);

void FindMaxMin1BlockMSA(uint8 *in, int *min, int *max, int width);

void m4v_h263_CombinedHorzVertFilter_msa(uint8 *rec, int width, int height,
                                         int16 *qp_store, int chr, uint8 *pp_mod);

void m4v_h263_CombinedHorzVertFilter_NoSoftDeblocking_msa(uint8 *rec,
                                                          int width,
                                                          int height,
                                                          int16 *qp_store,
                                                          int chr,
                                                          uint8 *pp_mod);

void HardFiltHorMSA(uint8 *ptr, int qp, int width);
void HardFiltVerMSA(uint8 *ptr, int qp, int width);
void SoftFiltHorMSA(uint8 *ptr, int qp, int width);
void SoftFiltVerMSA(uint8 *ptr, int qp, int width);
#endif /* #ifdef M4VH263DEC_MSA */
#endif /* #ifndef _PROTOTYPES_MSA_H */
