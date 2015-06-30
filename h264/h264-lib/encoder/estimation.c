/*****************************************************************************
*
*  T264 AVC CODEC
*
*  Copyright(C) 2004-2005 llcc <lcgate1@yahoo.com.cn>
*               2004-2005 visionany <visionany@yahoo.com.cn>
*	2005.1.4 CloudWu<sywu@sohu.com>	modify diamond_search() 
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
#include "estimation.h"
#ifndef CHIP_DM642
#include "memory.h"
#endif
#include "assert.h"
#include "bitstream.h"
#include "inter.h"

//overlapped check points 
static int32_t
check_vec(int32_t i, T264_vector_t* vec)
{
    int32_t j;

    for(j = 0 ; j < i ; j ++)
    {
        if (vec[i].x == vec[j].x && vec[i].y == vec[j].y && vec[i].refno == vec[j].refno)
            return 1;
    }

    return 0;
}

uint32_t
T264_search(T264_t* t, T264_search_context_t* context)
{
    uint32_t sad = -1;
    int32_t i;
    int32_t best = 0;

    T264_vector_t mv_pred;

    int32_t limit_x = context->limit_x;
    int32_t limit_y = context->limit_y;
    int32_t stride_cur = t->stride;
    int32_t stride_ref = t->edged_stride;
    int32_t list_index = context->list_index;

    // start point of current and reference block 
    int32_t row = context->offset / t->edged_stride;
    int32_t col = context->offset % t->edged_stride;
    uint8_t* cur = t->cur.Y[0] + row * stride_cur + col;
    uint8_t* ref_st;

    uint8_t* ref;
    int8_t best_ref_no;

    //adaptive thresholds
    uint32_t th0;

    th0 = context->height * context->width; //256 for median predictor (16x16)	

    // try median vector
    if (context->vec[0].refno >= 0)
    {
        // check this predictor
        mv_pred.refno = context->vec[0].refno;
        mv_pred.x = context->vec[0].x >> 2;
        mv_pred.y = context->vec[0].y >> 2;
        ref_st = t->ref[list_index][mv_pred.refno]->Y[0] + context->offset;
        ref = ref_st + mv_pred.y * stride_ref + mv_pred.x;
        sad = t->sad[context->mb_part](cur, stride_cur, ref, stride_ref) +
            t->mb.lambda * (eg_size_se(t->bs, (mv_pred.x << 2) - context->vec[0].x) + 
            eg_size_se(t->bs, (mv_pred.y << 2) - context->vec[0].y));
        if (sad < th0)
        {
            context->vec_best = context->vec[0];
            return sad;
        }
    }

    // try other predictors (set A)
    for (i = 1 ; i < context->vec_num ; i ++)
    {
        if (context->vec[i].refno >= 0)
        {
            if (!check_vec(i, context->vec)) //not checked before
            {
                uint32_t cursad;
                // check this predictor
                mv_pred.refno = context->vec[i].refno;
                mv_pred.x = context->vec[i].x >> 2;
                mv_pred.y = context->vec[i].y >> 2;
                ref_st = t->ref[list_index][mv_pred.refno]->Y[0] + context->offset;
                ref = ref_st + mv_pred.y * stride_ref + mv_pred.x;
                cursad = t->sad[context->mb_part](cur, stride_cur, ref, stride_ref) +
                    t->mb.lambda * (eg_size_se(t->bs, (mv_pred.x << 2) - context->vec[0].x) + 
                    eg_size_se(t->bs, (mv_pred.y << 2) - context->vec[0].y));

                if (cursad < sad)
                {
                    best = i;
                    sad = cursad;
                }
            }
        }
    }

    context->vec_best = context->vec[best];

    if (sad < th0)
        return sad;

    // ref_st of best reference frame
    if (context->vec[best].refno >= 0)
    {
        best_ref_no = context->vec[best].refno;	
    }
    else
    {
        context->vec_best.refno = 0;
        context->vec_best.x = context->vec_best.y = best_ref_no = 0;
    }
    ref_st = t->ref[list_index][best_ref_no]->Y[0] + context->offset;
    // diamond search 
    /* small diamond is the fastest, square diamond has slightly gain lower bitrate and
          pull the speed down severely. LDSP + SDSP maybe have some problem... */
//*/
    sad = small_diamond_search(t, cur, ref_st, context, stride_cur, stride_ref, sad);
/*/
    sad = square_diamond_search(t, cur, ref_st, context, stride_cur, stride_ref, sad);
/*//*
    sad = diamond_search(t, cur, ref_st, context, stride_cur, stride_ref, sad);
//*/
    return sad;	
}

