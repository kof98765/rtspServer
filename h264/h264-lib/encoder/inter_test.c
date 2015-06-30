/*****************************************************************************
 *
 *  T264 AVC CODEC
 *
 *  Copyright(C) 2004-2005 llcc <lcgate1@yahoo.com.cn>
 *               2004-2005 visionany <visionany@yahoo.com.cn>
 *
 *   llcc 2004-9-23
 *
 *   Test for sub-pel search after integer pel search in mode dicision(call mode1)
 *   vs only integer search in mode dicision in orgin design(call mode2).
 *   test result(jm80 have modified to conform to our condition):
 *   (1I + 299P), foreman.cif, qp = 30, no i16x16
 *               8x16(bytes)   16x8(bytes)
 *   jm80        743430        750905
 *   t264        746615        756162
 *   (mode1+spiral full search)
 *   t264        745866        755622
 *   (mode2+spiral full search)
 *   t264        792700        801691
 *   (mode1+pmvfast)
 *   t264        793219        804540
 *   (mode2+pmvfast)
 *   NOTE: if u want to test this file, please change pred_p16x16[16 * 16] in t264.h 
 *   to pred_p16x16[4][16 * 16].
 *   Yes, we will keep our older design.
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
#include "memory.h"
#include "T264.h"
#include "inter.h"
#include "intra.h"
#include "estimation.h"
#include "utility.h"
#include "interpolate.h"

void
T264_predict_mv(T264_t* t, int32_t list, int32_t i, int32_t width, T264_vector_t* vec)
{
    int32_t n;
    int32_t count = 0;
    int32_t idx;
    int32_t row;
    int32_t col;
    int32_t org;
    T264_vector_t vec_n[3];

    n = vec->refno;
    org = i;
    i = luma_index[i];

    col = org % 4;
    row = org / 4;

    vec_n[0] = t->mb.vec_ref[VEC_LUMA - 1 + row * 8 + col].vec;
    vec_n[1] = t->mb.vec_ref[VEC_LUMA - 8 + row * 8 + col].vec;
    vec_n[2] = t->mb.vec_ref[VEC_LUMA - 8 + row * 8 + col + width].vec;
    if (vec_n[2].refno == -2)
    {
        vec_n[2] = t->mb.vec_ref[VEC_LUMA - 8 + row * 8 + col - 1].vec;
    }
    if (((i & 3) == 3) || ((i & 3) == 2 && width == 2))
    {
        vec_n[2] = t->mb.vec_ref[VEC_LUMA - 8 + row * 8 + col - 1].vec;
    }
    if (t->mb.mb_part == MB_16x8)
    {
        if (i == 0 && n == vec_n[1].refno)
        {
            vec[0] = vec_n[1];
            return;
        }
        else if (i != 0 && n == vec_n[0].refno)
        {
            vec[0] = vec_n[0];
            return;
        }
    }
    else if (t->mb.mb_part == MB_8x16)
    {
        if (i == 0 && n == vec_n[0].refno)
        {
            vec[0] = vec_n[0];
            return;
        }
        else if (i != 0 && n == vec_n[2].refno)
        {
            vec[0] = vec_n[2];
            return;
        }
    }

    if (vec_n[0].refno == n)
    {
        count ++;
        idx = 0;
    }
    if (vec_n[1].refno == n)
    {
        count ++;
        idx = 1;
    }
    if (vec_n[2].refno == n)
    {
        count ++;
        idx = 2;
    }

    if (count > 1)
    {
        vec[0].x = T264_median(vec_n[0].x, vec_n[1].x, vec_n[2].x);
        vec[0].y = T264_median(vec_n[0].y, vec_n[1].y, vec_n[2].y);
        return;
    }
    else if (count == 1)
    {
        vec[0] = vec_n[idx];
        return;
    }
    else if (vec_n[1].refno == -2 && vec_n[2].refno == -2 && vec_n[0].refno != -2)
    {
        vec[0] = vec_n[0];
    }
    else
    {
        vec[0].x = T264_median(vec_n[0].x, vec_n[1].x, vec_n[2].x);
        vec[0].y = T264_median(vec_n[0].y, vec_n[1].y, vec_n[2].y);
        return;        
    }
}

int32_t 
T264_median(int32_t x, int32_t y, int32_t z)
{
    int32_t min, max;
    if (x < y)
    {
        min = x;
        max = y;
    }
    else
    {
        min = y;
        max = x;
    }

    if (z < min)
    {
        min = z;
    }
    else if (z > max)
    {
        max = z;
    }

    return x + y + z - min - max;
}

void
copy_nvec(T264_vector_t* src, T264_vector_t* dst, int32_t width, int32_t height, int32_t stride)
{
    int32_t i, j;

    for(i = 0 ; i < height ; i ++)
    {
        for(j = 0 ; j < width ; j ++)
        {
            dst[j] = src[0];
        }
        dst += stride;
    }
}

void
T264_inter_p16x16_mode_available(T264_t* t, int32_t preds[], int32_t* modes)
{
    if (t->flags & USE_FORCEBLOCKSIZE)
    {
        *modes = 0;
      
        if (t->param.block_size & SEARCH_16x16P)
            preds[(*modes) ++] = MB_16x16;
        if (t->param.block_size & SEARCH_16x8P)
            preds[(*modes) ++] = MB_16x8;
        if (t->param.block_size & SEARCH_8x16P)
            preds[(*modes) ++] = MB_8x16;

        return ;
    }

    if ((t->mb.mb_neighbour & (MB_LEFT | MB_TOP)) == (MB_LEFT | MB_TOP))
    {
        *modes = 0;

        preds[(*modes) ++] = MB_16x16;
        if (t->mb.vec_ref[VEC_LUMA - 1].part == MB_16x8)
        {
            preds[(*modes) ++] = MB_16x8;
        }
        if (t->mb.vec_ref[VEC_LUMA - 8].part == MB_8x16)
        {
            preds[(*modes) ++] = MB_8x16;
        }
    }
    else
    {
        // try all
        preds[0] = MB_16x16;
        preds[1] = MB_16x8;
        preds[2] = MB_8x16;
        *modes = 3;
    }
}

void
T264_inter_p8x8_mode_available(T264_t* t, int32_t preds[], int32_t* modes, int32_t sub_no)
{
    static const int32_t neighbour[] = 
    {
        0, MB_LEFT, MB_TOP, MB_LEFT| MB_TOP
    };

    int32_t mb_neighbour = t->mb.mb_neighbour| neighbour[sub_no];

    if (t->flags & USE_FORCEBLOCKSIZE)
    {
        *modes = 0;

        if (t->param.block_size & SEARCH_8x8P)
            preds[(*modes) ++] = MB_8x8;
        if (t->param.block_size & SEARCH_8x4P)
            preds[(*modes) ++] = MB_8x4;
        if (t->param.block_size & SEARCH_4x8P)
            preds[(*modes) ++] = MB_4x8;
        if (t->param.block_size & SEARCH_4x4P)
            preds[(*modes) ++] = MB_4x4;

        return ;
    }

    if ((mb_neighbour & (MB_LEFT | MB_TOP)) == (MB_LEFT | MB_TOP))
    {
        *modes = 0;

        preds[*modes ++] = MB_8x8;
        if (t->mb.vec_ref[VEC_LUMA - 8 + sub_no / 2 * 16 + sub_no % 2 * 4].part == MB_8x4)
        {
            preds[*modes ++] = MB_8x4;
        }
        if (t->mb.vec_ref[VEC_LUMA - 1 + sub_no / 2 * 16 + sub_no % 2 * 4].part == MB_4x8)
        {
            preds[*modes ++] = MB_4x8;
        }
        if (t->mb.vec_ref[VEC_LUMA - 8 + sub_no / 2 * 16 + sub_no % 2 * 4].part == MB_4x4 || 
            t->mb.vec_ref[VEC_LUMA - 1 + sub_no / 2 * 16 + sub_no % 2 * 4].part == MB_4x4)
        {
            preds[*modes ++] = MB_4x4;
        }
    }
    else
    {
        // try all
        preds[0] = MB_8x8;
        preds[1] = MB_8x4;
        preds[2] = MB_4x8;
        preds[3] = MB_4x4;
        *modes = 4;
    }
}

uint32_t
T264_mode_decision_inter_y(_RW T264_t* t)
{
    uint32_t sad;
    uint32_t sad_min = -1;
    uint8_t best_mode;
    uint8_t part;
    int32_t i, n;
    int32_t preds[7];
    int32_t modes;
    search_data_t s0;
    subpart_search_data_t s1;
    T264_vector_t vec_bak[16 * 16]; 

    typedef uint32_t (*p16x16_function_t)(T264_t*, search_data_t* s);
    static p16x16_function_t p16x16_function[] = 
    {
        T264_mode_decision_inter_16x16p,
        T264_mode_decision_inter_16x8p,
        T264_mode_decision_inter_8x16p
    };

    T264_inter_p16x16_mode_available(t, preds, &modes);

    best_mode = P_L0;
	
	//SKIP
/*	if(modes > 0) 
	{
		sad = p16x16_function[0](t, &s0);
		if(t->mb.mb_mode == P_SKIP)
		{
			sad_min = sad;
			return sad_min;
		}
		else
		{
			sad_min = sad;
			part = 0;
		}
	}

 */
    for(n = 0 ; n < modes ; n ++)
    {
        int32_t mode = preds[n];
        memcpy(vec_bak, t->mb.vec, sizeof(vec_bak));
        sad = p16x16_function[mode](t, &s0);

        if (sad < sad_min)
        {
            part = mode;
            sad_min = sad;
        }
        else
        {
            memcpy(t->mb.vec, vec_bak, sizeof(vec_bak));
        }
    }

    if (t->flags & USE_SUBBLOCK)
    {
        uint32_t sub_sad_all = 0;

        typedef uint32_t (*p8x8_function_t)(T264_t*, int32_t, subpart_search_data_t* s);
        static p8x8_function_t p8x8_function[] = 
        {
            T264_mode_decision_inter_8x8p,
            T264_mode_decision_inter_8x8p,
            T264_mode_decision_inter_8x4p,
            T264_mode_decision_inter_4x8p,
            T264_mode_decision_inter_4x4p
        };

        for(i = 0 ; i < 4 ; i ++)
        {
            uint32_t sub_sad;
            uint32_t sub_sad_min = -1;

            T264_inter_p8x8_mode_available(t, preds, &modes, i);

            for(n = 0 ; n < modes ; n ++)
            {
                int32_t mode = preds[n];
                T264_vector_t vec_bak[4];

                vec_bak[0] = t->mb.vec[0][i / 2 * 8 + i % 2 * 2 + 0];
                vec_bak[1] = t->mb.vec[0][i / 2 * 8 + i % 2 * 2 + 1];
                vec_bak[2] = t->mb.vec[0][i / 2 * 8 + i % 2 * 2 + 4];
                vec_bak[3] = t->mb.vec[0][i / 2 * 8 + i % 2 * 2 + 5];

                sub_sad = p8x8_function[mode - MB_8x8](t, i, &s1);

                if (sub_sad < sub_sad_min)
                {
                    sub_sad_min = sub_sad;
                    t->mb.submb_part[i / 2 * 8 + i % 2 * 2 + 0] = mode;
                    t->mb.submb_part[i / 2 * 8 + i % 2 * 2 + 1] = mode;
                    t->mb.submb_part[i / 2 * 8 + i % 2 * 2 + 4] = mode;
                    t->mb.submb_part[i / 2 * 8 + i % 2 * 2 + 5] = mode;
                }
                else
                {
                    // restore current best mode
                    t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 0].vec = vec_bak[0];
                    t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 1].vec = vec_bak[1];
                    t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 8].vec = vec_bak[2];
                    t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 9].vec = vec_bak[3];

                    t->mb.vec[0][i / 2 * 8 + i % 2 * 2 + 0] = vec_bak[0];
                    t->mb.vec[0][i / 2 * 8 + i % 2 * 2 + 1] = vec_bak[1];
                    t->mb.vec[0][i / 2 * 8 + i % 2 * 2 + 4] = vec_bak[2];
                    t->mb.vec[0][i / 2 * 8 + i % 2 * 2 + 5] = vec_bak[3];
                }
            }

            sub_sad_all += sub_sad_min;
        }

        if (sub_sad_all < sad_min)
        {
            part = MB_8x8;
            sad_min = sub_sad_all;
        }
    }

    sad = T264_mode_decision_intra_y(t);
    // xxx
    if (0 && sad <= sad_min)
    {
        best_mode = t->mb.mb_mode;
        sad_min = sad;
    }
    else
    {
        t->mb.mb_part = part;
    }

    t->mb.mb_mode = best_mode;
    t->mb.sad = sad_min;

    return sad_min;
}

