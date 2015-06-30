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

#ifndef _DECODE_H_
#define _DECODE_H_

void T264dec_mb_decode_intra_y(T264_t* t);
void T264dec_mb_decode_intra_uv(T264_t* t);

void T264dec_mb_decode_interp_y(T264_t* t);
void T264dec_mb_decode_interp_uv(T264_t* t);

void T264dec_mb_decode_interb_y(T264_t* t);
void T264dec_mb_decode_interb_uv(T264_t* t);
void T264_mb4x4_interb_mc(T264_t* t,T264_vector_t vec[2][16],uint8_t* ref);
void T264_mb4x4_interb_uv_mc(T264_t* t,T264_vector_t vecPredicted[2][16],uint8_t* pred_u,uint8_t* pred_v);
#endif