uint32_t
small_diamond_search(T264_t* t, uint8_t* cur, uint8_t* ref_st, T264_search_context_t* context, int32_t stride_cur, int32_t stride_ref, uint32_t sad)
{
    int32_t limit_x = context->limit_x;
    int32_t limit_y = context->limit_y;
    //start mv
    int32_t mvx = context->vec_best.x >> 2;
    int32_t mvy = context->vec_best.y >> 2;
    // sad for start mv
    uint8_t* ref;
    uint8_t* better_ref;
    int32_t better_mvx;
    int32_t better_mvy;
    uint32_t cursad;
    uint8_t stop = 0;

    ref_st += mvy * stride_ref + mvx;

    better_mvx = mvx;
    better_mvy = mvy;
    better_ref = ref_st;

    while(!stop)
    {
        stop = 1;

        // search 4 points of sdsp
        {
#define CHECK_CANDIDATE(x_offset, y_offset, ref_offset) ref = ref_st + ref_offset; cursad = t->sad[context->mb_part](cur, stride_cur, ref, stride_ref) + t->mb.lambda * (eg_size_se(t->bs, ((mvx + x_offset) << 2) - context->vec[0].x) + eg_size_se(t->bs, ((mvy + y_offset) << 2) - context->vec[0].y));if (cursad < sad){sad = cursad;better_ref = ref;better_mvx = mvx + x_offset;better_mvy = mvy + y_offset;stop = 0;}
            // left
            CHECK_CANDIDATE(-1, 0, -1);
            // right
            CHECK_CANDIDATE(1, 0, 1);
            // top
            CHECK_CANDIDATE(0, -1, -stride_ref);
            // bottom
            CHECK_CANDIDATE(0, 1, stride_ref);
        }

        mvx = better_mvx;
        mvy = better_mvy;
        ref_st = better_ref;
        if (ABS(mvx) > limit_x || ABS(mvy) > limit_y)
        {
            break;
        }
    }

    // final mv
    context->vec_best.x = mvx << 2;
    context->vec_best.y = mvy << 2;
    // mostly we use sad as cmp function
    if (t->cmp[context->mb_part] == t->sad[context->mb_part])
        return sad;

    ref = ref_st + mvy * stride_ref + mvx;
    sad = t->cmp[context->mb_part](cur, stride_cur, ref, stride_ref) +
        t->mb.lambda * (eg_size_se(t->bs, context->vec_best.x - context->vec[0].x) + 
        eg_size_se(t->bs, context->vec_best.y - context->vec[0].y));
    return sad;
}

uint32_t 
square_diamond_search(T264_t* t, uint8_t* cur, uint8_t* ref_st, T264_search_context_t* context, int32_t stride_cur, int32_t stride_ref, uint32_t sad)
{
    int32_t limit_x = context->limit_x;
    int32_t limit_y = context->limit_y;
    //start mv
    int32_t mvx = context->vec_best.x >> 2;
    int32_t mvy = context->vec_best.y >> 2;
    // sad for start mv
    uint32_t cursad;
    uint8_t* ref;
    uint8_t stop = 0;
    uint8_t* better_ref;
    int32_t better_mvx;
    int32_t better_mvy;

    //	start mv is zero?
    if(mvx == 0 && mvy == 0)
        return small_diamond_search(t, cur, ref_st, context, stride_cur, stride_ref, sad);

    ref_st += mvy * stride_ref + mvx;

    better_mvx = mvx;
    better_mvy = mvy;
    better_ref = ref_st;

    while(!stop)
    {
        stop = 1;

        // search 8 points of sdsp
        {
            // left
            CHECK_CANDIDATE(-1, 0, -1);
            // right
            CHECK_CANDIDATE(1, 0, 1);
            // top-left
            CHECK_CANDIDATE(-1, -1, -stride_ref - 1);
            // top
            CHECK_CANDIDATE(0, -1, -stride_ref);
            // top-right
            CHECK_CANDIDATE(1, -1, -stride_ref + 1);
            // bottom-left
            CHECK_CANDIDATE(-1, 1, stride_ref - 1);
            // bottom
            CHECK_CANDIDATE(0, 1, stride_ref);
            // bottom-right
            CHECK_CANDIDATE(1, 1, stride_ref + 1);
        }

        mvx = better_mvx;
        mvy = better_mvy;
        ref_st = better_ref;
        if (ABS(mvx) > limit_x || ABS(mvy) > limit_y)
        {
            break;
        }
    }

    // final mv
    context->vec_best.x = mvx << 2;
    context->vec_best.y = mvy << 2;
    // mostly we use sad as cmp function
    if (t->cmp[context->mb_part] == t->sad[context->mb_part])
        return sad;

    ref = ref_st + mvy * stride_ref + mvx;
    sad = t->cmp[context->mb_part](cur, stride_cur, ref, stride_ref) +
        t->mb.lambda * (eg_size_se(t->bs, context->vec_best.x - context->vec[0].x) + 
        eg_size_se(t->bs, context->vec_best.y - context->vec[0].y));
    return sad;
}