/*
 	0   median
    1   left
    2   top
    3   topright
    4   topleft
    5   0, 0
    6   last frame
 */
static void
get_pmv(T264_t* t, T264_vector_t* vec, int32_t part, int32_t idx, int32_t width, int32_t* n)
{
    int32_t count = 0;
    int32_t row;
    int32_t col;
    int32_t i;

    vec->refno = 0;
    T264_predict_mv(t, 0, idx, width, vec);
    
    idx = luma_index[idx];
    col = idx % 4;
    row = idx / 4;

    vec[1] = t->mb.vec_ref[VEC_LUMA - 1 + row * 8 + col].vec;   // left
    vec[2] = t->mb.vec_ref[VEC_LUMA - 8 + row * 8 + col].vec;   // top
    vec[3] = t->mb.vec_ref[VEC_LUMA - 8 + row * 8 + col + width].vec;   // top right
    vec[4] = t->mb.vec_ref[VEC_LUMA - 8 + row * 8 + col - 1].vec;       // left top

    for(i = 0 ; i < t->param.ref_num ; i ++)
    {
        if (i != vec[0].refno)
        {
            vec[5 + i].x = vec[0].x;
            vec[5 + i].y = vec[0].y;
        }
        else
        {
            vec[5 + i].x = vec[5 + i].y = 0;
        }
        vec[5 + i].refno = i;
    }
    *n = 5 + t->param.ref_num;
}

