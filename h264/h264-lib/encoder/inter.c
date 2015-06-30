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
#include "portab.h"
#include "stdio.h"
#ifndef CHIP_DM642
#include "memory.h"
#endif
#include "T264.h"
#include "inter.h"
#include "intra.h"
#include "estimation.h"
#include "utility.h"
#include "interpolate.h"
#include "bitstream.h"

//#define USE_PREV_DETECT

uint32_t
T264_predict_sad(T264_t* t, int32_t list)
{
    return T264_median(t->mb.sad_ref[0], t->mb.sad_ref[1], t->mb.sad_ref[2]);
}

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

    vec_n[0] = t->mb.vec_ref[VEC_LUMA - 1 + row * 8 + col].vec[list];
    vec_n[1] = t->mb.vec_ref[VEC_LUMA - 8 + row * 8 + col].vec[list];
    vec_n[2] = t->mb.vec_ref[VEC_LUMA - 8 + row * 8 + col + width].vec[list];
    if (vec_n[2].refno == -2)
    {
        vec_n[2] = t->mb.vec_ref[VEC_LUMA - 8 + row * 8 + col - 1].vec[list];
    }
    if (((i & 3) == 3) || ((i & 3) == 2 && width == 2))
    {
        vec_n[2] = t->mb.vec_ref[VEC_LUMA - 8 + row * 8 + col - 1].vec[list];
    }
    if (t->mb.mb_part == MB_16x8)
    {
        if (i == 0 && n == vec_n[1].refno)
        {
            vec[0].x = vec_n[1].x;
            vec[0].y = vec_n[1].y;
            return;
        }
        else if (i != 0 && n == vec_n[0].refno)
        {
            vec[0].x = vec_n[0].x;
            vec[0].y = vec_n[0].y;
            return;
        }
    }
    else if (t->mb.mb_part == MB_8x16)
    {
        if (i == 0 && n == vec_n[0].refno)
        {
            vec[0].x = vec_n[0].x;
            vec[0].y = vec_n[0].y;
            return;
        }
        else if (i != 0 && n == vec_n[2].refno)
        {
            vec[0].x = vec_n[2].x;
            vec[0].y = vec_n[2].y;
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
        vec[0].x = vec_n[idx].x;
        vec[0].y = vec_n[idx].y;
        return;
    }
    else if (vec_n[1].refno == -2 && vec_n[2].refno == -2 && vec_n[0].refno != -2)
    {
        vec[0].x = vec_n[0].x;
        vec[0].y = vec_n[0].y;
    }
    else
    {
        vec[0].x = T264_median(vec_n[0].x, vec_n[1].x, vec_n[2].x);
        vec[0].y = T264_median(vec_n[0].y, vec_n[1].y, vec_n[2].y);
        return;        
    }
}

