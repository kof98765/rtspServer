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

#include "stdio.h"
#include "T264.h"
#include "interpolate.h"
#include "bitstream.h"

//  1/4 pixel search
uint32_t
T264_quarter_pixel_search(T264_t* t, int32_t list_index, uint8_t* src, T264_frame_t* refframe, int32_t offset, T264_vector_t* vec, T264_vector_t* vec_median, uint32_t sad_org, int32_t w, int32_t h, uint8_t* residual, int32_t mb_part)
{
    DECLARE_ALIGNED_MATRIX(data1, 16, 16, uint8_t, CACHE_SIZE);
    DECLARE_ALIGNED_MATRIX(data2, 16, 16, uint8_t, CACHE_SIZE);

    uint32_t sad = sad_org;
    uint8_t* ref;
    int16_t x, y;
    int32_t ref_cost = REFCOST(vec[0].refno);

    x = vec[0].x &= ~3;
    y = vec[0].y &= ~3;
    ref = refframe->Y[0] + offset + (y >> 2) * t->edged_stride + (x >> 2);
    
    if (t->flags & USE_HALFPEL)
    {
        uint8_t* refcur;
       // right half pel
        refcur = refframe->Y[1] + offset + (y >> 2) * t->edged_stride + (x >> 2);
        sad = t->cmp[mb_part](src, t->stride, refcur, t->edged_stride) +
            t->mb.lambda * (eg_size_se(t->bs, (x + 2) - vec_median[0].x) + 
            eg_size_se(t->bs, y - vec_median[0].y)) + ref_cost;
        if (sad < sad_org)
        {
            sad_org = sad;
            vec[0].x = x + 2;
            vec[0].y = y;
            ref = refcur;
        }
        // left half pel
        refcur --;
        sad = t->cmp[mb_part](src, t->stride, refcur, t->edged_stride) +
            t->mb.lambda * (eg_size_se(t->bs, (x - 2) - vec_median[0].x) + 
            eg_size_se(t->bs, y - vec_median[0].y)) + ref_cost;
        if (sad < sad_org)
        {
            sad_org = sad;
            vec[0].x = x - 2;
            vec[0].y = y;
            ref = refcur;
        }
        // bottom half pel
        refcur = refframe->Y[2] + offset + (y >> 2) * t->edged_stride + (x >> 2);
        sad = t->cmp[mb_part](src, t->stride, refcur, t->edged_stride) +
            t->mb.lambda * (eg_size_se(t->bs, x - vec_median[0].x) + 
            eg_size_se(t->bs, y + 2 - vec_median[0].y)) + ref_cost;
        if (sad < sad_org)
        {
            sad_org = sad;
            vec[0].x = x;
            vec[0].y = y + 2;
            ref = refcur;
        }
        // top half pel
        refcur -= t->edged_stride;
        sad = t->cmp[mb_part](src, t->stride, refcur, t->edged_stride) +
            t->mb.lambda * (eg_size_se(t->bs, x - vec_median[0].x) + 
            eg_size_se(t->bs, y - 2 - vec_median[0].y)) + ref_cost;
        if (sad < sad_org)
        {
            sad_org = sad;
            vec[0].x = x;
            vec[0].y = y - 2;
            ref = refcur;
        }
        // bottom-right half pel
        refcur = refframe->Y[3] + offset + (y >> 2) * t->edged_stride + (x >> 2);
        sad = t->cmp[mb_part](src, t->stride, refcur, t->edged_stride) +
            t->mb.lambda * (eg_size_se(t->bs, x + 2 - vec_median[0].x) + 
            eg_size_se(t->bs, y + 2 - vec_median[0].y)) + ref_cost;
        if (sad < sad_org)
        {
            sad_org = sad;
            vec[0].x = x + 2;
            vec[0].y = y + 2;
            ref = refcur;
        }
        // bottom-left half pel
        refcur --;
        sad = t->cmp[mb_part](src, t->stride, refcur, t->edged_stride) +
            t->mb.lambda * (eg_size_se(t->bs, x - 2 - vec_median[0].x) + 
            eg_size_se(t->bs, y + 2 - vec_median[0].y)) + ref_cost;
        if (sad < sad_org)
        {
            sad_org = sad;
            vec[0].x = x - 2;
            vec[0].y = y + 2;
            ref = refcur;
        }
        // top-left half pel
        refcur -= t->edged_stride;
        sad = t->cmp[mb_part](src, t->stride, refcur, t->edged_stride) +
            t->mb.lambda * (eg_size_se(t->bs, x - 2 - vec_median[0].x) + 
            eg_size_se(t->bs, y - 2 - vec_median[0].y)) + ref_cost;
        if (sad < sad_org)
        {
            sad_org = sad;
            vec[0].x = x - 2;
            vec[0].y = y - 2;
            ref = refcur;
        }
        // top-right half pel
        refcur ++;
        sad = t->cmp[mb_part](src, t->stride, refcur, t->edged_stride) +
            t->mb.lambda * (eg_size_se(t->bs, x + 2 - vec_median[0].x) + 
            eg_size_se(t->bs, y - 2 - vec_median[0].y)) + ref_cost;
        if (sad < sad_org)
        {
            sad_org = sad;
            vec[0].x = x + 2;
            vec[0].y = y - 2;
            ref = refcur;
        }

        // quarter pel search
        if (t->flags & USE_QUARTPEL)
        {
            int16_t n;
            int32_t i;
            uint8_t* p_min = data1;
            uint8_t* p_buffer = data2;
            uint32_t sad_half = sad_org;

            static const int8_t index[2 * 2][8][8] = 
            {
                {
                    {0, 1, 0, 0, 0, 0, 1, 0}, {0, 1, 0, 0,-1, 0,-1, 0},
                    {0, 2, 0, 0, 0, 0, 0, 1}, {0, 2, 0, 0, 0,-1, 0,-1},
                    {2, 1, 0,-1,-1, 0,-1,-1}, {2, 1, 0,-1, 0, 0, 1,-1},
                    {2, 1, 0, 0,-1, 0,-1, 1}, {2, 1, 0, 0, 0, 0, 1, 1}
                },
                {
                    {0, 1, 1, 0, 0, 0, 1, 0}, {0, 1, 0, 0, 0, 0,-1, 0},
                    {1, 3, 0, 0, 0, 0, 0, 1}, {1, 3, 0, 0, 0,-1, 0,-1},
                    {2, 1, 0,-1, 0, 0,-1,-1}, {2, 1, 1,-1, 0, 0, 1,-1},
                    {2, 1, 0, 0, 0, 0,-1, 1}, {2, 1, 1, 0, 0, 0, 1, 1}
                },
                {
                    {2, 3, 0, 0, 0, 0, 1, 0}, {2, 3, 0, 0,-1, 0,-1, 0},
                    {2, 0, 0, 0, 0, 1, 0, 1}, {2, 0, 0, 0, 0, 0, 0,-1},
                    {2, 1, 0, 0,-1, 0,-1,-1}, {2, 1, 0, 0, 0, 0, 1,-1},
                    {2, 1, 0, 0,-1, 1,-1, 1}, {2, 1, 0, 0, 0, 1, 1, 1}
                },
                {
                    {3, 2, 0, 0, 1, 0, 1, 0}, {3, 2, 0, 0, 0, 0,-1, 0},
                    {3, 1, 0, 0, 0, 1, 0, 1}, {3, 1, 0, 0, 0, 0, 0,-1},
                    {1, 2, 0, 0, 0, 0,-1,-1}, {1, 2, 0, 0, 1, 0, 1,-1},
                    {1, 2, 0, 1, 0, 0,-1, 1}, {1, 2, 0, 1, 1, 0, 1, 1}
                }
            };

            x = ((uint16_t)vec[0].x) & (uint16_t)~1;
            y = ((uint16_t)vec[0].y) & (uint16_t)~1;

            n = ((y & 2)) | ((x & 2) >> 1);
            for(i = 0 ; i < t->subpel_pts ; i ++)
            {
                t->pia[mb_part](
                    t->ref[list_index][vec[0].refno]->Y[index[n][i][0]] + offset + ((y >> 2) + index[n][i][3]) * t->edged_stride + (x >> 2) + index[n][i][2],
                    t->ref[list_index][vec[0].refno]->Y[index[n][i][1]] + offset + ((y >> 2) + index[n][i][5]) * t->edged_stride + (x >> 2) + index[n][i][4],
                    t->edged_stride, t->edged_stride, p_buffer, 16);
                sad = t->cmp[mb_part](src, t->stride, p_buffer, 16) +
                    t->mb.lambda * (eg_size_se(t->bs, x + index[n][i][6] - vec_median[0].x) + 
                    eg_size_se(t->bs, y +index[n][i][7] - vec_median[0].y)) + ref_cost;
                if (sad < sad_org)
                {
                    sad_org = sad;
                    vec[0].x = x + index[n][i][6];
                    vec[0].y = y + index[n][i][7];
                    SWAP(uint8_t, p_min, p_buffer);
                    //t->memcpy_stride_u(data, w, h, 16, residual, 16);
                }
            }
            if (sad_org < sad_half)
            {
                t->memcpy_stride_u(p_min, w, h, 16, residual, 16);
            }
            else
            {
                t->memcpy_stride_u(ref, w, h, t->edged_stride, residual, 16);
            }
        }
        else
        {
            t->memcpy_stride_u(ref, w, h, t->edged_stride, residual, 16);
        }
        sad = sad_org;
    }
    else
    {
        // x & y always integer pel
        t->memcpy_stride_u(ref, w, h, t->edged_stride, residual, 16);
    }
    return sad;
}

