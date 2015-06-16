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

#ifndef AVC_PROTOTYPES_MSA_H
#define AVC_PROTOTYPES_MSA_H

#include "avc_types.h"

#ifdef H264ENC_MSA
/* loop filter */
void avc_loopfilter_luma_intra_edge_hor_msa(uint8 *data, uint8 alpha_in,
                                            uint8 beta_in, uint32 img_width);
void avc_loopfilter_luma_intra_edge_ver_msa(uint8 *data, uint8 alpha_in,
                                            uint8 beta_in, uint32 img_width);
void avc_loopfilter_cbcr_intra_edge_hor_msa(uint8 *data_cb, uint8 *data_cr,
                                            uint8 alpha_in, uint8 beta_in,
                                            uint32 img_width);
void avc_loopfilter_cbcr_intra_edge_ver_msa(uint8 *data_cb, uint8 *data_cr,
                                            uint8 alpha_in, uint8 beta_in,
                                            uint32 img_width);
void avc_loopfilter_luma_inter_edge_ver_msa(uint8 *data, uint8 bs0, uint8 bs1,
                                            uint8 bs2, uint8 bs3,
                                            uint8 tc0, uint8 tc1,
                                            uint8 tc2, uint8 tc3,
                                            uint8 alpha_in, uint8 beta_in,
                                            uint32 img_width);
void avc_loopfilter_luma_inter_edge_hor_msa(uint8 *data,
                                            uint8 bs0, uint8 bs1,
                                            uint8 bs2, uint8 bs3,
                                            uint8 tc0, uint8 tc1,
                                            uint8 tc2, uint8 tc3,
                                            uint8 alpha_in, uint8 beta_in,
                                            uint32 image_width);
void avc_loopfilter_cbcr_inter_edge_ver_msa(uint8 *data_cb, uint8 *data_cr,
                                            uint8 bs0, uint8 bs1,
                                            uint8 bs2, uint8 bs3,
                                            uint8 tc0, uint8 tc1,
                                            uint8 tc2, uint8 tc3,
                                            uint8 alpha_in, uint8 beta_in,
                                            uint32 img_width);
void avc_loopfilter_cbcr_inter_edge_hor_msa(uint8 *data_cb, uint8 *data_cr,
                                            uint8 bs0, uint8 bs1,
                                            uint8 bs2, uint8 bs3,
                                            uint8 tc0, uint8 tc1,
                                            uint8 tc2, uint8 tc3,
                                            uint8 alpha_in, uint8 beta_in,
                                            uint32 img_width);

/* mc/inter predict functions */
void avc_luma_hz_8w_msa(uint8 *src, int32 src_stride,
                        uint8 *dst, int32 dst_stride, int32 height);
void avc_luma_hz_16w_msa(uint8 *src, int32 src_stride,
                         uint8 *dst, int32 dst_stride, int32 height);
void avc_luma_hz_qrt_8w_msa(uint8 *src, int32 src_stride,
                            uint8 *dst, int32 dst_stride,
                            int32 height, uint8 hor_offset);
void avc_luma_hz_qrt_16w_msa(uint8 *src, int32 src_stride,
                             uint8 *dst, int32 dst_stride,
                             int32 height, uint8 hor_offset);
void avc_luma_vt_8w_msa(uint8 * __restrict src, int32 src_stride,
                        uint8 * __restrict dst, int32 dst_stride, int32 height);
void avc_luma_vt_16w_msa(uint8 * __restrict src, int32 src_stride,
                         uint8 * __restrict dst, int32 dst_stride, int32 height);
void avc_luma_vt_qrt_8w_msa(uint8 * __restrict src, int32 src_stride,
                            uint8 * __restrict dst, int32 dst_stride,
                            int32 height, uint8 ver_offset);
void avc_luma_vt_qrt_16w_msa(uint8 * __restrict src, int32 src_stride,
                             uint8 * __restrict dst, int32 dst_stride,
                             int32 height, uint8 ver_offset);
void avc_luma_mid_8w_msa(uint8 *src, int32 src_stride,
                         uint8 *dst, int32 dst_stride, int32 height);
void avc_luma_mid_16w_msa(uint8 *src, int32 src_stride,
                          uint8 *dst, int32 dst_stride,
                          int32 height);
void avc_luma_midh_qrt_8w_msa(uint8 *src, int32 src_stride,
                              uint8 *dst, int32 dst_stride,
                              int32 height, uint8 horiz_offset);
void avc_luma_midh_qrt_16w_msa(uint8 *src, int32 src_stride,
                               uint8 *dst, int32 dst_stride,
                               int32 height, uint8 horiz_offset);
void avc_luma_midv_qrt_8w_msa(uint8 *src, int32 src_stride,
                              uint8 *dst, int32 dst_stride,
                              int32 height, uint8 vert_offset);
void avc_luma_midv_qrt_16w_msa(uint8 *src, int32 src_stride,
                               uint8 *dst, int32 dst_stride,
                               int32 height, uint8 vert_offset);