void
T264_predict_mv_skip(T264_t* t, int32_t list, T264_vector_t* vec)
{
    T264_vector_t vec_n[2];
    int32_t zero_left, zero_top;

    vec_n[0] = t->mb.vec_ref[VEC_LUMA - 1].vec[0];
    vec_n[1] = t->mb.vec_ref[VEC_LUMA - 8].vec[0];
    vec[0].refno = 0;

    zero_left = vec_n[0].refno == -2 ? 1 : vec_n[0].refno == 0 && vec_n[0].x == 0 && vec_n[0].y == 0 ? 1 : 0;
    zero_top  = vec_n[1].refno == -2 ? 1 : vec_n[1].refno == 0 && vec_n[1].x == 0 && vec_n[1].y == 0 ? 1 : 0;

    if (zero_left || zero_top)
    {
        vec[0].x = vec[0].y = 0;
    }
    else
    {
        T264_predict_mv(t, 0, 0, 4, vec);
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

int32_t
T264_get_pos_sad(T264_t* t, uint8_t* ref, T264_vector_t* vec)
{
    uint8_t* tmp;
    int32_t x, y;
    int32_t list_index = 0;
    uint32_t sad;

    static const int8_t index[4][4][6] = 
    {
        {{0, 0, 0, 0, 0, 0}, {0, 1, 0, 0, 0, 0}, {1, 1, 0, 0, 0, 0}, {1, 0, 0, 0, 1, 0}},
        {{0, 2, 0, 0, 0, 0}, {1, 2, 0, 0, 0, 0}, {1, 3, 0, 0, 0, 0}, {1, 2, 0, 0, 1, 0}},
        {{2, 2, 0, 0, 0, 0}, {2, 3, 0, 0, 0, 0}, {3, 3, 0, 0, 0, 0}, {3, 2, 0, 0, 1, 0}},
        {{2, 0, 0, 0, 0, 1}, {2, 1, 0, 0, 0, 1}, {3, 1, 0, 0, 0, 1}, {1, 2, 0, 1, 1, 0}}
    };

    // need subpel
    if ((t->flags & (USE_QUARTPEL)) == (USE_QUARTPEL))
    {
        x = (vec->x & 3);
        y = (vec->y & 3);

        if (index[y][x][0] == index[y][x][1])
        {
            tmp = t->ref[list_index][vec->refno]->Y[index[y][x][0]] + ((t->mb.mb_y << 4) + (vec->y >> 2)) * t->edged_stride + 
                ((t->mb.mb_x << 4) + (vec->x >> 2));
            t->memcpy_stride_u(tmp, 16, 16, t->edged_stride, ref, 16);
        }
        else
        {
            t->pia[MB_16x16](t->ref[list_index][vec->refno]->Y[index[y][x][0]] + ((t->mb.mb_y << 4) + (vec->y >> 2) + index[y][x][3]) * t->edged_stride + (t->mb.mb_x << 4) + (vec->x >> 2) + index[y][x][2], 
                t->ref[list_index][vec->refno]->Y[index[y][x][1]] + ((t->mb.mb_y << 4) + (vec->y >> 2) + index[y][x][5]) * t->edged_stride + (t->mb.mb_x << 4) + (vec->x >> 2) + index[y][x][4],
                t->edged_stride, t->edged_stride, ref, 16);
        }

        // rc will use sad value
        sad = t->cmp[MB_16x16](t->mb.src_y, t->stride, ref, 16);

        return sad;
    }

    return -1;
}

int32_t
T264_detect_pskip(T264_t* t, uint32_t sad_t)
{
    // detect p skip has a two-step process. here try to find suitable skip mv
    //  and encode post will decide if use skip mode or not
    DECLARE_ALIGNED_MATRIX(ref, 16, 16, uint8_t, CACHE_SIZE);

    T264_vector_t vec;
    uint8_t* tmp;
    int32_t x, y;
    int32_t i, j;
    int32_t list_index = 0;

    static const int8_t index[4][4][6] = 
    {
        {{0, 0, 0, 0, 0, 0}, {0, 1, 0, 0, 0, 0}, {1, 1, 0, 0, 0, 0}, {1, 0, 0, 0, 1, 0}},
        {{0, 2, 0, 0, 0, 0}, {1, 2, 0, 0, 0, 0}, {1, 3, 0, 0, 0, 0}, {1, 2, 0, 0, 1, 0}},
        {{2, 2, 0, 0, 0, 0}, {2, 3, 0, 0, 0, 0}, {3, 3, 0, 0, 0, 0}, {3, 2, 0, 0, 1, 0}},
        {{2, 0, 0, 0, 0, 1}, {2, 1, 0, 0, 0, 1}, {3, 1, 0, 0, 0, 1}, {1, 2, 0, 1, 1, 0}}
    };

    T264_predict_mv_skip(t, 0, &vec);

    // need subpel
    if ((t->flags & (USE_QUARTPEL)) == (USE_QUARTPEL))
    {
        x = (vec.x & 3);
        y = (vec.y & 3);

        if (index[y][x][0] == index[y][x][1])
        {
            tmp = t->ref[list_index][vec.refno]->Y[index[y][x][0]] + ((t->mb.mb_y << 4) + (vec.y >> 2)) * t->edged_stride + 
                ((t->mb.mb_x << 4) + (vec.x >> 2));
            t->memcpy_stride_u(tmp, 16, 16, t->edged_stride, ref, 16);
        }
        else
        {
            t->pia[MB_16x16](t->ref[list_index][vec.refno]->Y[index[y][x][0]] + ((t->mb.mb_y << 4) + (vec.y >> 2) + index[y][x][3]) * t->edged_stride + (t->mb.mb_x << 4) + (vec.x >> 2) + index[y][x][2], 
                        t->ref[list_index][vec.refno]->Y[index[y][x][1]] + ((t->mb.mb_y << 4) + (vec.y >> 2) + index[y][x][5]) * t->edged_stride + (t->mb.mb_x << 4) + (vec.x >> 2) + index[y][x][4],
                        t->edged_stride, t->edged_stride, ref, 16);
        }


        // rc will use sad value
        t->mb.sad = t->cmp[MB_16x16](t->mb.src_y, t->stride, ref, 16);
        if (t->mb.sad < sad_t)
        {
            return FALSE;
        }

        {
            // use foreman.cif, qp = 30, waste 35497 times encode vs. total times 80266
            DECLARE_ALIGNED_MATRIX(dct, 16, 16, int16_t, 16);

            int32_t qp = t->qp_y;
            int32_t i, j;
            int16_t* curdct;
            // we will count coeff cost, from jm80
            int32_t run, k;
            int32_t coeff_cost, total_cost;

            total_cost = 0;

            t->expand8to16sub(ref, 16 / 4, 16 / 4, dct, t->mb.src_y, t->stride);
            for(i = 0 ; i < 4 ; i ++)
            {
                coeff_cost = 0;
                for(j = 0 ; j < 4 ; j ++)
                {
                    int32_t idx = 4 * i + j;
                    int32_t idx_r = luma_index[idx];
                    curdct = dct + 16 * idx_r;

                    t->fdct4x4(curdct);

                    t->quant4x4(curdct, qp, FALSE);
                    scan_zig_4x4(t->mb.dct_y_z[idx], curdct);
                    {
                        run = -1;
                        for(k = 0 ; k < 16 ; k ++)
                        {
                            run ++;
                            if (t->mb.dct_y_z[idx][k] != 0)
                            {
                                if (ABS(t->mb.dct_y_z[idx][k]) > 1)
                                {
                                    return FALSE;
                                }
                                else
                                {
                                    coeff_cost += COEFF_COST[run];
                                    run = -1;
                                }
                            }
                        }
                    }
                }
                if (coeff_cost <= t->param.luma_coeff_cost)
                {
                    int32_t idx_r = luma_index[4 * i];
                    memset(t->mb.dct_y_z[4 * i], 0, 16 * sizeof(int16_t));
                    memset(dct + 16 * idx_r, 0, 16 * sizeof(int16_t));

                    idx_r = luma_index[4 * i + 1];
                    memset(t->mb.dct_y_z[4 * i + 1], 0, 16 * sizeof(int16_t));
                    memset(dct + 16 * idx_r, 0, 16 * sizeof(int16_t));

                    idx_r = luma_index[4 * i + 2];
                    memset(t->mb.dct_y_z[4 * i + 2], 0, 16 * sizeof(int16_t));
                    memset(dct + 16 * idx_r, 0, 16 * sizeof(int16_t));

                    idx_r = luma_index[4 * i + 3];
                    memset(t->mb.dct_y_z[4 * i + 3], 0, 16 * sizeof(int16_t));
                    memset(dct + 16 * idx_r, 0, 16 * sizeof(int16_t));
                    coeff_cost = 0;
                }
                else
                {
                    total_cost += coeff_cost;
                    if (total_cost > 5)
                        return FALSE;
                }
            }

            memset(dct, 0, 16 * 16 * sizeof(int16_t));
            memset(t->mb.dct_y_z, 0, sizeof(int16_t) * 16 * 16);

            t->contract16to8add(dct, 16 / 4, 16 / 4, ref, t->mb.dst_y, t->edged_stride);
        }

        {
            DECLARE_ALIGNED_MATRIX(pred_u, 8, 8, uint8_t, CACHE_SIZE);
            DECLARE_ALIGNED_MATRIX(pred_v, 8, 8, uint8_t, CACHE_SIZE);
            DECLARE_ALIGNED_MATRIX(dct, 10, 8, int16_t, CACHE_SIZE);

            int32_t qp = t->qp_uv;
            int16_t* curdct;
            uint8_t* start;
            uint8_t* dst;

            uint8_t* src;

            // we will count coeff cost, from jm80
            int32_t run, k;
            int32_t coeff_cost;

            src = t->ref[0][vec.refno]->U + ((t->mb.mb_y << 3) + (vec.y >> 3)) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3);
            t->eighth_pixel_mc_u(src, t->edged_stride_uv, pred_u, vec.x, vec.y, 8, 8);
            src = t->ref[0][vec.refno]->V + ((t->mb.mb_y << 3) + (vec.y >> 3)) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3);
            t->eighth_pixel_mc_u(src, t->edged_stride_uv, pred_v, vec.x, vec.y, 8, 8);

            start = pred_u;
            src   = t->mb.src_u;
            dst   = t->mb.dst_u;
            for(j = 0 ; j < 2 ; j ++)
            {
                coeff_cost = 0;

                t->expand8to16sub(start, 8 / 4, 8 / 4, dct, src, t->stride_uv);
                curdct = dct;
                for(i = 0 ; i < 4 ; i ++)
                {
                    run = -1;

                    t->fdct4x4(curdct);
                    dct[64 + i] = curdct[0];

                    t->quant4x4(curdct, qp, FALSE);
                    scan_zig_4x4(t->mb.dct_uv_z[j][i], curdct);
                    {
                        for(k = 1 ; k < 16 ; k ++)
                        {
                            run ++;
                            if (t->mb.dct_uv_z[j][i][k] != 0)
                            {
                                if (ABS(t->mb.dct_uv_z[j][i][k]) > 1)
                                {
                                    coeff_cost += 16 * 16 * 256;
                                    return FALSE;
                                }
                                else
                                {
                                    coeff_cost += COEFF_COST[run];
                                    run = -1;
                                }
                            }
                        }
                    }

                    curdct += 16;
                }
                if (coeff_cost < CHROMA_COEFF_COST)
                {
                    memset(&t->mb.dct_uv_z[j][0][0], 0, 4 * 16 * sizeof(int16_t));
                    memset(dct, 0, 8 * 8 * sizeof(int16_t));
                }
                else
                {
                    return FALSE;
                }
                t->fdct2x2dc(curdct);
                t->quant2x2dc(curdct, qp, FALSE);
                scan_zig_2x2(t->mb.dc2x2_z[j], curdct);
                if (array_non_zero_count(t->mb.dc2x2_z[j], 4) != 0)
                {
                    return FALSE;
                }

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

        t->mb.mb_mode = P_SKIP;
        t->mb.mb_part = MB_16x16;
        memcpy(t->mb.pred_p16x16, ref, sizeof(uint8_t) * 16 * 16);
        copy_nvec(&vec, &t->mb.vec[0][0], 4, 4, 4);

        return TRUE;
    }
    
    return FALSE;
}

uint32_t
T264_mode_decision_interp_y(_RW T264_t* t)
{
    uint32_t sad;
    uint32_t sad_min = -1;
    uint8_t best_mode;
    uint8_t sub_part[4];
    uint8_t part;
    int32_t i, n;
    int32_t preds[9];	
    int32_t modes;
    search_data_t s0;
    subpart_search_data_t s1;

    typedef uint32_t (*p16x16_function_t)(T264_t*, search_data_t* s);
    static const p16x16_function_t p16x16_function[] = 
    {
        T264_mode_decision_inter_16x16p,
        T264_mode_decision_inter_16x8p,
        T264_mode_decision_inter_8x16p
    };

    // xxx
#ifdef USE_PREV_DETECT
    uint32_t sad_median;    // for skip detect
    sad_median = T264_predict_sad(t, 0);
    // p skip detection
    if (T264_detect_pskip(t, sad_median))
        return t->mb.sad;
#endif

    T264_inter_p16x16_mode_available(t, preds, &modes);

    best_mode = P_MODE;
    s0.list_index = 0;
    s1.list_index = 0;
	
    for(n = 0 ; n < modes ; n ++)
    {
        int32_t mode = preds[n];
        sad = p16x16_function[mode](t, &s0);

        if (sad < sad_min)
        {
			part = mode;
            sad_min = sad;
        }
    }

    if (t->flags & USE_SUBBLOCK)
    {
        uint32_t sub_sad_all = 0;

        typedef uint32_t (*p8x8_function_t)(T264_t*, int32_t, subpart_search_data_t* s);
        static const p8x8_function_t p8x8_function[] = 
        {
            T264_mode_decision_inter_8x8p,
            T264_mode_decision_inter_8x8p,
            T264_mode_decision_inter_8x4p,
            T264_mode_decision_inter_4x8p,
            T264_mode_decision_inter_4x4p
        };
        s1.vec[0][0].refno = s0.vec[0].refno;

        for(i = 0 ; i < 4 ; i ++)
        {
            uint32_t sub_sad;
            uint32_t sub_sad_min = -1;

            T264_inter_p8x8_mode_available(t, preds, &modes, i);

            for(n = 0 ; n < modes ; n ++)
            {
                int32_t mode = preds[n];
                T264_vector_t vec_bak[4];

                vec_bak[0] = t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 0].vec[0];
                vec_bak[1] = t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 1].vec[0];
                vec_bak[2] = t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 8].vec[0];
                vec_bak[3] = t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 9].vec[0];

                sub_sad = p8x8_function[mode - MB_8x8](t, i, &s1);

                if (sub_sad < sub_sad_min)
                {
                    sub_part[i] = mode;
                    sub_sad_min = sub_sad;
                }
                else
                {
                    // restore current best mode
                    t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 0].vec[0] = vec_bak[0];
                    t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 1].vec[0] = vec_bak[1];
                    t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 8].vec[0] = vec_bak[2];
                    t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 9].vec[0] = vec_bak[3];
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

    switch (part)
    {
    case MB_16x8:
        sad_min  = T264_quarter_pixel_search(t, 0, s0.src[1], s0.ref[1], s0.offset[1], &s0.vec[1], &s0.vec_median[1], s0.sad[1], 16, 8, t->mb.pred_p16x16, MB_16x8);
        sad_min += T264_quarter_pixel_search(t, 0, s0.src[2], s0.ref[2], s0.offset[2], &s0.vec[2], &s0.vec_median[2], s0.sad[2], 16, 8, t->mb.pred_p16x16 + 16 * 8, MB_16x8);
        copy_nvec(&s0.vec[1], &t->mb.vec[0][0], 4, 2, 4);
        copy_nvec(&s0.vec[2], &t->mb.vec[0][8], 4, 2, 4);
        break;
    case MB_8x16:
        sad_min  = T264_quarter_pixel_search(t, 0, s0.src[3], s0.ref[3], s0.offset[3], &s0.vec[3], &s0.vec_median[3], s0.sad[3], 8, 16, t->mb.pred_p16x16, MB_8x16);
        sad_min += T264_quarter_pixel_search(t, 0, s0.src[4], s0.ref[4], s0.offset[4], &s0.vec[4], &s0.vec_median[4], s0.sad[4], 8, 16, t->mb.pred_p16x16 + 8, MB_8x16);
        copy_nvec(&s0.vec[3], &t->mb.vec[0][0], 2, 4, 4);
        copy_nvec(&s0.vec[4], &t->mb.vec[0][2], 2, 4, 4);
        break;
    case MB_8x8:
    case MB_8x8ref0:
        sad_min = 0;
        for(i = 0 ; i < 4 ; i ++)
        {
            switch(sub_part[i])
            {
            case MB_8x8:
                sad_min += T264_quarter_pixel_search(t, 0, s1.src[i][0], s1.ref[i][0], s1.offset[i][0], &s1.vec[i][0], &s1.vec_median[i][0], s1.sad[i][0], 8, 8, t->mb.pred_p16x16 + i / 2 * 16 * 8 + i % 2 * 8, MB_8x8);
                t->mb.vec[0][i / 2 * 8 + i % 2 * 2 + 0] = s1.vec[i][0];
                t->mb.vec[0][i / 2 * 8 + i % 2 * 2 + 1] = s1.vec[i][0];
                t->mb.vec[0][i / 2 * 8 + i % 2 * 2 + 4] = s1.vec[i][0];
                t->mb.vec[0][i / 2 * 8 + i % 2 * 2 + 5] = s1.vec[i][0];
                sad_min += eg_size_ue(t->bs, MB_8x8);
        	    break;
            case MB_8x4:
                sad_min += T264_quarter_pixel_search(t, 0, s1.src[i][1], s1.ref[i][1], s1.offset[i][1], &s1.vec[i][1], &s1.vec_median[i][1], s1.sad[i][1], 8, 4, t->mb.pred_p16x16 + i / 2 * 16 * 8 + i % 2 * 8, MB_8x4);
                sad_min += T264_quarter_pixel_search(t, 0, s1.src[i][2], s1.ref[i][2], s1.offset[i][2], &s1.vec[i][2], &s1.vec_median[i][2], s1.sad[i][2], 8, 4, t->mb.pred_p16x16 + i / 2 * 16 * 8 + i % 2 * 8 + 16 * 4, MB_8x4);
                t->mb.vec[0][i / 2 * 8 + i % 2 * 2 + 0] = s1.vec[i][1];
                t->mb.vec[0][i / 2 * 8 + i % 2 * 2 + 1] = s1.vec[i][1];
                t->mb.vec[0][i / 2 * 8 + i % 2 * 2 + 4] = s1.vec[i][2];
                t->mb.vec[0][i / 2 * 8 + i % 2 * 2 + 5] = s1.vec[i][2];
                sad_min += eg_size_ue(t->bs, MB_8x4);
        	    break;
            case MB_4x8:
                sad_min += T264_quarter_pixel_search(t, 0, s1.src[i][3], s1.ref[i][3], s1.offset[i][3], &s1.vec[i][3], &s1.vec[i][3], s1.sad[i][3], 4, 8, t->mb.pred_p16x16 + i / 2 * 16 * 8 + i % 2 * 8, MB_4x8);
                sad_min += T264_quarter_pixel_search(t, 0, s1.src[i][4], s1.ref[i][4], s1.offset[i][4], &s1.vec[i][4], &s1.vec[i][4], s1.sad[i][4], 4, 8, t->mb.pred_p16x16 + i / 2 * 16 * 8 + i % 2 * 8 + 4, MB_4x8);
                t->mb.vec[0][i / 2 * 8 + i % 2 * 2 + 0] = s1.vec[i][3];
                t->mb.vec[0][i / 2 * 8 + i % 2 * 2 + 1] = s1.vec[i][4];
                t->mb.vec[0][i / 2 * 8 + i % 2 * 2 + 4] = s1.vec[i][3];
                t->mb.vec[0][i / 2 * 8 + i % 2 * 2 + 5] = s1.vec[i][4];
                sad_min += eg_size_ue(t->bs, MB_4x8);
                break;
            case MB_4x4:
                sad_min += T264_quarter_pixel_search(t, 0, s1.src[i][5], s1.ref[i][5], s1.offset[i][5], &s1.vec[i][5], &s1.vec[i][5], s1.sad[i][5], 4, 4, t->mb.pred_p16x16 + i / 2 * 16 * 8 + i % 2 * 8, MB_4x4);
                sad_min += T264_quarter_pixel_search(t, 0, s1.src[i][6], s1.ref[i][6], s1.offset[i][6], &s1.vec[i][6], &s1.vec[i][6], s1.sad[i][6], 4, 4, t->mb.pred_p16x16 + i / 2 * 16 * 8 + i % 2 * 8 + 4, MB_4x4);
                sad_min += T264_quarter_pixel_search(t, 0, s1.src[i][7], s1.ref[i][7], s1.offset[i][7], &s1.vec[i][7], &s1.vec[i][7], s1.sad[i][7], 4, 4, t->mb.pred_p16x16 + i / 2 * 16 * 8 + i % 2 * 8 + 16 * 4, MB_4x4);
                sad_min += T264_quarter_pixel_search(t, 0, s1.src[i][8], s1.ref[i][8], s1.offset[i][8], &s1.vec[i][8], &s1.vec[i][8], s1.sad[i][8], 4, 4, t->mb.pred_p16x16 + i / 2 * 16 * 8 + i % 2 * 8 + 16 * 4 + 4, MB_4x4);
                t->mb.vec[0][i / 2 * 8 + i % 2 * 2 + 0] = s1.vec[i][5];
                t->mb.vec[0][i / 2 * 8 + i % 2 * 2 + 1] = s1.vec[i][6];
                t->mb.vec[0][i / 2 * 8 + i % 2 * 2 + 4] = s1.vec[i][7];
                t->mb.vec[0][i / 2 * 8 + i % 2 * 2 + 5] = s1.vec[i][8];
                sad_min += eg_size_ue(t->bs, MB_4x4);
                break;
            default:
                break;
            }
            t->mb.submb_part[i / 2 * 8 + i % 2 * 2 + 0] = sub_part[i];
            t->mb.submb_part[i / 2 * 8 + i % 2 * 2 + 1] = sub_part[i];
            t->mb.submb_part[i / 2 * 8 + i % 2 * 2 + 4] = sub_part[i];
            t->mb.submb_part[i / 2 * 8 + i % 2 * 2 + 5] = sub_part[i];
        }
    	break;
    default:
        break;
    }

    // 3ks chenm
    if (t->flags & USE_INTRAININTER)
        sad = T264_mode_decision_intra_y(t);
    else
        sad = -1;
    if (sad <= sad_min)
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

    return t->mb.sad;
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
void
get_pmv(T264_t* t, int32_t list, T264_vector_t* vec, int32_t part, int32_t idx, int32_t width, int32_t* n)
{
    int32_t count = 0;
    int32_t row;
    int32_t col;

    T264_predict_mv(t, list, idx, width, vec);
    
    col = idx % 4;
    row = idx / 4;

    vec[1].x = t->mb.vec_ref[VEC_LUMA - 1 + row * 8 + col].vec[list].x;
    vec[1].y = t->mb.vec_ref[VEC_LUMA - 1 + row * 8 + col].vec[list].y;
    vec[1].refno = vec[0].refno;
    vec[2].x = t->mb.vec_ref[VEC_LUMA - 8 + row * 8 + col].vec[list].x;
    vec[2].y = t->mb.vec_ref[VEC_LUMA - 8 + row * 8 + col].vec[list].y;
    vec[2].refno = vec[0].refno;
    vec[3].x = t->mb.vec_ref[VEC_LUMA - 8 + row * 8 + col + width].vec[list].x;
    vec[3].y = t->mb.vec_ref[VEC_LUMA - 8 + row * 8 + col + width].vec[list].y;
    vec[3].refno = vec[0].refno;
    vec[4].x = t->mb.vec_ref[VEC_LUMA - 8 + row * 8 + col - 1].vec[list].x;
    vec[4].y = t->mb.vec_ref[VEC_LUMA - 8 + row * 8 + col - 1].vec[list].y;
    vec[4].refno = vec[0].refno;

    vec[5 + 0].x = 0;
    vec[5 + 0].y = 0;
    vec[5 + 0].refno = 0;
    *n = 5 + 1;
}

