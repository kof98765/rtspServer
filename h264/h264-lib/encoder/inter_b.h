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

#ifndef _INTERB_H_
#define _INTERB_H_

#include "T264.h"

uint32_t T264_mode_decision_interb_y(_RW T264_t* t);

uint32_t T264_mode_decision_inter_16x16b(_RW T264_t* t, T264_vector_t vec_best[2][2], uint8_t* pred, uint8_t* part);
uint32_t T264_mode_decision_inter_16x8b(_RW T264_t* t, T264_vector_t vec_best[2][2], uint8_t* pred, uint8_t* part);
uint32_t T264_mode_decision_inter_8x16b(_RW T264_t* t, T264_vector_t vec_best[2][2], uint8_t* pred, uint8_t* part);

uint32_t T264_mode_decision_inter_8x8b(_RW T264_t * t, int32_t i, T264_vector_t vec_best[2], uint8_t* pred, uint8_t* part);
uint32_t T264_mode_decision_inter_8x4b(_RW T264_t * t, int32_t i, subpart_search_data_t* s);
uint32_t T264_mode_decision_inter_4x8b(_RW T264_t * t, int32_t i, subpart_search_data_t* s);
uint32_t T264_mode_decision_inter_4x4b(_RW T264_t * t, int32_t i, subpart_search_data_t* s);
uint32_t T264_mode_decision_inter_direct_16x16b(_RW T264_t* t, T264_vector_t vec_best[2][4 * 4], uint8_t* pred, uint8_t* part);

void T264_encode_interb_uv(_RW T264_t* t);

void T264_inter_b16x16_mode_available(T264_t* t, uint8_t preds[], int32_t* modes);
void T264_inter_b8x8_mode_available(T264_t* t, uint8_t preds[], int32_t* modes);

void T264_get_direct_mv (T264_t* t, T264_vector_t vec_direct[2][16]);
#endif
