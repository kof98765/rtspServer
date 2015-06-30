/*****************************************************************************
*
*  T264 AVC CODEC
*
*  Copyright(C) 2004-2005 llcc <lcgate1@yahoo.com.cn>
*               2004-2005 visionany <visionany@yahoo.com.cn>
*	2005.1.5 CloudWu<sywu@sohu.com> Modify T264_get_direct_mv
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
#include "stdio.h"
#ifndef CHIP_DM642
#include "memory.h"
#endif
#include "T264.h"
#include "inter.h"
#include "inter_b.h"
#include "intra.h"
#include "estimation.h"
#include "utility.h"
#include "interpolate.h"
#include "bitstream.h"
#include <assert.h>
#include "../decoder/block.h"

static int32_t __inline
T264_detect_direct_16x16(T264_t* t)
{
    T264_vector_t vec_direct[2][16];

    T264_get_direct_mv(t, vec_direct);

    if (memcmp(vec_direct, t->mb.vec, sizeof(vec_direct)) == 0)
    {
        return 1;
    }

    return 0;
}

uint32_t
T264_mode_decision_interb_y(_RW T264_t* t)
{
    DECLARE_ALIGNED_MATRIX(pred_8x8, 16, 16, uint8_t, CACHE_SIZE);
    DECLARE_ALIGNED_MATRIX(pred_16x8, 16, 16, uint8_t, CACHE_SIZE);
    DECLARE_ALIGNED_MATRIX(pred_8x16, 16, 16, uint8_t, CACHE_SIZE);
	DECLARE_ALIGNED_MATRIX(pred_p16x16, 16, 16, uint8_t, CACHE_SIZE);
    uint32_t sad;
    uint32_t sad_16x8;
    uint32_t sad_8x16;
	uint32_t sad_16x16;
    uint32_t sad_min;
    uint8_t sub_part[4];
    uint8_t part;
    uint8_t part_16x8[2];
    uint8_t part_8x16[2];
    uint8_t* p_min = t->mb.pred_p16x16;
	uint32_t	i,j;

    T264_vector_t vec_16x16[2][2], vec_8x8[4][2];
    T264_vector_t vec_16x8[2][2];
    T264_vector_t vec_8x16[2][2];
    T264_vector_t vec_4x4[2][16];

    sad_min = T264_mode_decision_inter_16x16b(t, vec_16x16, t->mb.pred_p16x16, &part);
    t->mb.mb_mode = B_MODE;
    copy_nvec(&vec_16x16[0][0], &t->mb.vec[0][0], 4, 4, 4);
    copy_nvec(&vec_16x16[0][1], &t->mb.vec[1][0], 4, 4, 4);
    t->mb.mb_part = MB_16x16;
    t->mb.mb_part2[0] = part;
/*
    if (t->mb.is_copy)
    {
        t->mb.sad = sad_min;
        
        return sad_min;
    }
    */

    if (t->param.block_size & SEARCH_16x8B)
    {
        vec_16x8[0][0].refno = vec_16x16[0][0].refno;
        sad_16x8 = T264_mode_decision_inter_16x8b(t, vec_16x8, pred_16x8, part_16x8);
        if (sad_16x8 < sad_min)
        {
            p_min = pred_16x8;
            t->mb.mb_part = MB_16x8;
            t->mb.mb_part2[0] = part_16x8[0];
            t->mb.mb_part2[1] = part_16x8[1];
            sad_min = sad_16x8;
            copy_nvec(&vec_16x8[0][0], &t->mb.vec[0][0], 4, 2, 4);
            copy_nvec(&vec_16x8[0][1], &t->mb.vec[1][0], 4, 2, 4);
            copy_nvec(&vec_16x8[1][0], &t->mb.vec[0][8], 4, 2, 4);
            copy_nvec(&vec_16x8[1][1], &t->mb.vec[1][8], 4, 2, 4);
        }
    }

    if (t->param.block_size & SEARCH_8x16B)
    {
        vec_8x16[0][0].refno = vec_16x16[0][0].refno;
        sad_8x16 = T264_mode_decision_inter_8x16b(t, vec_8x16, pred_8x16, part_8x16);
        if (sad_8x16 < sad_min)
        {
            p_min = pred_8x16;
            t->mb.mb_part = MB_8x16;
            t->mb.mb_part2[0] = part_8x16[0];
            t->mb.mb_part2[1] = part_8x16[1];
            sad_min = sad_8x16;
            copy_nvec(&vec_8x16[0][0], &t->mb.vec[0][0], 2, 4, 4);
            copy_nvec(&vec_8x16[0][1], &t->mb.vec[1][0], 2, 4, 4);
            copy_nvec(&vec_8x16[1][0], &t->mb.vec[0][2], 2, 4, 4);
            copy_nvec(&vec_8x16[1][1], &t->mb.vec[1][2], 2, 4, 4);
        }
    }

    if (t->param.block_size & SEARCH_8x8B)
    {
        sad = 0;
        for(i = 0 ; i < 4 ; i ++)
        {
			vec_8x8[i][0].refno = vec_16x16[0][0].refno;
            sad += T264_mode_decision_inter_8x8b(t, i, vec_8x8[i], pred_8x8, &sub_part[i]);
            t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 0].vec[0] =
            t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 1].vec[0] =
            t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 8].vec[0] =
            t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 9].vec[0] = vec_8x8[i][0];

            t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 0].vec[1] =
            t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 1].vec[1] =
            t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 8].vec[1] =
            t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 9].vec[1] = vec_8x8[i][1];
        }

        if (sad < sad_min)
        {
            sad_min = sad;
            t->mb.mb_part = MB_8x8;
            p_min = pred_8x8;

            t->mb.submb_part[0] = sub_part[0];
            t->mb.submb_part[luma_index[4]] = sub_part[1];
            t->mb.submb_part[luma_index[8]] = sub_part[2];
            t->mb.submb_part[luma_index[12]] = sub_part[3];

            t->mb.vec[0][0] =
            t->mb.vec[0][1] = 
            t->mb.vec[0][4] = 
            t->mb.vec[0][5] = vec_8x8[0][0];
            t->mb.vec[1][0] =
            t->mb.vec[1][1] = 
            t->mb.vec[1][4] = 
            t->mb.vec[1][5] = vec_8x8[0][1];

            t->mb.vec[0][2] =
            t->mb.vec[0][3] = 
            t->mb.vec[0][6] = 
            t->mb.vec[0][7] = vec_8x8[1][0];
            t->mb.vec[1][2] =
            t->mb.vec[1][3] = 
            t->mb.vec[1][6] = 
            t->mb.vec[1][7] = vec_8x8[1][1];

            t->mb.vec[0][8] =
            t->mb.vec[0][9] = 
            t->mb.vec[0][12] = 
            t->mb.vec[0][13] = vec_8x8[2][0];
            t->mb.vec[1][8] =
            t->mb.vec[1][9] = 
            t->mb.vec[1][12] = 
            t->mb.vec[1][13] = vec_8x8[2][1];

            t->mb.vec[0][10] =
            t->mb.vec[0][11] = 
            t->mb.vec[0][14] = 
            t->mb.vec[0][15] = vec_8x8[3][0];
            t->mb.vec[1][10] =
            t->mb.vec[1][11] = 
            t->mb.vec[1][14] = 
            t->mb.vec[1][15] = vec_8x8[3][1];
        }
    }

	t->mb.is_copy = 0;
    sad_16x16 = T264_mode_decision_inter_direct_16x16b(t, vec_4x4, pred_p16x16, &part);
	if(sad_16x16 < sad_min)
	{
		for(i = 0;i < 2; ++i)
			for(j = 0;j < 16; ++j)
				t->mb.vec[i][j] = vec_4x4[i][j];
		p_min = pred_p16x16;
		t->mb.is_copy = 1;
		t->mb.mb_part = MB_16x16;
		t->mb.mb_part2[0] = part;	//part;
		sad_min = sad_16x16;
	}
    if (t->flags & USE_INTRAININTER)
        sad = T264_mode_decision_intra_y(t);
    else
        sad = -1;

    if (sad <= sad_min)
    {
		t->mb.is_copy = 0;
        sad_min = sad;
    }
    else
    {
        t->mb.mb_mode = B_MODE;
        if (p_min != t->mb.pred_p16x16)
            memcpy(t->mb.pred_p16x16, p_min, 16 * 16 * sizeof(uint8_t));
/*        if (t->mb.mb_part == MB_16x16)
            t->mb.is_copy = T264_detect_direct_16x16(t);*/
    }

    t->mb.sad = sad_min;

    return t->mb.sad;
}