uint32_t
T264_mode_decision_inter_16x16p(_RW T264_t* t, search_data_t* s)
{
    DECLARE_ALIGNED_MATRIX(pred_16x16, 16, 16, uint8_t, 16);
    T264_vector_t vec[5 + 10];  // NOTE: max 10 refs
    T264_search_context_t context;
    T264_vector_t vec_skip;
    int32_t num;
    int32_t i;
    uint32_t sad;
    uint8_t* p_min = t->mb.pred_p16x16;
    uint8_t* p_buf = pred_16x16;

    vec[0].refno = 0;
    get_pmv(t, 0, vec, MB_16x16, 0, 4, &num);

    context.height = 16;
    context.width  = 16;
    context.limit_x= t->param.search_x;
    context.limit_y= t->param.search_y;
    context.vec    = vec;
    context.vec_num= num;
    context.mb_part= MB_16x16;
    context.offset = (t->mb.mb_y << 4) * t->edged_stride + (t->mb.mb_x << 4);
    context.list_index = s->list_index;

    s->src[0] = t->mb.src_y;
    s->sad[0] = t->search(t, &context);
    s->vec[0] = context.vec_best;
    s->ref[0] = t->ref[context.list_index][s->vec[0].refno];
    s->offset[0] = context.offset;
    s->vec_median[0] = vec[0];
    s->sad[0] = T264_quarter_pixel_search(t, 0, s->src[0], s->ref[0], s->offset[0], &s->vec[0], &s->vec_median[0], s->sad[0], 16, 16, p_min, MB_16x16);

    for(i = 1 ; i < t->refl0_num ; i ++)
    {
        vec[0].refno = i;
        get_pmv(t, 0, vec, MB_16x16, 0, 4, &num);
        context.vec_num = 1;
        sad = t->search(t, &context);
        sad+= REFCOST(context.vec_best.refno);
        sad = T264_quarter_pixel_search(t, 0, s->src[0], t->ref[0][i], 
            s->offset[0], &context.vec_best, &vec[0], sad, 16, 16, p_buf, MB_16x16);
        if (sad < s->sad[0])
        {
            SWAP(uint8_t, p_buf, p_min);
            s->sad[0] = sad;
            s->vec[0] = context.vec_best;
            s->ref[0] = t->ref[0][i];
            s->vec_median[0] = vec[0];
        }
    }

    // xxx
#ifndef USE_PREV_DETECT
    T264_predict_mv_skip(t, 0, &vec_skip);
    sad = T264_get_pos_sad(t, p_buf, &vec_skip);
    sad += t->mb.lambda * (eg_size_se(t->bs, (t->mb.vec[0][0].x - vec[0].x)) +
                           eg_size_se(t->bs, (t->mb.vec[0][0].y - vec[0].y)));
#else
    sad = -1;
#endif
    if (sad < s->sad[0] + (uint32_t)8 * t->mb.lambda)
    {
        s->sad[0] = sad;
        SWAP(uint8_t, p_buf, p_min);
        copy_nvec(&vec_skip, &t->mb.vec[0][0], 4, 4, 4);
    }
    else
    {
        copy_nvec(&s->vec[0], &t->mb.vec[0][0], 4, 4, 4);
    }

    if (p_min != t->mb.pred_p16x16)
    {
        memcpy(t->mb.pred_p16x16, p_min, sizeof(uint8_t) * 16 * 16);
    }

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

    vec[0].refno = s->vec[0].refno;
    get_pmv(t, 0, vec, MB_16x8, 0, 4, &num);

    vec[num ++] = s->vec[0];

    context.height = 8;
    context.width  = 16;
    context.limit_x= t->param.search_x;
    context.limit_y= t->param.search_y;
    context.vec    = vec;
    context.vec_num= num;
    context.mb_part= MB_16x8;
    context.offset = (t->mb.mb_y << 4) * t->edged_stride + (t->mb.mb_x << 4);
    context.list_index = s->list_index;

    s->src[1] = t->mb.src_y;
    s->sad[1] = t->search(t, &context);
    s->sad[1]+= REFCOST(context.vec_best.refno);
    s->vec[1] = context.vec_best;
    s->ref[1] = t->ref[context.list_index][s->vec[1].refno];
    s->offset[1] = context.offset;
    s->vec_median[1] = vec[0];

    t->mb.vec_ref[VEC_LUMA + 8].vec[0] = s->vec[1];
    get_pmv(t, 0, vec, MB_16x8, luma_index[8], 4, &num);

    vec[num ++] = s->vec[0];

    s->src[2] = t->mb.src_y + 8 * t->stride;
    context.offset += 8 * t->edged_stride;
    s->sad[2] = t->search(t, &context);
    s->sad[2]+= REFCOST(context.vec_best.refno);
    s->vec[2] = context.vec_best;
    s->ref[2] = t->ref[context.list_index][s->vec[2].refno];
    s->offset[2] = context.offset;
    s->vec_median[2] = vec[0];

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
    vec[0].refno = s->vec[0].refno;
    get_pmv(t, 0, vec, MB_8x16, 0, 2, &num);

    vec[num ++] = s->vec[0];

    context.height = 16;
    context.width  = 8;
    context.limit_x= t->param.search_x;
    context.limit_y= t->param.search_y;
    context.vec    = vec;
    context.vec_num= num;
    context.mb_part= MB_8x16;
    context.offset = (t->mb.mb_y << 4) * t->edged_stride + (t->mb.mb_x << 4);
    context.list_index = s->list_index;

    s->src[3] = t->mb.src_y;
    s->sad[3] = t->search(t, &context);
    s->sad[3]+= REFCOST(context.vec_best.refno);
    s->vec[3] = context.vec_best;
    s->ref[3] = t->ref[context.list_index][s->vec[3].refno];
    s->offset[3] = context.offset;
    s->vec_median[3] = vec[0];

    t->mb.vec_ref[VEC_LUMA + 1].vec[0] = s->vec[3];
    get_pmv(t, 0, vec, MB_8x16, luma_index[4], 2, &num);

    vec[num ++] = s->vec[0];

    s->src[4] = t->mb.src_y + 8;
    context.offset += 8;
    s->sad[4] = t->search(t, &context);
    s->sad[4]+= REFCOST(context.vec_best.refno);
    s->vec[4] = context.vec_best;
    s->ref[4] = t->ref[context.list_index][s->vec[4].refno];
    s->offset[4] = context.offset;
    s->vec_median[4] = vec[0];

    t->mb.mb_part = old_part;
    return s->sad[3] + s->sad[4];
}