void avc_luma_hv_qrt_8w_msa(uint8 *src_x, uint8 *src_y, int32 src_stride,
                            uint8 *dst, int32 dst_stride, int32 height);
void avc_luma_hv_qrt_16w_msa(uint8 *src_x, uint8 *src_y, int32 src_stride,
                             uint8 *dst, int32 dst_stride, int32 height);
void avc_chroma_hz_4w_msa(uint8 *src, int32 src_stride,
                          uint8 *dst, int32 dst_stride,
                          uint32 coeff0, uint32 coeff1, int32 height);
void avc_chroma_hz_8w_msa(uint8 *src, int32 src_stride,
                          uint8 *dst, int32 dst_stride,
                          uint32 coeff0, uint32 coeff1, int32 height);
void avc_chroma_vt_4w_msa(uint8 *src, int32 src_stride,
                          uint8 *dst, int32 dst_stride,
                          uint32 coeff0, uint32 coeff1, int32 height);
void avc_chroma_vt_8w_msa(uint8 *src, int32 src_stride,
                          uint8 *dst, int32 dst_stride,
                          uint32 coeff0, uint32 coeff1, int32 height);
void avc_chroma_hv_4w_msa(uint8 *src, int32 src_stride,
                          uint8 *dst, int32 dst_stride,
                          uint32 coeff_horiz0, uint32 coeff_horiz1,
                          uint32 coeff_vert0, uint32 coeff_vert1, int32 height);
void avc_chroma_hv_8w_msa(uint8 *src, int32 src_stride,
                          uint8 *dst, int32 dst_stride,
                          uint32 coeff_horiz0, uint32 coeff_horiz1,
                          uint32 coeff_vert0, uint32 coeff_vert1, int32 height);

/* fdct */
int32 avc_transform_4x4_msa(uint8 *src_ptr, int32 src_stride,
                           uint8 *pred_ptr, int32 pred_stride,
                           uint8 *dst, int32 dst_stride,
                           int16 *coef, int32 coef_stride,
                           int32 *coef_cost, int32 *level,
                           int32 *run, uint8 is_intra_mb,
                           uint8 rq, uint8 qq, int32 qp_const);
void avc_mb_inter_idct_16x16_msa(uint8 *src_ptr, int32 src_stride,
                                 int16 *coef, int32 coef_stride,
                                 uint8 *mb_nz_coef, int16 cbp);
void avc_luma_transform_16x16_msa(uint8 *src, int32 src_stride,
                                  uint8 *pred, int32 pred_stride,
                                  uint8 *dst, int32 dst_stride,
                                  int16 *coef, int32 coef_stride,
                                  int32 *level_dc, int32 *run_dc,
                                  int32 *level_ac, int32 *run_ac,
                                  uint8 *mb_nz_coef, uint32 *cbp,
                                  uint8 rq, uint8 qq,
                                  int32 qp_const,
                                  int32 *num_coef_dc);
void avc_chroma_transform_8x8_msa(uint8 *src, int32 src_stride,
                                  uint8 *pred, int32 pred_stride,
                                  uint8 *dst, int32 dst_stride,
                                  int16 *coef, int32 coef_stride,
                                  int32 *coef_cost,
                                  int32 *level_dc, int32 *run_dc,
                                  int32 *level_ac, int32 *run_ac,
                                  uint8 is_intra_mb,
                                  int32 *num_coef_cdc,
                                  uint8 *mb_nz_coef, uint32 *cbp,
                                  uint8 cr, uint8 rq,
                                  uint8 qq, int32 qp_const);
uint32 avc_calc_residue_satd_4x4_msa(uint8 *src_ptr, int32 src_stride,
                                     uint8 *pred_ptr, int32 pred_stride);
uint32 avc_calc_residue_satd_16x16_msa(uint8 *src, int32 src_stride,
                                       uint8 *pred, int32 pred_stride);

/* motion estimation */
void avc_calc_qpel_data_msa(uint8 **pref_data, uint8 *qpel_buf, uint8 hpel_pos);
void avc_horiz_filter_6taps_17width_msa(uint8 *src, int32 src_stride,
                                        uint8 *dst, int32 dst_stride,
                                        int32 height);
void avc_vert_filter_6taps_18width_msa(uint8 *src, int32 src_stride,
                                       uint8 *dst, int32 dst_stride,
                                       int32 height);
void avc_mid_filter_6taps_17width_msa(uint8 *src, int32 src_stride,
                                      uint8 *dst, int32 dst_stride,
                                      int32 height);
uint32 sad16x16_msa(uint8 *src_ptr, int32 src_stride,
                    uint8 *ref_ptr, int32 ref_stride);

/* intra prediction */
void intra_predict_hor_vert_dc_16x16_msa(uint8 *src_top, uint8 *src_left,
                                         int32 src_stride_left, uint8 *dst_vert,
                                         uint8 *dst_horiz, uint8 *dst_dc,
                                         int32 dst_stride, uint8 is_above,
                                         uint8 is_left);
#endif /* #ifdef H264ENC_MSA */
#endif /* #ifndef AVC_PROTOTYPES_MSA_H */
