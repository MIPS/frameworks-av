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

#ifndef H264SWDEC_PROTOTYPES_MSA_H
#define H264SWDEC_PROTOTYPES_MSA_H

#include "basetype.h"

#ifdef H264DEC_MSA
typedef struct {
    u8 top;
    u8 left;
} bS_t;

#define FILTER_LEFT_EDGE    0x04
#define FILTER_TOP_EDGE     0x02
#define FILTER_INNER_EDGE   0x01

u32 InnerBoundaryStrength(mbStorage_t *mb1, u32 i1, u32 i2);
u32 EdgeBoundaryStrength(mbStorage_t *mb1, mbStorage_t *mb2, u32 i1, u32 i2);

/* loop filter functions */
u32 avc_get_boundary_strengths_msa(mbStorage_t *mb, bS_t *bs, u32 flags);
void avc_loopfilter_luma_intra_edge_hor_msa(u8 *data, u8 alpha_in,
                                            u8 beta_in, u32 img_width);
void avc_loopfilter_luma_intra_edge_ver_msa(u8 *data, u8 alpha_in,
                                            u8 beta_in, u32 img_width);
void avc_loopfilter_cbcr_intra_edge_hor_msa(u8 *data_cb, u8 *data_cr,
                                            u8 alpha_in, u8 beta_in,
                                            u32 img_width);
void avc_loopfilter_cbcr_intra_edge_ver_msa(u8 *data_cb, u8 *data_cr,
                                            u8 alpha_in, u8 beta_in,
                                            u32 img_width);
void avc_loopfilter_luma_inter_edge_ver_msa(u8 *data,
                                            u8 bs0, u8 bs1, u8 bs2, u8 bs3,
                                            u8 tc0, u8 tc1, u8 tc2, u8 tc3,
                                            u8 alpha_in, u8 beta_in,
                                            u32 img_width);
void avc_loopfilter_luma_inter_edge_hor_msa(u8 *data,
                                            u8 bs0, u8 bs1, u8 bs2, u8 bs3,
                                            u8 tc0, u8 tc1, u8 tc2, u8 tc3,
                                            u8 alpha_in, u8 beta_in,
                                            u32 image_width);

void avc_loopfilter_cbcr_inter_edge_ver_msa(u8 *data_cb, u8 *data_cr,
                                            u8 bs0, u8 bs1, u8 bs2, u8 bs3,
                                            u8 tc0, u8 tc1, u8 tc2, u8 tc3,
                                            u8 alpha_in, u8 beta_in,
                                            u32 img_width);
void avc_loopfilter_cbcr_inter_edge_hor_msa(u8 *data_cb, u8 *data_cr,
                                            u8 bs0, u8 bs1, u8 bs2, u8 bs3,
                                            u8 tc0, u8 tc1, u8 tc2, u8 tc3,
                                            u8 alpha_in, u8 beta_in,
                                            u32 img_width);
#endif  /* #ifdef H264DEC_MSA */
#endif  /* #ifndef H264SWDEC_PROTOTYPES_MSA_H */