uint32_t
T264_mode_decision_inter_8x8p(_RW T264_t * t, int32_t i, subpart_search_data_t* s)
{
    T264_vector_t vec[5 + 10];  // NOTE: max 10 refs
    T264_search_context_t context;
    int32_t num;

    vec[0].refno = s->vec[0][0].refno;
    get_pmv(t, 0, vec, MB_8x8, luma_index[4 * i], 2, &num);

    context.height = 8;
    context.width  = 8;
    context.limit_x= t->param.search_x;
    context.limit_y= t->param.search_y;
    context.vec    = vec;
    context.vec_num= num;
    context.mb_part= MB_8x8;
    context.offset = ((t->mb.mb_y << 4) + i / 2 * 8) * t->edged_stride + (t->mb.mb_x << 4) + i % 2 * 8;
    context.list_index = s->list_index;

    s->src[i][0] = t->mb.src_y + (i / 2 * 8) * t->stride + i % 2 * 8;
    s->sad[i][0] = t->search(t, &context);
    s->sad[i][0]+= REFCOST(context.vec_best.refno);
    s->vec[i][0] = context.vec_best;
    s->offset[i][0] = context.offset;
    s->ref[i][0] = t->ref[context.list_index][s->vec[i][0].refno];
    s->vec_median[i][0] = vec[0];

    t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 0].vec[0] =
    t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 1].vec[0] =
    t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 8].vec[0] =
    t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 9].vec[0] = s->vec[i][0];

    return s->sad[i][0] + eg_size_ue(t->bs, MB_8x8);
}