void
T264_pia_u_c(uint8_t* p1, uint8_t* p2, int32_t p1_stride, int32_t p2_stride, uint8_t* dst, int32_t dst_stride,
             int32_t w,int32_t h)
{
    int32_t i, j;

    for(i = 0 ; i < h ; i ++)
    {
        for(j = 0 ; j < w ; j ++)
        {
            dst[j] = (p1[j] + p2[j] + 1) >> 1;
        }
        p1 += p1_stride;
        p2 += p2_stride;
        dst+= dst_stride;
    }
}

#define PIAFUNC(w, h, base) void T264_##base##_u_##w##x##h##_c(uint8_t* p1, uint8_t* p2, int32_t p1_stride, int32_t p2_stride, uint8_t* dst, int32_t dst_stride) { T264_##base##_u_c(p1,p2,p1_stride, p2_stride,dst,dst_stride,w,h); }


PIAFUNC(16, 16, pia)
PIAFUNC(16, 8,  pia)
PIAFUNC(8,  16, pia)
PIAFUNC(8,  8,  pia)
PIAFUNC(8,  4,  pia)
PIAFUNC(4,  8,  pia)
PIAFUNC(4,  4,  pia)
PIAFUNC(2,  2,  pia)

void
T264_eighth_pixel_mc_u_c(uint8_t* src, int32_t src_stride, uint8_t* dst, int16_t mvx, int16_t mvy, int32_t width, int32_t height)
{
    int32_t x, y;
    int32_t i, j;

    x = mvx & 0x7;
    y = mvy & 0x7;

    for (i = 0 ; i < height ; i ++)
    {
        for(j = 0 ; j < width ; j ++)
        {
            dst[j] = ((8 - x) * (8 - y) * src[j]  + x * (8 - y) * src[j + 1] + 
                (8 - x) * y * src[j + src_stride] + x * y * src[j + src_stride+ 1] + 32) >> 6;
        }
        src += src_stride;
        dst += 8;
    }
}