#define MINPOSITIVE(x, y) (x >= 0 && y >= 0) ? T264_MIN(x, y) : T264_MAX(x, y)
static int32_t __inline
T264_get_ref_idx(T264_t* t, int32_t list)
{
    int32_t ref_idx;
    T264_vector_t vec[3];

    vec[0] = t->mb.vec_ref[VEC_LUMA - 1].vec[list];
    vec[1] = t->mb.vec_ref[VEC_LUMA - 8].vec[list];
    vec[2] = t->mb.vec_ref[VEC_LUMA - 4].vec[list];
    if (vec[2].refno == -2)
    {
        vec[2] = t->mb.vec_ref[VEC_LUMA - 8 - 1].vec[list];
    }

    ref_idx = MINPOSITIVE(vec[1].refno, vec[2].refno);
    ref_idx = MINPOSITIVE(vec[0].refno, ref_idx);

    return ref_idx;
}
#undef MINPOSITIVE

static void __inline
T264_try_get_col_zero_mv(T264_t* t, T264_vector_t vec_direct[2][16], int32_t refl0, int32_t refl1)
{
    int32_t i;
    T264_mb_context_t* mb_n = &t->ref[1][0]->mb[t->mb.mb_xy];

	if(refl0 == 0 || refl1 == 0)
    for (i = 0 ; i < 4 * 4 ; i ++)
    {
        if (((mb_n->vec[0][i].refno == 0) && 
            ABS(mb_n->vec[0][i].x) <= 1 &&
            ABS(mb_n->vec[0][i].y) <= 1) ||
			((mb_n->vec[0][i].refno == -1) && (mb_n->vec[1][i].refno == 0) &&
            ABS(mb_n->vec[1][i].x) <= 1 && ABS(mb_n->vec[1][i].y) <= 1))
        {
            if (refl0 == 0)
            {
                vec_direct[0][i].refno = 0;
                vec_direct[0][i].x = 0;
                vec_direct[0][i].y = 0;
            }
            if (refl1 == 0)
            {
                vec_direct[1][i].refno = 0;
                vec_direct[1][i].x = 0;
                vec_direct[1][i].y = 0;
            }
        }
    }
}