uint32_t
T264_mode_decision_inter_8x4p(_RW T264_t * t, int32_t i, subpart_search_data_t* s)
{
    T264_vector_t vec[5 + 10];  // NOTE: max 10 refs
    T264_search_context_t context;
    int32_t num;

    vec[0].refno = s->vec[0][0].refno;
    get_pmv(t, 0, vec, MB_8x4, luma_index[4 * i + 0], 2, &num);

    context.height = 4;
    context.width  = 8;
    context.limit_x= t->param.search_x;
    context.limit_y= t->param.search_y;
    context.vec    = vec;
    /* because get_pmv maybe return some ref does not equal, and the sub 8x8 ref framenum must be the same, so candidate we use only median vector */
    context.vec_num= 1; 
    context.mb_part= MB_8x4;
    context.offset = ((t->mb.mb_y << 4) + i / 2 * 8) * t->edged_stride + (t->mb.mb_x << 4) + i % 2 * 8;
    context.list_index = s->list_index;

    s->src[i][1] = t->mb.src_y + (i / 2 * 8) * t->stride + i % 2 * 8;
    s->sad[i][1] = t->search(t, &context);
    s->sad[i][1]+= REFCOST(context.vec_best.refno);
    s->vec[i][1] = context.vec_best;
    s->offset[i][1] = context.offset;
    s->ref[i][1] = t->ref[context.list_index][s->vec[i][1].refno];
    s->vec_median[i][1] = vec[0];

    t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 0].vec[0] =
    t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 1].vec[0] = s->vec[i][1];
    get_pmv(t, 0, vec, MB_8x4, luma_index[4 * i + 2], 2, &num);

    s->src[i][2] = s->src[i][1] + 4 * t->stride;
    context.offset += 4 * t->edged_stride;
    s->sad[i][2] = t->search(t, &context);
    s->sad[i][2]+= REFCOST(context.vec_best.refno);
    s->vec[i][2] = context.vec_best;
    s->offset[i][2] = context.offset;
    s->ref[i][2] = t->ref[context.list_index][s->vec[i][2].refno];
    s->vec_median[i][2] = vec[0];
    t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 8].vec[0] =
    t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 9].vec[0] = s->vec[i][2];

    return s->sad[i][1] + s->sad[i][2] + eg_size_ue(t->bs, MB_8x4);
}