uint32_t
diamond_search(T264_t* t, uint8_t* cur, uint8_t* ref_st, T264_search_context_t* context, int32_t stride_cur, int32_t stride_ref, uint32_t sad)
{
    int32_t limit_x = context->limit_x;
    int32_t limit_y = context->limit_y;
    //start mv
    int32_t mvx = context->vec_best.x >> 2;
    int32_t mvy = context->vec_best.y >> 2;
    // sad for start mv
    uint8_t* ref;
    uint8_t* better_ref;
    int32_t better_mvx;
    int32_t better_mvy;
    uint32_t cursad;
    uint8_t stop = 0;

    ref_st += mvy * stride_ref + mvx;

    better_mvx = mvx;
    better_mvy = mvy;
    better_ref = ref_st;

    // large diamond
    while(!stop)
    {
        stop = 1;

        // search 8 points of ldsp
        {
            // left
            CHECK_CANDIDATE(-2, 0, -2);
            // right
            CHECK_CANDIDATE(2, 0, 2);
            // top
            CHECK_CANDIDATE(0, -2, -(stride_ref << 1));
            // bottom
            CHECK_CANDIDATE(0, 2, (stride_ref << 1));
            // top-left
            CHECK_CANDIDATE(-1, -1, -stride_ref - 1);
            // top-right
            CHECK_CANDIDATE(1, -1, -stride_ref + 1);
            // bottom-left
            CHECK_CANDIDATE(-1, 1, stride_ref - 1);
            // bottom-right
            CHECK_CANDIDATE(1, 1, stride_ref + 1);
        }

        mvx = better_mvx;
        mvy = better_mvy;
        ref_st = better_ref;
        if (ABS(mvx) > limit_x || ABS(mvy) > limit_y)
        {
            break;
        }
    }

    // small diamond
	stop = 0;
    while(!stop)
    {
        stop = 1;

        // search 4 points of sdsp
        {
            // left
            CHECK_CANDIDATE(-1, 0, -1);
            // right
            CHECK_CANDIDATE(1, 0, 1);
            // top
            CHECK_CANDIDATE(0, -1, -stride_ref);
            // bottom
            CHECK_CANDIDATE(0, 1, stride_ref);
        }

        mvx = better_mvx;
        mvy = better_mvy;
        ref_st = better_ref;
        if (ABS(mvx) > limit_x || ABS(mvy) > limit_y)
        {
            break;
        }
    }
	
    // final mv
    context->vec_best.x = mvx << 2;
    context->vec_best.y = mvy << 2;
    // mostly we use sad as cmp function
    if (t->cmp[context->mb_part] == t->sad[context->mb_part])
        return sad;

    ref = ref_st + mvy * stride_ref + mvx;
    sad = t->cmp[context->mb_part](cur, stride_cur, ref, stride_ref) +
        t->mb.lambda * (eg_size_se(t->bs, context->vec_best.x - context->vec[0].x) + 
        eg_size_se(t->bs, context->vec_best.y - context->vec[0].y));
    return sad;
}

/*
*	Full Search
*/