void
T264_get_direct_mv(T264_t* t, T264_vector_t vec_direct[2][16])
{
    int32_t refl0, refl1;
    uint8_t mb_part;

    if (t->param.direct_flag)
    {
        // l0
        refl0 = T264_get_ref_idx(t, 0);
        refl1 = T264_get_ref_idx(t, 1);

        // directZeroPredictionFlag = 1
        if (refl0 < 0 && refl1 < 0)
        {
			int	i,j;
            //memset(vec_direct, 0, sizeof(vec_direct));
			for(i = 0;i < 2;++i)
				for(j = 0;j < 16; ++j)
				{
					vec_direct[i][j].refno = 0;
						vec_direct[i][j].x = 0;
							vec_direct[i][j].y = 0;
				}
            return;
        }

	    mb_part = t->mb.mb_part;	//save old
	    t->mb.mb_part = MB_16x16;

        if (refl0 < 0)
        {
            vec_direct[0][0].refno = refl0;
            vec_direct[0][0].x = vec_direct[0][0].y = 0;
            copy_nvec(&vec_direct[0][0], &vec_direct[0][0], 4, 4, 4);
        }
        else
        {
            // when refl0 > 0 use T264_predict_mv
            T264_vector_t vec;

            vec.refno = refl0;
            T264_predict_mv(t, 0, 0, 4, &vec); 
            copy_nvec(&vec, &vec_direct[0][0], 4, 4, 4);
        }

        if (refl1 < 0)
        {
            vec_direct[1][0].refno = refl1;
            vec_direct[1][0].x = vec_direct[1][0].y = 0;
            copy_nvec(&vec_direct[1][0], &vec_direct[1][0], 4, 4, 4);
        }
        else
        {
            T264_vector_t vec;

            vec.refno = refl1;
            T264_predict_mv(t, 1, 0, 4, &vec); 
            copy_nvec(&vec, &vec_direct[1][0], 4, 4, 4);
        }

        // colZeroFlag
        T264_try_get_col_zero_mv(t, vec_direct, refl0, refl1);
		
		t->mb.mb_part = mb_part;	//restore
    }
    else
    {
        assert(0);
    }    
}

uint32_t
T264_mode_decision_inter_direct_16x16b(_RW T264_t* t, T264_vector_t predVec[2][4 * 4], uint8_t* pred, uint8_t* part)
{
    uint32_t sad;

	T264_get_direct_mv(t,predVec);
	T264_mb4x4_interb_mc(t,predVec,pred);
	sad = t->cmp[MB_16x16](pred,16,t->mb.src_y,t->stride);
	*part = B_DIRECT_16x16;

	return sad;
}

uint32_t
T264_mode_decision_inter_16x16b(_RW T264_t* t, T264_vector_t vec_best[2][2], uint8_t* pred, uint8_t* part)
{
    DECLARE_ALIGNED_MATRIX(pred_l1, 16, 16, uint8_t, CACHE_SIZE);
    DECLARE_ALIGNED_MATRIX(pred_bi, 16, 16, uint8_t, CACHE_SIZE);
    
    T264_vector_t vec[5 + 10];  // NOTE: max 10 refs
    T264_search_context_t context;
    int32_t num;
    uint32_t sad_min, sad;
    uint8_t* pred_l0 = pred;
    uint8_t* p_min = pred_l0;
    T264_vector_t vec_median;
    int32_t i;
    uint8_t* p_buf = pred_l1;

    context.height = 16;
    context.width  = 16;
    context.limit_x= t->param.search_x;
    context.limit_y= t->param.search_y;
    context.vec    = vec;
    context.mb_part= MB_16x16;
    context.offset = (t->mb.mb_y << 4) * t->edged_stride + (t->mb.mb_x << 4);

    // list 0
    vec[0].refno = 0;
    get_pmv(t, 0, vec, MB_16x16, 0, 4, &num);
    context.vec_num= num;
    context.list_index = 0;

    sad_min = t->search(t, &context);
    sad_min+= REFCOST(context.vec_best.refno);
    sad_min = T264_quarter_pixel_search(t, 0, t->mb.src_y, t->ref[0][context.vec_best.refno], context.offset, 
        &context.vec_best, &vec[0], sad_min, 16, 16, p_min, MB_16x16);
    for(i = 1 ; i < t->refl0_num ; i ++)
    {
        T264_vector_t vec_best_bak, vec_median_bak;
        
        vec_median_bak = vec[0];
        vec_best_bak = context.vec_best;
        
        vec[0].refno = i;
        get_pmv(t, 0, vec, MB_16x16, 0, 4, &num);
        context.vec_num= 1;
        sad = t->search(t, &context);
        sad+= REFCOST(context.vec_best.refno);
        sad = T264_quarter_pixel_search(t, 0, t->mb.src_y, t->ref[0][i], context.offset, 
            &context.vec_best, &vec[0], sad, 16, 16, p_buf, MB_16x16);
        if (sad < sad_min)
        {
            SWAP(uint8_t, p_buf, p_min);
            sad_min = sad;
        }
        else
        {
            vec[0] = vec_median_bak;
            context.vec_best = vec_best_bak;
        }
    }
    if (p_min != pred_l0)
    {
        memcpy(pred_l0, p_min, sizeof(uint8_t) * 16 * 16);
    }

    vec_best[0][0] = context.vec_best;
    vec_median = vec[0];
    *part = B_L0_16x16;

    // list 1
    vec_best[0][1].refno = -1;
    vec_best[0][1].x = 0;
    vec_best[0][1].y = 0;
    if (t->refl1_num > 0)
    {
        uint32_t sad_l1, sad_bi;

        vec[0].refno = 0;
        get_pmv(t, 1, vec, MB_16x16, 0, 4, &num);
        context.vec_num = num;
        context.list_index = 1;

        sad_l1 = t->search(t, &context);
        sad_l1+= REFCOST1(context.vec_best.refno);
        sad_l1 = T264_quarter_pixel_search(t, 1, t->mb.src_y, t->ref[1][context.vec_best.refno], context.offset, 
            &context.vec_best, &vec[0], sad_l1, 16, 16, pred_l1, MB_16x16);
        vec_best[0][1] = context.vec_best;

        t->pia[MB_16x16](pred_l0, pred_l1, 16, 16, pred_bi, 16);
        sad_bi = t->cmp[MB_16x16](t->mb.src_y, t->stride, pred_bi, 16) + t->mb.lambda *
            (eg_size_se(t->bs, vec_best[0][0].x - vec_median.x) +
             eg_size_se(t->bs, vec_best[0][0].y - vec_median.y) +
             eg_size_se(t->bs, vec_best[0][1].x - vec[0].x) +
             eg_size_se(t->bs, vec_best[0][1].y - vec[0].y)) +
             REFCOST(vec_best[0][0].refno);

        if (sad_bi < sad_min)
        {
            sad_min = sad_bi;
            *part = B_Bi_16x16;
            memcpy(pred_l0, pred_bi, 256 * sizeof(uint8_t));
        }
        if (sad_l1 < sad_min)
        {
            sad_min = sad_l1;
            *part = B_L1_16x16;
            memcpy(pred_l0, pred_l1, 256 * sizeof(uint8_t));

            vec_best[0][0].refno = -1;
            vec_best[0][0].x = 0;
            vec_best[0][0].y = 0;
        }
        if (*part == B_L0_16x16)
        {
            vec_best[0][1].refno = -1;
            vec_best[0][1].x = 0;
            vec_best[0][1].y = 0;
        }
    }
//    t->mb.is_copy = T264_detect_direct_16x16(t, vec_best[0]);

    return sad_min;
}