uint32_t
T264_mode_decision_inter_4x8p(_RW T264_t * t, int32_t i, subpart_search_data_t* s)
{
    T264_vector_t vec[5 + 10];  // NOTE: max 10 refs
    T264_search_context_t context;
    int32_t num;

    vec[0].refno = s->vec[0][0].refno;
    get_pmv(t, 0, vec, MB_4x8, luma_index[4 * i + 0], 1, &num);

    context.height = 8;
    context.width  = 4;
    context.limit_x= t->param.search_x;
    context.limit_y= t->param.search_y;
    context.vec    = vec;
    /* see 4x8 note */
    context.vec_num= 1; /* num; */
    context.mb_part= MB_4x8;
    context.offset = ((t->mb.mb_y << 4) + i / 2 * 8) * t->edged_stride + (t->mb.mb_x << 4) + i % 2 * 8;
    context.list_index = s->list_index;

    s->src[i][3] = t->mb.src_y + (i / 2 * 8) * t->stride + i % 2 * 8;
    s->sad[i][3] = t->search(t, &context);
    s->sad[i][3]+= REFCOST(context.vec_best.refno);
    s->vec[i][3] = context.vec_best;
    s->offset[i][3] = context.offset;
    s->ref[i][3] = t->ref[context.list_index][s->vec[i][3].refno];
    s->vec_median[i][3] = vec[0];

    t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 0].vec[0] =
    t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 8].vec[0] = s->vec[i][3];
    get_pmv(t, 0, vec, MB_4x8, luma_index[4 * i + 1], 1, &num);

    s->src[i][4] = s->src[i][3] + 4;
    context.offset += 4;
    s->sad[i][4] = t->search(t, &context);
    s->sad[i][4]+= REFCOST(context.vec_best.refno);
    s->vec[i][4] = context.vec_best;
    s->offset[i][4] = context.offset;
    s->ref[i][4] = t->ref[context.list_index][s->vec[i][4].refno];
    s->vec_median[i][4] = vec[0];

    t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 1].vec[0] =
    t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 9].vec[0] = s->vec[i][4];

    return s->sad[i][3] + s->sad[i][4] + eg_size_ue(t->bs, MB_4x8);
}

uint32_t
T264_mode_decision_inter_4x4p(_RW T264_t * t, int32_t i, subpart_search_data_t* s)
{
    T264_vector_t vec[5 + 10];  // NOTE: max 10 refs
    T264_search_context_t context;
    int32_t num;

    vec[0].refno = s->vec[0][0].refno;
    get_pmv(t, 0, vec, MB_4x4, luma_index[4 * i + 0], 1, &num);

    context.height = 4;
    context.width  = 4;
    context.limit_x= t->param.search_x;
    context.limit_y= t->param.search_y;
    context.vec    = vec;
    /* see 4x8 note */
    context.vec_num= 1; /* num; */
    context.mb_part= MB_4x4;
    context.offset = ((t->mb.mb_y << 4) + i / 2 * 8) * t->edged_stride + (t->mb.mb_x << 4) + i % 2 * 8;
    context.list_index = s->list_index;

    s->src[i][5] = t->mb.src_y + (i / 2 * 8) * t->stride + i % 2 * 8;
    s->sad[i][5] = t->search(t, &context);
    s->sad[i][5]+= REFCOST(context.vec_best.refno);
    s->vec[i][5] = context.vec_best;
    s->offset[i][5] = context.offset;
    s->ref[i][5] = t->ref[context.list_index][s->vec[i][5].refno];
    s->vec_median[i][5] = vec[0];

    t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 0].vec[0] = s->vec[i][5];
    get_pmv(t, 0, vec, MB_4x4, luma_index[4 * i + 1], 1, &num);

    s->src[i][6] = s->src[i][5] + 4;
    context.offset += 4;
    s->sad[i][6] = t->search(t, &context);
    s->sad[i][6]+= REFCOST(context.vec_best.refno);
    s->vec[i][6] = context.vec_best;
    s->offset[i][6] = context.offset;
    s->ref[i][6] = t->ref[context.list_index][s->vec[i][6].refno];
    s->vec_median[i][6] = vec[0];

    t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 1].vec[0] = s->vec[i][6];
    get_pmv(t, 0, vec, MB_4x4, luma_index[4 * i + 2], 1, &num);

    s->src[i][7] = s->src[i][5] + 4 * t->stride;
    context.offset += 4 * t->edged_stride - 4;
    s->sad[i][7] = t->search(t, &context);
    s->sad[i][7]+= REFCOST(context.vec_best.refno);
    s->vec[i][7] = context.vec_best;
    s->offset[i][7] = context.offset;
    s->ref[i][7] = t->ref[context.list_index][s->vec[i][7].refno];
    s->vec_median[i][7] = vec[0];

    t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 8].vec[0] = s->vec[i][7];
    get_pmv(t, 0, vec, MB_4x4, luma_index[4 * i + 3], 1, &num);

    s->src[i][8] = s->src[i][7] + 4;
    context.offset += 4;
    s->sad[i][8] = t->search(t, &context);
    s->sad[i][8]+= REFCOST(context.vec_best.refno);
    s->vec[i][8] = context.vec_best;
    s->offset[i][8] = context.offset;
    s->ref[i][8] = t->ref[context.list_index][s->vec[i][8].refno];
    s->vec_median[i][8] = vec[0];

    t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 9].vec[0] = s->vec[i][8];

    return s->sad[i][5] + s->sad[i][6] + s->sad[i][7] + s->sad[i][8] + eg_size_ue(t->bs, MB_4x4);
}