uint32_t
T264_search_full(T264_t* t, T264_search_context_t* context)
{
    uint32_t sad;
    uint32_t cursad;
    int32_t i, j;

    int16_t mb_xy = t->mb.mb_xy;
    int16_t mb_x = t->mb.mb_x;
    int16_t mb_y = t->mb.mb_y;
    int32_t height = context->height;
    int32_t width = context->width;
    int32_t limit_x = context->limit_x;
    int32_t limit_y = context->limit_y;
    int32_t stride_cur = t->stride;
    int32_t stride_ref = t->edged_stride;
    int32_t list_index = context->list_index;

    // start point of current and reference block 
    int32_t row = context->offset / t->edged_stride;
    int32_t col = context->offset % t->edged_stride;
    uint8_t* cur = t->cur.Y[0] + row * stride_cur + col;
    uint8_t* ref_st = t->ref[list_index][0]->Y[0] + row * stride_ref + col;	
    uint8_t* ref;
    context->vec_best.refno = 0;

    // full search
    sad = width * height * 255;
    for(i = -limit_y + (context->vec[0].y >> 2); i <= (limit_y + (context->vec[0].y >> 2)) ; i++)
        for(j = -limit_x + (context->vec[0].x >> 2); j <= (limit_x + (context->vec[0].x >> 2)) ; j++)
        {
            ref = ref_st + i * stride_ref + j;
            cursad = t->sad[context->mb_part](cur, stride_cur, ref, stride_ref) +
                t->mb.lambda * (eg_size_se(t->bs, (j << 2) - context->vec[0].x) + 
                eg_size_se(t->bs, (i << 2) - context->vec[0].y));
            if(cursad < sad)
            {
                sad = cursad;
                context->vec_best.y = i;
                context->vec_best.x = j;
            }
        }

        ref = ref_st + context->vec_best.y * stride_ref + context->vec_best.x;
        context->vec_best.y <<= 2;
        context->vec_best.x <<= 2;

        sad = t->cmp[context->mb_part](cur, t->stride, ref, t->edged_stride) +
            t->mb.lambda * (eg_size_se(t->bs, context->vec_best.x - context->vec[0].x) + 
            eg_size_se(t->bs, context->vec_best.y - context->vec[0].y));

        return sad;	
}

// xxx, never used, just for compare to jm80.
uint32_t
T264_spiral_search_full(T264_t* t, T264_search_context_t* context)
{
    uint32_t sad;
    uint32_t cursad;
    int32_t i, j, k, l;

    int16_t mb_xy = t->mb.mb_xy;
    int16_t mb_x = t->mb.mb_x;
    int16_t mb_y = t->mb.mb_y;
    int32_t height = context->height;
    int32_t width = context->width;
    int32_t limit_x = context->limit_x;
    int32_t limit_y = context->limit_y;
    int32_t stride_cur = t->stride;
    int32_t stride_ref = t->edged_stride;
    int32_t list_index = context->list_index;

    // start point of current and reference block 
    int32_t row = context->offset / t->edged_stride;
    int32_t col = context->offset % t->edged_stride;
    uint8_t* cur = t->cur.Y[0] + row * stride_cur + col;
    uint8_t* ref_st = t->ref[list_index][context->vec[0].refno]->Y[0] + row * stride_ref + col;	
    uint8_t* ref;
    int32_t spiral_search_x[33 * 33];
    int32_t spiral_search_y[33 * 33];
    context->vec_best.refno = context->vec[0].refno;

    spiral_search_x[0] = spiral_search_y[0] = 0;
    for (k=1, l=1; l<=T264_MAX(1,16); l++)
    {
        for (i=-l+1; i< l; i++)
        {
            spiral_search_x[k] = l;  spiral_search_y[k++] =  i;
            spiral_search_x[k] =  -l;  spiral_search_y[k++] =  i;
        }
        for (i=-l;   i<=l; i++)
        {
            spiral_search_x[k] =  i;  spiral_search_y[k++] = l;
            spiral_search_x[k] =  i;  spiral_search_y[k++] =  -l;
        }
    }

    // full search
    sad = width * height * 255;
    for(k = 0 ; k < 33 * 33 ; k ++)
    {
        i = (context->vec[0].y / 4) + spiral_search_y[k];
        j = (context->vec[0].x / 4) + spiral_search_x[k];

        ref = ref_st + i * stride_ref + j;
        cursad = t->sad[context->mb_part](cur, stride_cur, ref, stride_ref) +
            t->mb.lambda * (eg_size_se(t->bs, (j << 2) - context->vec[0].x) + 
            eg_size_se(t->bs, (i << 2) - context->vec[0].y));
        if(cursad < sad)
        {
            sad = cursad;
            context->vec_best.y = i;
            context->vec_best.x = j;
        }
    }

    ref = ref_st + context->vec_best.y * stride_ref + context->vec_best.x;
    context->vec_best.y <<= 2;
    context->vec_best.x <<= 2;

    sad = t->cmp[context->mb_part](cur, t->stride, ref, t->edged_stride) +
        t->mb.lambda * (eg_size_se(t->bs, context->vec_best.x - context->vec[0].x) + 
        eg_size_se(t->bs, context->vec_best.y - context->vec[0].y));

    return sad;	
}