static __inline int32_t
tapfilter_h(uint8_t* p)
{
    return p[-2] - 5 * p[-1] + 20 * p[0] + 20 * p[1] - 5 * p[2] + p[3];
}

void
interpolate_halfpel_h_c(uint8_t* src, int32_t src_stride, uint8_t* dst, int32_t dst_stride, int32_t width, int32_t height)
{
    int32_t i, j;
    int32_t tmp;

    for (i = 0 ; i < height ; i ++)
    {
        for (j = 0 ; j < width ; j ++)
        {
            tmp = (tapfilter_h(src + j) + 16) >> 5;
            dst[j] = CLIP1(tmp);
        }
        src += src_stride;
        dst += dst_stride;
    }
}

static __inline int32_t
tapfilter_v(uint8_t* p, int32_t stride)
{
    return p[-2 * stride] - 5 * p[-stride] + 20 * p[0] + 20 * p[stride] - 5 * p[2 * stride] + p[3 * stride];
}

void
interpolate_halfpel_v_c(uint8_t* src, int32_t src_stride, uint8_t* dst, int32_t dst_stride, int32_t width, int32_t height)
{
    int32_t i, j;
    int32_t tmp;

    for (i = 0 ; i < height ; i ++)
    {
        for (j = 0 ; j < width ; j ++)
        {
            tmp = (tapfilter_v(src + j, src_stride) + 16) >> 5;
            dst[j] = CLIP1(tmp);
        }
        src += src_stride;
        dst += dst_stride;
    }
}