uint32_t
T264_mode_decision_inter_16x8b(_RW T264_t* t, T264_vector_t vec_best[2][2], uint8_t* pred, uint8_t* part)
{
    DECLARE_ALIGNED_MATRIX(pred_l1_m, 16, 16, uint8_t, CACHE_SIZE);
    DECLARE_ALIGNED_MATRIX(pred_bi_m, 16, 16, uint8_t, CACHE_SIZE);

    T264_vector_t vec[5 + 10];  // NOTE: max 10 refs
    T264_search_context_t context;
    int32_t num;
    uint32_t sad_min;
    uint32_t sad_all = 0;
    uint8_t* pred_l0 = pred;
    uint8_t* pred_l1 = pred_l1_m;
    uint8_t* pred_bi = pred_bi_m;
    T264_vector_t vec_median;
    uint8_t old_part = t->mb.mb_part;
    int32_t i;
    uint8_t* src;

    t->mb.mb_part = MB_16x8;

    context.height = 8;
    context.width  = 16;
    context.limit_x= t->param.search_x;
    context.limit_y= t->param.search_y;
    context.vec    = vec;
    context.mb_part= MB_16x8;
    context.offset = (t->mb.mb_y << 4) * t->edged_stride + (t->mb.mb_x << 4);
    src = t->mb.src_y;

    for (i = 0 ; i < 2 ; i ++)
    {
        // list 0
        vec[0].refno = vec_best[0][0].refno;
        get_pmv(t, 0, vec, MB_16x8, luma_index[8 * i], 4, &num);
        context.vec_num= num;
        context.list_index = 0;

        sad_min = t->search(t, &context);
        sad_min+= REFCOST(context.vec_best.refno);
        sad_min = T264_quarter_pixel_search(t, 0, src, t->ref[0][context.vec_best.refno], context.offset, 
            &context.vec_best, &vec[0], sad_min, 16, 8, pred_l0, MB_16x8);
        vec_best[i][0] = context.vec_best;
        vec_median = vec[0];
        part[i] = B_L0_16x8;

        // list 1
        vec_best[i][1].refno = -1;
        vec_best[i][1].x = 0;
        vec_best[i][1].y = 0;
        if (t->refl1_num > 0)
        {
            uint32_t sad_l1, sad_bi;

            vec[0].refno = 0;
            get_pmv(t, 1, vec, MB_16x8, luma_index[8 * i], 4, &num);
            context.vec_num = num;
            context.list_index = 1;

            sad_l1 = t->search(t, &context);
            sad_l1+= REFCOST1(context.vec_best.refno);
            sad_l1 = T264_quarter_pixel_search(t, 1, src, t->ref[1][context.vec_best.refno], context.offset, 
                &context.vec_best, &vec[0], sad_l1, 16, 8, pred_l1, MB_16x8);
            vec_best[i][1] = context.vec_best;

            t->pia[MB_16x8](pred_l0, pred_l1, 16, 16, pred_bi, 16);
            sad_bi = t->cmp[MB_16x8](src, t->stride, pred_bi, 16) + t->mb.lambda *
                (eg_size_se(t->bs, vec_best[i][0].x - vec_median.x) +
                eg_size_se(t->bs, vec_best[i][0].y - vec_median.y) +
                eg_size_se(t->bs, vec_best[i][1].x - vec[0].x) +
                eg_size_se(t->bs, vec_best[i][1].y - vec[0].y)) +
                REFCOST(vec_best[i][0].refno);

            if (sad_bi < sad_min)
            {
                sad_min = sad_bi;
                part[i] = B_Bi_16x8;
                memcpy(pred_l0, pred_bi, sizeof(uint8_t) * 16 * 8);
            }
            if (sad_l1 < sad_min)
            {
                sad_min = sad_l1;
                part[i] = B_L1_16x8;
                memcpy(pred_l0, pred_l1, sizeof(uint8_t) * 16 * 8);

                vec_best[i][0].refno = -1;
                vec_best[i][0].x = 0;
                vec_best[i][0].y = 0;
            }
            if (part[i] == B_L0_16x8)
            {
                vec_best[i][1].refno = -1;
                vec_best[i][1].x = 0;
                vec_best[i][1].y = 0;
            }
        }

        pred_l0 += 16 * 8;
        pred_l1 += 16 * 8;
        pred_bi += 16 * 8;
        t->mb.vec_ref[VEC_LUMA + 8].vec[0] = vec_best[i][0];
        t->mb.vec_ref[VEC_LUMA + 8].vec[1] = vec_best[i][1];
        context.offset += 8 * t->edged_stride;
        src += 8 * t->stride;
        sad_all += sad_min;
    }
    t->mb.mb_part = old_part;

    return sad_all;
}