uint32_t
T264_mode_decision_inter_16x16p(_RW T264_t* t, search_data_t* s)
{
    T264_vector_t vec[5 + 10];  // NOTE: max 10 refs
    T264_search_context_t context;
    int32_t num;
	uint8_t is_skip = 0;

    get_pmv(t, vec, MB_16x16, 0, 4, &num);

    context.height = 16;
    context.width  = 16;
    context.limit_x= t->param.search_x;
    context.limit_y= t->param.search_y;
    context.vec    = vec;
    context.vec_num= num;
    context.offset = (t->mb.mb_y << 4) * t->edged_stride + (t->mb.mb_x << 4);

    s->src[0] = t->mb.src_y;
    s->sad[0] = t->search(t, &context);
    s->vec[0] = context.vec_best;
    s->ref[0] = t->refl0[s->vec[0].refno];
    s->offset[0] = context.offset;
    s->vec_median[0] = vec[0];

    s->sad[0] = T264_quarter_pixel_search(t, s->src[0], s->ref[0], s->offset[0], &s->vec[0], &s->vec_median[0], s->sad[0], 16, 16, t->mb.pred_p16x16[0]);
    copy_nvec(&s->vec[0], &t->mb.vec[0][0], 4, 4, 4);
	/*/SKIP
/*
	if(t->mb.mb_x == 0 || t->mb.mb_y == 0)
	{
		if(s->vec[0].x == 0 && s->vec[0].y == 0 && s->vec[0].refno == 0)
			is_skip = 1;
	}
	else
	{
		if(s->vec[0].x == context.vec[0].x && s->vec[0].y == context.vec[0].y 
			&& s->vec[0].refno == 0 && context.vec[0].refno == 0)
			is_skip = 1;
	}
*/

/*

	//SKIP
	if(   ((t->mb.mb_neighbour & MB_LEFT) != MB_LEFT && s->vec[0].x == 0 && s->vec[0].y == 0)
       || ((t->mb.mb_neighbour & MB_TOP) != MB_TOP && s->vec[0].x == 0 && s->vec[0].y == 0)
//	   || (t->mb.vec_ref[VEC_LUMA - 1].vec.refno == 0 && t->mb.vec_ref[VEC_LUMA - 1].vec.x == 0 && t->mb.vec_ref[VEC_LUMA - 1].vec.y == 0 && s->vec[0].x == 0 && s->vec[0].y == 0)
//	   || (t->mb.vec_ref[VEC_LUMA - 8].vec.refno == 0 && t->mb.vec_ref[VEC_LUMA - 8].vec.x == 0 && t->mb.vec_ref[VEC_LUMA - 8].vec.y == 0 && s->vec[0].x == 0 && s->vec[0].y == 0)	
	   )
	{
		sad = T264_quarter_pixel_search(t, s->src[0], s->ref[0], s->offset[0], &s->vec[0], &s->vec_median[0], s->sad[0], 16, 16, t->mb.pred_p16x16);
        copy_nvec(&s->vec[0], &t->mb.vec[0][0], 4, 4, 4);
	    t->mb.mb_mode = P_SKIP;
		t->mb.mb_part = MB_16x16;
		t->mb.sad = sad;		
	}

	else if(s->vec[0].x == context.vec[0].x && s->vec[0].y == context.vec[0].y 
		&& s->vec[0].refno == 0 && context.vec[0].refno == 0)
	{
		//All residual zero?
		int32_t nz = 0;
		int32_t idx;
		sad = T264_quarter_pixel_search(t, s->src[0], s->ref[0], s->offset[0], &s->vec[0], &s->vec_median[0], s->sad[0], 16, 16, t->mb.pred_p16x16);
        copy_nvec(&s->vec[0], &t->mb.vec[0][0], 4, 4, 4);
		T264_encode_inter_y(t);
		for(idx = 0; idx < 16; idx++)
			nz += array_non_zero_count(t->mb.dct_y_z[idx], 16);
//		T264_encode_inter_uv(t);
//		for(idx = 0; idx < 8; idx++)
//			nz += array_non_zero_count(t->mb.dct_uv_z[idx / 4][idx % 4], 16);
		if(nz == 0)
		{
//            if ((t->mb.mb_neighbour & MB_LEFT) != MB_LEFT || (t->mb.mb_neighbour & MB_TOP) != MB_TOP ||
//                (t->mb.vec_ref[VEC_LUMA - 1].vec.refno == 0 && t->mb.vec_ref[VEC_LUMA - 1].vec.x == 0 && t->mb.vec_ref[VEC_LUMA - 1].vec.y == 0) ||
//                (t->mb.vec_ref[VEC_LUMA - 8].vec.refno == 0 && t->mb.vec_ref[VEC_LUMA - 8].vec.x == 0 && t->mb.vec_ref[VEC_LUMA - 8].vec.y == 0))
//            {
//                uint32_t flags = t->flags;
//                s->vec[0].x = s->vec[0].y = s->vec[0].refno = 0;
//                copy_nvec(&s->vec[0], &t->mb.vec[0][0], 4, 4, 4);
//                t->flags &= ~(USE_HALFPEL |USE_QUARTPEL);
//                sad = T264_quarter_pixel_search(t, s->src[0], s->ref[0], s->offset[0], &s->vec[0], &s->vec_median[0], s->sad[0], 16, 16, t->mb.pred_p16x16);
//                t->flags = flags;
//                T264_encode_inter_y(t);
//            }
            T264_encode_inter_uv(t);
		    t->mb.mb_mode = P_SKIP;
			t->mb.mb_part = MB_16x16;
			t->mb.sad = sad;

			if(context.vec[0].x !=0 && context.vec[0].y != 0)
				printf("MVx=%d,MVy=%d\n",context.vec[0].x,context.vec[0].y);
		}
		else
		{
			t->mb.mb_mode = P_L0;
			t->mb.mb_part = MB_16x16;
		}
	}
	*/


    return s->sad[0];
}

