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

#include <stdio.h>
#include <assert.h>
#include "T264.h"
#include "bitstream.h"
#include "typedecision.h"
#include "estimation.h"

#define INTRA_THRESH 2000
#define SEARCH_THRESH 1000

static uint32_t
MBAnalysis(T264_t* t, int32_t y, int32_t x)
{
    uint32_t sad;
    T264_vector_t vec;
    uint8_t* src;
    uint8_t* dst;
    // 1, we try the previous frame vector
    // Attention: we rely that the buffer management method is the simplest one
    vec = t->refn[1].mb[y * t->mb_stride + x].vec[0][0];
    
    src = t->cur.Y[0]  + (y << 4) * t->stride + (x << 4);
    dst = t->refn[1].Y[0] + ((vec.y >> 2) + (y << 4)) * t->edged_stride + (x << 4) + (vec.x >> 2);

    sad = t->sad[MB_16x16](src, t->stride, dst, t->edged_stride);

    // 2, if sad is great than thresh, we try small_diamond_search
    if (sad > SEARCH_THRESH)
    {
        // diamond maybe enough
        T264_search_context_t context;
        context.limit_x = 16;
        context.limit_y = 16;
        context.vec_best = vec;
        context.vec = &vec;
        context.mb_part = MB_16x16;

        sad = small_diamond_search(t, src, dst, &context, t->stride, t->edged_stride, sad);
    }

    return sad;
}

static int32_t
MEAnalysis(T264_t* t)
{
    int32_t i, j;
    uint32_t sad = 0;
    uint32_t intra_thresh = INTRA_THRESH * t->mb_height * t->mb_width;

    if ((t->frame_id - t->last_i_frame_id) % t->param.idrframe == 0)
        return SLICE_IDR;

    if ((t->frame_id - t->last_i_frame_id) % t->param.iframe == 0)
        return SLICE_I;
    
    // we believe that only 2 gop per second at least
    if ((int32_t)(t->frame_id - t->last_i_frame_id) > (int32_t)(t->param.framerate / 2))
    {
        // previous has set a keyframe
        for (i = 0 ; i < t->mb_height ; i ++)
        {
            for (j = 0 ; j < t->mb_width ; j ++)
            {
                sad += MBAnalysis(t, i, j);
            }
        }

        t->emms();
    }
    else
    {
        sad = 0;
    }

    if (sad > intra_thresh)
        return SLICE_I;

    // p, b we just only use the 
    if (t->pending_bframes_num < t->param.b_num)
        return SLICE_B;

    return SLICE_P;
}

int32_t
decision_slice_type(T264_t* t)
{
    if ((t->flags & USE_SCENEDETECT) == 0)
    {
        if (t->frame_no % t->param.idrframe == 0)
        {
            return SLICE_IDR;
        }
        if (t->frame_no % t->param.iframe == 0)
        {
            return SLICE_I;
        }
        else    // P or B pic
        {
            if (t->frame_no % (t->param.b_num + 1) == 0)
            {
                return SLICE_P;
            }
            else
            {
                return SLICE_B;
            }
        }
    }
    else
    {
        return MEAnalysis(t);
    }
}
