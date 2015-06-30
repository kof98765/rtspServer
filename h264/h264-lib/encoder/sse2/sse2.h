/*****************************************************************************
*
*  T264 AVC CODEC
*
*  Copyright(C) 2004-2005 llcc <lcgate1@yahoo.com.cn>
*               2004-2005 visionany <visionany@yahoo.com.cn>
*
*  This program is free software ; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation ; either version 2 of the License, or
*  (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY ; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program ; if not, write to the Free Software
*  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
*
****************************************************************************/

#ifndef _SSE2_H_
#define _SSE2_H_

int32_t T264_detect_cpu();
// 16x16 luma
void T264_predict_16x16_mode_0_sse2 (uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left);
void T264_predict_16x16_mode_1_sse2 (uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left);
void T264_predict_16x16_mode_2_sse2 (uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left);
void T264_predict_16x16_mode_20_sse2(uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left);
void T264_predict_16x16_mode_21_sse2(uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left);
void T264_predict_16x16_mode_22_sse2(uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left);
void T264_predict_16x16_mode_3_sse2 (uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left);

// 4x4 luma
void T264_predict_4x4_mode_0_sse2(uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left);
void T264_predict_4x4_mode_1_sse2(uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left);
void T264_predict_4x4_mode_2_sse2(uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left);
void T264_predict_4x4_mode_20_sse2(uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left);
void T264_predict_4x4_mode_21_sse2(uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left);
void T264_predict_4x4_mode_22_sse2(uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left);
void T264_predict_4x4_mode_3_sse2(uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left);	
void T264_predict_4x4_mode_4_sse2(uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left);
void T264_predict_4x4_mode_5_sse2(uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left);
void T264_predict_4x4_mode_6_sse2(uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left);
void T264_predict_4x4_mode_7_sse2(uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left);
void T264_predict_4x4_mode_8_sse2(uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left);

// 8x8 chroma
void T264_predict_8x8_mode_0_sse2 (uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left);
void T264_predict_8x8_mode_1_sse2 (uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left);
void T264_predict_8x8_mode_2_sse2 (uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left);
void T264_predict_8x8_mode_20_sse2(uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left);
void T264_predict_8x8_mode_21_sse2(uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left);
void T264_predict_8x8_mode_22_sse2(uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left);
void T264_predict_8x8_mode_3_sse2 (uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left);

// dct & quant
void dct4x4_mmx(int16_t* data);
void dct4x4dc_mmx(int16_t* data);
void idct4x4_mmx(int16_t* data);
void idct4x4dc_mmx(int16_t* data);

void quant4x4_sse2(int16_t* data, const int32_t Qp, int32_t is_intra);
void quant4x4dc_sse2(int16_t* data, const int32_t Qp);
void quant2x2dc_sse2(int16_t* data, const int32_t Qp, int32_t is_intra);
void iquant4x4_sse2(int16_t* data, const int32_t Qp);
void iquant4x4dc_sse2(int16_t* data, const int32_t Qp);
void iquant2x2dc_sse2(int16_t* data, const int32_t Qp);

