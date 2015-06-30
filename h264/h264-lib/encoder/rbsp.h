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

#ifndef _RBSP_H_
#define _RBSP_H_

#include "T264.h"
//typedef struct T264_t T264_t;

void nal_unit_init(_RW T264_nal_t* nal, int32_t nal_ref_idc, int32_t nal_unit_type);
void nal_unit_write(_R T264_t* t, _R T264_nal_t* nal);
void seq_set_init(_R T264_t* t, _RW T264_seq_set_t* seq);
void seq_set_write(_R T264_t* t, _RW T264_seq_set_t* seq);
void pic_set_init(_R T264_t* t, _RW T264_pic_set_t* pic);
void pic_set_write(_R T264_t* t, _RW T264_pic_set_t* pic);
void slice_header_init(_R T264_t* t, _RW T264_slice_t* slice);
void slice_header_write(_R T264_t* t, _RW T264_slice_t* slice);
void rbsp_trailing_bits(_R T264_t* t);
/* our custom flags */
void custom_set_init(_R T264_t* t, _RW T264_custom_set_t* set);
void custom_set_write(_R T264_t* t, _RW T264_custom_set_t* set);

#endif
