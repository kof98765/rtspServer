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

#ifndef _INTERPOLATE_H_
#define _INTERPOLATE_H_

uint32_t T264_quarter_pixel_search(T264_t* t, int32_t list_index, uint8_t* src, T264_frame_t* refframe, int32_t offset, T264_vector_t* vec, T264_vector_t* vec_median, uint32_t sad_org, int32_t w, int32_t h, uint8_t* residual, int32_t mb_part);
void T264_eighth_pixel_mc_u_c(uint8_t* src, int32_t src_stride, uint8_t* dst, int16_t mvx, int16_t mvy, int32_t width, int32_t height);

void interpolate_halfpel_h_c(uint8_t* src, int32_t src_stride, uint8_t* dst, int32_t dst_stride, int32_t width, int32_t height);
void interpolate_halfpel_v_c(uint8_t* src, int32_t src_stride, uint8_t* dst, int32_t dst_stride, int32_t width, int32_t height);
void interpolate_halfpel_hv_c(uint8_t* src, int32_t src_stride, uint8_t* dst, int32_t dst_stride, int32_t width, int32_t height);

#define REFCOST(refno)  2 * t->mb.lambda * T264_MIN(refno, 1)
//#define REFCOST(refno)  2 * t->mb.lambda * eg_size_te(t->bs, t->refl0_num, refno)
#define REFCOST1(refno)  0

void T264_pia_u_16x16_c(uint8_t* p1, uint8_t* p2, int32_t p1_stride, int32_t p2_stride, uint8_t* dst, int32_t dst_stride);
void T264_pia_u_16x8_c(uint8_t* p1, uint8_t* p2, int32_t p1_stride, int32_t p2_stride, uint8_t* dst, int32_t dst_stride);
void T264_pia_u_8x16_c(uint8_t* p1, uint8_t* p2, int32_t p1_stride, int32_t p2_stride, uint8_t* dst, int32_t dst_stride);
void T264_pia_u_8x8_c(uint8_t* p1, uint8_t* p2, int32_t p1_stride, int32_t p2_stride, uint8_t* dst, int32_t dst_stride);
void T264_pia_u_8x4_c(uint8_t* p1, uint8_t* p2, int32_t p1_stride, int32_t p2_stride, uint8_t* dst, int32_t dst_stride);
void T264_pia_u_4x8_c(uint8_t* p1, uint8_t* p2, int32_t p1_stride, int32_t p2_stride, uint8_t* dst, int32_t dst_stride);
void T264_pia_u_4x4_c(uint8_t* p1, uint8_t* p2, int32_t p1_stride, int32_t p2_stride, uint8_t* dst, int32_t dst_stride);
void T264_pia_u_2x2_c(uint8_t* p1, uint8_t* p2, int32_t p1_stride, int32_t p2_stride, uint8_t* dst, int32_t dst_stride);

#endif