uint32_t
T264_mode_decision_inter_16x8p(_RW T264_t* t, search_data_t* s)
{
    T264_vector_t vec[5 + 10];  // NOTE: max 10 refs
    T264_search_context_t context;
    int32_t num;
    uint8_t old_part = t->mb.mb_part;

    t->mb.mb_part = MB_16x8;

    get_pmv(t, vec, MB_16x8, 0, 4, &num);

    context.height = 8;
    context.width  = 16;
    context.limit_x= t->param.search_x;
    context.limit_y= t->param.search_y;
    context.vec    = vec;
    context.vec_num= num;
    context.offset = (t->mb.mb_y << 4) * t->edged_stride + (t->mb.mb_x << 4);

    s->src[1] = t->mb.src_y;
    s->sad[1] = t->search(t, &context);
    s->vec[1] = context.vec_best;
    s->ref[1] = t->refl0[s->vec[1].refno];
    s->offset[1] = context.offset;
    s->vec_median[1] = vec[0];

    s->sad[1] = T264_quarter_pixel_search(t, s->src[1], s->ref[1], s->offset[1], &s->vec[1], &s->vec_median[1], s->sad[1], 16, 8, t->mb.pred_p16x16[1]);
    copy_nvec(&s->vec[1], &t->mb.vec[0][0], 4, 2, 4);
    t->mb.vec_ref[VEC_LUMA + 8].vec = s->vec[1];
    get_pmv(t, vec, MB_16x8, luma_index[8], 4, &num);

    s->src[2] = t->mb.src_y + 8 * t->stride;
    context.offset += 8 * t->edged_stride;
    s->sad[2] = t->search(t, &context);
    s->vec[2] = context.vec_best;
    s->ref[2] = t->refl0[s->vec[2].refno];
    s->offset[2] = context.offset;
    s->vec_median[2] = vec[0];

    s->sad[2] = T264_quarter_pixel_search(t, s->src[2], s->ref[2], s->offset[2], &s->vec[2], &s->vec_median[2], s->sad[2], 16, 8, t->mb.pred_p16x16[1] + 16 * 8);
    copy_nvec(&s->vec[2], &t->mb.vec[0][8], 4, 2, 4);

    t->mb.mb_part = old_part;

    return s->sad[1] + s->sad[2];
}

uint32_t
T264_mode_decision_inter_8x16p(_RW T264_t * t, search_data_t* s)
{
    T264_vector_t vec[5 + 10];  // NOTE: max 10 refs
    T264_search_context_t context;
    int32_t num;
    uint8_t old_part = t->mb.mb_part;

    t->mb.mb_part = MB_8x16;
    get_pmv(t, vec, MB_8x16, 0, 2, &num);

    context.height = 16;
    context.width  = 8;
    context.limit_x= t->param.search_x;
    context.limit_y= t->param.search_y;
    context.vec    = vec;
    context.vec_num= num;
    context.offset = (t->mb.mb_y << 4) * t->edged_stride + (t->mb.mb_x << 4);

    s->src[3] = t->mb.src_y;
    s->sad[3] = t->search(t, &context);
    s->vec[3] = context.vec_best;
    s->ref[3] = t->refl0[s->vec[3].refno];
    s->offset[3] = context.offset;
    s->vec_median[3] = vec[0];

    s->sad[3] = T264_quarter_pixel_search(t, s->src[3], s->ref[3], s->offset[3], &s->vec[3], &s->vec_median[3], s->sad[3], 8, 16, t->mb.pred_p16x16[2]);
    copy_nvec(&s->vec[3], &t->mb.vec[0][0], 2, 4, 4);
    t->mb.vec_ref[VEC_LUMA + 1].vec = s->vec[3];
    // xxx
    //printf("mb: %d, x: %d, y: %d, sad: %d\n", t->mb.mb_xy, s->vec[3].x, s->vec[3].y, s->sad[3]);

    get_pmv(t, vec, MB_8x16, luma_index[4], 2, &num);

    s->src[4] = t->mb.src_y + 8;
    context.offset += 8;
    s->sad[4] = t->search(t, &context);
    s->vec[4] = context.vec_best;
    s->ref[4] = t->refl0[s->vec[4].refno];
    s->offset[4] = context.offset;
    s->vec_median[4] = vec[0];

    s->sad[4] = T264_quarter_pixel_search(t, s->src[4], s->ref[4], s->offset[4], &s->vec[4], &s->vec_median[4], s->sad[4], 8, 16, t->mb.pred_p16x16[2] + 8);
    copy_nvec(&s->vec[4], &t->mb.vec[0][2], 2, 4, 4);

    t->mb.mb_part = old_part;

    // xxx
    //printf("mb: %d, x: %d, y: %d, sad: %d\n", t->mb.mb_xy, s->vec[4].x, s->vec[4].y, s->sad[4]);

    return s->sad[3] + s->sad[4];
}

