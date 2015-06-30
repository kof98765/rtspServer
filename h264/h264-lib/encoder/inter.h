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

#ifndef _INTER_H_
#define _INTER_H_

#include "T264.h"

typedef struct  
{
    T264_vector_t vec[1 + 2 + 2];
    T264_vector_t vec_median[1 + 2 + 2];
    uint8_t* src[1 + 2 + 2];
    T264_frame_t* ref[1 + 2 + 2];
    uint32_t sad[1 + 2 + 2];
    int32_t  offset[1 + 2 + 2];
    int32_t  list_index;
} search_data_t;

typedef struct  
{
    T264_vector_t vec[4][1 + 2 + 2 + 4];
    T264_vector_t vec_median[4][1 + 2 + 2 + 4];
    uint8_t* src[4][1 + 2 + 2 + 4];
    T264_frame_t* ref[4][1 + 2 + 2 + 4];
    uint32_t sad[4][1 + 2 + 2 + 4];
    int32_t  offset[4][1 + 2 + 2 + 4];
    int32_t  list_index;
} subpart_search_data_t;


/*
 *	1，为当前MB依次调用decision p和i开头的函数，获取最佳预测模式
 *  2，精确搜索
 *  3，运动补偿，
 */
uint32_t T264_mode_decision_interp_y(_RW T264_t* t);

uint32_t T264_mode_decision_inter_16x16p(_RW T264_t* t, search_data_t* s);
uint32_t T264_mode_decision_inter_16x8p(_RW T264_t* t, search_data_t* s);
uint32_t T264_mode_decision_inter_8x16p(_RW T264_t * t, search_data_t* s);

uint32_t T264_mode_decision_inter_8x8p(_RW T264_t * t, int32_t i, subpart_search_data_t* s);
uint32_t T264_mode_decision_inter_8x4p(_RW T264_t * t, int32_t i, subpart_search_data_t* s);
uint32_t T264_mode_decision_inter_4x8p(_RW T264_t * t, int32_t i, subpart_search_data_t* s);
uint32_t T264_mode_decision_inter_4x4p(_RW T264_t * t, int32_t i, subpart_search_data_t* s);

void T264_encode_inter_y(_RW T264_t* t);

void T264_encode_inter_uv(_RW T264_t* t);

int32_t T264_median(int32_t x, int32_t y, int32_t z);
void T264_predict_mv(T264_t* t, int32_t list, int32_t i, int32_t width, T264_vector_t* vec);

void T264_encode_inter_16x16p(_RW T264_t* t, uint8_t* pred);

void T264_predict_mv_skip(T264_t* t, int32_t list, T264_vector_t* vec);

void copy_nvec(T264_vector_t* src, T264_vector_t* dst, int32_t width, int32_t height, int32_t stride);

void get_pmv(T264_t* t, int32_t list, T264_vector_t* vec, int32_t part, int32_t idx, int32_t width, int32_t* n);
void T264_transform_inter_uv(_RW T264_t* t, uint8_t* pred_u, uint8_t* pred_v);

#endif
