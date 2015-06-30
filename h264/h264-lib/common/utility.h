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

#ifndef _UTILITY_H_
#define _UTILITY_H_

void* T264_malloc(int32_t size, int32_t alignment);
void  T264_free(void* p);

int32_t array_non_zero_count(int16_t *v, int32_t i_count);

void expand8to16_c(uint8_t* src, int32_t src_stride, int32_t quarter_width, int32_t quarter_height, int16_t* dst);
void contract16to8_c(int16_t* src, int32_t quarter_width, int32_t quarter_height, uint8_t* dst, int32_t dst_stride);
void contract16to8add_c(int16_t* src, int32_t quarter_width, int32_t quarter_height, uint8_t* org, uint8_t* dst, int32_t dst_stride);
void memcpy_stride_u_c(void* src, int32_t width, int32_t height, int32_t src_stride, void* dst, int32_t dst_stride);
void expand8to16sub_c(uint8_t* pred, int32_t quarter_width, int32_t quarter_height, int16_t* dst, uint8_t* src, int32_t src_stride);
uint32_t T264_satd_i16x16_u_c(uint8_t* src, int32_t src_stride, uint8_t* data, int32_t dst_stride);
uint32_t T264_sad_u_16x16_c(uint8_t* src, int32_t src_stride, uint8_t* data, int32_t dst_stride);
uint32_t T264_sad_u_16x8_c(uint8_t* src, int32_t src_stride, uint8_t* data, int32_t dst_stride);
uint32_t T264_sad_u_8x16_c(uint8_t* src, int32_t src_stride, uint8_t* data, int32_t dst_stride);
uint32_t T264_sad_u_8x8_c(uint8_t* src, int32_t src_stride, uint8_t* data, int32_t dst_stride);
uint32_t T264_sad_u_8x4_c(uint8_t* src, int32_t src_stride, uint8_t* data, int32_t dst_stride);
uint32_t T264_sad_u_4x8_c(uint8_t* src, int32_t src_stride, uint8_t* data, int32_t dst_stride);
uint32_t T264_sad_u_4x4_c(uint8_t* src, int32_t src_stride, uint8_t* data, int32_t dst_stride);

uint32_t T264_satd_u_16x16_c(uint8_t* src, int32_t src_stride, uint8_t* data, int32_t dst_stride);
uint32_t T264_satd_u_16x8_c(uint8_t* src, int32_t src_stride, uint8_t* data, int32_t dst_stride);
uint32_t T264_satd_u_8x16_c(uint8_t* src, int32_t src_stride, uint8_t* data, int32_t dst_stride);
uint32_t T264_satd_u_8x8_c(uint8_t* src, int32_t src_stride, uint8_t* data, int32_t dst_stride);
uint32_t T264_satd_u_8x4_c(uint8_t* src, int32_t src_stride, uint8_t* data, int32_t dst_stride);
uint32_t T264_satd_u_4x8_c(uint8_t* src, int32_t src_stride, uint8_t* data, int32_t dst_stride);
uint32_t T264_satd_u_4x4_c(uint8_t* src, int32_t src_stride, uint8_t* data, int32_t dst_stride);

//////////////////////////////////////////////////////////////////////////
// inline 
static __inline void
scan_zig_4x4(int16_t* zig, int16_t* dct)
{
    zig[0]  = dct[0];
    zig[1]  = dct[1];
    zig[2]  = dct[4];
    zig[3]  = dct[8];
    zig[4]  = dct[5];
    zig[5]  = dct[2];
    zig[6]  = dct[3];
    zig[7]  = dct[6];
    zig[8]  = dct[9];
    zig[9]  = dct[12];
    zig[10] = dct[13];
    zig[11] = dct[10];
    zig[12] = dct[7];
    zig[13] = dct[11];
    zig[14] = dct[14];
    zig[15] = dct[15];
}

// inline 
static __inline void
unscan_zig_4x4(int16_t* zig, int16_t* dct)
{
    dct[0]  = zig[0] ;
    dct[1]  = zig[1] ;
    dct[4]  = zig[2] ;
    dct[8]  = zig[3] ;
    dct[5]  = zig[4] ;
    dct[2]  = zig[5] ;
    dct[3]  = zig[6] ;
    dct[6]  = zig[7] ;
    dct[9]  = zig[8] ;
    dct[12] = zig[9] ;
    dct[13] = zig[10];
    dct[10] = zig[11];
    dct[7]  = zig[12];
    dct[11] = zig[13];
    dct[14] = zig[14];
    dct[15] = zig[15];
}

static __inline void
scan_zig_2x2(int16_t* zig, int16_t* dct)
{
    zig[0] = dct[0];
    zig[1] = dct[1];
    zig[2] = dct[2];
    zig[3] = dct[3];
}

static __inline void
unscan_zig_2x2(int16_t* zig, int16_t* dct)
{
    dct[0] = zig[0];
    dct[1] = zig[1];
    dct[2] = zig[2];
    dct[3] = zig[3];
}
//////////////////////////////////////////////////////////////////////////
#endif
