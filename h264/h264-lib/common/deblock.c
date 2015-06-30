/*****************************************************************************
*
*  T264 AVC CODEC
*
*  Copyright(C) 2004-2005 llcc <lcgate1@yahoo.com.cn>
*               2004-2005 visionany <visionany@yahoo.com.cn>
*			Added support for B frame,20041223 Cloud Wu<sywu@sohu.com>
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

#include "stdio.h"
#include "T264.h"
#include "assert.h"

#define IS_INTRA(mode) (mode <= I_16x16)

static const uint8_t deblock_threshold_a[] = 
{
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 4, 4, 5, 6, 7, 8, 9, 10, 12, 13,
    15,17,20,22,25,28,32,36,40,45,50,56,63,71,
    80,90,101,113,127,144,162,182,203,226,255,255
};

static const uint8_t deblock_threshold_b[] = 
{
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4,
    6, 6, 7, 7, 8, 8, 9, 9,10,10,11,11,12,
    12,13,13,14,14,15,15,16,16,17,17,18,18
};

static const uint8_t deblock_threshold_tc0[][3] = 
{
    { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 },
    { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 },
    { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 1 },
    { 0, 0, 1 }, { 0, 0, 1 }, { 0, 0, 1 }, { 0, 1, 1 }, { 0, 1, 1 }, { 1, 1, 1 },
    { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 2 }, { 1, 1, 2 }, { 1, 1, 2 },
    { 1, 1, 2 }, { 1, 2, 3 }, { 1, 2, 3 }, { 2, 2, 3 }, { 2, 2, 4 }, { 2, 3, 4 },
    { 2, 3, 4 }, { 3, 3, 5 }, { 3, 4, 6 }, { 3, 4, 6 }, { 4, 5, 7 }, { 4, 5, 8 },
    { 4, 6, 9 }, { 5, 7,10 }, { 6, 8,11 }, { 6, 8,13 }, { 7,10,14 }, { 8,11,16 },
    { 9,12,18 }, {10,13,20 }, {11,15,23 }, {13,17,25 }
};

static void __inline
get_strength(T264_t* t, uint8_t bS[4], int32_t dir, int32_t edge, T264_mb_context_t* cur, T264_mb_context_t* prev)
{
    int32_t row_stride, col_stride;
    int32_t n;
    
    if (dir == 0)
    {
        row_stride = 4;
        col_stride = 1;
    }
    else
    {
        row_stride = 1;
        col_stride = 4;
    }

    if (IS_INTRA(cur->mb_mode) || IS_INTRA(prev->mb_mode))
    {
        bS[0] = bS[1] = bS[2] = bS[3] = (edge == 0) ? 4 : 3;
    }
    else
    {
        for(n = 0 ; n < 4 ; n ++)
        {
            int32_t pos = n * row_stride + edge * col_stride;
            int32_t pos_n;
            if (cur == prev)
            {	//If is macro blcok border
                pos_n = dir == 0 ? pos - 1 : pos - 4;
            }
            else
            {
                pos_n = dir == 0 ? 4 * n + 3 : 12 + n;
            }
            if (cur->nnz[pos] || prev->nnz[pos_n])
            {
                bS[n] = 2;
            }
            else if (t->slice_type == SLICE_P) 
            {
                int32_t ref, ref_n;
                ref = cur->vec[0][pos].refno;
                ref_n = prev->vec[0][pos_n].refno;
                assert(cur->vec[0][pos].refno != -2);
                assert(prev->vec[0][pos_n].refno != -2);
                bS[n] = ((ref != ref_n) |
                    (ABS(cur->vec[0][pos].x - prev->vec[0][pos_n].x) >= 4) |
                    (ABS(cur->vec[0][pos].y - prev->vec[0][pos_n].y) >= 4));
            }
            else 
            {				
                int32_t ref0,ref1, ref_n0,ref_n1;
                
                ref0 = cur->vec[0][pos].refno;
                ref1 = cur->vec[1][pos].refno;                
                ref_n0 = prev->vec[0][pos_n].refno;  
                ref_n1 = prev->vec[1][pos_n].refno;       

				ref0 = ref0 < 0? -1 : ref0;
				ref1 = ref1 < 0? -1 : ref1 + MAX_REFFRAMES;
                ref_n0 = ref_n0 < 0? -1 : ref_n0;  
                ref_n1 = ref_n1 < 0? -1 : ref_n1 + MAX_REFFRAMES;  

                assert(ref0 > -1 || ref1 > -1);
				if((ref0 == ref_n0 && ref1 == ref_n1) ||
					(ref1 == ref_n0 && ref0 == ref_n1))
				{	
                	bS[n] = 0;	
                	if(ref0 == ref1)
               		{	//if all use the same reference frame
						bS[n] = (
						(ABS(cur->vec[0][pos].x - prev->vec[0][pos_n].x) >= 4) |
						(ABS(cur->vec[0][pos].y - prev->vec[0][pos_n].y) >= 4) |
						(ABS(cur->vec[1][pos].x - prev->vec[1][pos_n].x) >= 4) |
						(ABS(cur->vec[1][pos].y - prev->vec[1][pos_n].y) >= 4))
						&&
						(
						(ABS(cur->vec[0][pos].x - prev->vec[1][pos_n].x) >= 4) |
						(ABS(cur->vec[0][pos].y - prev->vec[1][pos_n].y) >= 4) |
						(ABS(cur->vec[1][pos].x - prev->vec[0][pos_n].x) >= 4) |
						(ABS(cur->vec[1][pos].y - prev->vec[0][pos_n].y) >= 4));								
               		}else
             		{
	             		if(ref0 == ref_n0)
	               		{	//if list0 reference frame are the same
	                		bS[n] = 
	                		(ABS(cur->vec[0][pos].x - prev->vec[0][pos_n].x) >= 4) |
							(ABS(cur->vec[0][pos].y - prev->vec[0][pos_n].y) >= 4) |
							(ABS(cur->vec[1][pos].x - prev->vec[1][pos_n].x) >= 4) |
							(ABS(cur->vec[1][pos].y - prev->vec[1][pos_n].y) >= 4);
	                	}else
	               		{
	               			bS[n] = 
               				(ABS(cur->vec[0][pos].x - prev->vec[1][pos_n].x) >= 4) |
							(ABS(cur->vec[0][pos].y - prev->vec[1][pos_n].y) >= 4) |
							(ABS(cur->vec[1][pos].x - prev->vec[0][pos_n].x) >= 4) |
							(ABS(cur->vec[1][pos].y - prev->vec[0][pos_n].y) >= 4);
	               		}
            		}
                }else
               	{
                	bS[n] =1;
               	}
            }
       }
    }
}

static __inline void
EdgeLoopH(T264_t* t, uint8_t* dst, int32_t stride, uint8_t indexA, int32_t alpha, int32_t beta, uint8_t bS, int32_t is_chroma)
{
    int32_t ap, aq, tmp;
    uint8_t p[4];
    uint8_t q[4];

    q[0] = dst[0];
    p[0] = dst[-1];

    if (ABS(p[0] - q[0]) < alpha)
    {
        p[1] = dst[-2];
        if (ABS(p[1] - p[0]) < beta)
        {
            q[1] = dst[1];
            if (ABS(q[1] - q[0]) < beta)
            {
                if (bS < 4)
                {
                    int32_t tc0, tc, delta;

                    tc0 = deblock_threshold_tc0[indexA][bS - 1];
                    if (is_chroma == 0)
                    {
                        q[2] = dst[2];
                        p[2] = dst[-3];
                        ap = ABS(p[2] - p[0]);
                        aq = ABS(q[2] - q[0]);

                        tc =  tc0;
                        if (ap < beta)
                        {
                            dst[-2] = p[1] + clip3(((p[2] + ((p[0] + q[0] + 1) >> 1) - (p[1] << 1)) >> 1), -tc0, tc0);
                            tc ++;
                        }
                        if (aq < beta)
                        {
                            dst[1] = q[1] + clip3(((q[2] + ((p[0] + q[0] + 1) >> 1) - (q[1] << 1)) >> 1), -tc0, tc0);
                            tc ++;
                        }
                    }
                    else
                    {
                        tc = tc0 + 1;
                    }
                    delta = clip3(((((q[0] - p[0]) << 2) + (p[1] - q[1]) + 4) >> 3), -tc, tc);
                    tmp = p[0] + delta;
                    dst[-1] = CLIP1(tmp);
                    tmp = q[0] - delta;
                    dst[0] = CLIP1(tmp);
                }
                else
                {
                    if (ABS(p[0] - q[0]) < ((alpha >> 2) + 2))
                    {
                        if (is_chroma == 0)
                        {
                            q[2] = dst[2];
                            p[2] = dst[-3];
                            ap = ABS(p[2] - p[0]);
                            aq = ABS(q[2] - q[0]);

                            if (ap < beta)
                            {
                                p[3] = dst[-4];

                                dst[-1] = (p[2] + 2 * p[1] + 2 * p[0] + 2 * q[0] + q[1] + 4) >> 3;
                                dst[-2] = (p[2] + p[1] + p[0] + q[0] + 2) >> 2;
                                dst[-3] = (2 * p[3] + 3 * p[2] + p[1] + p[0] + q[0] + 4) >> 3;
                            }
                            else
                            {
                                dst[-1] = (2 * p[1] + p[0] + q[1] + 2 ) >> 2;
                            }
                            if (aq < beta)
                            {
                                q[3] = dst[3];

                                dst[0] = (p[1] + 2 * p[0] + 2 * q[0] + 2 * q[1] + q[2] + 4) >> 3;
                                dst[1] = (p[0] + q[0] + q[1] + q[2] + 2) >> 2;
                                dst[2] = (2 * q[3] + 3 * q[2] + q[1] + q[0] + p[0] + 4) >> 3;
                            }
                            else
                            {
                                dst[0] = (2 * q[1] + q[0] + p[1] + 2 ) >> 2;
                            }
                        }
                        else
                        {
                            dst[-1] = (2 * p[1] + p[0] + q[1] + 2 ) >> 2;
                            dst[0] = (2 * q[1] + q[0] + p[1] + 2 ) >> 2;
                        }
                    }
                    else
                    {
                        dst[-1] = (2 * p[1] + p[0] + q[1] + 2 ) >> 2;
                        dst[0] = (2 * q[1] + q[0] + p[1] + 2 ) >> 2;
                    }
                }
            }
        }
    }
}

static __inline void
EdgeLoopV(T264_t* t, uint8_t* dst, int32_t stride, uint8_t indexA, int32_t alpha, int32_t beta, uint8_t bS, int32_t is_chroma)
{
    int32_t ap, aq, tmp;
    uint8_t p[4];
    uint8_t q[4];

    q[0] = dst[0];
    p[0] = dst[-1 * stride];

    if (ABS(p[0] - q[0]) < alpha)
    {
        p[1] = dst[-2 * stride];
        if (ABS(p[1] - p[0]) < beta)
        {
            q[1] = dst[1 * stride];
            if (ABS(q[1] - q[0]) < beta)
            {
                if (bS < 4)
                {
                    int32_t tc0, tc, delta;

                    tc0 = deblock_threshold_tc0[indexA][bS - 1];
                    if (is_chroma == 0)
                    {
                        q[2] = dst[2 * stride];
                        p[2] = dst[-3 * stride];
                        ap = ABS(p[2] - p[0]);
                        aq = ABS(q[2] - q[0]);

                        tc =  tc0;
                        if (ap < beta)
                        {
                            dst[-2 * stride] = p[1] + clip3(((p[2] + ((p[0] + q[0] + 1) >> 1) - (p[1] << 1)) >> 1), -tc0, tc0);
                            tc ++;
                        }
                        if (aq < beta)
                        {
                            dst[1 * stride] = q[1] + clip3(((q[2] + ((p[0] + q[0] + 1) >> 1) - (q[1] << 1)) >> 1), -tc0, tc0);
                            tc ++;
                        }
                    }
                    else
                    {
                        tc = tc0 + 1;
                    }
                    delta = clip3(((((q[0] - p[0]) << 2) + (p[1] - q[1]) + 4) >> 3), -tc, tc);
                    tmp = p[0] + delta;
                    dst[-1 * stride] = CLIP1(tmp);
                    tmp = q[0] - delta;
                    dst[0] = CLIP1(tmp);
                }
                else
                {
                    if (ABS(p[0] - q[0]) < ((alpha >> 2) + 2))
                    {
                        if (is_chroma == 0)
                        {
                            q[2] = dst[2 * stride];
                            p[2] = dst[-3 * stride];
                            ap = ABS(p[2] - p[0]);
                            aq = ABS(q[2] - q[0]);

                            if (ap < beta)
                            {
                                p[3] = dst[-4 * stride];

                                dst[-1 * stride] = (p[2] + 2 * p[1] + 2 * p[0] + 2 * q[0] + q[1] + 4) >> 3;
                                dst[-2 * stride] = (p[2] + p[1] + p[0] + q[0] + 2) >> 2;
                                dst[-3 * stride] = (2 * p[3] + 3 * p[2] + p[1] + p[0] + q[0] + 4) >> 3;
                            }
                            else
                            {
                                dst[-1 * stride] = (2 * p[1] + p[0] + q[1] + 2 ) >> 2;
                            }
                            if (aq < beta)
                            {
                                q[3] = dst[3 * stride];

                                dst[0 * stride] = (p[1] + 2 * p[0] + 2 * q[0] + 2 * q[1] + q[2] + 4) >> 3;
                                dst[1 * stride] = (p[0] + q[0] + q[1] + q[2] + 2) >> 2;
                                dst[2 * stride] = (2 * q[3] + 3 * q[2] + q[1] + q[0] + p[0] + 4) >> 3;
                            }
                            else
                            {
                                dst[0] = (2 * q[1] + q[0] + p[1] + 2 ) >> 2;
                            }
                        }
                        else
                        {
                            dst[-1 * stride] = (2 * p[1] + p[0] + q[1] + 2 ) >> 2;
                            dst[0] = (2 * q[1] + q[0] + p[1] + 2 ) >> 2;
                        }
                    }
                    else
                    {
                        dst[-1 * stride] = (2 * p[1] + p[0] + q[1] + 2 ) >> 2;
                        dst[0] = (2 * q[1] + q[0] + p[1] + 2 ) >> 2;
                    }
                }
            }
        }
    }
}

void __inline
deblock_mb_chroma(T264_t*t, int32_t x, int32_t y, uint8_t* dst_uv, uint8_t bSV[4][4], uint8_t bSH[4][4],
                  uint8_t indexAc, int32_t alphac, int32_t betac)
{
    uint8_t* dst;
    int32_t i;

    if (x != 0)
    {
        dst = dst_uv;
        for(i = 0 ; i < 4 ; i ++)
        {
            if (bSV[0][i] > 0)
            {
                EdgeLoopH(t, dst, t->edged_stride_uv, indexAc, alphac, betac, bSV[0][i], 1);
                dst += t->edged_stride_uv;
                EdgeLoopH(t, dst, t->edged_stride_uv, indexAc, alphac, betac, bSV[0][i], 1);
                dst += t->edged_stride_uv;
            }
            else
            {
                dst += 2 * t->edged_stride_uv;
            }
        }
    }
    {
        dst = dst_uv + 4;
        for(i = 0 ; i < 4 ; i ++)
        {
            if (bSV[2][i] > 0)
            {
                EdgeLoopH(t, dst, t->edged_stride_uv, indexAc, alphac, betac, bSV[2][i], 1);
                dst += t->edged_stride_uv;
                EdgeLoopH(t, dst, t->edged_stride_uv, indexAc, alphac, betac, bSV[2][i], 1);
                dst += t->edged_stride_uv;
            }
            else
            {
                dst += 2 * t->edged_stride_uv;
            }
        }
    }

    // u
    if (y != 0)
    {
        dst = dst_uv;
        for(i = 0 ; i < 4 ; i ++)
        {
            if (bSH[0][i] > 0)
            {
                EdgeLoopV(t, dst, t->edged_stride_uv, indexAc, alphac, betac, bSH[0][i], 1);
                dst ++;
                EdgeLoopV(t, dst, t->edged_stride_uv, indexAc, alphac, betac, bSH[0][i], 1);
                dst ++;
            }
            else
            {
                dst += 2;
            }
        }
    }
    {
        dst = dst_uv + 4 * t->edged_stride_uv;
        for(i = 0 ; i < 4 ; i ++)
        {
            if (bSH[2][i] > 0)
            {
                EdgeLoopV(t, dst, t->edged_stride_uv, indexAc, alphac, betac, bSH[2][i], 1);
                dst ++;
                EdgeLoopV(t, dst, t->edged_stride_uv, indexAc, alphac, betac, bSH[2][i], 1);
                dst ++;
            }
            else
            {
                dst += 2;
            }
        }
    }
}
void __inline
deblock_mb_chroma_(T264_t*t, int32_t x, int32_t y, uint8_t* org_dst_u, uint8_t* org_dst_v, uint8_t bSV[4][4], uint8_t bSH[4][4],
                  uint8_t indexAc, int32_t alphac, int32_t betac)
{
    uint8_t* dst_u, *dst_v;
    int32_t i;

    if (x != 0)
    {
        dst_u = org_dst_u;
        dst_v = org_dst_v;
        for(i = 0 ; i < 4 ; i ++)
        {
            if (bSV[0][i] > 0)
            {
                EdgeLoopH(t, dst_u, t->edged_stride_uv, indexAc, alphac, betac, bSV[0][i], 1);
                dst_u += t->edged_stride_uv;
                EdgeLoopH(t, dst_u, t->edged_stride_uv, indexAc, alphac, betac, bSV[0][i], 1);
                dst_u += t->edged_stride_uv;

                EdgeLoopH(t, dst_v, t->edged_stride_uv, indexAc, alphac, betac, bSV[0][i], 1);
                dst_v += t->edged_stride_uv;
                EdgeLoopH(t, dst_v, t->edged_stride_uv, indexAc, alphac, betac, bSV[0][i], 1);
                dst_v += t->edged_stride_uv;
            }
            else
            {
                dst_u += 2 * t->edged_stride_uv;
                dst_v += 2 * t->edged_stride_uv;
            }
        }
    }
    {
        dst_u = org_dst_u + 4;
        dst_v = org_dst_v + 4;
        for(i = 0 ; i < 4 ; i ++)
        {
            if (bSV[2][i] > 0)
            {
                EdgeLoopH(t, dst_u, t->edged_stride_uv, indexAc, alphac, betac, bSV[2][i], 1);
                dst_u += t->edged_stride_uv;
                EdgeLoopH(t, dst_u, t->edged_stride_uv, indexAc, alphac, betac, bSV[2][i], 1);
                dst_u += t->edged_stride_uv;

                EdgeLoopH(t, dst_v, t->edged_stride_uv, indexAc, alphac, betac, bSV[2][i], 1);
                dst_v += t->edged_stride_uv;
                EdgeLoopH(t, dst_v, t->edged_stride_uv, indexAc, alphac, betac, bSV[2][i], 1);
                dst_v += t->edged_stride_uv;
            }
            else
            {
                dst_u += 2 * t->edged_stride_uv;
                dst_v += 2 * t->edged_stride_uv;
            }
        }
    }

    // u
    if (y != 0)
    {
        dst_u = org_dst_u;
        dst_v = org_dst_v;
        for(i = 0 ; i < 4 ; i ++)
        {
            if (bSH[0][i] > 0)
            {
                EdgeLoopV(t, dst_u, t->edged_stride_uv, indexAc, alphac, betac, bSH[0][i], 1);
                dst_u ++;
                EdgeLoopV(t, dst_u, t->edged_stride_uv, indexAc, alphac, betac, bSH[0][i], 1);
                dst_u ++;

                EdgeLoopV(t, dst_v, t->edged_stride_uv, indexAc, alphac, betac, bSH[0][i], 1);
                dst_v ++;
                EdgeLoopV(t, dst_v, t->edged_stride_uv, indexAc, alphac, betac, bSH[0][i], 1);
                dst_v ++;
            }
            else
            {
                dst_u += 2;
                dst_v += 2;
            }
        }
    }
    {
        dst_u = org_dst_u + 4 * t->edged_stride_uv;
        dst_v = org_dst_v + 4 * t->edged_stride_uv;
        for(i = 0 ; i < 4 ; i ++)
        {
            if (bSH[2][i] > 0)
            {
                EdgeLoopV(t, dst_u, t->edged_stride_uv, indexAc, alphac, betac, bSH[2][i], 1);
                dst_u ++;
                EdgeLoopV(t, dst_u, t->edged_stride_uv, indexAc, alphac, betac, bSH[2][i], 1);
                dst_u ++;

                EdgeLoopV(t, dst_v, t->edged_stride_uv, indexAc, alphac, betac, bSH[2][i], 1);
                dst_v ++;
                EdgeLoopV(t, dst_v, t->edged_stride_uv, indexAc, alphac, betac, bSH[2][i], 1);
                dst_v ++;
            }
            else
            {
                dst_u += 2;
                dst_v += 2;
            }
        }
    }
}

void
deblock_mb(T264_t* t, int32_t y, int32_t x, T264_frame_t* f)
{
    int32_t i, j;
    T264_mb_context_t* cur, *prev;
    uint8_t qpav, qpavc;
    uint8_t indexA, indexB, indexAc, indexBc;
    int32_t alpha, beta, alphac, betac;
    uint8_t bSV[4][4];
    uint8_t bSH[4][4];
    uint8_t* dst_y, *dst_u, *dst_v;
    int32_t mb_xy = t->mb_stride * y + x;
    uint8_t* dst;
    
    cur = &f->mb[mb_xy];
    dst_y = f->Y[0] + (y << 4) * t->edged_stride + (x << 4);
    dst_u = f->U + (y << 3) * t->edged_stride_uv + (x << 3);
    dst_v = f->V + (y << 3) * t->edged_stride_uv + (x << 3);
    // xxx current mb_qp_delta == 0 !!
    qpav = (cur->mb_qp_delta + cur->mb_qp_delta + t->qp_y + t->qp_y + 1) >> 1;
    indexA = clip3(qpav + 0, 0, 51);
    indexB = clip3(qpav + 0, 0, 51);
    alpha = deblock_threshold_a[indexA];
    beta  = deblock_threshold_b[indexB];

    qpavc = (cur->mb_qp_delta + cur->mb_qp_delta + t->qp_uv + t->qp_uv + 1) >> 1;
    indexAc = clip3(qpavc + 0, 0, 51);
    indexBc = clip3(qpavc + 0, 0, 51);
    alphac = deblock_threshold_a[indexAc];
    betac  = deblock_threshold_b[indexBc];

    // vertical first
    // mb edge
    if (x != 0)
    {
        prev = &f->mb[mb_xy - 1];
        get_strength(t, bSV[0], 0, 0, cur, prev);
        
        dst = dst_y;
        for(i = 0 ; i < 4 ; i ++)
        {
            if (bSV[0][i] > 0)
            {
                EdgeLoopH(t, dst, t->edged_stride, indexA, alpha, beta, bSV[0][i], 0);
                dst += t->edged_stride;
                EdgeLoopH(t, dst, t->edged_stride, indexA, alpha, beta, bSV[0][i], 0);
                dst += t->edged_stride;
                EdgeLoopH(t, dst, t->edged_stride, indexA, alpha, beta, bSV[0][i], 0);
                dst += t->edged_stride;
                EdgeLoopH(t, dst, t->edged_stride, indexA, alpha, beta, bSV[0][i], 0);
                dst += t->edged_stride;
            }
            else
            {
                dst += 4 * t->edged_stride;
            }
        }
    }
    for(j = 1 ; j < 4 ; j ++)
    {
        prev = cur;
        get_strength(t, bSV[j], 0, j, cur, prev);

        dst = dst_y + 4 * j;
        for(i = 0 ; i < 4 ; i ++)
        {
            if (bSV[j][i] > 0)
            {
                EdgeLoopH(t, dst, t->edged_stride, indexA, alpha, beta, bSV[j][i], 0);
                dst += t->edged_stride;
                EdgeLoopH(t, dst, t->edged_stride, indexA, alpha, beta, bSV[j][i], 0);
                dst += t->edged_stride;
                EdgeLoopH(t, dst, t->edged_stride, indexA, alpha, beta, bSV[j][i], 0);
                dst += t->edged_stride;
                EdgeLoopH(t, dst, t->edged_stride, indexA, alpha, beta, bSV[j][i], 0);
                dst += t->edged_stride;
            }
            else
            {
                dst += 4 * t->edged_stride;
            }
        }
    }

    // mb edge
    if (y != 0)
    {
        prev = &f->mb[mb_xy - t->mb_stride];
        get_strength(t, bSH[0], 1, 0, cur, prev);

        dst = dst_y;
        for(i = 0 ; i < 4 ; i ++)
        {
            if (bSH[0][i] > 0)
            {
                EdgeLoopV(t, dst, t->edged_stride, indexA, alpha, beta, bSH[0][i], 0);
                dst ++;
                EdgeLoopV(t, dst, t->edged_stride, indexA, alpha, beta, bSH[0][i], 0);
                dst ++;
                EdgeLoopV(t, dst, t->edged_stride, indexA, alpha, beta, bSH[0][i], 0);
                dst ++;
                EdgeLoopV(t, dst, t->edged_stride, indexA, alpha, beta, bSH[0][i], 0);
                dst ++;
            }
            else
            {
                dst += 4;
            }
        }
    }
    for(j = 1 ; j < 4 ; j ++)
    {
        prev = cur;
        get_strength(t, bSH[j], 1, j, cur, prev);

        dst = dst_y + 4 * j * t->edged_stride;
        for(i = 0 ; i < 4 ; i ++)
        {
            if (bSH[j][i] > 0)
            {
                EdgeLoopV(t, dst, t->edged_stride, indexA, alpha, beta, bSH[j][i], 0);
                dst ++;
                EdgeLoopV(t, dst, t->edged_stride, indexA, alpha, beta, bSH[j][i], 0);
                dst ++;
                EdgeLoopV(t, dst, t->edged_stride, indexA, alpha, beta, bSH[j][i], 0);
                dst ++;
                EdgeLoopV(t, dst, t->edged_stride, indexA, alpha, beta, bSH[j][i], 0);
                dst ++;
            }
            else
            {
                dst += 4;
            }
        }
    }

    deblock_mb_chroma(t, x, y, dst_u, bSV, bSH, indexAc, alphac, betac);
    deblock_mb_chroma(t, x, y, dst_v, bSV, bSH, indexAc, alphac, betac);
}

void
T264_deblock_frame(T264_t* t, T264_frame_t* f)
{
    int32_t i, j;
    
    for (i = 0 ; i < t->mb_height ; i ++)
    {
        for (j = 0 ; j < t->mb_width ; j ++)
        {			
            deblock_mb(t, i, j, f);
        }
    }
}