void
T264_encode_inter_16x16p(_RW T264_t* t, uint8_t* pred)
{
    DECLARE_ALIGNED_MATRIX(dct, 16, 16, int16_t, 16);

    int32_t qp = t->qp_y;
    int32_t i, j;
    int16_t* curdct;
    // we will count coeff cost, from jm80
    int32_t run, k;
    int32_t coeff_cost, total_cost;

    total_cost = 0;

    t->expand8to16sub(pred, 16 / 4, 16 / 4, dct, t->mb.src_y, t->stride);
	
    for(i = 0 ; i < 4 ; i ++)
    {
        coeff_cost = 0;
        for(j = 0 ; j < 4 ; j ++)
        {
            int32_t idx = 4 * i + j;
            int32_t idx_r = luma_index[idx];
            curdct = dct + 16 * idx_r;

            t->fdct4x4(curdct);

            t->quant4x4(curdct, qp, FALSE);
            scan_zig_4x4(t->mb.dct_y_z[idx], curdct);
            if (coeff_cost <= 5)
            {
                run = -1;
                for(k = 0 ; k < 16 ; k ++)
                {
                    run ++;
                    if (t->mb.dct_y_z[idx][k] != 0)
                    {
                        if (ABS(t->mb.dct_y_z[idx][k]) > 1)
                        {
                            coeff_cost += 16 * 16 * 256;  // big enough number
                            break;
                        }
                        else
                        {
                            coeff_cost += COEFF_COST[run];
                            run = -1;
                        }
                    }
                }
            }
            else
            {
                coeff_cost = 16 * 16 * 256;
            }

            t->iquant4x4(curdct, qp);
            t->idct4x4(curdct);
        }
        if (coeff_cost <= t->param.luma_coeff_cost)
        {
            int32_t idx_r = luma_index[4 * i];
            memset(t->mb.dct_y_z[4 * i], 0, 16 * sizeof(int16_t));
            memset(dct + 16 * idx_r, 0, 16 * sizeof(int16_t));

            idx_r = luma_index[4 * i + 1];
            memset(t->mb.dct_y_z[4 * i + 1], 0, 16 * sizeof(int16_t));
            memset(dct + 16 * idx_r, 0, 16 * sizeof(int16_t));

            idx_r = luma_index[4 * i + 2];
            memset(t->mb.dct_y_z[4 * i + 2], 0, 16 * sizeof(int16_t));
            memset(dct + 16 * idx_r, 0, 16 * sizeof(int16_t));

            idx_r = luma_index[4 * i + 3];
            memset(t->mb.dct_y_z[4 * i + 3], 0, 16 * sizeof(int16_t));
            memset(dct + 16 * idx_r, 0, 16 * sizeof(int16_t));
            coeff_cost = 0;
        }
        total_cost += coeff_cost;
    }

    if (total_cost <= 5)
    {
        memset(dct, 0, 16 * 16 * sizeof(int16_t));
        memset(t->mb.dct_y_z, 0, sizeof(int16_t) * 16 * 16);	
    }

    t->contract16to8add(dct, 16 / 4, 16 / 4, pred, t->mb.dst_y, t->edged_stride);

}