uint32_t
T264_mode_decision_inter_8x16b(_RW T264_t* t, T264_vector_t vec_best[2][2], uint8_t* pred, uint8_t* part)
{
    DECLARE_ALIGNED_MATRIX(pred_l1_m, 16, 16, uint8_t, CACHE_SIZE);
    DECLARE_ALIGNED_MATRIX(pred_bi_m, 16, 16, uint8_t, CACHE_SIZE);

    T264_vector_t vec[5 + 10];  // NOTE: max 10 refs
    T264_search_context_t context;
    int32_t num;
    uint32_t sad_min;
    uint32_t sad_all = 0;
    uint8_t* pred_l0 = pred;
    uint8_t* pred_l1 = pred_l1_m;
    uint8_t* pred_bi = pred_bi_m;
    T264_vector_t vec_median;
    uint8_t old_part = t->mb.mb_part;
    int32_t i;
    uint8_t* src;

    t->mb.mb_part = MB_8x16;

    context.height = 16;
    context.width  = 8;
    context.limit_x= t->param.search_x;
    context.limit_y= t->param.search_y;
    context.vec    = vec;
    context.mb_part= MB_8x16;
    context.offset = (t->mb.mb_y << 4) * t->edged_stride + (t->mb.mb_x << 4);
    src = t->mb.src_y;

    for (i = 0 ; i < 2 ; i ++)
    {
        // list 0
        vec[0].refno = vec_best[0][0].refno;
        get_pmv(t, 0, vec, MB_8x16, luma_index[4 * i], 2, &num);
        context.vec_num= num;
        context.list_index = 0;

        sad_min = t->search(t, &context);
        sad_min+= REFCOST(context.vec_best.refno);
        sad_min = T264_quarter_pixel_search(t, 0, src, t->ref[0][context.vec_best.refno], context.offset, 
            &context.vec_best, &vec[0], sad_min, 8, 16, pred_l0, MB_8x16);
        vec_best[i][0] = context.vec_best;
        vec_median = vec[0];
        part[i] = B_L0_8x16;

        // list 1
        vec_best[i][1].refno = -1;
        vec_best[i][1].x = 0;
        vec_best[i][1].y = 0;
        if (t->refl1_num > 0)
        {
            uint32_t sad_l1, sad_bi;

            vec[0].refno = 0;
            get_pmv(t, 1, vec, MB_8x16, luma_index[4 * i], 2, &num);
            context.vec_num = num;
            context.list_index = 1;

            sad_l1 = t->search(t, &context);
            sad_l1+= REFCOST1(context.vec_best.refno);
            sad_l1 = T264_quarter_pixel_search(t, 1, src, t->ref[1][context.vec_best.refno], context.offset, 
                &context.vec_best, &vec[0], sad_l1, 8, 16, pred_l1, MB_8x16);
            vec_best[i][1] = context.vec_best;

            t->pia[MB_8x16](pred_l0, pred_l1, 16, 16, pred_bi, 16);
            sad_bi = t->cmp[MB_8x16](src, t->stride, pred_bi, 16) + t->mb.lambda *
                (eg_size_se(t->bs, vec_best[i][0].x - vec_median.x) +
                eg_size_se(t->bs, vec_best[i][0].y - vec_median.y) +
                eg_size_se(t->bs, vec_best[i][1].x - vec[0].x) +
                eg_size_se(t->bs, vec_best[i][1].y - vec[0].y)) +
                REFCOST(vec_best[i][0].refno);

            if (sad_bi < sad_min)
            {
                sad_min = sad_bi;
                part[i] = B_Bi_8x16;
                t->memcpy_stride_u(pred_bi, 8, 16, 16, pred_l0, 16);
            }
            if (sad_l1 < sad_min)
            {
                sad_min = sad_l1;
                part[i] = B_L1_8x16;
                t->memcpy_stride_u(pred_l1, 8, 16, 16, pred_l0, 16);

                vec_best[i][0].refno = -1;
                vec_best[i][0].x = 0;
                vec_best[i][0].y = 0;
            }
            if (part[i] == B_L0_8x16)
            {
                vec_best[i][1].refno = -1;
                vec_best[i][1].x = 0;
                vec_best[i][1].y = 0;
            }
        }

        pred_l0 += 8;
        pred_l1 += 8;
        pred_bi += 8;
        t->mb.vec_ref[VEC_LUMA + 1].vec[0] = vec_best[i][0];
        t->mb.vec_ref[VEC_LUMA + 1].vec[1] = vec_best[i][1];
        context.offset += 8;
        src += 8;
        sad_all += sad_min;
    }
    t->mb.mb_part = old_part;

    return sad_all;
}