// use vertical to generate this pic
void
interpolate_halfpel_hv_c(uint8_t* src, int32_t src_stride, uint8_t* dst, int32_t dst_stride, int32_t width, int32_t height)
{
    int32_t i, j;
    int32_t tmp;

    for (i = 0 ; i < height + 0 ; i ++)
    {
        for (j = 0 ; j < width + 0 ; j ++)
        {
            tmp = (
                  (src[j - 2 - 2 * src_stride] - 5 * src[j - 1 - 2 * src_stride] + 20 * src[j - 2 * src_stride] + 20 * src[j + 1 - 2 * src_stride] - 5 * src[j + 2 - 2 * src_stride] + src[j + 3 - 2 * src_stride]) +
           (-5) * (src[j - 2 - 1 * src_stride] - 5 * src[j - 1 - 1 * src_stride] + 20 * src[j - 1 * src_stride] + 20 * src[j + 1 - 1 * src_stride] - 5 * src[j + 2 - 1 * src_stride] + src[j + 3 - 1 * src_stride]) +
           (20) * (src[j - 2 - 0 * src_stride] - 5 * src[j - 1 - 0 * src_stride] + 20 * src[j - 0 * src_stride] + 20 * src[j + 1 - 0 * src_stride] - 5 * src[j + 2 - 0 * src_stride] + src[j + 3 - 0 * src_stride]) +
           (20) * (src[j - 2 + 1 * src_stride] - 5 * src[j - 1 + 1 * src_stride] + 20 * src[j + 1 * src_stride] + 20 * src[j + 1 + 1 * src_stride] - 5 * src[j + 2 + 1 * src_stride] + src[j + 3 + 1 * src_stride]) +
           (-5) * (src[j - 2 + 2 * src_stride] - 5 * src[j - 1 + 2 * src_stride] + 20 * src[j + 2 * src_stride] + 20 * src[j + 1 + 2 * src_stride] - 5 * src[j + 2 + 2 * src_stride] + src[j + 3 + 2 * src_stride]) +
                  (src[j - 2 + 3 * src_stride] - 5 * src[j - 1 + 3 * src_stride] + 20 * src[j + 3 * src_stride] + 20 * src[j + 1 + 3 * src_stride] - 5 * src[j + 2 + 3 * src_stride] + src[j + 3 + 3 * src_stride]) +
                  512) >> 10;
            dst[j] = CLIP1(tmp);
        }
        src += src_stride;
        dst += dst_stride;
    }
}