uint32_t
T264_mode_decision_inter_8x8p(_RW T264_t * t, int32_t i, subpart_search_data_t* s)
{
    T264_vector_t vec[5 + 10];  // NOTE: max 10 refs
    T264_search_context_t context;
    int32_t num;

    get_pmv(t, vec, MB_8x8, luma_index[4 * i], 2, &num);

    context.height = 8;
    context.width  = 8;
    context.limit_x= t->param.search_x;
    context.limit_y= t->param.search_y;
    context.vec    = vec;
    context.vec_num= num;
    context.offset = ((t->mb.mb_y << 4) + i / 2 * 8) * t->edged_stride + (t->mb.mb_x << 4) + i % 2 * 8;

    s->src[i][0] = t->mb.src_y + (i / 2 * 8) * t->stride + i % 2 * 8;
    s->sad[i][0] = t->search(t, &context);
    s->vec[i][0] = context.vec_best;
    s->offset[i][0] = context.offset;
    s->ref[i][0] = t->refl0[s->vec[i][0].refno];
    s->vec_median[i][0] = vec[0];

    s->sad[i][0] = T264_quarter_pixel_search(t, s->src[i][0], s->ref[i][0], s->offset[i][0], &s->vec[i][0], &s->vec_median[i][0], s->sad[i][0], 8, 8, t->mb.pred_p16x16[3] + i / 2 * 16 * 8 + i % 2 * 8);

    t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 0].vec =
    t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 1].vec =
    t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 8].vec =
    t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 9].vec = s->vec[i][0];

    t->mb.vec[0][i / 2 * 8 + i % 2 * 2 + 0] = s->vec[i][0];
    t->mb.vec[0][i / 2 * 8 + i % 2 * 2 + 1] = s->vec[i][0];
    t->mb.vec[0][i / 2 * 8 + i % 2 * 2 + 4] = s->vec[i][0];
    t->mb.vec[0][i / 2 * 8 + i % 2 * 2 + 5] = s->vec[i][0];
    // xxx
    printf("mb: %d, x: %d, y: %d, mx: %d, my: %d, sad: %d\n", t->mb.mb_xy, s->vec[i][0].x, s->vec[i][0].y, vec[0].x, vec[0].y, s->sad[i][0]);

    return s->sad[i][0];
}

uint32_t
T264_mode_decision_inter_8x4p(_RW T264_t * t, int32_t i, subpart_search_data_t* s)
{
    T264_vector_t vec[5 + 10];  // NOTE: max 10 refs
    T264_search_context_t context;
    int32_t num;

    get_pmv(t, vec, MB_8x4, luma_index[4 * i + 0], 2, &num);

    context.height = 4;
    context.width  = 8;
    context.limit_x= t->param.search_x;
    context.limit_y= t->param.search_y;
    context.vec    = vec;
    context.vec_num= num;
    context.offset = ((t->mb.mb_y << 4) + i / 2 * 8) * t->edged_stride + (t->mb.mb_x << 4) + i % 2 * 8;

    s->src[i][1] = t->mb.src_y + (i / 2 * 8) * t->stride + i % 2 * 8;
    s->sad[i][1] = t->search(t, &context);
    s->vec[i][1] = context.vec_best;
    s->offset[i][1] = context.offset;
    s->ref[i][1] = t->refl0[s->vec[i][1].refno];
    s->vec_median[i][1] = vec[0];

    s->sad[i][1] = T264_quarter_pixel_search(t, s->src[i][1], s->ref[i][1], s->offset[i][1], &s->vec[i][1], &s->vec_median[i][1], s->sad[i][1], 8, 4, t->mb.pred_p8x8 + i / 2 * 16 * 8 + i % 2 * 8);
    t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 0].vec =
    t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 1].vec = s->vec[i][1];
    t->mb.vec[0][i / 2 * 8 + i % 2 * 2 + 0] = s->vec[i][1];
    t->mb.vec[0][i / 2 * 8 + i % 2 * 2 + 1] = s->vec[i][1];
    get_pmv(t, vec, MB_8x4, luma_index[4 * i + 2], 2, &num);

    s->src[i][2] = s->src[i][1] + 4 * t->stride;
    context.offset += 4 * t->edged_stride;
    s->sad[i][2] = t->search(t, &context);
    s->vec[i][2] = context.vec_best;
    s->offset[i][2] = context.offset;
    s->ref[i][2] = t->refl0[s->vec[i][2].refno];
    s->vec_median[i][2] = vec[0];

    s->sad[i][2] = T264_quarter_pixel_search(t, s->src[i][2], s->ref[i][2], s->offset[i][2], &s->vec[i][2], &s->vec_median[i][2], s->sad[i][2], 8, 4, t->mb.pred_p8x8 + i / 2 * 16 * 8 + i % 2 * 8 + 16 * 4);
    t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 8].vec =
    t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 9].vec = s->vec[i][2];
    t->mb.vec[0][i / 2 * 8 + i % 2 * 2 + 4] = s->vec[i][2];
    t->mb.vec[0][i / 2 * 8 + i % 2 * 2 + 5] = s->vec[i][2];

    return s->sad[i][1] + s->sad[i][2];
}