uint32_t
T264_mode_decision_inter_8x8b(_RW T264_t * t, int32_t i, T264_vector_t vec_best[2], uint8_t* pred, uint8_t* part)
{
    DECLARE_ALIGNED_MATRIX(pred_l1, 16, 16, uint8_t, CACHE_SIZE);
    DECLARE_ALIGNED_MATRIX(pred_bi, 16, 16, uint8_t, CACHE_SIZE);

    T264_vector_t vec[5 + 10];  // NOTE: max 10 refs
    T264_search_context_t context;
    int32_t num;
    uint32_t sad_min;
    T264_vector_t vec_median;
    uint8_t* pred_l0 = pred;
    uint8_t* src = t->mb.src_y + (i / 2 * 8) * t->stride + i % 2 * 8;
    uint8_t* dst = pred + i / 2 * 16 * 8 + i % 2 * 8;

    context.height = 8;
    context.width  = 8;
    context.limit_x= t->param.search_x;
    context.limit_y= t->param.search_y;
    context.vec    = vec;
    context.mb_part= MB_8x8;
    context.offset = ((t->mb.mb_y << 4) + i / 2 * 8) * t->edged_stride + (t->mb.mb_x << 4) + i % 2 * 8;

    vec[0].refno = vec_best[0].refno;
    get_pmv(t, 0, vec, MB_8x8, luma_index[4 * i], 2, &num);
    context.vec_num= num;
    context.list_index = 0;

    sad_min = t->search(t, &context);
    sad_min+= eg_size_ue(t->bs, B_L0_8x8) + REFCOST(context.vec_best.refno);
    sad_min = T264_quarter_pixel_search(t, 0, src, t->ref[0][context.vec_best.refno], context.offset, 
        &context.vec_best, &vec[0], sad_min, 8, 8, dst, MB_8x8);
    vec_best[0] = context.vec_best;
    vec_median = vec[0];
    *part = B_L0_8x8;

    // list 1
    vec_best[1].refno = -1;
    vec_best[1].x = 0;
    vec_best[1].y = 0;
    if (t->refl1_num > 0)
    {
        uint32_t sad_l1, sad_bi;

        vec[0].refno = 0;
        get_pmv(t, 1, vec, MB_8x8, luma_index[4 * i], 2, &num);
        context.vec_num= num;
        context.list_index = 1;

        sad_l1 = t->search(t, &context);
        sad_l1+= eg_size_ue(t->bs, B_L1_8x8) + REFCOST1(context.vec_best.refno);
        sad_l1 = T264_quarter_pixel_search(t, 1, src, t->ref[1][context.vec_best.refno], context.offset, 
            &context.vec_best, &vec[0], sad_l1, 8, 8, pred_l1, MB_8x8);
        vec_best[1] = context.vec_best;

        t->pia[MB_8x8](dst, pred_l1, 16, 16, pred_bi, 16);
        sad_bi = t->cmp[MB_8x8](src, t->stride, pred_bi, 16) + t->mb.lambda *
           (eg_size_se(t->bs, vec_best[0].x - vec_median.x) +
            eg_size_se(t->bs, vec_best[0].y - vec_median.y) +
            eg_size_se(t->bs, vec_best[1].x - vec[0].x) +
            eg_size_se(t->bs, vec_best[1].y - vec[0].y) + 
            2 * T264_MIN(context.vec_best.refno, 1)) +
            eg_size_ue(t->bs, B_Bi_8x8) +
            REFCOST(vec_best[0].refno);
        if (sad_bi < sad_min)
        {
            sad_min = sad_bi;
            *part = B_Bi_8x8;
            t->memcpy_stride_u(pred_bi, 8, 8, 16, dst, 16);
        }
        if (sad_l1 < sad_min)
        {
            sad_min = sad_l1;
            *part = B_L1_8x8;
            t->memcpy_stride_u(pred_l1, 8, 8, 16, dst, 16);
            
            vec_best[0].refno = -1;
            vec_best[0].x = 0;
            vec_best[0].y = 0;
        }
        if (*part == B_L0_8x8)
        {
            vec_best[1].refno = -1;
            vec_best[1].x = 0;
            vec_best[1].y = 0;
        }
    }

    return sad_min;
}