// me
void T264_eighth_pixel_mc_u_sse2(uint8_t* src, int32_t src_stride, uint8_t* dst, int16_t mvx, int16_t mvy, int32_t width, int32_t height);
void interpolate_halfpel_h_sse2(uint8_t* src, int32_t src_stride, uint8_t* dst, int32_t dst_stride, int32_t width, int32_t height);
void interpolate_halfpel_v_sse2(uint8_t* src, int32_t src_stride, uint8_t* dst, int32_t dst_stride, int32_t width, int32_t height);
void interpolate_halfpel_hv_sse2(uint8_t* src, int32_t src_stride, uint8_t* dst, int32_t dst_stride, int32_t width, int32_t height);
void T264_pia_u_16x16_sse2(uint8_t* p1, uint8_t* p2, int32_t p1_stride, int32_t p2_stride, uint8_t* dst, int32_t dst_stride);
void T264_pia_u_16x8_sse2(uint8_t* p1, uint8_t* p2, int32_t p1_stride, int32_t p2_stride, uint8_t* dst, int32_t dst_stride);
void T264_pia_u_16x16_sse(uint8_t* p1, uint8_t* p2, int32_t p1_stride, int32_t p2_stride, uint8_t* dst, int32_t dst_stride);
void T264_pia_u_16x8_sse(uint8_t* p1, uint8_t* p2, int32_t p1_stride, int32_t p2_stride, uint8_t* dst, int32_t dst_stride);
void T264_pia_u_8x16_sse(uint8_t* p1, uint8_t* p2, int32_t p1_stride, int32_t p2_stride, uint8_t* dst, int32_t dst_stride);
void T264_pia_u_8x8_sse(uint8_t* p1, uint8_t* p2, int32_t p1_stride, int32_t p2_stride, uint8_t* dst, int32_t dst_stride);
void T264_pia_u_8x4_sse(uint8_t* p1, uint8_t* p2, int32_t p1_stride, int32_t p2_stride, uint8_t* dst, int32_t dst_stride);
void T264_pia_u_4x8_mmx(uint8_t* p1, uint8_t* p2, int32_t p1_stride, int32_t p2_stride, uint8_t* dst, int32_t dst_stride);
void T264_pia_u_4x4_mmx(uint8_t* p1, uint8_t* p2, int32_t p1_stride, int32_t p2_stride, uint8_t* dst, int32_t dst_stride);

// utility
void expand8to16_sse2(uint8_t* src, int32_t src_stride, int32_t quarter_width, int32_t quarter_height, int16_t* dst);
void contract16to8_sse2(int16_t* src, int32_t quarter_width, int32_t quarter_height, uint8_t* dst, int32_t dst_stride);
void contract16to8add_sse2(int16_t* src, int32_t quarter_width, int32_t quarter_height, uint8_t* org, uint8_t* dst, int32_t dst_stride);
void memcpy_stride_u_sse2(void* src, int32_t width, int32_t height, int32_t src_stride, void* dst, int32_t dst_stride);
void expand8to16sub_sse2(uint8_t* pred, int32_t quarter_width, int32_t quarter_height, int16_t* dst, uint8_t* src, int32_t src_stride);
uint32_t T264_sad_u_16x16_sse2(uint8_t* src, int32_t src_stride, uint8_t* data, int32_t dst_stride);
uint32_t T264_sad_u_16x8_sse2(uint8_t* src, int32_t src_stride, uint8_t* data, int32_t dst_stride);
uint32_t T264_sad_u_8x16_sse(uint8_t* src, int32_t src_stride, uint8_t* data, int32_t dst_stride);
uint32_t T264_sad_u_8x8_sse(uint8_t* src, int32_t src_stride, uint8_t* data, int32_t dst_stride);
uint32_t T264_sad_u_8x4_sse(uint8_t* src, int32_t src_stride, uint8_t* data, int32_t dst_stride);
uint32_t T264_sad_u_4x8_sse(uint8_t* src, int32_t src_stride, uint8_t* data, int32_t dst_stride);
uint32_t T264_sad_u_4x4_sse(uint8_t* src, int32_t src_stride, uint8_t* data, int32_t dst_stride);
uint32_t T264_satd_16x16_u_sse2(uint8_t* src, int32_t src_stride, uint8_t* data, int32_t dst_stride);

void contract16to8_4x4_mmx(uint16_t* src, int32_t src_stride, uint8_t* dst, int32_t dst_stride);
void contract16to8_mmx(int16_t* src, int32_t quarter_width, int32_t quarter_height, uint8_t* dst, int32_t dst_stride);
void contract16to8add_4x4_mmx(uint16_t* src,uint8_t* pred, int32_t pred_stride,uint8_t* dst, int32_t dst_stride);
void contract16to8add_mmx(int16_t* src, int32_t quarter_width, int32_t quarter_height, uint8_t* pred, uint8_t* dst, int32_t dst_stride);
void expand8to16sub_4x4_mmx(uint8_t* pred,int32_t pred_stride,int8_t* src,int32_t src_stride,uint16_t* dst);
void expand8to16sub_mmx(uint8_t* pred, int32_t quarter_width, int32_t quarter_height, int16_t* dst , uint8_t* src, int32_t src_stride);

void T264_emms_mmx();

#endif