uint32_t
T264_mode_decision_inter_4x8p(_RW T264_t * t, int32_t i, subpart_search_data_t* s)
{
    T264_vector_t vec[5 + 10];  // NOTE: max 10 refs
    T264_search_context_t context;
    int32_t num;

    get_pmv(t, vec, MB_4x8, luma_index[4 * i + 0], 1, &num);

    context.height = 8;
    context.width  = 4;
    context.limit_x= t->param.search_x;
    context.limit_y= t->param.search_y;
    context.vec    = vec;
    context.vec_num= num;
    context.offset = ((t->mb.mb_y << 4) + i / 2 * 8) * t->edged_stride + (t->mb.mb_x << 4) + i % 2 * 8;

    s->src[i][3] = t->mb.src_y + (i / 2 * 8) * t->stride + i % 2 * 8;
    s->sad[i][3] = t->search(t, &context);
    s->vec[i][3] = context.vec_best;
    s->offset[i][3] = context.offset;
    s->ref[i][3] = t->refl0[s->vec[i][3].refno];
    s->vec_median[i][3] = vec[0];

    s->sad[i][3] = T264_quarter_pixel_search(t, s->src[i][3], s->ref[i][3], s->offset[i][3], &s->vec[i][3], &s->vec[i][3], s->sad[i][3], 4, 8, t->mb.pred_p8x8 + i / 2 * 16 * 8 + i % 2 * 8);

    t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 0].vec =
    t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 8].vec = s->vec[i][3];
    t->mb.vec[0][i / 2 * 8 + i % 2 * 2 + 0] = s->vec[i][3];
    t->mb.vec[0][i / 2 * 8 + i % 2 * 2 + 4] = s->vec[i][3];
    get_pmv(t, vec, MB_4x8, luma_index[4 * i + 1], 1, &num);

    s->src[i][4] = s->src[i][3] + 4;
    context.offset += 4;
    s->sad[i][4] = t->search(t, &context);
    s->vec[i][4] = context.vec_best;
    s->offset[i][4] = context.offset;
    s->ref[i][4] = t->refl0[s->vec[i][4].refno];
    s->vec_median[i][4] = vec[0];

    s->sad[i][4] = T264_quarter_pixel_search(t, s->src[i][4], s->ref[i][4], s->offset[i][4], &s->vec[i][4], &s->vec[i][4], s->sad[i][4], 4, 8, t->mb.pred_p8x8 + i / 2 * 16 * 8 + i % 2 * 8 + 4);
    t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 1].vec =
    t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 9].vec = s->vec[i][4];
    t->mb.vec[0][i / 2 * 8 + i % 2 * 2 + 1] = s->vec[i][4];
    t->mb.vec[0][i / 2 * 8 + i % 2 * 2 + 5] = s->vec[i][4];

    return s->sad[i][3] + s->sad[i][4];
}

uint32_t
T264_mode_decision_inter_4x4p(_RW T264_t * t, int32_t i, subpart_search_data_t* s)
{
    T264_vector_t vec[5 + 10];  // NOTE: max 10 refs
    T264_search_context_t context;
    int32_t num;

    get_pmv(t, vec, MB_4x4, luma_index[4 * i + 0], 1, &num);

    context.height = 4;
    context.width  = 4;
    context.limit_x= t->param.search_x;
    context.limit_y= t->param.search_y;
    context.vec    = vec;
    context.vec_num= num;
    context.offset = ((t->mb.mb_y << 4) + i / 2 * 8) * t->edged_stride + (t->mb.mb_x << 4) + i % 2 * 8;

    s->src[i][5] = t->mb.src_y + (i / 2 * 8) * t->stride + i % 2 * 8;
    s->sad[i][5] = t->search(t, &context);
    s->vec[i][5] = context.vec_best;
    s->offset[i][5] = context.offset;
    s->ref[i][5] = t->refl0[s->vec[i][5].refno];
    s->vec_median[i][5] = vec[0];

    s->sad[i][5] = T264_quarter_pixel_search(t, s->src[i][5], s->ref[i][5], s->offset[i][5], &s->vec[i][5], &s->vec[i][5], s->sad[i][5], 4, 4, t->mb.pred_p8x8 + i / 2 * 16 * 8 + i % 2 * 8);
    t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 0].vec =
    t->mb.vec[0][i / 2 * 8 + i % 2 * 2 + 0] = s->vec[i][5];
    get_pmv(t, vec, MB_4x4, luma_index[4 * i + 1], 1, &num);

    s->src[i][6] = s->src[i][5] + 4;
    context.offset += 4;
    s->sad[i][6] = t->search(t, &context);
    s->vec[i][6] = context.vec_best;
    s->offset[i][6] = context.offset;
    s->ref[i][6] = t->refl0[s->vec[i][6].refno];
    s->vec_median[i][6] = vec[0];

    s->sad[i][6] = T264_quarter_pixel_search(t, s->src[i][6], s->ref[i][6], s->offset[i][6], &s->vec[i][6], &s->vec[i][6], s->sad[i][6], 4, 4, t->mb.pred_p8x8 + i / 2 * 16 * 8 + i % 2 * 8 + 4);
    t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 1].vec =
    t->mb.vec[0][i / 2 * 8 + i % 2 * 2 + 1] = s->vec[i][6];
    get_pmv(t, vec, MB_4x4, luma_index[4 * i + 2], 1, &num);

    s->src[i][7] = s->src[i][5] + 4 * t->stride;
    context.offset += 4 * t->edged_stride - 4;
    s->sad[i][7] = t->search(t, &context);
    s->vec[i][7] = context.vec_best;
    s->offset[i][7] = context.offset;
    s->ref[i][7] = t->refl0[s->vec[i][7].refno];
    s->vec_median[i][7] = vec[0];

    s->sad[i][7] = T264_quarter_pixel_search(t, s->src[i][7], s->ref[i][7], s->offset[i][7], &s->vec[i][7], &s->vec[i][7], s->sad[i][7], 4, 4, t->mb.pred_p8x8 + i / 2 * 16 * 8 + i % 2 * 8 + 16 * 4);
    t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 8].vec =
    t->mb.vec[0][i / 2 * 8 + i % 2 * 2 + 4] = s->vec[i][7];
    get_pmv(t, vec, MB_4x4, luma_index[4 * i + 3], 1, &num);

    s->src[i][8] = s->src[i][7] + 4;
    context.offset += 4;
    s->sad[i][8] = t->search(t, &context);
    s->vec[i][8] = context.vec_best;
    s->offset[i][8] = context.offset;
    s->ref[i][8] = t->refl0[s->vec[i][8].refno];
    s->vec_median[i][8] = vec[0];

    s->sad[i][8] = T264_quarter_pixel_search(t, s->src[i][8], s->ref[i][8], s->offset[i][8], &s->vec[i][8], &s->vec[i][8], s->sad[i][8], 4, 4, t->mb.pred_p8x8 + i / 2 * 16 * 8 + i % 2 * 8 + 16 * 4 + 4);
    t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 9].vec =
    t->mb.vec[0][i / 2 * 8 + i % 2 * 2 + 5] = s->vec[i][8];

    return s->sad[i][5] + s->sad[i][6] + s->sad[i][7] + s->sad[i][8];
}