void
T264_encode_interb_uv(_RW T264_t* t)
{
    DECLARE_ALIGNED_MATRIX(pred_u, 8, 8, uint8_t, CACHE_SIZE);
    DECLARE_ALIGNED_MATRIX(pred_v, 8, 8, uint8_t, CACHE_SIZE);
    DECLARE_ALIGNED_MATRIX(pred_u_l0, 8, 8, uint8_t, CACHE_SIZE);
    DECLARE_ALIGNED_MATRIX(pred_v_l0, 8, 8, uint8_t, CACHE_SIZE);
    DECLARE_ALIGNED_MATRIX(pred_u_l1, 8, 8, uint8_t, CACHE_SIZE);
    DECLARE_ALIGNED_MATRIX(pred_v_l1, 8, 8, uint8_t, CACHE_SIZE);

    T264_vector_t vec;
    uint8_t* src, *dst;
    int32_t i;

	if(t->mb.is_copy)
	{			
		T264_mb4x4_interb_uv_mc(t,t->mb.vec,pred_u,pred_v);
	}else
    switch (t->mb.mb_part)
    {
    case MB_16x16:
        {
            switch (t->mb.mb_part2[0])
            {
            case B_L0_16x16:
                vec = t->mb.vec[0][0];
                src = t->ref[0][vec.refno]->U + ((t->mb.mb_y << 3) + (vec.y >> 3)) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3);
                dst = pred_u;
                t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 8, 8);
                src = t->ref[0][vec.refno]->V + ((t->mb.mb_y << 3) + (vec.y >> 3)) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3);
                dst = pred_v;
                t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 8, 8);
                break;
            case B_L1_16x16:
                vec = t->mb.vec[1][0];
                src = t->ref[1][vec.refno]->U + ((t->mb.mb_y << 3) + (vec.y >> 3)) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3);
                dst = pred_u;
                t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 8, 8);
                src = t->ref[1][vec.refno]->V + ((t->mb.mb_y << 3) + (vec.y >> 3)) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3);
                dst = pred_v;
                t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 8, 8);
                break;
            case B_Bi_16x16:
                vec = t->mb.vec[0][0];
                src = t->ref[0][vec.refno]->U + ((t->mb.mb_y << 3) + (vec.y >> 3)) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3);
                dst = pred_u_l0;
                t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 8, 8);
                src = t->ref[0][vec.refno]->V + ((t->mb.mb_y << 3) + (vec.y >> 3)) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3);
                dst = pred_v_l0;
                t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 8, 8);

                vec = t->mb.vec[1][0];
                src = t->ref[1][vec.refno]->U + ((t->mb.mb_y << 3) + (vec.y >> 3)) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3);
                dst = pred_u_l1;
                t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 8, 8);
                src = t->ref[1][vec.refno]->V + ((t->mb.mb_y << 3) + (vec.y >> 3)) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3);
                dst = pred_v_l1;
                t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 8, 8);

                t->pia[MB_8x8](pred_u_l0, pred_u_l1, 8, 8, pred_u, 8);
                t->pia[MB_8x8](pred_v_l0, pred_v_l1, 8, 8, pred_v, 8);
                break;
            }
        }
        break;
    case MB_16x8:
        {
            for (i = 0 ; i < 2 ; i ++)
            {
                switch (t->mb.mb_part2[i])
                {
                case B_L0_16x8:
                    vec = t->mb.vec[0][i * 8];
                    src = t->ref[0][vec.refno]->U + ((t->mb.mb_y << 3) + (vec.y >> 3) + i * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3);
                    dst = pred_u + i * 8 * 4;
                    t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 8, 4);
                    src = t->ref[0][vec.refno]->V + ((t->mb.mb_y << 3) + (vec.y >> 3) + i * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3);
                    dst = pred_v + i * 8 * 4;
                    t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 8, 4);
                    break;
                case B_L1_16x8:
                    vec = t->mb.vec[1][i * 8];
                    src = t->ref[1][vec.refno]->U + ((t->mb.mb_y << 3) + (vec.y >> 3) + i * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3);
                    dst = pred_u + i * 8 * 4;
                    t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 8, 4);
                    src = t->ref[1][vec.refno]->V + ((t->mb.mb_y << 3) + (vec.y >> 3) + i * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3);
                    dst = pred_v + i * 8 * 4;
                    t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 8, 4);
                    break;
                case B_Bi_16x8:
                    vec = t->mb.vec[0][i * 8];
                    src = t->ref[0][vec.refno]->U + ((t->mb.mb_y << 3) + (vec.y >> 3) + i * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3);
                    dst = pred_u_l0 + i * 8 * 4;
                    t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 8, 4);
                    src = t->ref[0][vec.refno]->V + ((t->mb.mb_y << 3) + (vec.y >> 3) + i * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3);
                    dst = pred_v_l0 + i * 8 * 4;
                    t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 8, 4);

                    vec = t->mb.vec[1][i * 8];
                    src = t->ref[1][vec.refno]->U + ((t->mb.mb_y << 3) + (vec.y >> 3) + i * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3);
                    dst = pred_u_l1 + i * 8 * 4;
                    t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 8, 4);
                    src = t->ref[1][vec.refno]->V + ((t->mb.mb_y << 3) + (vec.y >> 3) + i * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3);
                    dst = pred_v_l1 + i * 8 * 4;
                    t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 8, 4);

                    t->pia[MB_8x4](pred_u_l0 + i * 8 * 4, pred_u_l1 + i * 8 * 4, 8, 8, pred_u + i * 8 * 4, 8);
                    t->pia[MB_8x4](pred_v_l0 + i * 8 * 4, pred_v_l1 + i * 8 * 4, 8, 8, pred_v + i * 8 * 4, 8);
                    break;
                }
            }
        }
        break;
    case MB_8x16:
        {
            for (i = 0 ; i < 2 ; i ++)
            {
                switch (t->mb.mb_part2[i])
                {
                case B_L0_8x16:
                    vec = t->mb.vec[0][i * 2];
                    src = t->ref[0][vec.refno]->U + ((t->mb.mb_y << 3) + (vec.y >> 3)) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + i * 4;
                    dst = pred_u + i * 4;
                    t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 4, 8);
                    src = t->ref[0][vec.refno]->V + ((t->mb.mb_y << 3) + (vec.y >> 3)) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + i * 4;
                    dst = pred_v + i * 4;
                    t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 4, 8);
                    break;
                case B_L1_8x16:
                    vec = t->mb.vec[1][i * 2];
                    src = t->ref[1][vec.refno]->U + ((t->mb.mb_y << 3) + (vec.y >> 3)) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + i * 4;
                    dst = pred_u + i * 4;
                    t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 4, 8);
                    src = t->ref[1][vec.refno]->V + ((t->mb.mb_y << 3) + (vec.y >> 3)) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + i * 4;
                    dst = pred_v + i * 4;
                    t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 4, 8);
                    break;
                case B_Bi_8x16:
                    vec = t->mb.vec[0][i * 2];
                    src = t->ref[0][vec.refno]->U + ((t->mb.mb_y << 3) + (vec.y >> 3)) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + i * 4;
                    dst = pred_u_l0 + i * 4;
                    t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 4, 8);
                    src = t->ref[0][vec.refno]->V + ((t->mb.mb_y << 3) + (vec.y >> 3)) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + i * 4;
                    dst = pred_v_l0 + i * 4;
                    t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 4, 8);

                    vec = t->mb.vec[1][i * 2];
                    src = t->ref[1][vec.refno]->U + ((t->mb.mb_y << 3) + (vec.y >> 3)) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + i * 4;
                    dst = pred_u_l1 + i * 4;
                    t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 4, 8);
                    src = t->ref[1][vec.refno]->V + ((t->mb.mb_y << 3) + (vec.y >> 3)) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + i * 4;
                    dst = pred_v_l1 + i * 4;
                    t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 4, 8);

                    t->pia[MB_4x8](pred_u_l0 + i * 4, pred_u_l1 + i * 4, 8, 8, pred_u + i * 4, 8);
                    t->pia[MB_4x8](pred_v_l0 + i * 4, pred_v_l1 + i * 4, 8, 8, pred_v + i * 4, 8);
                    break;
                }
            }
        }
        break;
    case MB_8x8:
        for(i = 0 ; i < 4 ; i ++)
        {
            int32_t offset = i / 2 * 32 + i % 2 * 4;
            switch(t->mb.submb_part[luma_index[4 * i]])
            {
            case B_L0_8x8:
                vec = t->mb.vec[0][luma_index[4 * i]];
                src = t->ref[0][vec.refno]->U + ((t->mb.mb_y << 3) + (vec.y >> 3) + i / 2 * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + (i % 2 * 4);
                dst = pred_u + offset;
                t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 4, 4);
                src = t->ref[0][vec.refno]->V + ((t->mb.mb_y << 3) + (vec.y >> 3) + i / 2 * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + (i % 2 * 4);
                dst = pred_v + offset;
                t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 4, 4);
                break;
            case B_L1_8x8:
                vec = t->mb.vec[1][luma_index[4 * i]];
                src = t->ref[1][vec.refno]->U + ((t->mb.mb_y << 3) + (vec.y >> 3) + i / 2 * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + (i % 2 * 4);
                dst = pred_u + offset;
                t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 4, 4);
                src = t->ref[1][vec.refno]->V + ((t->mb.mb_y << 3) + (vec.y >> 3) + i / 2 * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + (i % 2 * 4);
                dst = pred_v + offset;
                t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 4, 4);
                break;
            case B_Bi_8x8:
                vec = t->mb.vec[0][luma_index[4 * i]];
                src = t->ref[0][vec.refno]->U + ((t->mb.mb_y << 3) + (vec.y >> 3) + i / 2 * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + (i % 2 * 4);
                dst = pred_u_l0 + offset;
                t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 4, 4);
                src = t->ref[0][vec.refno]->V + ((t->mb.mb_y << 3) + (vec.y >> 3) + i / 2 * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + (i % 2 * 4);
                dst = pred_v_l0 + offset;
                t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 4, 4);

                vec = t->mb.vec[1][luma_index[4 * i]];
                src = t->ref[1][vec.refno]->U + ((t->mb.mb_y << 3) + (vec.y >> 3) + i / 2 * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + (i % 2 * 4);
                dst = pred_u_l1 + offset;
                t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 4, 4);
                src = t->ref[1][vec.refno]->V + ((t->mb.mb_y << 3) + (vec.y >> 3) + i / 2 * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + (i % 2 * 4);
                dst = pred_v_l1 + offset;
                t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 4, 4);

                t->pia[MB_4x4](pred_u_l0 + offset, pred_u_l1 + offset, 8, 8, pred_u + offset, 8);
                t->pia[MB_4x4](pred_v_l0 + offset, pred_v_l1 + offset, 8, 8, pred_v + offset, 8);
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
