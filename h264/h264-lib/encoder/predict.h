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

#ifndef _PREDICT_H
#define _PREDICT_H

/******************************************************
*	predict.h
*	Intra prediction for 16x16 luma, 4x4 luma, and 
						 8x8 chroma
******************************************************/
//
// 单纯的预测函数：只完成预测，预测值被两个地方使用，计算残差和从残差恢复图像
//
void T264_predict_16x16_mode_0_c (uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left);
void T264_predict_16x16_mode_1_c (uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left);
void T264_predict_16x16_mode_2_c (uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left);
void T264_predict_16x16_mode_20_c(uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left);
void T264_predict_16x16_mode_21_c(uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left);
void T264_predict_16x16_mode_22_c(uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left);
void T264_predict_16x16_mode_3_c (uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left);

//	4x4 luma	(6 functions)
void T264_predict_4x4_mode_0_c(uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left);
void T264_predict_4x4_mode_1_c(uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left);
void T264_predict_4x4_mode_2_c(uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left);
void T264_predict_4x4_mode_20_c(uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left);
void T264_predict_4x4_mode_21_c(uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left);
void T264_predict_4x4_mode_22_c(uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left);

//Mode 3 Intra_4x4_DIAGONAL_DOWNLEFT  when Top are available
void T264_predict_4x4_mode_3_c(uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left);	
//Mode 4 Intra_4x4_DIAGONAL_DOWNRIGHT when Top and left are available
void T264_predict_4x4_mode_4_c(uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left);
//Mode 5 Intra_4x4_VERTICAL_RIGHT when Top and left are available
void T264_predict_4x4_mode_5_c(uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left);
//Mode 6 Intra_4x4_HORIZONTAL_DOWN when Top and left are available
void T264_predict_4x4_mode_6_c(uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left);
//Mode 7 Intra_4x4_HORIZONTAL_LEFT when Top are available
void T264_predict_4x4_mode_7_c(uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left);
//Mode 8 Intra_4x4_HORIZONTAL_UP when Left are available
void T264_predict_4x4_mode_8_c(uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left);

//	8x8 chroma	(7 functions)
void T264_predict_8x8_mode_0_c (uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left);
void T264_predict_8x8_mode_1_c (uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left);
void T264_predict_8x8_mode_2_c (uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left);
void T264_predict_8x8_mode_20_c(uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left);
void T264_predict_8x8_mode_21_c(uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left);
void T264_predict_8x8_mode_22_c(uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left);
void T264_predict_8x8_mode_3_c (uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left);

#ifndef _PREDICT_H_1
#define _PREDICT_H_1
typedef void (*T264_predict_16x16_mode_t)(uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left);
typedef void (*T264_predict_4x4_mode_t)  (uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left);
typedef void (*T264_predict_8x8_mode_t)  (uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left);
#endif

#endif