void
T264_encode_inter_16x16p(_RW T264_t* t, uint8_t* pred)
{
    DECLARE_ALIGNED_MATRIX(dct, 16, 16, int16_t, 16);

    int32_t qp = t->qp_y;
    int32_t i;
    int16_t* curdct;

    t->expand8to16sub(pred, 16 / 4, 16 / 4, dct, t->mb.src_y, t->stride);
    curdct = dct;
    for(i = 0 ; i < 16 ; i ++)
    {
        t->fdct4x4(curdct);

        t->quant4x4(curdct, qp, FALSE);
        scan_zig_4x4(t->mb.dct_y_z[luma_index[i]], curdct);
        t->iquant4x4(curdct, qp);
        t->idct4x4(curdct);

        curdct += 16;
    }

    t->contract16to8add(dct, 16 / 4, 16 / 4, pred, t->mb.dst_y, t->edged_stride);
}

void
T264_encode_inter_y(_RW T264_t* t)
{
    T264_encode_inter_16x16p(t, t->mb.pred_p16x16[t->mb.mb_part]);
}

// NOTE: this routine will merge with T264_encode_intra_uv
void
T264_transform_inter_uv(_RW T264_t* t, uint8_t* pred_u, uint8_t* pred_v)
{
    DECLARE_ALIGNED_MATRIX(dct, 10, 8, int16_t, CACHE_SIZE);

    int32_t qp = t->qp_uv;
    int32_t i, j;
    int16_t* curdct;
    uint8_t* start;
    uint8_t* dst;
    uint8_t* src;

    start = pred_u;
    src   = t->mb.src_u;
    dst   = t->mb.dst_u;
    for(j = 0 ; j < 2 ; j ++)
    {
        t->expand8to16sub(start, 8 / 4, 8 / 4, dct, src, t->stride_uv);
        curdct = dct;
        for(i = 0 ; i < 4 ; i ++)
        {
            t->fdct4x4(curdct);
            dct[64 + i] = curdct[0];

            t->quant4x4(curdct, qp, FALSE);
            scan_zig_4x4(t->mb.dct_uv_z[j][i], curdct);
            t->iquant4x4(curdct, qp);

            curdct += 16;
        }
        t->fdct2x2dc(curdct);
        t->quant2x2dc(curdct, qp, FALSE);
        scan_zig_2x2(t->mb.dc2x2_z[j], curdct);
        t->iquant2x2dc(curdct, qp);
        t->idct2x2dc(curdct);

        curdct = dct;
        for(i = 0 ; i < 4 ; i ++)
        {
            curdct[0] = dct[64 + i];
            t->idct4x4(curdct);
            curdct += 16;
        }

        t->contract16to8add(dct, 8 / 4, 8 / 4, start, dst, t->edged_stride_uv);

        //
        // change to v
        //
        start = pred_v;
        dst   = t->mb.dst_v;
        src   = t->mb.src_v;
    }
}

