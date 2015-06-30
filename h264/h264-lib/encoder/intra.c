/*****************************************************************************
 *
 *  T264 AVC CODEC
 *
 *  Copyright(C) 2004-2005 llcc <lcgate1@yahoo.com.cn>
 *               2004-2005 visionany <visionany@yahoo.com.cn>
 *
 *	2004.11.18	Cloud Wu < sywu@sohu.com> Add 4x4 Intrmode 3 and 7
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
#include "intra.h"
#include "utility.h"
#include "cavlc.h"
#include "bitstream.h"

//
// NOTE: (t->flags & (INTRA_16x16 | INTRA_4x4)) != 0
//

uint32_t
T264_mode_decision_intra_y(_RW T264_t* t)
{
    uint32_t sad16x16 = -1;
    uint32_t sad4x4   = -1;

    if (t->flags & USE_INTRA16x16)
        sad16x16 = T264_mode_decision_intra_16x16(t);
    if (t->flags & USE_INTRA4x4)
        sad4x4   = T264_mode_decision_intra_4x4(t);

    if (sad16x16 < sad4x4)
    {
        t->mb.mb_mode = I_16x16;
        t->mb.sad = sad16x16;
    }
    else
    {
        t->mb.mb_mode = I_4x4;
        t->mb.sad = sad4x4;
    }

    return t->mb.sad;
}

uint32_t
T264_mode_decision_intra_16x16(_RW T264_t* t)
{
    DECLARE_ALIGNED_MATRIX(pred16x16, 16, 16, uint8_t, CACHE_SIZE);
    DECLARE_ALIGNED_MATRIX(topcache, 1, 16 + CACHE_SIZE, uint8_t, CACHE_SIZE);
    DECLARE_ALIGNED_MATRIX(leftcache, 1, 16 + CACHE_SIZE, uint8_t, CACHE_SIZE);

    uint32_t sad16x16 = -1;
    uint8_t* pred16x16free0 = pred16x16;
    uint8_t* pred16x16free1 = t->mb.pred_i16x16;
    int32_t modes;
    int32_t bestmode;
    int32_t preds[9];
    int32_t i;
    uint8_t* top, *left;

    static const uint8_t fixmode[] = 
    {
        Intra_16x16_TOP,
        Intra_16x16_LEFT,
        Intra_16x16_DC,
        Intra_16x16_PLANE,
        Intra_16x16_DC,
        Intra_16x16_DC,
        Intra_16x16_DC
    };

    top  = &topcache[CACHE_SIZE];
    left = &leftcache[CACHE_SIZE];

    T264_intra_16x16_available(t, preds, &modes, top, left);

    for(i = 0 ; i < modes ; i ++)
    {    
        int32_t mode = preds[i];
        uint32_t sad;

        //
        // pred
        //
        t->pred16x16[mode](
            pred16x16free1,
            16,
            top,
            left);

        //  Now use satd for 16x16 Intra
        //  Thomascatlee@163.com
        
        sad = t->T264_satd_16x16_u(t->mb.src_y, t->stride, pred16x16free1, 16) + 
            t->mb.lambda * eg_size_ue(t->bs, fixmode[mode]);

        if (sad < sad16x16)
        {
            SWAP(uint8_t, pred16x16free0, pred16x16free1);
            sad16x16 = sad;
            bestmode = mode;
        }
    }

    if (pred16x16free0 != t->mb.pred_i16x16)
    {
        memcpy(t->mb.pred_i16x16, pred16x16free0, sizeof(uint8_t) * 16 * 16);
    }

	//fixed prediction mode DCLEFT DCTOP DC128 = DC
    t->mb.mode_i16x16 = fixmode[bestmode];

    return sad16x16;
}

uint32_t
T264_mode_decision_intra_4x4(T264_t* t)
{
    DECLARE_ALIGNED_MATRIX(pred4x40, 4, 5, uint8_t, CACHE_SIZE);
    DECLARE_ALIGNED_MATRIX(pred4x41, 4, 5, uint8_t, CACHE_SIZE);
    DECLARE_ALIGNED_MATRIX(topcache,  8 + CACHE_SIZE, 1, uint8_t, CACHE_SIZE);
    DECLARE_ALIGNED_MATRIX(leftcache, 4 + CACHE_SIZE, 1, uint8_t, CACHE_SIZE);

    uint32_t sad_all = 0;
    uint32_t sad4x4;
    int32_t i, j;
    uint8_t* src;
    uint8_t* dst;
    uint8_t* p0, *p1;
    uint8_t* left;
    uint8_t* top;
    int32_t preds[9];
    int32_t modes;
    int32_t bestmode;

    static const uint8_t fixmode[] = 
    {
        Intra_4x4_TOP,
        Intra_4x4_LEFT,
        Intra_4x4_DC,

        Intra_4x4_DIAGONAL_DOWNLEFT,	
        Intra_4x4_DIAGONAL_DOWNRIGHT,	
        Intra_4x4_VERTICAL_RIGHT,	
        Intra_4x4_HORIZONTAL_DOWN,	
        Intra_4x4_VERTICAL_LEFT,	
        Intra_4x4_HORIZONTAL_UP,	
        Intra_4x4_DC,
        Intra_4x4_DC,
        Intra_4x4_DC
    };

    p0 = pred4x40;
    p1 = pred4x41;
    left = &leftcache[CACHE_SIZE];
    top  = &topcache[CACHE_SIZE];

    for(i = 0 ; i < 16 ; i ++)
    {
        int32_t row = i / 4;
        int32_t col = i % 4;
        int32_t pred_mode;

        src = t->mb.src_y + (row * t->stride << 2) + (col << 2);
        dst = t->mb.dst_y + (row * t->edged_stride << 2) + (col << 2);

        pred_mode = T264_mb_predict_intra4x4_mode(t, luma_index[i]);

        T264_intra_4x4_available(t, i, preds, &modes, dst, left, top);
        sad4x4 = -1;
        for(j = 0 ; j < modes ; j ++)
        {
            uint32_t sad;
            int32_t mode = preds[j];

            t->pred4x4[mode](p1, 4, top, left);

            sad = t->cmp[MB_4x4](src, t->stride, p1, 4) +
                (pred_mode == fixmode[mode] ? 0 : 4 * t->mb.lambda);
                //t->mb.lambda * (pred_mode == fixmode[mode] ? 1 : 4);
            if (sad < sad4x4)
            {
                SWAP(uint8_t, p0, p1);
                sad4x4 = sad;
                bestmode = mode;
            }
        }

		//fixed prediction mode DCLEFT DCTOP DC128 = DC
        t->mb.i4x4_pred_mode_ref[IPM_LUMA + col + row * 8] =
        t->mb.mode_i4x4[luma_index[i]] = fixmode[bestmode];

        sad_all += sad4x4;

        T264_encode_intra_4x4(t, p0, i);
    }
    
    sad_all += t->mb.lambda * 24;
    return sad_all;
}

void
T264_intra_16x16_available(T264_t* t, int32_t preds[], int32_t* modes, uint8_t* top, uint8_t* left)
{
    uint8_t* p;
    int32_t i;

    if ((t->mb.mb_neighbour & (MB_LEFT | MB_TOP)) == (MB_LEFT | MB_TOP))
    {
        preds[0] = Intra_16x16_TOP;
        preds[1] = Intra_16x16_LEFT;
        preds[2] = Intra_16x16_DC;
        preds[3] = Intra_16x16_PLANE;
        *modes = 4;

        p = t->mb.dst_y - t->edged_stride;
        for(i = -1 ; i < 16 ; i ++)
        {
            top[i] = p[i];
        }

        p --;

        for(i = -1 ; i < 16 ; i ++)
        {
            left[i] = p[0];
            p += t->edged_stride;
        }
    }
    else if(t->mb.mb_neighbour & MB_LEFT)
    {
        preds[0] = Intra_16x16_LEFT;
        preds[1] = Intra_16x16_DCLEFT;
        *modes = 2;

        p = t->mb.dst_y - 1;

        for(i = 0 ; i < 16 ; i ++)
        {
            left[i] = p[0];
            p += t->edged_stride;
        }
    }
    else if(t->mb.mb_neighbour & MB_TOP)
    {
        preds[0] = Intra_16x16_TOP;
        preds[1] = Intra_16x16_DCTOP;
        *modes = 2;

        p = t->mb.dst_y - t->edged_stride;
        for(i = 0 ; i < 16 ; i ++)
        {
            top[i] = p[i];
        }
    }
    else
    {
        preds[0] = Intra_16x16_DC128;
        *modes = 1;
    }
}

void
T264_intra_4x4_available(T264_t* t, int32_t idx, int32_t preds[], int32_t* modes, uint8_t* dst, uint8_t* left, uint8_t* top)
{
    static const int32_t neighbour[] =
    {
        0, MB_LEFT, MB_LEFT, MB_LEFT,
        MB_TOP| MB_TOPRIGHT, MB_LEFT| MB_TOP,              MB_LEFT |MB_TOP| MB_TOPRIGHT, MB_LEFT| MB_TOP,
        MB_TOP| MB_TOPRIGHT, MB_LEFT| MB_TOP| MB_TOPRIGHT, MB_LEFT |MB_TOP| MB_TOPRIGHT, MB_LEFT| MB_TOP,
        MB_TOP| MB_TOPRIGHT, MB_LEFT| MB_TOP,              MB_LEFT |MB_TOP| MB_TOPRIGHT, MB_LEFT| MB_TOP
    };
    static const int32_t fix[] =
    {
        ~0, ~0, ~0, ~0,
        ~0, ~MB_TOPRIGHT, ~0, ~MB_TOPRIGHT,
        ~0, ~0, ~0, ~MB_TOPRIGHT,
        ~0, ~MB_TOPRIGHT, ~0, ~MB_TOPRIGHT
    };

    uint8_t* p;
    int32_t i;
    int32_t mb_neighbour = (t->mb.mb_neighbour| neighbour[idx]) & fix[idx];

	if((idx & 3) == 3)	//if is the right-most sub-block
		if(t->mb.mb_x == t->mb_width - 1)	//if is th last MB in horizontal
			mb_neighbour &= ~MB_TOPRIGHT;	//no top-right exist
	if ((mb_neighbour & (MB_LEFT | MB_TOP)) == (MB_LEFT | MB_TOP))
    {
        preds[0] = Intra_4x4_TOP;
        preds[1] = Intra_4x4_LEFT;
        preds[2] = Intra_4x4_DC;

        preds[3] = Intra_4x4_DIAGONAL_DOWNLEFT;
        preds[4] = Intra_4x4_DIAGONAL_DOWNRIGHT;
        preds[5] = Intra_4x4_VERTICAL_RIGHT;
        preds[6] = Intra_4x4_HORIZONTAL_DOWN;
        preds[7] = Intra_4x4_VERTICAL_LEFT;
        preds[8] = Intra_4x4_HORIZONTAL_UP;
        *modes = 9;

        p = dst - t->edged_stride;
        if (mb_neighbour & MB_TOPRIGHT)
        {
            for(i = -1 ; i < 8 ; i ++)
            {
                 top[i] = p[i];
            }
        }
        else
        {
            for(i = -1 ; i < 4 ; i ++)
            {
                 top[i] = p[i];
            }

            //to fill padded 4 positions
            for( ; i < 8 ; ++ i)
                top[i] = p[3];
        }

        p --;

        for(i = -1 ; i < 4 ; i ++)
        {
            left[i] = p[0];
            p += t->edged_stride;
        }
    }
    else if(mb_neighbour & MB_LEFT)
    {
        preds[0] = Intra_4x4_LEFT;
        preds[1] = Intra_4x4_DCLEFT;

        preds[2] = Intra_4x4_HORIZONTAL_UP;
        *modes = 3;

        p = dst - 1;

        for(i = 0 ; i < 4 ; i ++)
        {
            left[i] = p[0];
            p += t->edged_stride;
        }
    }
    else if(mb_neighbour & MB_TOP)
    {
        preds[0] = Intra_4x4_TOP;
        preds[1] = Intra_4x4_DCTOP;
        
        preds[2] = Intra_4x4_DIAGONAL_DOWNLEFT;
        preds[3] = Intra_4x4_VERTICAL_LEFT;
        *modes = 4;

        p = dst - t->edged_stride;
        if (mb_neighbour & MB_TOPRIGHT)
        {
            for(i = -1 ; i < 8 ; i ++)
            {
                top[i] = p[i];
            }
        }
        else
        {
            for(i = -1 ; i < 4 ; i ++)
            {
                top[i] = p[i];
            }
            
            for( ; i < 8 ; ++ i)
                top[i] = p[3];
        }
    }
    else
    {
        preds[0] = Intra_4x4_DC128;
        *modes = 1;
    }
}

void 
T264_encode_intra_y(_RW T264_t* t)
{
    if (t->mb.mb_mode == I_16x16)
    {
        T264_encode_intra_16x16(t);
    }
    else if (t->mb.mb_mode == I_4x4)
    {
    }
}

void
T264_encode_intra_16x16(_RW T264_t* t)
{
    DECLARE_ALIGNED_MATRIX(dct, 17, 16, int16_t, 16);

    int32_t qp = t->qp_y;
    int32_t i;
    int16_t* curdct;

    t->expand8to16sub(t->mb.pred_i16x16, 16 / 4, 16 / 4, dct, t->mb.src_y, t->stride);
    curdct = dct;
    for(i = 0 ; i < 16 ; i ++)
    {
        t->fdct4x4(curdct);
        dct[256 + i] = curdct[0];

        t->quant4x4(curdct, qp, TRUE);
        scan_zig_4x4(t->mb.dct_y_z[luma_index[i]], curdct);
        t->iquant4x4(curdct, qp);

        curdct += 16;
    }

    t->fdct4x4dc(curdct);
    t->quant4x4dc(curdct, qp);
    scan_zig_4x4(t->mb.dc4x4_z, curdct);
    // i don't know why to do so, if someone knows tell me.
    t->idct4x4dc(curdct);
    t->iquant4x4dc(curdct, qp);

    curdct = dct;
    for(i = 0 ; i < 16 ; i ++)
    {
        curdct[0] = dct[256 + i];
        t->idct4x4(curdct);
        curdct += 16;
    }

    t->contract16to8add(dct, 16 / 4, 16 / 4, t->mb.pred_i16x16, t->mb.dst_y, t->edged_stride);
}

void
T264_encode_intra_4x4(_RW T264_t* t, uint8_t* pred, int32_t i)
{
    DECLARE_ALIGNED_MATRIX(dct, 1, 16, int16_t, 16);

    int32_t qp = t->qp_y;
    int32_t row = i / 4;
    int32_t col = i % 4;

    //residual saved in t->pred_16x16_4x4
    uint8_t* src = t->mb.src_y + (row * t->stride << 2) + (col << 2);

    //reconstructed MB saved in t->dst
    uint8_t* dst = t->mb.dst_y + (row * t->edged_stride << 2) + (col << 2);

    t->expand8to16sub(pred, 4 / 4, 4 / 4, dct, src, t->stride);

    t->fdct4x4(dct);
    t->quant4x4(dct, qp, t->slice_type == SLICE_I);
    scan_zig_4x4(t->mb.dct_y_z[luma_index[i]], dct);
    t->iquant4x4(dct, qp);
    t->idct4x4(dct);

    t->contract16to8add(dct, 4 / 4, 4 / 4, pred, dst, t->edged_stride);
}

uint32_t
T264_mode_decision_intra_uv(_RW T264_t* t)
{
    DECLARE_ALIGNED_MATRIX(pred8x8u, 8, 8, uint8_t, CACHE_SIZE);
    DECLARE_ALIGNED_MATRIX(pred8x8v, 8, 8, uint8_t, CACHE_SIZE);
    DECLARE_ALIGNED_MATRIX(topcacheu, 1, 8 + CACHE_SIZE, uint8_t, CACHE_SIZE);
    DECLARE_ALIGNED_MATRIX(leftcacheu, 1, 8 + CACHE_SIZE, uint8_t, CACHE_SIZE);
    DECLARE_ALIGNED_MATRIX(topcachev, 1, 8 + CACHE_SIZE, uint8_t, CACHE_SIZE);
    DECLARE_ALIGNED_MATRIX(leftcachev, 1, 8 + CACHE_SIZE, uint8_t, CACHE_SIZE);
    
    uint32_t sad8x8 = -1;
    uint8_t* pred8x8freeu0 = pred8x8u;
    uint8_t* pred8x8freeu1 = t->mb.pred_i8x8u;
    uint8_t* pred8x8freev0 = pred8x8v;
    uint8_t* pred8x8freev1 = t->mb.pred_i8x8v;
    int32_t modes;
    int32_t bestmode;
    int32_t preds[9];
    int32_t i;
    uint8_t* top_u, *left_u;
    uint8_t* top_v, *left_v;

    static const uint8_t fixmode[] =
    {
        Intra_8x8_DC,
        Intra_8x8_LEFT,
        Intra_8x8_TOP,
        Intra_8x8_PLANE,
        Intra_8x8_DC,
        Intra_8x8_DC,
        Intra_8x8_DC
    };

    top_u  = &topcacheu[CACHE_SIZE];
    top_v  = &topcachev[CACHE_SIZE];
    left_u = &leftcacheu[CACHE_SIZE];
    left_v = &leftcachev[CACHE_SIZE];

    T264_intra_8x8_available(t, preds, &modes, top_u, left_u, top_v, left_v);

    for(i = 0 ; i < modes ; i ++)
    {    
        int32_t mode = preds[i];
        uint32_t sad;

        t->pred8x8[mode](
            pred8x8freeu1,
            8,
            top_u,
            left_u);
        t->pred8x8[mode](
            pred8x8freev1,
            8,
            top_v,
            left_v);

        sad = t->cmp[MB_8x8](t->mb.src_u, t->stride_uv, pred8x8freeu1, 8) + 
              t->cmp[MB_8x8](t->mb.src_v, t->stride_uv, pred8x8freev1, 8) +
              t->mb.lambda * eg_size_ue(t->bs, fixmode[mode]);
        if (sad < sad8x8)
        {
            SWAP(uint8_t, pred8x8freeu0, pred8x8freeu1);
            SWAP(uint8_t, pred8x8freev0, pred8x8freev1);
            sad8x8 = sad;
            bestmode = mode;
        }
    }

    if (pred8x8freeu0 != t->mb.pred_i8x8u)
    {
        memcpy(t->mb.pred_i8x8u, pred8x8freeu0, sizeof(uint8_t) * 8 * 8);
    }
    if (pred8x8freev0 != t->mb.pred_i8x8v)
    {
        memcpy(t->mb.pred_i8x8v, pred8x8freev0, sizeof(uint8_t) * 8 * 8);
    }

	//fixed prediction mode DCLEFT DCTOP DC128 = DC
    t->mb.mb_mode_uv = fixmode[bestmode];

    return sad8x8;
}

void
T264_intra_8x8_available(T264_t* t, int32_t preds[], int32_t* modes, uint8_t* top_u, uint8_t* left_u, uint8_t* top_v, uint8_t* left_v)
{
    int32_t i;
    uint8_t* p_u, *p_v;

    if ((t->mb.mb_neighbour & (MB_LEFT | MB_TOP)) == (MB_LEFT | MB_TOP))
    {
        preds[0] = Intra_8x8_DC;
        preds[1] = Intra_8x8_TOP;
        preds[2] = Intra_8x8_LEFT;
        preds[3] = Intra_8x8_PLANE;
        *modes = 4;

        p_u = t->mb.dst_u - t->edged_stride_uv;
        p_v = t->mb.dst_v - t->edged_stride_uv;
        for(i = -1 ; i < 8 ; i ++)
        {
            top_u[i] = p_u[i];
            top_v[i] = p_v[i];
        }

        p_u --;
        p_v --;

        for(i = -1 ; i < 8 ; i ++)
        {
            left_u[i] = p_u[0];
            left_v[i] = p_v[0];
            p_u += t->edged_stride_uv;
            p_v += t->edged_stride_uv;
        }
    }
    else if(t->mb.mb_neighbour & MB_LEFT)
    {
        preds[0] = Intra_8x8_DCLEFT;
        preds[1] = Intra_8x8_LEFT;
        *modes = 2;

        p_u = t->mb.dst_u - 1;
        p_v = t->mb.dst_v - 1;

        for(i = 0 ; i < 8 ; i ++)
        {
            left_u[i] = p_u[0];
            left_v[i] = p_v[0];
            p_u += t->edged_stride_uv;
            p_v += t->edged_stride_uv;
        }
    }
    else if(t->mb.mb_neighbour & MB_TOP)
    {
        preds[0] = Intra_8x8_DCTOP;
        preds[1] = Intra_8x8_TOP;
        *modes = 2;

        p_u = t->mb.dst_u - t->edged_stride_uv;
        p_v = t->mb.dst_v - t->edged_stride_uv;
        for(i = 0 ; i < 8 ; i ++)
        {
            top_u[i] = p_u[i];
            top_v[i] = p_v[i];
        }
    }
    else
    {
        preds[0] = Intra_8x8_DC128;
        *modes = 1;
    }
}

void
T264_encode_intra_uv(_RW T264_t* t)
{
    DECLARE_ALIGNED_MATRIX(dct, 10, 8, int16_t, CACHE_SIZE);

    int32_t qp = t->qp_uv;
    int32_t i, j;
    int16_t* curdct;
    uint8_t* start;
    uint8_t* dst;
    uint8_t* src;
    int32_t intra = t->slice_type == SLICE_I ? 1 : 0;

    start = t->mb.pred_i8x8u;
    src   = t->mb.src_u;
    dst   = t->mb.dst_u;
    for(j = 0 ; j < 2 ; j ++)
    {
        int32_t run, k;
        int32_t coeff_cost;

        coeff_cost = 0;

        t->expand8to16sub(start, 8 / 4, 8 / 4, dct, src, t->stride_uv);
        curdct = dct;
        for(i = 0 ; i < 4 ; i ++)
        {
            run = -1;

            // we will count coeff cost, from jm80
            t->fdct4x4(curdct);
            dct[64 + i] = curdct[0];

            t->quant4x4(curdct, qp, intra);
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
        t->quant2x2dc(curdct, qp, intra);
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
        start = t->mb.pred_i8x8v;
        dst   = t->mb.dst_v;
        src   = t->mb.src_v;
    }
}
