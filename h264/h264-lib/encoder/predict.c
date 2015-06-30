/*****************************************************************************
 *
 *  T264 AVC CODEC
 *
 *  Copyright(C) 2004-2005 llcc <lcgate1@yahoo.com.cn>
 *               2004-2005 visionany <visionany@yahoo.com.cn>
 *
 *
 *  Cloud Wu 2004-9-30 add intra prediction mode 3-8
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

#include "portab.h"
#include "predict.h"
#ifndef CHIP_DM642
#include <memory.h>
#endif

//
// Vertical
//
void
T264_predict_16x16_mode_0_c(uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left)
{
    int32_t i, j;

    for(i = 0 ; i < 16 ; i ++)
    {
        for(j = 0 ; j < 16 ; j ++)
        {
            dst[j] = top[j];
        }
        dst += dst_stride;
    }
}

//
// Horizontal
//
void
T264_predict_16x16_mode_1_c(uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left)
{
    int32_t i, j;

    for(i = 0 ; i < 16 ; i ++)
    {
        for(j = 0 ; j < 16 ; j ++)
        {
            dst[j] = left[i];
        }
        dst += dst_stride;
    }
}

//
// top & left all available
//
void
T264_predict_16x16_mode_2_c(uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left)
{
    int32_t H, V, m;
    int32_t i, j;

    H = V = 0;

    for(i = 0 ; i < 16 ; i ++)
    {
        H += top[i];
        V += left[i];
    }

    m = (H + V + 16) >> 5;

    for(i = 0 ; i < 16 ; i ++)
    {
        for(j = 0 ; j < 16 ; j ++)
        {
            dst[j] = m;
        }
        dst += dst_stride;
    }
}

//
// top available
//
void
T264_predict_16x16_mode_20_c(uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left)
{
    int32_t H, m;
    int32_t i, j;

    H = 0;

    for(i = 0 ; i < 16 ; i ++)
    {
        H += top[i];
    }

    m = (H + 8) >> 4;

    for(i = 0 ; i < 16 ; i ++)
    {
        for(j = 0 ; j < 16 ; j ++)
        {
            dst[j] = m;
        }
        dst += dst_stride;
    }
}

//
// left available
//
void
T264_predict_16x16_mode_21_c(uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left)
{
    int32_t V, m;
    int32_t i, j;

    V = 0;

    for(i = 0 ; i < 16 ; i ++)
    {
        V += left[i];
    }

    m = (V + 8) >> 4;

    for(i = 0 ; i < 16 ; i ++)
    {
        for(j = 0 ; j < 16 ; j ++)
        {
            dst[j] = m;
        }
        dst += dst_stride;
    }
}

//
// none available
//
void
T264_predict_16x16_mode_22_c(uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left)
{
    int32_t i, j;

    for(i = 0 ; i < 16 ; i ++)
    {
        for(j = 0 ; j < 16 ; j ++)
        {
            dst[j] = 128;
        }
        dst += dst_stride;
    }
}

//
// Plane
//
void
T264_predict_16x16_mode_3_c(uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left)
{
    int32_t a, b, c, H, V;
    int32_t i, j;

    H = V = 0;

    for(i = 0 ; i < 8 ; i ++)
    {
        H += (i + 1) * (top[8 + i] - top[6 - i]);
        V += (i + 1) * (left[8 + i] - left[6 - i]);
    }

    a = (left[15] + top[15]) << 4;
    b = (5 * H + 32) >> 6;
    c = (5 * V + 32) >> 6;

    for(i = 0 ; i < 16 ; i ++)
    {
        for(j = 0 ; j < 16 ; j ++)
        {
            int32_t tmp = (a + b * (j - 7) + c * (i - 7) + 16) >> 5;
            tmp = CLIP1(tmp);
            dst[j] = tmp;
        }
        dst += dst_stride;
    }
}

//Mode 0: Vertical
void T264_predict_4x4_mode_0_c(uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left)
{
    int32_t i, j;
    
    for(i = 0; i < 4; i++)
    {
        for(j = 0; j < 4; j++)
        {
            dst[j] = top[j];
        }
        dst += dst_stride;
    }
}

//Mode 1: Horizontal
void T264_predict_4x4_mode_1_c(uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left)
{
    int32_t i, j;

    for(i = 0; i < 4; i++)
    {
        for(j = 0; j < 4; j++)
        {
            dst[j] = left[i];
        }
        dst += dst_stride;
    }
}

//Mode 2: DC
void T264_predict_4x4_mode_2_c(uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left)
{
    int32_t i, j;
    int32_t H, V, m;

    H = V = 0;
    for(i = 0; i < 4; i++)
    {
        H += top[i];
        V += left[i];
    }
    m = (H + V + 4) >> 3;

    for(i = 0; i < 4; i++)
    {
        for(j = 0; j < 4; j++)
        {
            dst[j] = m;
        }
        dst += dst_stride;
    }	
}

//Mode 20 DC top
void T264_predict_4x4_mode_20_c(uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left)
{
    int32_t i, j;
    int32_t V, m;

    V = 0;
    for(i = 0; i < 4; i++)
    {
        V += top[i];
    }
    m = (V + 2) >> 2;

    for(i = 0; i < 4; i++)
    {
        for(j = 0; j < 4; j++)
        {
            dst[j] =  m;
        }
        dst += dst_stride;
    }	
}

//Mode 21 DC left
void T264_predict_4x4_mode_21_c(uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left)
{
    int32_t i, j;
    int32_t H, m;

    H = 0;
    for(i = 0; i < 4; i++)
    {
        H += left[i];
    }
    m = (H + 2) >> 2;

    for(i = 0; i < 4; i++)
    {
        for(j = 0; j < 4; j++)
        {
            dst[j] = m;
        }
        dst += dst_stride;
    }
}

//Mode 22 DC 128
void T264_predict_4x4_mode_22_c(uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left)
{
    memset(dst,128,16);
}

//Mode 3 Intra_4x4_DIAGONAL_DOWNLEFT when Top are available
void T264_predict_4x4_mode_3_c(uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left)
{
	uint8_t	*cur_dst = dst;

	*cur_dst = (top[0] + top[2] + (top[1] << 1) + 2) >> 2;
    *(cur_dst + 1) = *(cur_dst + 4) = (top[1] + top[3] + (top[2] << 1) + 2) >> 2;
    *(cur_dst + 2) = *(cur_dst + 5) = *(cur_dst + 8) = (top[2] + top[4] + (top[3] << 1) + 2) >> 2;
    *(cur_dst + 3) =  *(cur_dst + 6) =  *(cur_dst + 9) =  *(cur_dst + 12) = (top[3] + top[5] + (top[4] << 1) + 2) >> 2;
    *(cur_dst + 7) =  *(cur_dst + 10) = *(cur_dst + 13) = (top[4] + top[6] + (top[5] << 1) + 2) >> 2;
    *(cur_dst + 11) =  *(cur_dst + 14) = (top[5] + top[7] + (top[6] << 1) + 2) >> 2;
    *(cur_dst + 15) = (top[6] + (top[7] << 1) + top[7] + 2) >> 2;
}

//Mode 4 Intra_4x4_DIAGONAL_DOWNRIGHT when Top and left are available
void T264_predict_4x4_mode_4_c(uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left)
{
	uint8_t	*cur_dst = dst;

    *(cur_dst + 12) = (left[3] + (left[2] << 1) + left[1] + 2) >> 2; 
    *(cur_dst + 8) = *(cur_dst + 13) = (left[2] + (left[1] << 1) + left[0] + 2) >> 2; 
    *(cur_dst + 4) = *(cur_dst + 9) = *(cur_dst + 14) = (left[1] + (left[0] << 1) + *(left - 1) + 2) >> 2; 
    *(cur_dst) = *(cur_dst + 5) = *(cur_dst + 10) = *(cur_dst + 15) = (left[0] + (*(left - 1) << 1) + top[0] + 2) >> 2; 
    *(cur_dst + 1) = *(cur_dst + 6) = *(cur_dst + 11) = (*(top - 1) + (top[0] << 1) + top[1] + 2) >> 2;
    *(cur_dst + 2) = *(cur_dst + 7) = (top[0] + (top[1] << 1) + top[2] + 2) >> 2;
    *(cur_dst + 3) = (top[1] + (top[2] << 1) + top[3] + 2) >> 2;	
}

//Mode 5 Intra_4x4_VERTICAL_RIGHT when Top and left are available
void T264_predict_4x4_mode_5_c(uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left)
{
	uint8_t	*cur_dst = dst;

	*cur_dst = *(cur_dst + 9) = (*(top - 1) + top[0] + 1) >> 1;
    *(cur_dst + 1) = *(cur_dst + 10) = (top[0] + top[1] + 1) >> 1;
    *(cur_dst + 2) = *(cur_dst + 11) = (top[1] + top[2] + 1) >> 1;
    *(cur_dst + 3) = (top[2] + top[3] + 1) >> 1;
    *(cur_dst + 4) = *(cur_dst + 13) = (left[0] + (*(top - 1) << 1) + top[0] + 2) >> 2;
    *(cur_dst + 5) = *(cur_dst + 14) = (*(top - 1) + (top[0] << 1) + top[1] + 2) >> 2;
    *(cur_dst + 6) = *(cur_dst + 15) = (top[0] + (top[1] << 1) + top[2] + 2) >> 2;
    *(cur_dst + 7) = (top[1] + (top[2] << 1) + top[3] + 2) >> 2;
    *(cur_dst + 8) = (*(top - 1) + (left[0] << 1) + left[1] + 2) >> 2;
    *(cur_dst + 12) = (left[0] + (left[1] << 1) + left[2] + 2) >> 2;
	
}

//Mode 6 Intra_4x4_HORIZONTAL_DOWN when Top and left are available
void T264_predict_4x4_mode_6_c(uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left)
{
	uint8_t	*cur_dst = dst;
	
	*cur_dst = *(cur_dst + 6) = (*(top - 1) + left[0] + 1) >> 1;
    *(cur_dst + 1) = *(cur_dst + 7) = (left[0] + (*(left - 1) << 1) + top[0] + 2) >> 2;
    *(cur_dst + 2) = (*(top - 1) + (top[0] << 1) + top[1] + 2) >> 2;
    *(cur_dst + 3) = (top[0] + (top[1] << 1) + top[2] + 2) >> 2;
    *(cur_dst + 4) = *(cur_dst + 10) = (left[0] + left[1] + 1) >> 1;
    *(cur_dst + 5) = *(cur_dst + 11) = (*(left - 1) + (left[0] << 1) + left[1] + 2) >> 2;
    *(cur_dst + 8) = *(cur_dst + 14) = (left[1] + left[2] + 1) >> 1;
    *(cur_dst + 9) = *(cur_dst + 15) = (left[0] + (left[1] << 1) + left[2] + 2) >> 2;
    *(cur_dst + 12) = (left[2] + left[3] + 1) >> 1;
    *(cur_dst + 13) = (left[1] + (left[2] << 1) + left[3] + 2)  >> 2;
}

//Mode 7 Intra_4x4_VERTICAL_LEFT when Top are available
void T264_predict_4x4_mode_7_c(uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left)
{
	uint8_t	*cur_dst = dst;

	*cur_dst = (top[0] + top[1] + 1) >> 1;
    *(cur_dst + 1) = *(cur_dst + 8) = (top[1] + top[2] + 1) >> 1;
    *(cur_dst + 2) = *(cur_dst + 9) = (top[2] + top[3] + 1) >> 1;;
    *(cur_dst + 3) = *(cur_dst + 10) = (top[3] + top[4] + 1) >> 1;
    *(cur_dst + 11) = (top[4] + top[5] + 1) >> 1;
    *(cur_dst + 4) = (top[0] + (top[1] << 1) + top[2] + 2) >> 2;
    *(cur_dst + 5) = *(cur_dst + 12) = (top[1] + (top[2] << 1) + top[3] + 2) >> 2;
    *(cur_dst + 6) = *(cur_dst + 13) = (top[2] + (top[3] << 1) + top[4] + 2) >> 2;
    *(cur_dst + 7) = *(cur_dst + 14) = (top[3] + (top[4] << 1) + top[5] + 2) >> 2;
    *(cur_dst + 15) = (top[4] + (top[5] << 1) + top[6] + 2) >> 2;
}


//Mode 8 Intra_4x4_HORIZONTAL_UP when Left are available
void T264_predict_4x4_mode_8_c(uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left)
{
	uint8_t	*cur_dst = dst;

	*cur_dst = (left[0] + left[1] + 1) >> 1;
    *(cur_dst + 1) = (left[0] + 2*left[1] + left[2] + 2) >> 2;
    *(cur_dst + 2) = *(cur_dst + 4) = (left[1] + left[2] + 1) >> 1;
    *(cur_dst + 3) = *(cur_dst + 5) = (left[1] + 2*left[2] + left[3] + 2) >> 2;
    *(cur_dst + 6) = *(cur_dst + 8) = (left[2] + left[3] + 1) >> 1;
    *(cur_dst + 7) = *(cur_dst + 9) = (left[2] +  (left[3] << 1) + left[3] + 2) >> 2;
  	*(cur_dst + 12) = *(cur_dst + 10) = *(cur_dst + 11) = *(cur_dst + 13) 
				= *(cur_dst + 14) = *(cur_dst + 15) = left[3];
}

//
// Vertical
//
void
T264_predict_8x8_mode_0_c(uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left)
{
    int32_t i, j;

    for(i = 0 ; i < 8 ; i ++)
    {
        for(j = 0 ; j < 8 ; j ++)
        {
            dst[j] = top[j];
        }
        dst += dst_stride;
    }
}

//
// Horizontal
//
void
T264_predict_8x8_mode_1_c(uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left)
{
    int32_t i, j;

    for(i = 0 ; i < 8 ; i ++)
    {
        for(j = 0 ; j < 8 ; j ++)
        {
            dst[j] = left[i];
        }
        dst += dst_stride;
    }
}

//
// top & left all available
//
void
T264_predict_8x8_mode_2_c(uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left)
{
    int32_t H1, V1;
    int32_t H2, V2;
    int32_t m[4];
    int32_t i, j;

    H1 = V1 = H2 = V2 = 0;

    for(i = 0 ; i < 4 ; i ++)
    {
        H1   += top[i];
        V1   += left[i];
        H2   += top[i + 4];
        V2   += left[i + 4];
    }

    m[0] = (H1 + V1 + 4) >> 3;
    m[1] = (H2 + 2) >> 2;
    m[2] = (V2 + 2) >> 2;
    m[3] = (H2 + V2 + 4) >> 3;

    for(i = 0 ; i < 4 ; i ++)
    {
        for(j = 0 ; j < 4 ; j ++)
        {
            dst[j] = m[0];
        }
        for(      ; j < 8 ; j ++)
        {
            dst[j] = m[1];
        }
        dst += dst_stride;
    }
    for(     ; i < 8 ; i ++)
    {
        for(j = 0 ; j < 4 ; j ++)
        {
            dst[j] = m[2];
        }
        for(      ; j < 8 ; j ++)
        {
            dst[j] = m[3];
        }
        dst += dst_stride;
    }
}

//
// top available
//
void
T264_predict_8x8_mode_20_c(uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left)
{
    int32_t H1, m1;
    int32_t H2, m2;
    int32_t i, j;

    H1 = H2 = 0;

    for(i = 0 ; i < 4 ; i ++)
    {
        H1 += top[i];
        H2 += top[i + 4];
    }

    m1 = (H1 + 2) >> 2;
    m2 = (H2 + 2) >> 2;

    for(i = 0 ; i < 8 ; i ++)
    {
        for(j = 0 ; j < 4 ; j ++)
        {
            dst[j] = m1;
        }
        for(      ; j < 8 ; j ++)
        {
            dst[j] = m2;
        }
        dst += dst_stride;
    }
}

//
// left available
//
void
T264_predict_8x8_mode_21_c(uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left)
{
    int32_t V1, m1;
    int32_t V2, m2;
    int32_t i, j;

    V1 = V2 = 0;

    for(i = 0 ; i < 4 ; i ++)
    {
        V1 += left[i];
        V2 += left[i + 4];
    }

    m1 = (V1 + 2) >> 2;
    m2 = (V2 + 2) >> 2;

    for(i = 0 ; i < 4 ; i ++)
    {
        for(j = 0 ; j < 8 ; j ++)
        {
            dst[j] = m1;
        }
        dst += dst_stride;
    }
    for(      ; i < 8 ; i ++)
    {
        for(j = 0 ; j < 8 ; j ++)
        {
            dst[j] = m2;
        }
        dst += dst_stride;
    }
}

//
// none available
//
void
T264_predict_8x8_mode_22_c(uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left)
{
    int32_t i, j;

    for(i = 0 ; i < 8 ; i ++)
    {
        for(j = 0 ; j < 8 ; j ++)
        {
            dst[j] = 128;
        }
        dst += dst_stride;
    }
}

//
// Plane
//
void
T264_predict_8x8_mode_3_c(uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left)
{
    int32_t a, b, c, H, V;
    int32_t i, j;

    H = V = 0;

    for(i = 0 ; i < 4 ; i ++)
    {
        H += (i + 1) * (top[4 + i] - top[2 - i]);
        V += (i + 1) * (left[4 + i] - left[2 - i]);
    }

    a = (left[7] + top[7]) << 4;
    b = (17 * H + 16) >> 5;
    c = (17 * V + 16) >> 5;

    for(i = 0 ; i < 8 ; i ++)
    {
        for(j = 0 ; j < 8 ; j ++)
        {
            int32_t tmp = (a + b * (j - 3) + c * (i - 3) + 16) >> 5;
            dst[j] = CLIP1(tmp);
        }
        dst += dst_stride;
    }
}
