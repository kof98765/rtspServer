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

#ifndef _INTRA_H_
#define _INTRA_H_

#include "T264.h"

uint32_t T264_mode_decision_intra_y(_RW T264_t* t);
uint32_t T264_mode_decision_intra_16x16(_RW T264_t* t);
uint32_t T264_mode_decision_intra_4x4(_RW T264_t * t);

void T264_intra_16x16_available(T264_t* t, int32_t preds[], int32_t* modes, uint8_t* top, uint8_t* left);
void T264_intra_4x4_available(T264_t* t, int32_t i, int32_t preds[], int32_t* modes, uint8_t* dst, uint8_t* left, uint8_t* top);

void T264_encode_intra_y(_RW T264_t* t);
void T264_encode_intra_16x16(_RW T264_t* t);
void T264_encode_intra_4x4(_RW T264_t* t, uint8_t* pred, int32_t i);

uint32_t T264_mode_decision_intra_uv(_RW T264_t* t);
void T264_intra_8x8_available(T264_t* t, int32_t preds[], int32_t* modes, uint8_t* top_u, uint8_t* left_u, uint8_t* top_v, uint8_t* left_v);
void T264_encode_intra_uv(_RW T264_t* t);

#endif