void
T264_encode_inter_y(_RW T264_t* t)
{
    T264_encode_inter_16x16p(t, t->mb.pred_p16x16);
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
        // we will count coeff cost, from jm80
        int32_t run, k;
        int32_t coeff_cost;

        coeff_cost = 0;

        t->expand8to16sub(start, 8 / 4, 8 / 4, dct, src, t->stride_uv);
        curdct = dct;
        for(i = 0 ; i < 4 ; i ++)
        {
            run = -1;

            t->fdct4x4(curdct);
            dct[64 + i] = curdct[0];

            t->quant4x4(curdct, qp, FALSE);
            scan_zig_4x4(t->mb.dct_uv_z[j][i], curdct);
            {
                for(k = 1 ; k < 16 ; k ++)
                {
                    run ++;
                    if (t->mb.dct_uv_z[j][i][k] != 0)
                    {
                        if (ABS(t->mb.dct_uv_z[j][i][k]) > 1)
                        {
                            coeff_cost += 16 * 16 * 256;
                            break;
                        }
                        else
                        {
                            coeff_cost += COEFF_COST[run];
                            run = -1;
                        }
                    }
                }
            }
            t->iquant4x4(curdct, qp);

            curdct += 16;
        }
        if (coeff_cost < CHROMA_COEFF_COST)
        {
            memset(&t->mb.dct_uv_z[j][0][0], 0, 4 * 16 * sizeof(int16_t));
            memset(dct, 0, 8 * 8 * sizeof(int16_t));
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
    int32_t list_index = 0;

    switch (t->mb.mb_part)
    {
    case MB_16x16:
        vec = t->mb.vec[0][0];
        src = t->ref[list_index][vec.refno]->U + ((t->mb.mb_y << 3) + (vec.y >> 3)) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3);
        dst = pred_u;
        t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 8, 8);
        src = t->ref[list_index][vec.refno]->V + ((t->mb.mb_y << 3) + (vec.y >> 3)) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3);
        dst = pred_v;
        t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 8, 8);
        break;
    case MB_16x8:
        vec = t->mb.vec[0][0];
        src_u = t->ref[list_index][vec.refno]->U + ((t->mb.mb_y << 3) + (vec.y >> 3)) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3);
        dst_u = pred_u;
        t->eighth_pixel_mc_u(src_u, t->edged_stride_uv, dst_u, vec.x, vec.y, 8, 4);
        src = t->ref[list_index][vec.refno]->V + ((t->mb.mb_y << 3) + (vec.y >> 3)) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3);
        dst = pred_v;
        t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 8, 4);

        vec = t->mb.vec[0][luma_index[8]];
        src_u = t->ref[list_index][vec.refno]->U + ((t->mb.mb_y << 3) + (vec.y >> 3)) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) +
            4 * t->edged_stride_uv;
        dst_u += 4 * 8;
        t->eighth_pixel_mc_u(src_u, t->edged_stride_uv, dst_u, vec.x, vec.y, 8, 4);
        src = t->ref[list_index][vec.refno]->V + ((t->mb.mb_y << 3) + (vec.y >> 3)) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + 
            4 * t->edged_stride_uv;
        dst += 4 * 8;
        t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 8, 4);
        break;
    case MB_8x16:
        vec = t->mb.vec[0][0];
        src_u = t->ref[list_index][vec.refno]->U + ((t->mb.mb_y << 3) + (vec.y >> 3)) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3);
        dst_u = pred_u;
        t->eighth_pixel_mc_u(src_u, t->edged_stride_uv, dst_u, vec.x, vec.y, 4, 8);
        src = t->ref[list_index][vec.refno]->V + ((t->mb.mb_y << 3) + (vec.y >> 3)) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3);
        dst = pred_v;
        t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 4, 8);

        vec = t->mb.vec[0][luma_index[4]];
        src_u = t->ref[list_index][vec.refno]->U + ((t->mb.mb_y << 3) + (vec.y >> 3)) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + 4;
        dst_u += 4;
        t->eighth_pixel_mc_u(src_u, t->edged_stride_uv, dst_u, vec.x, vec.y, 4, 8);
        src = t->ref[list_index][vec.refno]->V + ((t->mb.mb_y << 3) + (vec.y >> 3)) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + 4;
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
                src = t->ref[list_index][vec.refno]->U + ((t->mb.mb_y << 3) + (vec.y >> 3) + i / 2 * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + (i % 2 * 4);
                dst = pred_u + i / 2 * 32 + i % 2 * 4;
                t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 4, 4);
                src = t->ref[list_index][vec.refno]->V + ((t->mb.mb_y << 3) + (vec.y >> 3) + i / 2 * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + (i % 2 * 4);
                dst = pred_v + i / 2 * 32 + i % 2 * 4;
                t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 4, 4);
                break;
            case MB_8x4:
                vec = t->mb.vec[0][luma_index[4 * i]];
                src_u = t->ref[list_index][vec.refno]->U + ((t->mb.mb_y << 3) + (vec.y >> 3) + i / 2 * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + (i % 2 * 4);
                dst_u = pred_u + i / 2 * 32 + i % 2 * 4;
                t->eighth_pixel_mc_u(src_u, t->edged_stride_uv, dst_u, vec.x, vec.y, 4, 2);
                src = t->ref[list_index][vec.refno]->V + ((t->mb.mb_y << 3) + (vec.y >> 3) + i / 2 * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + (i % 2 * 4);
                dst = pred_v + i / 2 * 32 + i % 2 * 4;
                t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 4, 2);

                vec = t->mb.vec[0][luma_index[4 * i + 2]];
                src_u = t->ref[list_index][vec.refno]->U + ((t->mb.mb_y << 3) + (vec.y >> 3) + i / 2 * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + (i % 2 * 4) + 
                    2 * t->edged_stride_uv;
                dst_u += 2 * 8;
                t->eighth_pixel_mc_u(src_u, t->edged_stride_uv, dst_u, vec.x, vec.y, 4, 2);
                src = t->ref[list_index][vec.refno]->V + ((t->mb.mb_y << 3) + (vec.y >> 3) + i / 2 * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + (i % 2 * 4) +
                    2 * t->edged_stride_uv;
                dst += 2 * 8;
                t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 4, 2);
                break;
            case MB_4x8:
                vec = t->mb.vec[0][luma_index[4 * i]];
                src_u = t->ref[list_index][vec.refno]->U + ((t->mb.mb_y << 3) + (vec.y >> 3) + i / 2 * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + (i % 2 * 4);
                dst_u = pred_u + i / 2 * 32 + i % 2 * 4;
                t->eighth_pixel_mc_u(src_u, t->edged_stride_uv, dst_u, vec.x, vec.y, 2, 4);
                src = t->ref[list_index][vec.refno]->V + ((t->mb.mb_y << 3) + (vec.y >> 3) + i / 2 * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + (i % 2 * 4);
                dst = pred_v + i / 2 * 32 + i % 2 * 4;
                t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 2, 4);

                vec = t->mb.vec[0][luma_index[4 * i + 1]];
                src_u = t->ref[list_index][vec.refno]->U + ((t->mb.mb_y << 3) + (vec.y >> 3) + i / 2 * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + (i % 2 * 4) + 2;
                dst_u += 2;
                t->eighth_pixel_mc_u(src_u, t->edged_stride_uv, dst_u, vec.x, vec.y, 2, 4);
                src = t->ref[list_index][vec.refno]->V + ((t->mb.mb_y << 3) + (vec.y >> 3) + i / 2 * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + (i % 2 * 4) + 2;
                dst += 2;
                t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 2, 4);
                break;
            case MB_4x4:
                vec = t->mb.vec[0][luma_index[4 * i]];
                src_u = t->ref[list_index][vec.refno]->U + ((t->mb.mb_y << 3) + (vec.y >> 3) + i / 2 * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + (i % 2 * 4);
                dst_u = pred_u + i / 2 * 32 + i % 2 * 4;
                t->eighth_pixel_mc_u(src_u, t->edged_stride_uv, dst_u, vec.x, vec.y, 2, 2);
                src = t->ref[list_index][vec.refno]->V + ((t->mb.mb_y << 3) + (vec.y >> 3) + i / 2 * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + (i % 2 * 4);
                dst = pred_v + i / 2 * 32 + i % 2 * 4;
                t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 2, 2);

                vec = t->mb.vec[0][luma_index[4 * i + 1]];
                src_u = t->ref[list_index][vec.refno]->U + ((t->mb.mb_y << 3) + (vec.y >> 3) + i / 2 * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + (i % 2 * 4) + 2;
                dst_u += 2;
                t->eighth_pixel_mc_u(src_u, t->edged_stride_uv, dst_u, vec.x, vec.y, 2, 2);
                src = t->ref[list_index][vec.refno]->V + ((t->mb.mb_y << 3) + (vec.y >> 3) + i / 2 * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + (i % 2 * 4) + 2;
                dst += 2;
                t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 2, 2);

                vec = t->mb.vec[0][luma_index[4 * i + 2]];
                src_u = t->ref[list_index][vec.refno]->U + ((t->mb.mb_y << 3) + (vec.y >> 3) + i / 2 * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + (i % 2 * 4) + 
                    2 * t->edged_stride_uv;
                dst_u += 2 * 8 - 2;
                t->eighth_pixel_mc_u(src_u, t->edged_stride_uv, dst_u, vec.x, vec.y, 2, 2);
                src = t->ref[list_index][vec.refno]->V + ((t->mb.mb_y << 3) + (vec.y >> 3) + i / 2 * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + (i % 2 * 4) + 
                    2 * t->edged_stride_uv;
                dst += 2 * 8 - 2;
                t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 2, 2);

                vec = t->mb.vec[0][luma_index[4 * i + 3]];
                src_u = t->ref[list_index][vec.refno]->U + ((t->mb.mb_y << 3) + (vec.y >> 3) + i / 2 * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + (i % 2 * 4) +
                    2 * t->edged_stride_uv + 2;
                dst_u += 2;
                t->eighth_pixel_mc_u(src_u, t->edged_stride_uv, dst_u, vec.x, vec.y, 2, 2);
                src = t->ref[list_index][vec.refno]->V + ((t->mb.mb_y << 3) + (vec.y >> 3) + i / 2 * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + (i % 2 * 4) + 
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