void
T264_encode_inter_uv(_RW T264_t* t)
{
    DECLARE_ALIGNED_MATRIX(pred_u, 8, 8, uint8_t, CACHE_SIZE);
    DECLARE_ALIGNED_MATRIX(pred_v, 8, 8, uint8_t, CACHE_SIZE);

    T264_vector_t vec;
    uint8_t* src, *dst;
    uint8_t* src_u, *dst_u;
    int32_t i;

    switch (t->mb.mb_part)
    {
    case MB_16x16:
        vec = t->mb.vec[0][0];
        src = t->refl0[vec.refno]->U + ((t->mb.mb_y << 3) + (vec.y >> 3)) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3);
        dst = pred_u;
        t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 8, 8);
        src = t->refl0[vec.refno]->V + ((t->mb.mb_y << 3) + (vec.y >> 3)) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3);
        dst = pred_v;
        t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 8, 8);
        break;
    case MB_16x8:
        vec = t->mb.vec[0][0];
        src_u = t->refl0[vec.refno]->U + ((t->mb.mb_y << 3) + (vec.y >> 3)) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3);
        dst_u = pred_u;
        t->eighth_pixel_mc_u(src_u, t->edged_stride_uv, dst_u, vec.x, vec.y, 8, 4);
        src = t->refl0[vec.refno]->V + ((t->mb.mb_y << 3) + (vec.y >> 3)) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3);
        dst = pred_v;
        t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 8, 4);

        vec = t->mb.vec[0][luma_index[8]];
        src_u = t->refl0[vec.refno]->U + ((t->mb.mb_y << 3) + (vec.y >> 3)) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) +
            4 * t->edged_stride_uv;
        dst_u += 4 * 8;
        t->eighth_pixel_mc_u(src_u, t->edged_stride_uv, dst_u, vec.x, vec.y, 8, 4);
        src = t->refl0[vec.refno]->V + ((t->mb.mb_y << 3) + (vec.y >> 3)) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + 
            4 * t->edged_stride_uv;
        dst += 4 * 8;
        t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 8, 4);
        break;
    case MB_8x16:
        vec = t->mb.vec[0][0];
        src_u = t->refl0[vec.refno]->U + ((t->mb.mb_y << 3) + (vec.y >> 3)) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3);
        dst_u = pred_u;
        t->eighth_pixel_mc_u(src_u, t->edged_stride_uv, dst_u, vec.x, vec.y, 4, 8);
        src = t->refl0[vec.refno]->V + ((t->mb.mb_y << 3) + (vec.y >> 3)) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3);
        dst = pred_v;
        t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 4, 8);

        vec = t->mb.vec[0][luma_index[4]];
        src_u = t->refl0[vec.refno]->U + ((t->mb.mb_y << 3) + (vec.y >> 3)) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + 4;
        dst_u += 4;
        t->eighth_pixel_mc_u(src_u, t->edged_stride_uv, dst_u, vec.x, vec.y, 4, 8);
        src = t->refl0[vec.refno]->V + ((t->mb.mb_y << 3) + (vec.y >> 3)) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + 4;
        dst += 4;
        t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 4, 8);
        break;
    case MB_8x8:
    case MB_8x8ref0:
        for(i = 0 ; i < 4 ; i ++)
        {
            switch(t->mb.submb_part[luma_index[4 * i]])
            {
            case MB_8x8:
                vec = t->mb.vec[0][luma_index[4 * i]];
                src = t->refl0[vec.refno]->U + ((t->mb.mb_y << 3) + (vec.y >> 3) + i / 2 * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + (i % 2 * 4);
                dst = pred_u + i / 2 * 32 + i % 2 * 4;
                t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 4, 4);
                src = t->refl0[vec.refno]->V + ((t->mb.mb_y << 3) + (vec.y >> 3) + i / 2 * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + (i % 2 * 4);
                dst = pred_v + i / 2 * 32 + i % 2 * 4;
                t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 4, 4);
                break;
            case MB_8x4:
                vec = t->mb.vec[0][luma_index[4 * i]];
                src_u = t->refl0[vec.refno]->U + ((t->mb.mb_y << 3) + (vec.y >> 3) + i / 2 * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + (i % 2 * 4);
                dst_u = pred_u + i / 2 * 32 + i % 2 * 4;
                t->eighth_pixel_mc_u(src_u, t->edged_stride_uv, dst_u, vec.x, vec.y, 4, 2);
                src = t->refl0[vec.refno]->V + ((t->mb.mb_y << 3) + (vec.y >> 3) + i / 2 * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + (i % 2 * 4);
                dst = pred_v + i / 2 * 32 + i % 2 * 4;
                t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 4, 2);

                vec = t->mb.vec[0][luma_index[4 * i + 2]];
                src_u = t->refl0[vec.refno]->U + ((t->mb.mb_y << 3) + (vec.y >> 3) + i / 2 * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + (i % 2 * 4) + 
                    2 * t->edged_stride_uv;
                dst_u += 2 * 8;
                t->eighth_pixel_mc_u(src_u, t->edged_stride_uv, dst_u, vec.x, vec.y, 4, 2);
                src = t->refl0[vec.refno]->V + ((t->mb.mb_y << 3) + (vec.y >> 3) + i / 2 * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + (i % 2 * 4) +
                    2 * t->edged_stride_uv;
                dst += 2 * 8;
                t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 4, 2);
                break;
            case MB_4x8:
                vec = t->mb.vec[0][luma_index[4 * i]];
                src_u = t->refl0[vec.refno]->U + ((t->mb.mb_y << 3) + (vec.y >> 3) + i / 2 * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + (i % 2 * 4);
                dst_u = pred_u + i / 2 * 32 + i % 2 * 4;
                t->eighth_pixel_mc_u(src_u, t->edged_stride_uv, dst_u, vec.x, vec.y, 2, 4);
                src = t->refl0[vec.refno]->V + ((t->mb.mb_y << 3) + (vec.y >> 3) + i / 2 * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + (i % 2 * 4);
                dst = pred_v + i / 2 * 32 + i % 2 * 4;
                t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 2, 4);

                vec = t->mb.vec[0][luma_index[4 * i + 1]];
                src_u = t->refl0[vec.refno]->U + ((t->mb.mb_y << 3) + (vec.y >> 3) + i / 2 * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + (i % 2 * 4) + 2;
                dst_u += 2;
                t->eighth_pixel_mc_u(src_u, t->edged_stride_uv, dst_u, vec.x, vec.y, 2, 4);
                src = t->refl0[vec.refno]->V + ((t->mb.mb_y << 3) + (vec.y >> 3) + i / 2 * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + (i % 2 * 4) + 2;
                dst += 2;
                t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 2, 4);
                break;
            case MB_4x4:
                vec = t->mb.vec[0][luma_index[4 * i]];
                src_u = t->refl0[vec.refno]->U + ((t->mb.mb_y << 3) + (vec.y >> 3) + i / 2 * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + (i % 2 * 4);
                dst_u = pred_u + i / 2 * 32 + i % 2 * 4;
                t->eighth_pixel_mc_u(src_u, t->edged_stride_uv, dst_u, vec.x, vec.y, 2, 2);
                src = t->refl0[vec.refno]->V + ((t->mb.mb_y << 3) + (vec.y >> 3) + i / 2 * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + (i % 2 * 4);
                dst = pred_v + i / 2 * 32 + i % 2 * 4;
                t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 2, 2);

                vec = t->mb.vec[0][luma_index[4 * i + 1]];
                src_u = t->refl0[vec.refno]->U + ((t->mb.mb_y << 3) + (vec.y >> 3) + i / 2 * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + (i % 2 * 4) + 2;
                dst_u += 2;
                t->eighth_pixel_mc_u(src_u, t->edged_stride_uv, dst_u, vec.x, vec.y, 2, 2);
                src = t->refl0[vec.refno]->V + ((t->mb.mb_y << 3) + (vec.y >> 3) + i / 2 * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + (i % 2 * 4) + 2;
                dst += 2;
                t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 2, 2);

                vec = t->mb.vec[0][luma_index[4 * i + 2]];
                src_u = t->refl0[vec.refno]->U + ((t->mb.mb_y << 3) + (vec.y >> 3) + i / 2 * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + (i % 2 * 4) + 
                    2 * t->edged_stride_uv;
                dst_u += 2 * 8 - 2;
                t->eighth_pixel_mc_u(src_u, t->edged_stride_uv, dst_u, vec.x, vec.y, 2, 2);
                src = t->refl0[vec.refno]->V + ((t->mb.mb_y << 3) + (vec.y >> 3) + i / 2 * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + (i % 2 * 4) + 
                    2 * t->edged_stride_uv;
                dst += 2 * 8 - 2;
                t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 2, 2);

                vec = t->mb.vec[0][luma_index[4 * i + 3]];
                src_u = t->refl0[vec.refno]->U + ((t->mb.mb_y << 3) + (vec.y >> 3) + i / 2 * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + (i % 2 * 4) +
                    2 * t->edged_stride_uv + 2;
                dst_u += 2;
                t->eighth_pixel_mc_u(src_u, t->edged_stride_uv, dst_u, vec.x, vec.y, 2, 2);
                src = t->refl0[vec.refno]->V + ((t->mb.mb_y << 3) + (vec.y >> 3) + i / 2 * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + (i % 2 * 4) + 
                    2 * t->edged_stride_uv + 2;
                dst += 2;
                t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 2, 2);
                break;
            default:
                break;
            }
        }
        break;
    default:
        break;
    }

    T264_transform_inter_uv(t, pred_u, pred_v);
}
