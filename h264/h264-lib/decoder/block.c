/*****************************************************************************
*
*  T264 AVC CODEC
*
*  Copyright(C) 2004-2005 llcc <lcgate1@yahoo.com.cn>
*               2004-2005 visionany <visionany@yahoo.com.cn>
*   2005.2.24 CloudWu<sywu@sohu.com>	added support for B-frame MB16x16 support 
*   2005.3.2 CloudWu<sywu@sohu.com>	added support for B-frame MB16x8 and MB8x16,MB8x8 support
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
#include "utility.h"
#ifndef CHIP_DM642
#include "memory.h"
#endif
#include "assert.h"
#include "block.h"

/* intra */

static void __inline
T264dec_mb_decode_predict_i16x16_y(T264_t* t, uint8_t mode, uint8_t* pred, uint8_t* src)
{
    DECLARE_ALIGNED_MATRIX(topcache, 1, 16 + CACHE_SIZE, uint8_t, CACHE_SIZE);
    DECLARE_ALIGNED_MATRIX(leftcache, 1, 16 + CACHE_SIZE, uint8_t, CACHE_SIZE);

    uint8_t* p;
    int32_t i;
    uint8_t* top, *left;

    top  =  &topcache[CACHE_SIZE];
    left = &leftcache[CACHE_SIZE];

    if (mode == Intra_16x16_DC)
    {
        if ((t->mb.mb_neighbour & (MB_LEFT | MB_TOP)) == (MB_LEFT | MB_TOP))
        {
            mode = Intra_16x16_DC;

            p = src - t->edged_stride;
            for(i = 0 ; i < 16 ; i ++)
            {
                top[i] = p[i];
            }

            p = src - 1;
            for(i = 0 ; i < 16 ; i ++)
            {
                left[i] = p[0];
                p += t->edged_stride;
            }
        }
        else if(t->mb.mb_neighbour & MB_LEFT)
        {
            mode = Intra_16x16_DCLEFT;

            p = src - 1;

            for(i = 0 ; i < 16 ; i ++)
            {
                left[i] = p[0];
                p += t->edged_stride;
            }
        }
        else if(t->mb.mb_neighbour & MB_TOP)
        {
            mode = Intra_16x16_DCTOP;

            p = src - t->edged_stride;
            for(i = 0 ; i < 16 ; i ++)
            {
                top[i] = p[i];
            }
        }
        else
        {
            mode = Intra_16x16_DC128;
        }
    }
    else
    {
        switch(mode)
        {
        case Intra_16x16_TOP:
            p = src - t->edged_stride;
            for(i = 0 ; i < 16 ; i ++)
            {
                top[i] = p[i];
            }
            break;
        case Intra_16x16_LEFT:
            p = src - 1;

            for(i = 0 ; i < 16 ; i ++)
            {
                left[i] = p[0];
                p += t->edged_stride;
            }
            break;
        case Intra_16x16_PLANE:
            p = src - t->edged_stride;
            for(i = -1 ; i < 16 ; i ++)
            {
                top[i] = p[i];
            }

            p -= 1;
            for(i = -1 ; i < 16 ; i ++)
            {
                left[i] = p[0];
                p += t->edged_stride;
            }
            break;
        default:
            assert(0);
            break;
        }
    }

    t->pred16x16[mode](pred, 16, top, left);
}


static void __inline
T264dec_mb_decode_predict_i4x4_y(T264_t* t, uint8_t idx, uint8_t mode, uint8_t* pred, uint8_t* src)
{
    DECLARE_ALIGNED_MATRIX(topcache,  8 + CACHE_SIZE, 1, uint8_t, CACHE_SIZE);
    DECLARE_ALIGNED_MATRIX(leftcache, 4 + CACHE_SIZE, 1, uint8_t, CACHE_SIZE);

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
    uint8_t* top  = &topcache[CACHE_SIZE];
    uint8_t* left = &leftcache[CACHE_SIZE];

    if (mode == Intra_4x4_DC)
    {
        int32_t mb_neighbour = (t->mb.mb_neighbour| neighbour[idx]) & fix[idx];
        if ((mb_neighbour & (MB_LEFT | MB_TOP)) == (MB_LEFT | MB_TOP))
        {
            mode = Intra_4x4_DC;

            p = src - t->edged_stride;
            for(i = 0 ; i < 4 ; i ++)
            {
                top[i] = p[i];
            }

            p = src - 1;
            for(i = 0 ; i < 4 ; i ++)
            {
                left[i] = p[0];
                p += t->edged_stride;
            }
        }
        else if(mb_neighbour & MB_LEFT)
        {
            mode = Intra_4x4_DCLEFT;

            p = src - 1;

            for(i = 0 ; i < 4 ; i ++)
            {
                left[i] = p[0];
                p += t->edged_stride;
            }
        }
        else if(mb_neighbour & MB_TOP)
        {
            mode = Intra_4x4_DCTOP;

            p = src - t->edged_stride;
            for(i = 0 ; i < 4 ; i ++)
            {
                top[i] = p[i];
            }
        }
        else
        {
            mode = Intra_4x4_DC128;
        }
    }
    else
    {
        switch(mode)
        {
        case Intra_4x4_TOP:
            p = src - t->edged_stride;
            for(i = 0 ; i < 4 ; i ++)
            {
                top[i] = p[i];
            }
            break;
        case Intra_4x4_LEFT:
        case Intra_4x4_HORIZONTAL_UP:
            p = src - 1;
            for(i = 0 ; i < 4 ; i ++)
            {
                left[i] = p[0];
                p += t->edged_stride;
            }
            break;
        case Intra_4x4_DIAGONAL_DOWNLEFT:
        case Intra_4x4_VERTICAL_LEFT:
            {
                int32_t mb_neighbour = (t->mb.mb_neighbour| neighbour[idx]) & fix[idx];
            
                p = src - t->edged_stride;
                if((idx & 3) == 3 && t->mb.mb_x == t->mb_width - 1)    //if is the right-most sub-block, if is th last MB in horizontal, no top-right exist
                    mb_neighbour &= ~MB_TOPRIGHT;

                if (mb_neighbour & MB_TOPRIGHT)
                {
                    for(i = 0 ; i < 8 ; i ++)
                    {
                        top[i] = p[i];
                    }
                }
                else
                {
                    for(i = 0 ; i < 4 ; i ++)
                    {
                        top[i] = p[i];
                    }
                    top[4] = p[3];
                    top[5] = p[3];
                    top[6] = p[3];
                    top[7] = p[3];
                }
            }
            break;
        case Intra_4x4_DIAGONAL_DOWNRIGHT:
        case Intra_4x4_VERTICAL_RIGHT:
        case Intra_4x4_HORIZONTAL_DOWN:
            p = src - t->edged_stride;
            for(i = -1 ; i < 4 ; i ++)
            {
                top[i] = p[i];
            }

            p -= 1;
            for(i = -1 ; i < 4 ; i ++)
            {
                left[i] = p[0];
                p += t->edged_stride;
            }
            break;
        default:
            assert(0);
            break;
        }
    }

    t->pred4x4[mode](pred, 4, top, left);
}

static void __inline
T264dec_mb_decode_predict_i8x8_y(T264_t* t, uint8_t mode, uint8_t* pred_u, uint8_t* pred_v)
{
    DECLARE_ALIGNED_MATRIX(topcacheu, 1, 8 + CACHE_SIZE, uint8_t, CACHE_SIZE);
    DECLARE_ALIGNED_MATRIX(leftcacheu, 1, 8 + CACHE_SIZE, uint8_t, CACHE_SIZE);
    DECLARE_ALIGNED_MATRIX(topcachev, 1, 8 + CACHE_SIZE, uint8_t, CACHE_SIZE);
    DECLARE_ALIGNED_MATRIX(leftcachev, 1, 8 + CACHE_SIZE, uint8_t, CACHE_SIZE);

    uint8_t* p_u, *p_v;
    int32_t i;
    uint8_t* top_u, *left_u;
    uint8_t* top_v, *left_v;

    top_u  = &topcacheu[CACHE_SIZE];
    top_v  = &topcachev[CACHE_SIZE];
    left_u = &leftcacheu[CACHE_SIZE];
    left_v = &leftcachev[CACHE_SIZE];

    if (mode == Intra_8x8_DC)
    {
        if ((t->mb.mb_neighbour & (MB_LEFT | MB_TOP)) == (MB_LEFT | MB_TOP))
        {
            mode = Intra_8x8_DC;

            p_u = t->mb.src_u - t->edged_stride_uv;
            p_v = t->mb.src_v - t->edged_stride_uv;
            for(i = 0 ; i < 8 ; i ++)
            {
                top_u[i] = p_u[i];
                top_v[i] = p_v[i];
            }

            p_u = t->mb.src_u - 1;
            p_v = t->mb.src_v - 1;
            for(i = 0 ; i < 8 ; i ++)
            {
                left_u[i] = p_u[0];
                left_v[i] = p_v[0];
                p_u += t->edged_stride_uv;
                p_v += t->edged_stride_uv;
            }
        }
        else if(t->mb.mb_neighbour & MB_LEFT)
        {
            mode = Intra_8x8_DCLEFT;

            p_u = t->mb.src_u - 1;
            p_v = t->mb.src_v - 1;

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
            mode = Intra_8x8_DCTOP;

            p_u = t->mb.src_u - t->edged_stride_uv;
            p_v = t->mb.src_v - t->edged_stride_uv;
            for(i = 0 ; i < 8 ; i ++)
            {
                top_u[i] = p_u[i];
                top_v[i] = p_v[i];
            }
        }
        else
        {
            mode = Intra_8x8_DC128;
        }
    }
    else
    {
        switch(mode)
        {
        case Intra_8x8_TOP:
            p_u = t->mb.src_u - t->edged_stride_uv;
            p_v = t->mb.src_v - t->edged_stride_uv;
            for(i = 0 ; i < 8 ; i ++)
            {
                top_u[i] = p_u[i];
                top_v[i] = p_v[i];
            }
            break;
        case Intra_8x8_LEFT:
            p_u = t->mb.src_u - 1;
            p_v = t->mb.src_v - 1;

            for(i = 0 ; i < 8 ; i ++)
            {
                left_u[i] = p_u[0];
                left_v[i] = p_v[0];
                p_u += t->edged_stride_uv;
                p_v += t->edged_stride_uv;
            }
            break;
        case Intra_8x8_PLANE:
            p_u = t->mb.src_u - t->edged_stride_uv;
            p_v = t->mb.src_v - t->edged_stride_uv;
            for(i = -1 ; i < 8 ; i ++)
            {
                top_u[i] = p_u[i];
                top_v[i] = p_v[i];
            }

            p_u -= 1;
            p_v -= 1;
            for(i = -1 ; i < 8 ; i ++)
            {
                left_u[i] = p_u[0];
                p_u += t->edged_stride_uv;
                left_v[i] = p_v[0];
                p_v += t->edged_stride_uv;
            }
            break;
        default:
            assert(0);
            break;
        }
    }

    t->pred8x8[mode](pred_u, 8, top_u, left_u);
    t->pred8x8[mode](pred_v, 8, top_v, left_v);
}


static void __inline
T264dec_mb_decode_i16x16_y(T264_t* t)
{
    DECLARE_ALIGNED_MATRIX(dct, 1+16, 16, int16_t, CACHE_SIZE);
 
    int32_t qp = t->qp_y;
    int32_t i;
    int16_t* curdct;
    uint8_t* src;
    
    src = t->mb.src_y;

    T264dec_mb_decode_predict_i16x16_y(t, t->mb.mode_i16x16, t->mb.pred_i16x16, src);

    unscan_zig_4x4( t->mb.dc4x4_z, dct + 256 );
    t->iquant4x4dc(dct + 256, qp);
    t->idct4x4dc(dct + 256);

    curdct = dct;
    for( i = 0; i < 16; i++ )
    {
        unscan_zig_4x4( t->mb.dct_y_z[luma_index[i]], curdct );
        t->iquant4x4( curdct, qp );
        curdct[0] = dct[256 + i];
        t->idct4x4(curdct);
        curdct += 16;
    }

    t->contract16to8add(dct, 16 / 4, 16 / 4, t->mb.pred_i16x16, src, t->edged_stride);
}

static void __inline
T264dec_mb_decode_i4x4_y(T264_t* t)
{
    DECLARE_ALIGNED_MATRIX(pred, 4, 5, uint8_t, CACHE_SIZE);
    DECLARE_ALIGNED_MATRIX(dct, 1, 16, int16_t, 16);

    int32_t qp = t->qp_y;

    int32_t i;
    uint8_t* src;

    for(i = 0 ; i < 16 ; i ++)
    {
        int32_t row = i / 4;
        int32_t col = i % 4;

        src = t->mb.src_y + (row * t->edged_stride << 2) + (col << 2);

        T264dec_mb_decode_predict_i4x4_y(t, i, t->mb.mode_i4x4[luma_index[i]], pred, src);

        unscan_zig_4x4(t->mb.dct_y_z[luma_index[i]], dct);

        t->iquant4x4(dct, qp);
        t->idct4x4(dct);

        t->contract16to8add(dct, 4 / 4, 4 / 4, pred, src, t->edged_stride);
    }
}

void
T264dec_mb_decode_intra_y(T264_t* t)
{
    if (t->mb.mb_mode == I_4x4)
        T264dec_mb_decode_i4x4_y(t);
    else
        T264dec_mb_decode_i16x16_y(t);
}

void
T264dec_mb_decode_uv(T264_t* t, uint8_t* pred_u, uint8_t* pred_v)
{
    DECLARE_ALIGNED_MATRIX(dct, 10, 8, int16_t, CACHE_SIZE);

    int32_t qp = t->qp_uv;
    int32_t i, j;
    int16_t* curdct;
    uint8_t* start;
    uint8_t* src;

    start = pred_u;
    src   = t->mb.src_u;
    
    for(j = 0 ; j < 2 ; j ++)
    {
        unscan_zig_2x2(t->mb.dc2x2_z[j], dct + 64);
        t->iquant2x2dc(dct + 64, qp);
        t->idct2x2dc(dct + 64);

        curdct = dct;
        for(i = 0 ; i < 4 ; i ++)
        {
            unscan_zig_4x4(t->mb.dct_uv_z[j][i], curdct);
            t->iquant4x4(curdct, qp);
            curdct[0] = dct[64 + i];
            t->idct4x4(curdct);
            curdct += 16;
        }

        t->contract16to8add(dct, 8 / 4, 8 / 4, start, src, t->edged_stride_uv);

        //
        // change to v
        //
        start = pred_v;
        src   = t->mb.src_v;
    }
}

void
T264dec_mb_decode_intra_uv(T264_t* t)
{
    T264dec_mb_decode_predict_i8x8_y(t, t->mb.mb_mode_uv, t->mb.pred_i8x8u, t->mb.pred_i8x8v);

    T264dec_mb_decode_uv(t, t->mb.pred_i8x8u, t->mb.pred_i8x8v);
}
	
void
T264dec_mb_decode_interp_mc(T264_t* t, uint8_t* ref)
{
    T264_vector_t vec;
    uint8_t* tmp;
    int32_t x, y;
    int32_t i;
    int32_t list_idx = 0;
///////poly
    //static 
	const int8_t index_[4][4][6] = 
    {
        {{0, 0, 0, 0, 0, 0}, {0, 1, 0, 0, 0, 0}, {1, 1, 0, 0, 0, 0}, {1, 0, 0, 0, 1, 0}},
        {{0, 2, 0, 0, 0, 0}, {1, 2, 0, 0, 0, 0}, {1, 3, 0, 0, 0, 0}, {1, 2, 0, 0, 1, 0}},
        {{2, 2, 0, 0, 0, 0}, {2, 3, 0, 0, 0, 0}, {3, 3, 0, 0, 0, 0}, {3, 2, 0, 0, 1, 0}},
        {{2, 0, 0, 0, 0, 1}, {2, 1, 0, 0, 0, 1}, {3, 1, 0, 0, 0, 1}, {1, 2, 0, 1, 1, 0}}
    };

	
    switch(t->mb.mb_part)
    {
    case MB_16x16:
        vec = t->mb.vec[0][0];
        x = (vec.x & 3);
        y = (vec.y & 3);

        if (index_[y][x][0] == index_[y][x][1])
        {
            tmp = t->ref[list_idx][vec.refno]->Y[index_[y][x][0]] + ((t->mb.mb_y << 4) + (vec.y >> 2)) * t->edged_stride + 
                ((t->mb.mb_x << 4) + (vec.x >> 2));
            t->memcpy_stride_u(tmp, 16, 16, t->edged_stride, ref, 16);
        }
        else
        {
            t->pia[MB_16x16](t->ref[list_idx][vec.refno]->Y[index_[y][x][0]] + ((t->mb.mb_y << 4) + (vec.y >> 2) + index_[y][x][3]) * t->edged_stride + (t->mb.mb_x << 4) + (vec.x >> 2) + index_[y][x][2], 
                t->ref[list_idx][vec.refno]->Y[index_[y][x][1]] + ((t->mb.mb_y << 4) + (vec.y >> 2) + index_[y][x][5]) * t->edged_stride + (t->mb.mb_x << 4) + (vec.x >> 2) + index_[y][x][4],
                t->edged_stride, t->edged_stride, ref, 16);
        }
        break;
    case MB_16x8:
        vec = t->mb.vec[0][0];
        x = (vec.x & 3);
        y = (vec.y & 3);

        if (index_[y][x][0] == index_[y][x][1])
        {
            tmp = t->ref[list_idx][vec.refno]->Y[index_[y][x][0]] + ((t->mb.mb_y << 4) + (vec.y >> 2)) * t->edged_stride + 
                ((t->mb.mb_x << 4) + (vec.x >> 2));
            t->memcpy_stride_u(tmp, 16, 8, t->edged_stride, ref, 16);
        }
        else
        {
            t->pia[MB_16x8](t->ref[list_idx][vec.refno]->Y[index_[y][x][0]] + ((t->mb.mb_y << 4) + (vec.y >> 2) + index_[y][x][3]) * t->edged_stride + (t->mb.mb_x << 4) + (vec.x >> 2) + index_[y][x][2], 
                t->ref[list_idx][vec.refno]->Y[index_[y][x][1]] + ((t->mb.mb_y << 4) + (vec.y >> 2) + index_[y][x][5]) * t->edged_stride + (t->mb.mb_x << 4) + (vec.x >> 2) + index_[y][x][4],
                t->edged_stride, t->edged_stride, ref, 16);
        }

        vec = t->mb.vec[0][8];
        x = (vec.x & 3);
        y = (vec.y & 3);

        if (index_[y][x][0] == index_[y][x][1])
        {
            tmp = t->ref[list_idx][vec.refno]->Y[index_[y][x][0]] + ((t->mb.mb_y << 4) + (vec.y >> 2) + 8) * t->edged_stride + 
                ((t->mb.mb_x << 4) + (vec.x >> 2));
            t->memcpy_stride_u(tmp, 16, 8, t->edged_stride, ref + 16 * 8, 16);
        }
        else
        {
            t->pia[MB_16x8](t->ref[list_idx][vec.refno]->Y[index_[y][x][0]] + ((t->mb.mb_y << 4) + (vec.y >> 2) + index_[y][x][3] + 8) * t->edged_stride + (t->mb.mb_x << 4) + (vec.x >> 2) + index_[y][x][2], 
                t->ref[list_idx][vec.refno]->Y[index_[y][x][1]] + ((t->mb.mb_y << 4) + (vec.y >> 2) + index_[y][x][5] + 8) * t->edged_stride + (t->mb.mb_x << 4) + (vec.x >> 2) + index_[y][x][4],
                t->edged_stride, t->edged_stride, ref + 16 * 8, 16);
        }
        break;
    case MB_8x16:
        vec = t->mb.vec[0][0];
        x = (vec.x & 3);
        y = (vec.y & 3);

        if (index_[y][x][0] == index_[y][x][1])
        {
            tmp = t->ref[list_idx][vec.refno]->Y[index_[y][x][0]] + ((t->mb.mb_y << 4) + (vec.y >> 2)) * t->edged_stride + 
                ((t->mb.mb_x << 4) + (vec.x >> 2));
            t->memcpy_stride_u(tmp, 8, 16, t->edged_stride, ref, 16);
        }
        else
        {
            t->pia[MB_8x16](t->ref[list_idx][vec.refno]->Y[index_[y][x][0]] + ((t->mb.mb_y << 4) + (vec.y >> 2) + index_[y][x][3]) * t->edged_stride + (t->mb.mb_x << 4) + (vec.x >> 2) + index_[y][x][2], 
                t->ref[list_idx][vec.refno]->Y[index_[y][x][1]] + ((t->mb.mb_y << 4) + (vec.y >> 2) + index_[y][x][5]) * t->edged_stride + (t->mb.mb_x << 4) + (vec.x >> 2) + index_[y][x][4],
                t->edged_stride, t->edged_stride, ref, 16);
        }

        vec = t->mb.vec[0][2];
        x = (vec.x & 3);
        y = (vec.y & 3);

        if (index_[y][x][0] == index_[y][x][1])
        {
            tmp = t->ref[list_idx][vec.refno]->Y[index_[y][x][0]] + ((t->mb.mb_y << 4) + (vec.y >> 2)) * t->edged_stride + 
                ((t->mb.mb_x << 4) + (vec.x >> 2)) + 8;
            t->memcpy_stride_u(tmp, 8, 16, t->edged_stride, ref + 8, 16);
        }
        else
        {
            t->pia[MB_8x16](t->ref[list_idx][vec.refno]->Y[index_[y][x][0]] + ((t->mb.mb_y << 4) + (vec.y >> 2) + index_[y][x][3]) * t->edged_stride + (t->mb.mb_x << 4) + (vec.x >> 2) + index_[y][x][2] + 8, 
                t->ref[list_idx][vec.refno]->Y[index_[y][x][1]] + ((t->mb.mb_y << 4) + (vec.y >> 2) + index_[y][x][5]) * t->edged_stride + (t->mb.mb_x << 4) + (vec.x >> 2) + index_[y][x][4] + 8,
                t->edged_stride, t->edged_stride, ref + 8, 16);
        }
        break;
    case MB_8x8:
    case MB_8x8ref0:
        for(i = 0 ; i < 4 ; i ++)
        {
            int32_t offset1, offset2;
            switch(t->mb.submb_part[luma_index[4 * i]]) 
            {
            case MB_8x8:
                vec = t->mb.vec[0][luma_index[4 * i]];
                x = (vec.x & 3);
                y = (vec.y & 3);

                if (index_[y][x][0] == index_[y][x][1])
                {
                    offset1 = ((t->mb.mb_y << 4) + (vec.y >> 2) + i / 2 * 8) * t->edged_stride + ((t->mb.mb_x << 4) + (vec.x >> 2)) + i % 2 * 8;
                    tmp = t->ref[list_idx][vec.refno]->Y[index_[y][x][0]] + offset1;
                    t->memcpy_stride_u(tmp, 8, 8, t->edged_stride, ref + i / 2 * 16 * 8 + i % 2 * 8, 16);
                }
                else
                {
                    offset1 = ((t->mb.mb_y << 4) + (vec.y >> 2) + index_[y][x][3] + i / 2 * 8) * t->edged_stride + (t->mb.mb_x << 4) + (vec.x >> 2) + index_[y][x][2] + i % 2 * 8;
                    offset2 = ((t->mb.mb_y << 4) + (vec.y >> 2) + index_[y][x][5] + i / 2 * 8) * t->edged_stride + (t->mb.mb_x << 4) + (vec.x >> 2) + index_[y][x][4] + i % 2 * 8;
                    t->pia[MB_8x8](t->ref[list_idx][vec.refno]->Y[index_[y][x][0]] + offset1, 
                        t->ref[list_idx][vec.refno]->Y[index_[y][x][1]] + offset2,
                        t->edged_stride, t->edged_stride, ref + i / 2 * 16 * 8 + i % 2 * 8, 
                        16);
                }
                break;
            case MB_8x4:
                vec = t->mb.vec[0][luma_index[4 * i]];
                x = (vec.x & 3);
                y = (vec.y & 3);

                if (index_[y][x][0] == index_[y][x][1])
                {
                    offset1 = ((t->mb.mb_y << 4) + (vec.y >> 2) + i / 2 * 8) * t->edged_stride + ((t->mb.mb_x << 4) + (vec.x >> 2)) + i % 2 * 8;
                    tmp = t->ref[list_idx][vec.refno]->Y[index_[y][x][0]] + offset1;
                    t->memcpy_stride_u(tmp, 8, 4, t->edged_stride, ref + i / 2 * 16 * 8 + i % 2 * 8, 16);
                }
                else
                {
                    offset1 = ((t->mb.mb_y << 4) + (vec.y >> 2) + index_[y][x][3] + i / 2 * 8) * t->edged_stride + (t->mb.mb_x << 4) + (vec.x >> 2) + index_[y][x][2] + i % 2 * 8;
                    offset2 = ((t->mb.mb_y << 4) + (vec.y >> 2) + index_[y][x][5] + i / 2 * 8) * t->edged_stride + (t->mb.mb_x << 4) + (vec.x >> 2) + index_[y][x][4] + i % 2 * 8;
                    t->pia[MB_8x4](t->ref[list_idx][vec.refno]->Y[index_[y][x][0]] + offset1, 
                        t->ref[list_idx][vec.refno]->Y[index_[y][x][1]] + offset2,
                        t->edged_stride, t->edged_stride, ref + i / 2 * 16 * 8 + i % 2 * 8, 
                        16);
                }

                vec = t->mb.vec[0][luma_index[4 * i + 2]];
                x = (vec.x & 3);
                y = (vec.y & 3);

                if (index_[y][x][0] == index_[y][x][1])
                {
                    offset1 = ((t->mb.mb_y << 4) + (vec.y >> 2) + i / 2 * 8 + 4) * t->edged_stride + ((t->mb.mb_x << 4) + (vec.x >> 2)) + i % 2 * 8;
                    tmp = t->ref[list_idx][vec.refno]->Y[index_[y][x][0]] + offset1;
                    t->memcpy_stride_u(tmp, 8, 4, t->edged_stride, ref + i / 2  * 16 * 8 + 64 + i % 2 * 8, 16);
                }
                else
                {
                    offset1 = ((t->mb.mb_y << 4) + (vec.y >> 2) + index_[y][x][3] + i / 2 * 8 + 4) * t->edged_stride + (t->mb.mb_x << 4) + (vec.x >> 2) + index_[y][x][2] + i % 2 * 8;
                    offset2 = ((t->mb.mb_y << 4) + (vec.y >> 2) + index_[y][x][5] + i / 2 * 8 + 4) * t->edged_stride + (t->mb.mb_x << 4) + (vec.x >> 2) + index_[y][x][4] + i % 2 * 8;
                    t->pia[MB_8x4](t->ref[list_idx][vec.refno]->Y[index_[y][x][0]] + offset1, 
                        t->ref[list_idx][vec.refno]->Y[index_[y][x][1]] + offset2,
                        t->edged_stride, t->edged_stride, ref + i / 2 * 16 * 8 + i % 2 * 8 + 64, 
                        16);
                }
                break;
            case MB_4x8:
                vec = t->mb.vec[0][luma_index[4 * i]];
                x = (vec.x & 3);
                y = (vec.y & 3);

                if (index_[y][x][0] == index_[y][x][1])
                {
                    offset1 = ((t->mb.mb_y << 4) + (vec.y >> 2) + i / 2 * 8) * t->edged_stride + ((t->mb.mb_x << 4) + (vec.x >> 2)) + i % 2 * 8;
                    tmp = t->ref[list_idx][vec.refno]->Y[index_[y][x][0]] + offset1;
                    t->memcpy_stride_u(tmp, 4, 8, t->edged_stride, ref + i / 2 * 16 * 8 + i % 2 * 8, 16);
                }
                else
                {
                    offset1 = ((t->mb.mb_y << 4) + (vec.y >> 2) + index_[y][x][3] + i / 2 * 8) * t->edged_stride + (t->mb.mb_x << 4) + (vec.x >> 2) + index_[y][x][2] + i % 2 * 8;
                    offset2 = ((t->mb.mb_y << 4) + (vec.y >> 2) + index_[y][x][5] + i / 2 * 8) * t->edged_stride + (t->mb.mb_x << 4) + (vec.x >> 2) + index_[y][x][4] + i % 2 * 8;
                    t->pia[MB_4x8](t->ref[list_idx][vec.refno]->Y[index_[y][x][0]] + offset1, 
                        t->ref[list_idx][vec.refno]->Y[index_[y][x][1]] + offset2,
                        t->edged_stride, t->edged_stride, ref + i / 2 * 16 * 8 + i % 2 * 8, 
                        16);
                }

                vec = t->mb.vec[0][luma_index[4 * i + 1]];
                x = (vec.x & 3);
                y = (vec.y & 3);

                if (index_[y][x][0] == index_[y][x][1])
                {
                    offset1 = ((t->mb.mb_y << 4) + (vec.y >> 2) + i / 2 * 8) * t->edged_stride + ((t->mb.mb_x << 4) + (vec.x >> 2)) + i % 2 * 8 + 4;
                    tmp = t->ref[list_idx][vec.refno]->Y[index_[y][x][0]] + offset1;
                    t->memcpy_stride_u(tmp, 4, 8, t->edged_stride, ref + i / 2  * 16 * 8 + i % 2 * 8 + 4, 16);
                }
                else
                {
                    offset1 = ((t->mb.mb_y << 4) + (vec.y >> 2) + index_[y][x][3] + i / 2 * 8) * t->edged_stride + (t->mb.mb_x << 4) + (vec.x >> 2) + index_[y][x][2] + i % 2 * 8 + 4;
                    offset2 = ((t->mb.mb_y << 4) + (vec.y >> 2) + index_[y][x][5] + i / 2 * 8) * t->edged_stride + (t->mb.mb_x << 4) + (vec.x >> 2) + index_[y][x][4] + i % 2 * 8 + 4;
                    t->pia[MB_4x8](t->ref[list_idx][vec.refno]->Y[index_[y][x][0]] + offset1, 
                        t->ref[list_idx][vec.refno]->Y[index_[y][x][1]] + offset2,
                        t->edged_stride, t->edged_stride, ref + i / 2 * 16 * 8 + i % 2 * 8 + 4, 
                        16);
                }
                break;
            case MB_4x4:
                vec = t->mb.vec[0][luma_index[4 * i]];
                x = (vec.x & 3);
                y = (vec.y & 3);

                if (index_[y][x][0] == index_[y][x][1])
                {
                    offset1 = ((t->mb.mb_y << 4) + (vec.y >> 2) + i / 2 * 8) * t->edged_stride + ((t->mb.mb_x << 4) + (vec.x >> 2)) + i % 2 * 8;
                    tmp = t->ref[list_idx][vec.refno]->Y[index_[y][x][0]] + offset1;
                    t->memcpy_stride_u(tmp, 4, 4, t->edged_stride, ref + i / 2 * 16 * 8 + i % 2 * 8, 16);
                }
                else
                {
                    offset1 = ((t->mb.mb_y << 4) + (vec.y >> 2) + index_[y][x][3] + i / 2 * 8) * t->edged_stride + (t->mb.mb_x << 4) + (vec.x >> 2) + index_[y][x][2] + i % 2 * 8;
                    offset2 = ((t->mb.mb_y << 4) + (vec.y >> 2) + index_[y][x][5] + i / 2 * 8) * t->edged_stride + (t->mb.mb_x << 4) + (vec.x >> 2) + index_[y][x][4] + i % 2 * 8;
                    t->pia[MB_4x4](t->ref[list_idx][vec.refno]->Y[index_[y][x][0]] + offset1, 
                        t->ref[list_idx][vec.refno]->Y[index_[y][x][1]] + offset2,
                        t->edged_stride, t->edged_stride, ref + i / 2 * 16 * 8 + i % 2 * 8, 
                        16);
                }

                vec = t->mb.vec[0][luma_index[4 * i + 1]];
                x = (vec.x & 3);
                y = (vec.y & 3);

                if (index_[y][x][0] == index_[y][x][1])
                {
                    offset1 = ((t->mb.mb_y << 4) + (vec.y >> 2) + i / 2 * 8) * t->edged_stride + ((t->mb.mb_x << 4) + (vec.x >> 2)) + i % 2 * 8 + 4;
                    tmp = t->ref[list_idx][vec.refno]->Y[index_[y][x][0]] + offset1;
                    t->memcpy_stride_u(tmp, 4, 4, t->edged_stride, ref + i / 2  * 16 * 8 + i % 2 * 8 + 4, 16);
                }
                else
                {
                    offset1 = ((t->mb.mb_y << 4) + (vec.y >> 2) + index_[y][x][3] + i / 2 * 8) * t->edged_stride + (t->mb.mb_x << 4) + (vec.x >> 2) + index_[y][x][2] + i % 2 * 8 + 4;
                    offset2 = ((t->mb.mb_y << 4) + (vec.y >> 2) + index_[y][x][5] + i / 2 * 8) * t->edged_stride + (t->mb.mb_x << 4) + (vec.x >> 2) + index_[y][x][4] + i % 2 * 8 + 4;
                    t->pia[MB_4x4](t->ref[list_idx][vec.refno]->Y[index_[y][x][0]] + offset1, 
                        t->ref[list_idx][vec.refno]->Y[index_[y][x][1]] + offset2,
                        t->edged_stride, t->edged_stride, ref + i / 2 * 16 * 8 + i % 2 * 8 + 4, 
                        16);
                }

                vec = t->mb.vec[0][luma_index[4 * i + 2]];
                x = (vec.x & 3);
                y = (vec.y & 3);

                if (index_[y][x][0] == index_[y][x][1])
                {
                    offset1 = ((t->mb.mb_y << 4) + (vec.y >> 2) + i / 2 * 8 + 4) * t->edged_stride + ((t->mb.mb_x << 4) + (vec.x >> 2)) + i % 2 * 8;
                    tmp = t->ref[list_idx][vec.refno]->Y[index_[y][x][0]] + offset1;
                    t->memcpy_stride_u(tmp, 4, 4, t->edged_stride, ref + i / 2  * 16 * 8 + 64 + i % 2 * 8, 16);
                }
                else
                {
                    offset1 = ((t->mb.mb_y << 4) + (vec.y >> 2) + index_[y][x][3] + i / 2 * 8 + 4) * t->edged_stride + (t->mb.mb_x << 4) + (vec.x >> 2) + index_[y][x][2] + i % 2 * 8;
                    offset2 = ((t->mb.mb_y << 4) + (vec.y >> 2) + index_[y][x][5] + i / 2 * 8 + 4) * t->edged_stride + (t->mb.mb_x << 4) + (vec.x >> 2) + index_[y][x][4] + i % 2 * 8;
                    t->pia[MB_4x4](t->ref[list_idx][vec.refno]->Y[index_[y][x][0]] + offset1, 
                        t->ref[list_idx][vec.refno]->Y[index_[y][x][1]] + offset2,
                        t->edged_stride, t->edged_stride, ref + i / 2 * 16 * 8 + i % 2 * 8 + 64, 
                        16);
                }

                vec = t->mb.vec[0][luma_index[4 * i + 3]];
                x = (vec.x & 3);
                y = (vec.y & 3);

                if (index_[y][x][0] == index_[y][x][1])
                {
                    offset1 = ((t->mb.mb_y << 4) + (vec.y >> 2) + i / 2 * 8 + 4) * t->edged_stride + ((t->mb.mb_x << 4) + (vec.x >> 2)) + i % 2 * 8 + 4;
                    tmp = t->ref[list_idx][vec.refno]->Y[index_[y][x][0]] + offset1;
                    t->memcpy_stride_u(tmp, 4, 4, t->edged_stride, ref + i / 2  * 16 * 8 + 64 + i % 2 * 8 + 4, 16);
                }
                else
                {
                    offset1 = ((t->mb.mb_y << 4) + (vec.y >> 2) + index_[y][x][3] + i / 2 * 8 + 4) * t->edged_stride + (t->mb.mb_x << 4) + (vec.x >> 2) + index_[y][x][2] + i % 2 * 8 + 4;
                    offset2 = ((t->mb.mb_y << 4) + (vec.y >> 2) + index_[y][x][5] + i / 2 * 8 + 4) * t->edged_stride + (t->mb.mb_x << 4) + (vec.x >> 2) + index_[y][x][4] + i % 2 * 8 + 4;
                    t->pia[MB_4x4](t->ref[list_idx][vec.refno]->Y[index_[y][x][0]] + offset1, 
                        t->ref[list_idx][vec.refno]->Y[index_[y][x][1]] + offset2,
                        t->edged_stride, t->edged_stride, ref + i / 2 * 16 * 8 + i % 2 * 8 + 64 + 4, 
                        16);
                }
                break;
            }
        }
        break;
    default:
        assert(0);
        break;
    }
}

void
T264dec_mb_decode_interp_transform(T264_t* t, uint8_t* ref)
{
    DECLARE_ALIGNED_MATRIX(dct, 16, 16, int16_t, 16);
 
    int16_t* curdct = dct;
    int32_t i;

    for(i = 0 ; i < 16 ; i ++)
    {
        unscan_zig_4x4(t->mb.dct_y_z[luma_index[i]], curdct);

        t->iquant4x4(curdct, t->qp_y);
        t->idct4x4(curdct);
        curdct += 16;
    }
    t->contract16to8add(dct, 16 / 4, 16 / 4, ref, t->mb.src_y, t->edged_stride);
}

void 
T264dec_mb_decode_interp_y(T264_t* t)
{
    T264dec_mb_decode_interp_mc(t, t->mb.pred_p16x16);
    T264dec_mb_decode_interp_transform(t, t->mb.pred_p16x16);
}

void 
T264dec_mb_decode_interp_uv(T264_t* t)
{
    DECLARE_ALIGNED_MATRIX(pred_u, 8, 8, uint8_t, CACHE_SIZE);
    DECLARE_ALIGNED_MATRIX(pred_v, 8, 8, uint8_t, CACHE_SIZE);

    T264_vector_t vec;
    uint8_t* src, *dst;
    uint8_t* src_u, *dst_u;
    int32_t i;
    int32_t list_idx = 0;

    switch (t->mb.mb_part)
    {
    case MB_16x16:
        vec = t->mb.vec[0][0];
        src = t->ref[list_idx][vec.refno]->U + ((t->mb.mb_y << 3) + (vec.y >> 3)) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3);
        dst = pred_u;
        t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 8, 8);
        src = t->ref[list_idx][vec.refno]->V + ((t->mb.mb_y << 3) + (vec.y >> 3)) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3);
        dst = pred_v;
        t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 8, 8);
        break;
    case MB_16x8:
        vec = t->mb.vec[0][0];
        src_u = t->ref[list_idx][vec.refno]->U + ((t->mb.mb_y << 3) + (vec.y >> 3)) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3);
        dst_u = pred_u;
        t->eighth_pixel_mc_u(src_u, t->edged_stride_uv, dst_u, vec.x, vec.y, 8, 4);
        src = t->ref[list_idx][vec.refno]->V + ((t->mb.mb_y << 3) + (vec.y >> 3)) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3);
        dst = pred_v;
        t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 8, 4);

        vec = t->mb.vec[0][luma_index[8]];
        src_u = t->ref[list_idx][vec.refno]->U + ((t->mb.mb_y << 3) + (vec.y >> 3)) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) +
            4 * t->edged_stride_uv;
        dst_u += 4 * 8;
        t->eighth_pixel_mc_u(src_u, t->edged_stride_uv, dst_u, vec.x, vec.y, 8, 4);
        src = t->ref[list_idx][vec.refno]->V + ((t->mb.mb_y << 3) + (vec.y >> 3)) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + 
            4 * t->edged_stride_uv;
        dst += 4 * 8;
        t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 8, 4);
        break;
    case MB_8x16:
        vec = t->mb.vec[0][0];
        src_u = t->ref[list_idx][vec.refno]->U + ((t->mb.mb_y << 3) + (vec.y >> 3)) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3);
        dst_u = pred_u;
        t->eighth_pixel_mc_u(src_u, t->edged_stride_uv, dst_u, vec.x, vec.y, 4, 8);
        src = t->ref[list_idx][vec.refno]->V + ((t->mb.mb_y << 3) + (vec.y >> 3)) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3);
        dst = pred_v;
        t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 4, 8);

        vec = t->mb.vec[0][luma_index[4]];
        src_u = t->ref[list_idx][vec.refno]->U + ((t->mb.mb_y << 3) + (vec.y >> 3)) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + 4;
        dst_u += 4;
        t->eighth_pixel_mc_u(src_u, t->edged_stride_uv, dst_u, vec.x, vec.y, 4, 8);
        src = t->ref[list_idx][vec.refno]->V + ((t->mb.mb_y << 3) + (vec.y >> 3)) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + 4;
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
                src = t->ref[list_idx][vec.refno]->U + ((t->mb.mb_y << 3) + (vec.y >> 3) + i / 2 * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + (i % 2 * 4);
                dst = pred_u + i / 2 * 32 + i % 2 * 4;
                t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 4, 4);
                src = t->ref[list_idx][vec.refno]->V + ((t->mb.mb_y << 3) + (vec.y >> 3) + i / 2 * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + (i % 2 * 4);
                dst = pred_v + i / 2 * 32 + i % 2 * 4;
                t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 4, 4);
                break;
            case MB_8x4:
                vec = t->mb.vec[0][luma_index[4 * i]];
                src_u = t->ref[list_idx][vec.refno]->U + ((t->mb.mb_y << 3) + (vec.y >> 3) + i / 2 * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + (i % 2 * 4);
                dst_u = pred_u + i / 2 * 32 + i % 2 * 4;
                t->eighth_pixel_mc_u(src_u, t->edged_stride_uv, dst_u, vec.x, vec.y, 4, 2);
                src = t->ref[list_idx][vec.refno]->V + ((t->mb.mb_y << 3) + (vec.y >> 3) + i / 2 * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + (i % 2 * 4);
                dst = pred_v + i / 2 * 32 + i % 2 * 4;
                t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 4, 2);

                vec = t->mb.vec[0][luma_index[4 * i + 2]];
                src_u = t->ref[list_idx][vec.refno]->U + ((t->mb.mb_y << 3) + (vec.y >> 3) + i / 2 * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + (i % 2 * 4) + 
                    2 * t->edged_stride_uv;
                dst_u += 2 * 8;
                t->eighth_pixel_mc_u(src_u, t->edged_stride_uv, dst_u, vec.x, vec.y, 4, 2);
                src = t->ref[list_idx][vec.refno]->V + ((t->mb.mb_y << 3) + (vec.y >> 3) + i / 2 * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + (i % 2 * 4) +
                    2 * t->edged_stride_uv;
                dst += 2 * 8;
                t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 4, 2);
                break;
            case MB_4x8:
                vec = t->mb.vec[0][luma_index[4 * i]];
                src_u = t->ref[list_idx][vec.refno]->U + ((t->mb.mb_y << 3) + (vec.y >> 3) + i / 2 * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + (i % 2 * 4);
                dst_u = pred_u + i / 2 * 32 + i % 2 * 4;
                t->eighth_pixel_mc_u(src_u, t->edged_stride_uv, dst_u, vec.x, vec.y, 2, 4);
                src = t->ref[list_idx][vec.refno]->V + ((t->mb.mb_y << 3) + (vec.y >> 3) + i / 2 * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + (i % 2 * 4);
                dst = pred_v + i / 2 * 32 + i % 2 * 4;
                t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 2, 4);

                vec = t->mb.vec[0][luma_index[4 * i + 1]];
                src_u = t->ref[list_idx][vec.refno]->U + ((t->mb.mb_y << 3) + (vec.y >> 3) + i / 2 * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + (i % 2 * 4) + 2;
                dst_u += 2;
                t->eighth_pixel_mc_u(src_u, t->edged_stride_uv, dst_u, vec.x, vec.y, 2, 4);
                src = t->ref[list_idx][vec.refno]->V + ((t->mb.mb_y << 3) + (vec.y >> 3) + i / 2 * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + (i % 2 * 4) + 2;
                dst += 2;
                t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 2, 4);
                break;
            case MB_4x4:
                vec = t->mb.vec[0][luma_index[4 * i]];
                src_u = t->ref[list_idx][vec.refno]->U + ((t->mb.mb_y << 3) + (vec.y >> 3) + i / 2 * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + (i % 2 * 4);
                dst_u = pred_u + i / 2 * 32 + i % 2 * 4;
                t->eighth_pixel_mc_u(src_u, t->edged_stride_uv, dst_u, vec.x, vec.y, 2, 2);
                src = t->ref[list_idx][vec.refno]->V + ((t->mb.mb_y << 3) + (vec.y >> 3) + i / 2 * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + (i % 2 * 4);
                dst = pred_v + i / 2 * 32 + i % 2 * 4;
                t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 2, 2);

                vec = t->mb.vec[0][luma_index[4 * i + 1]];
                src_u = t->ref[list_idx][vec.refno]->U + ((t->mb.mb_y << 3) + (vec.y >> 3) + i / 2 * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + (i % 2 * 4) + 2;
                dst_u += 2;
                t->eighth_pixel_mc_u(src_u, t->edged_stride_uv, dst_u, vec.x, vec.y, 2, 2);
                src = t->ref[list_idx][vec.refno]->V + ((t->mb.mb_y << 3) + (vec.y >> 3) + i / 2 * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + (i % 2 * 4) + 2;
                dst += 2;
                t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 2, 2);

                vec = t->mb.vec[0][luma_index[4 * i + 2]];
                src_u = t->ref[list_idx][vec.refno]->U + ((t->mb.mb_y << 3) + (vec.y >> 3) + i / 2 * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + (i % 2 * 4) + 
                    2 * t->edged_stride_uv;
                dst_u += 2 * 8 - 2;
                t->eighth_pixel_mc_u(src_u, t->edged_stride_uv, dst_u, vec.x, vec.y, 2, 2);
                src = t->ref[list_idx][vec.refno]->V + ((t->mb.mb_y << 3) + (vec.y >> 3) + i / 2 * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + (i % 2 * 4) + 
                    2 * t->edged_stride_uv;
                dst += 2 * 8 - 2;
                t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 2, 2);

                vec = t->mb.vec[0][luma_index[4 * i + 3]];
                src_u = t->ref[list_idx][vec.refno]->U + ((t->mb.mb_y << 3) + (vec.y >> 3) + i / 2 * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + (i % 2 * 4) +
                    2 * t->edged_stride_uv + 2;
                dst_u += 2;
                t->eighth_pixel_mc_u(src_u, t->edged_stride_uv, dst_u, vec.x, vec.y, 2, 2);
                src = t->ref[list_idx][vec.refno]->V + ((t->mb.mb_y << 3) + (vec.y >> 3) + i / 2 * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec.x >> 3) + (i % 2 * 4) + 
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

    T264dec_mb_decode_uv(t, pred_u, pred_v);
}

    static const int8_t index_[4][4][6] = 
    {
        {{0, 0, 0, 0, 0, 0}, {0, 1, 0, 0, 0, 0}, {1, 1, 0, 0, 0, 0}, {1, 0, 0, 0, 1, 0}},
        {{0, 2, 0, 0, 0, 0}, {1, 2, 0, 0, 0, 0}, {1, 3, 0, 0, 0, 0}, {1, 2, 0, 0, 1, 0}},
        {{2, 2, 0, 0, 0, 0}, {2, 3, 0, 0, 0, 0}, {3, 3, 0, 0, 0, 0}, {3, 2, 0, 0, 1, 0}},
        {{2, 0, 0, 0, 0, 1}, {2, 1, 0, 0, 0, 1}, {3, 1, 0, 0, 0, 1}, {1, 2, 0, 1, 1, 0}}
    };

void 
T264_mb4x4_interb_uv_mc(T264_t* t,T264_vector_t vecPredicted[2][16],uint8_t* pred_u,uint8_t* pred_v)
{
    DECLARE_ALIGNED_MATRIX(pred_u_l1, 8, 8, uint8_t, CACHE_SIZE);
    DECLARE_ALIGNED_MATRIX(pred_v_l1, 8, 8, uint8_t, CACHE_SIZE);

    T264_vector_t vec;
    uint8_t* src, *dst;
    int32_t i;

    int32_t j;
    int32_t idx;
    int32_t offset_src,offset_dst;
    uint8_t *dstv;

    for(i = 0;i < 4; ++i)
    {
        for(j = 0;j < 4; ++j)
        {    //predict each 2x2 block
            idx = (i * 4) + j;
            offset_dst = ((i * 2) * 8) + (j << 1);
            vec = vecPredicted[0][idx];
            offset_src = ((t->mb.mb_y << 3) + ((i << 1) + (vec.y >> 3))) * t->edged_stride_uv + (t->mb.mb_x << 3) + (j << 1) + (vec.x >> 3);
            dstv = pred_v + offset_dst;
            dst = pred_u + offset_dst;
            if(vec.refno > -1)
            {
                src = t->ref[0][vec.refno]->U + offset_src;
                t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 2, 2);

                src = t->ref[0][vec.refno]->V + offset_src;
                t->eighth_pixel_mc_u(src, t->edged_stride_uv, dstv, vec.x, vec.y, 2, 2);
            }

            vec = vecPredicted[1][idx];
            offset_src = ((t->mb.mb_y << 3) + ((i << 1) + (vec.y >> 3))) * t->edged_stride_uv + (t->mb.mb_x << 3) + (j << 1) + (vec.x >> 3);
            if(vec.refno > -1)
            {
                if(vecPredicted[0][idx].refno > -1)
                {
                    dst = pred_u_l1 + offset_dst;
                    dstv = pred_v_l1 + offset_dst;
                }

                src = t->ref[1][vec.refno]->U + offset_src;
                t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec.x, vec.y, 2, 2);

                src = t->ref[1][vec.refno]->V + offset_src;
                t->eighth_pixel_mc_u(src, t->edged_stride_uv, dstv, vec.x, vec.y, 2, 2);
            }
            if(dst != pred_u + offset_dst)
            {
                t->pia[MB_2x2](dst, pred_u + offset_dst, 8, 8, pred_u + offset_dst, 8);
                t->pia[MB_2x2](dstv, pred_v + offset_dst, 8, 8, pred_v + offset_dst, 8);
            }
        }
    }
}

void 
T264_mb4x4_interb_mc(T264_t* t,T264_vector_t vec[2][16],uint8_t* ref)
{
    T264_vector_t vec0,vec1;
    uint8_t* tmp,*pred_tmp;
    int32_t x, y,i,j;
    int32_t list_idx,
            block_idx = 0;
    int32_t offset1, offset2;

    DECLARE_ALIGNED_MATRIX_H(pred_16x16bi, 16, 16, uint8_t, CACHE_SIZE);
 
    for(i = 0 ; i < 4 ; i ++)
    {
        for(j = 0;j < 4; ++j)
        {
            int32_t offset_base;

            vec0 = vec[0][block_idx];
            vec1 = vec[1][block_idx];
            x = (vec0.x & 3);
            y = (vec0.y & 3);
        //    offset_base = luma_inverse_y[block_idx] * 16 * 4 + luma_inverse_x[block_idx] * 4;
            offset_base = i * 16 * 4 + j * 4;
            pred_tmp = ref + offset_base;

            if(vec0.refno > -1)
            {
                    list_idx = 0;
                    if (index_[y][x][0] == index_[y][x][1])
                    {
                        offset1 = ((t->mb.mb_y << 4) + (vec0.y >> 2) + i * 4) * t->edged_stride + ((t->mb.mb_x << 4) + (vec0.x >> 2)) + j  * 4;
                        tmp = t->ref[list_idx][vec0.refno]->Y[index_[y][x][0]] + offset1;
                        t->memcpy_stride_u(tmp, 4, 4, t->edged_stride, pred_tmp, 16);
                    }
                    else
                    {
                        offset1 = ((t->mb.mb_y << 4) + (vec0.y >> 2) + index_[y][x][3] + i * 4) * t->edged_stride + (t->mb.mb_x << 4) + (vec0.x >> 2) + index_[y][x][2] + j * 4;
                        offset2 = ((t->mb.mb_y << 4) + (vec0.y >> 2) + index_[y][x][5] + i * 4) * t->edged_stride + (t->mb.mb_x << 4) + (vec0.x >> 2) + index_[y][x][4] + j * 4;
                        t->pia[MB_4x4](t->ref[list_idx][vec0.refno]->Y[index_[y][x][0]] + offset1, 
                            t->ref[list_idx][vec0.refno]->Y[index_[y][x][1]] + offset2,
                            t->edged_stride, t->edged_stride, pred_tmp,16);
                    }
                }
                x = (vec1.x & 3);
                y = (vec1.y & 3);
                if(vec1.refno > -1)
                {
                    list_idx = 1;
                    if(vec0.refno > -1)
                        pred_tmp = pred_16x16bi + offset_base;
                    if (index_[y][x][0] == index_[y][x][1])
                    {
                        offset1 = ((t->mb.mb_y << 4) + (vec1.y >> 2) + i * 4) * t->edged_stride + ((t->mb.mb_x << 4) + (vec1.x >> 2)) + j * 4;
                        tmp = t->ref[list_idx][vec1.refno]->Y[index_[y][x][0]] + offset1;
                        t->memcpy_stride_u(tmp, 4, 4, t->edged_stride, pred_tmp, 16);
                    }
                    else
                    {
                        offset1 = ((t->mb.mb_y << 4) + (vec1.y >> 2) + index_[y][x][3] + i * 4) * t->edged_stride + (t->mb.mb_x << 4) + (vec1.x >> 2) + index_[y][x][2] + j * 4;
                        offset2 = ((t->mb.mb_y << 4) + (vec1.y >> 2) + index_[y][x][5] + i * 4) * t->edged_stride + (t->mb.mb_x << 4) + (vec1.x >> 2) + index_[y][x][4] + j * 4;
                        t->pia[MB_4x4](t->ref[list_idx][vec1.refno]->Y[index_[y][x][0]] + offset1, 
                            t->ref[list_idx][vec1.refno]->Y[index_[y][x][1]] + offset2,
                            t->edged_stride, t->edged_stride, pred_tmp, 16);
                    }
                }
                if(pred_tmp != ref + offset_base)
                    t->pia[MB_4x4](pred_tmp,ref + offset_base,16,16,ref + offset_base,16);        
                ++block_idx;
        }
    }
}

void
T264dec_mb_decode_interb_mc(T264_t* t, uint8_t* ref)
{
    T264_vector_t vec0,vec1;
    uint8_t* tmp,*pred_tmp;
    int32_t x, y,i;
    int32_t list_idx;

    DECLARE_ALIGNED_MATRIX_H(pred_16x16bi, 16, 16, uint8_t, CACHE_SIZE);
 
    if(t->mb.is_copy)
        T264_mb4x4_interb_mc(t,t->mb.vec,ref);
    else
    switch(t->mb.mb_part)
    {
    case MB_16x16:
        vec0 = t->mb.vec[0][0];
        vec1 = t->mb.vec[1][0];
        x = (vec0.x & 3);
        y = (vec0.y & 3);
        pred_tmp = ref;    
        if(vec0.refno > -1)
        {
            list_idx = 0;
            if (index_[y][x][0] == index_[y][x][1])
            {   
                tmp = t->ref[list_idx][vec0.refno]->Y[index_[y][x][0]] + ((t->mb.mb_y << 4) + (vec0.y >> 2)) * t->edged_stride + 
                    ((t->mb.mb_x << 4) + (vec0.x >> 2));
                t->memcpy_stride_u(tmp, 16, 16, t->edged_stride, ref, 16);
            }
            else
            {  
                t->pia[MB_16x16](t->ref[list_idx][vec0.refno]->Y[index_[y][x][0]] + ((t->mb.mb_y << 4) + (vec0.y >> 2) + index_[y][x][3]) * t->edged_stride + (t->mb.mb_x << 4) + (vec0.x >> 2) + index_[y][x][2], 
                    t->ref[list_idx][vec0.refno]->Y[index_[y][x][1]] + ((t->mb.mb_y << 4) + (vec0.y >> 2) + index_[y][x][5]) * t->edged_stride + (t->mb.mb_x << 4) + (vec0.x >> 2) + index_[y][x][4],
                    t->edged_stride, t->edged_stride, ref, 16);
            }                              
        }
        if(vec1.refno > -1)
        {   //if bi-pred
                x = (vec1.x & 3);
                y = (vec1.y & 3);
                list_idx = 1;
                if(vec0.refno > -1) //if biPred
                    pred_tmp = pred_16x16bi;
                else
                    pred_tmp = ref;

                if (index_[y][x][0] == index_[y][x][1])
                {   
                    tmp = t->ref[list_idx][vec1.refno]->Y[index_[y][x][0]] + ((t->mb.mb_y << 4) + (vec1.y >> 2)) * t->edged_stride + 
                        ((t->mb.mb_x << 4) + (vec1.x >> 2));
                    t->memcpy_stride_u(tmp, 16, 16, t->edged_stride, pred_tmp, 16);
                }
                else
                {   
                    t->pia[MB_16x16](t->ref[list_idx][vec1.refno]->Y[index_[y][x][0]] + ((t->mb.mb_y << 4) + (vec1.y >> 2) + index_[y][x][3]) * t->edged_stride + (t->mb.mb_x << 4) + (vec1.x >> 2) + index_[y][x][2], 
                        t->ref[list_idx][vec1.refno]->Y[index_[y][x][1]] + ((t->mb.mb_y << 4) + (vec1.y >> 2) + index_[y][x][5]) * t->edged_stride + (t->mb.mb_x << 4) + (vec1.x >> 2) + index_[y][x][4],
                        t->edged_stride, t->edged_stride, pred_tmp, 16);
                }    
        } 
        if(pred_tmp != ref)
        {   //if biPred
            t->pia[MB_16x16](pred_tmp,ref,16,16,ref,16);            
        }
        break;
    case MB_16x8:
        vec0 = t->mb.vec[0][0];
        vec1 = t->mb.vec[1][0];
        pred_tmp = ref;   

        if(vec0.refno > -1)
        {
            list_idx = 0;
            x = (vec0.x & 3);
            y = (vec0.y & 3);
            if (index_[y][x][0] == index_[y][x][1])
            {
                tmp = t->ref[list_idx][vec0.refno]->Y[index_[y][x][0]] + ((t->mb.mb_y << 4) + (vec0.y >> 2)) * t->edged_stride + 
                    ((t->mb.mb_x << 4) + (vec0.x >> 2));
                t->memcpy_stride_u(tmp, 16, 8, t->edged_stride, ref, 16);
            }
            else
            {
                t->pia[MB_16x8](t->ref[list_idx][vec0.refno]->Y[index_[y][x][0]] + ((t->mb.mb_y << 4) + (vec0.y >> 2) + index_[y][x][3]) * t->edged_stride + (t->mb.mb_x << 4) + (vec0.x >> 2) + index_[y][x][2], 
                    t->ref[list_idx][vec0.refno]->Y[index_[y][x][1]] + ((t->mb.mb_y << 4) + (vec0.y >> 2) + index_[y][x][5]) * t->edged_stride + (t->mb.mb_x << 4) + (vec0.x >> 2) + index_[y][x][4],
                    t->edged_stride, t->edged_stride, ref, 16);
            }
        }
        if(vec1.refno > -1)
        {
            x = (vec1.x & 3);
            y = (vec1.y & 3);
            list_idx = 1;
            if(vec0.refno > -1) //if biPred
                pred_tmp = pred_16x16bi;
            else
                pred_tmp = ref;
            if (index_[y][x][0] == index_[y][x][1])
            {
                tmp = t->ref[list_idx][vec1.refno]->Y[index_[y][x][0]] + ((t->mb.mb_y << 4) + (vec1.y >> 2)) * t->edged_stride + 
                    ((t->mb.mb_x << 4) + (vec1.x >> 2));
                t->memcpy_stride_u(tmp, 16, 8, t->edged_stride, pred_tmp, 16);
            }
            else
            {
                t->pia[MB_16x8](t->ref[list_idx][vec1.refno]->Y[index_[y][x][0]] + ((t->mb.mb_y << 4) + (vec1.y >> 2) + index_[y][x][3]) * t->edged_stride + (t->mb.mb_x << 4) + (vec1.x >> 2) + index_[y][x][2], 
                    t->ref[list_idx][vec1.refno]->Y[index_[y][x][1]] + ((t->mb.mb_y << 4) + (vec1.y >> 2) + index_[y][x][5]) * t->edged_stride + (t->mb.mb_x << 4) + (vec1.x >> 2) + index_[y][x][4],
                    t->edged_stride, t->edged_stride, pred_tmp, 16);
            }
        }
        if(pred_tmp != ref)
        {   //if biPred
            t->pia[MB_16x8](pred_tmp,ref,16,16,ref,16);            
        }

        //For second MB16x8
        vec0 = t->mb.vec[0][8];
        vec1 = t->mb.vec[1][8];
        pred_tmp = ref + 16 * 8;    

        if(vec0.refno > -1)
        {
            x = (vec0.x & 3);
            y = (vec0.y & 3);
            list_idx = 0;
            if (index_[y][x][0] == index_[y][x][1])
            {
                tmp = t->ref[list_idx][vec0.refno]->Y[index_[y][x][0]] + ((t->mb.mb_y << 4) + (vec0.y >> 2) + 8) * t->edged_stride + 
                    ((t->mb.mb_x << 4) + (vec0.x >> 2));
                t->memcpy_stride_u(tmp, 16, 8, t->edged_stride, pred_tmp, 16);
            }
            else
            {
                t->pia[MB_16x8](t->ref[list_idx][vec0.refno]->Y[index_[y][x][0]] + ((t->mb.mb_y << 4) + (vec0.y >> 2) + index_[y][x][3] + 8) * t->edged_stride + (t->mb.mb_x << 4) + (vec0.x >> 2) + index_[y][x][2], 
                    t->ref[list_idx][vec0.refno]->Y[index_[y][x][1]] + ((t->mb.mb_y << 4) + (vec0.y >> 2) + index_[y][x][5] + 8) * t->edged_stride + (t->mb.mb_x << 4) + (vec0.x >> 2) + index_[y][x][4],
                    t->edged_stride, t->edged_stride, pred_tmp, 16);
            }
        }
        if(vec1.refno > -1)
        {
            x = (vec1.x & 3);
            y = (vec1.y & 3);
            list_idx = 1;
            if(vec0.refno > -1) //if biPred
                pred_tmp = pred_16x16bi + 16 * 8;
            if (index_[y][x][0] == index_[y][x][1])
            {
                tmp = t->ref[list_idx][vec1.refno]->Y[index_[y][x][0]] + ((t->mb.mb_y << 4) + (vec1.y >> 2) + 8) * t->edged_stride + 
                    ((t->mb.mb_x << 4) + (vec1.x >> 2));
                t->memcpy_stride_u(tmp, 16, 8, t->edged_stride,pred_tmp, 16);
            }
            else
            {
                t->pia[MB_16x8](t->ref[list_idx][vec1.refno]->Y[index_[y][x][0]] + ((t->mb.mb_y << 4) + (vec1.y >> 2) + index_[y][x][3] + 8) * t->edged_stride + (t->mb.mb_x << 4) + (vec1.x >> 2) + index_[y][x][2], 
                    t->ref[list_idx][vec1.refno]->Y[index_[y][x][1]] + ((t->mb.mb_y << 4) + (vec1.y >> 2) + index_[y][x][5] + 8) * t->edged_stride + (t->mb.mb_x << 4) + (vec1.x >> 2) + index_[y][x][4],
                    t->edged_stride, t->edged_stride, pred_tmp, 16);
            }
        }
        if(pred_tmp != ref + 16 * 8)
        {   //if biPred
            t->pia[MB_16x8](pred_tmp,ref + 16 * 8,16,16,ref + 16 * 8,16);            
        }

        break;
    case MB_8x16:
        pred_tmp = ref;
        vec0 = t->mb.vec[0][0];
        vec1 = t->mb.vec[1][0];
        if(vec0.refno > -1)
        {
            x = (vec0.x & 3);
            y = (vec0.y & 3);
            list_idx = 0;
            if (index_[y][x][0] == index_[y][x][1])
            {
                tmp = t->ref[list_idx][vec0.refno]->Y[index_[y][x][0]] + ((t->mb.mb_y << 4) + (vec0.y >> 2)) * t->edged_stride + 
                    ((t->mb.mb_x << 4) + (vec0.x >> 2));
                t->memcpy_stride_u(tmp, 8, 16, t->edged_stride, ref, 16);
            }
            else
            {
                t->pia[MB_8x16](t->ref[list_idx][vec0.refno]->Y[index_[y][x][0]] + ((t->mb.mb_y << 4) + (vec0.y >> 2) + index_[y][x][3]) * t->edged_stride + (t->mb.mb_x << 4) + (vec0.x >> 2) + index_[y][x][2], 
                    t->ref[list_idx][vec0.refno]->Y[index_[y][x][1]] + ((t->mb.mb_y << 4) + (vec0.y >> 2) + index_[y][x][5]) * t->edged_stride + (t->mb.mb_x << 4) + (vec0.x >> 2) + index_[y][x][4],
                    t->edged_stride, t->edged_stride, ref, 16);
            }
        }
        if(vec1.refno > -1)
        {
            list_idx = 1;
            x = (vec1.x & 3);
            y = (vec1.y & 3);
            if(vec0.refno > -1) //if biPred
                pred_tmp = pred_16x16bi;
            if (index_[y][x][0] == index_[y][x][1])
            {
                tmp = t->ref[list_idx][vec1.refno]->Y[index_[y][x][0]] + ((t->mb.mb_y << 4) + (vec1.y >> 2)) * t->edged_stride + 
                    ((t->mb.mb_x << 4) + (vec1.x >> 2));
                t->memcpy_stride_u(tmp, 8, 16, t->edged_stride, pred_tmp, 16);
            }
            else
            {
                t->pia[MB_8x16](t->ref[list_idx][vec1.refno]->Y[index_[y][x][0]] + ((t->mb.mb_y << 4) + (vec1.y >> 2) + index_[y][x][3]) * t->edged_stride + (t->mb.mb_x << 4) + (vec1.x >> 2) + index_[y][x][2], 
                    t->ref[list_idx][vec1.refno]->Y[index_[y][x][1]] + ((t->mb.mb_y << 4) + (vec1.y >> 2) + index_[y][x][5]) * t->edged_stride + (t->mb.mb_x << 4) + (vec1.x >> 2) + index_[y][x][4],
                    t->edged_stride, t->edged_stride,pred_tmp, 16);
            }
        }
        if(pred_tmp != ref)
        {   //if biPred
            t->pia[MB_8x16](pred_tmp,ref,16,16,ref,16);            
        }

        //for second MB8x16
        vec0 = t->mb.vec[0][2];
        vec1 = t->mb.vec[1][2];
        pred_tmp = ref + 8;
        if(vec0.refno > -1)
        {
            x = (vec0.x & 3);
            y = (vec0.y & 3);
            list_idx = 0;
            if (index_[y][x][0] == index_[y][x][1])
            {
                tmp = t->ref[list_idx][vec0.refno]->Y[index_[y][x][0]] + ((t->mb.mb_y << 4) + (vec0.y >> 2)) * t->edged_stride + 
                    ((t->mb.mb_x << 4) + (vec0.x >> 2)) + 8;
                t->memcpy_stride_u(tmp, 8, 16, t->edged_stride, pred_tmp, 16);
            }
            else
            {
                t->pia[MB_8x16](t->ref[list_idx][vec0.refno]->Y[index_[y][x][0]] + ((t->mb.mb_y << 4) + (vec0.y >> 2) + index_[y][x][3]) * t->edged_stride + (t->mb.mb_x << 4) + (vec0.x >> 2) + index_[y][x][2] + 8, 
                    t->ref[list_idx][vec0.refno]->Y[index_[y][x][1]] + ((t->mb.mb_y << 4) + (vec0.y >> 2) + index_[y][x][5]) * t->edged_stride + (t->mb.mb_x << 4) + (vec0.x >> 2) + index_[y][x][4] + 8,
                    t->edged_stride, t->edged_stride, pred_tmp, 16);
            }
        }
        if(vec1.refno > -1)
        {
            x = (vec1.x & 3);
            y = (vec1.y & 3);
            list_idx = 1;
            if(vec0.refno > -1) //if biPred
                pred_tmp = pred_16x16bi + 8;
            if (index_[y][x][0] == index_[y][x][1])
            {
                tmp = t->ref[list_idx][vec1.refno]->Y[index_[y][x][0]] + ((t->mb.mb_y << 4) + (vec1.y >> 2)) * t->edged_stride + 
                    ((t->mb.mb_x << 4) + (vec1.x >> 2)) + 8;
                t->memcpy_stride_u(tmp, 8, 16, t->edged_stride, pred_tmp, 16);
            }
            else
            {
                t->pia[MB_8x16](t->ref[list_idx][vec1.refno]->Y[index_[y][x][0]] + ((t->mb.mb_y << 4) + (vec1.y >> 2) + index_[y][x][3]) * t->edged_stride + (t->mb.mb_x << 4) + (vec1.x >> 2) + index_[y][x][2] + 8, 
                    t->ref[list_idx][vec1.refno]->Y[index_[y][x][1]] + ((t->mb.mb_y << 4) + (vec1.y >> 2) + index_[y][x][5]) * t->edged_stride + (t->mb.mb_x << 4) + (vec1.x >> 2) + index_[y][x][4] + 8,
                    t->edged_stride, t->edged_stride,pred_tmp, 16);
            }
        }
        if(pred_tmp != ref + 8)
        {   //if biPred
            t->pia[MB_8x16](pred_tmp,ref + 8,16,16,ref + 8,16);            
        }
        break;

    case MB_8x8:
        for(i = 0 ; i < 4 ; i ++)
        {
            int32_t offset1, offset2;
            switch(t->mb.submb_part[luma_index[4 * i]]) 
            {
            case MB_8x8:
                vec0 = t->mb.vec[0][luma_index[4 * i]];
                vec1 = t->mb.vec[1][luma_index[4 * i]];
                x = (vec0.x & 3);
                y = (vec0.y & 3);
                pred_tmp = ref + i / 2 * 16 * 8 + i % 2 * 8;

                if(vec0.refno > -1)
                {
                    list_idx = 0;
                    if (index_[y][x][0] == index_[y][x][1])
                    {
                        offset1 = ((t->mb.mb_y << 4) + (vec0.y >> 2) + i / 2 * 8) * t->edged_stride + ((t->mb.mb_x << 4) + (vec0.x >> 2)) + i % 2 * 8;
                        tmp = t->ref[list_idx][vec0.refno]->Y[index_[y][x][0]] + offset1;
                        t->memcpy_stride_u(tmp, 8, 8, t->edged_stride, pred_tmp, 16);
                    }
                    else
                    {
                        offset1 = ((t->mb.mb_y << 4) + (vec0.y >> 2) + index_[y][x][3] + i / 2 * 8) * t->edged_stride + (t->mb.mb_x << 4) + (vec0.x >> 2) + index_[y][x][2] + i % 2 * 8;
                        offset2 = ((t->mb.mb_y << 4) + (vec0.y >> 2) + index_[y][x][5] + i / 2 * 8) * t->edged_stride + (t->mb.mb_x << 4) + (vec0.x >> 2) + index_[y][x][4] + i % 2 * 8;
                        t->pia[MB_8x8](t->ref[list_idx][vec0.refno]->Y[index_[y][x][0]] + offset1, 
                            t->ref[list_idx][vec0.refno]->Y[index_[y][x][1]] + offset2,
                            t->edged_stride, t->edged_stride, pred_tmp,16);
                    }
                }
                x = (vec1.x & 3);
                y = (vec1.y & 3);
                if(vec1.refno > -1)
                {
                    list_idx = 1;
                    if(vec0.refno > -1)
                        pred_tmp = pred_16x16bi + i / 2 * 16 * 8 + i % 2 * 8;
                    if (index_[y][x][0] == index_[y][x][1])
                    {
                        offset1 = ((t->mb.mb_y << 4) + (vec1.y >> 2) + i / 2 * 8) * t->edged_stride + ((t->mb.mb_x << 4) + (vec1.x >> 2)) + i % 2 * 8;
                        tmp = t->ref[list_idx][vec1.refno]->Y[index_[y][x][0]] + offset1;
                        t->memcpy_stride_u(tmp, 8, 8, t->edged_stride, pred_tmp, 16);
                    }
                    else
                    {
                        offset1 = ((t->mb.mb_y << 4) + (vec1.y >> 2) + index_[y][x][3] + i / 2 * 8) * t->edged_stride + (t->mb.mb_x << 4) + (vec1.x >> 2) + index_[y][x][2] + i % 2 * 8;
                        offset2 = ((t->mb.mb_y << 4) + (vec1.y >> 2) + index_[y][x][5] + i / 2 * 8) * t->edged_stride + (t->mb.mb_x << 4) + (vec1.x >> 2) + index_[y][x][4] + i % 2 * 8;
                        t->pia[MB_8x8](t->ref[list_idx][vec1.refno]->Y[index_[y][x][0]] + offset1, 
                            t->ref[list_idx][vec1.refno]->Y[index_[y][x][1]] + offset2,
                            t->edged_stride, t->edged_stride, pred_tmp, 16);
                    }
                }
                if(pred_tmp != ref + i / 2 * 16 * 8 + i % 2 * 8)
                    t->pia[MB_8x8](pred_tmp,ref + i / 2 * 16 * 8 + i % 2 * 8,16,16,ref + i / 2 * 16 * 8 + i % 2 * 8,16);
                break;
            default:
                assert(0);
                break;
            }
        }
        break;
    default:    //only support MB16x16 B-frame
        assert(0);
        break;
    }
}

void 
T264dec_mb_decode_interb_y(T264_t* t)
{
    T264dec_mb_decode_interb_mc(t, t->mb.pred_p16x16);
    T264dec_mb_decode_interp_transform(t, t->mb.pred_p16x16);
}

void 
T264dec_mb_decode_interb_uv(T264_t* t)
{
    DECLARE_ALIGNED_MATRIX(pred_u, 8, 8, uint8_t, CACHE_SIZE);
    DECLARE_ALIGNED_MATRIX(pred_v, 8, 8, uint8_t, CACHE_SIZE);
    DECLARE_ALIGNED_MATRIX(pred_bi, 8, 8, uint8_t, CACHE_SIZE);

    T264_vector_t vec0,vec1;
    uint8_t* src, *dst;
    int32_t list_idx,i;

    if(t->mb.is_copy)
    {
        T264_mb4x4_interb_uv_mc(t,t->mb.vec,pred_u,pred_v);
    }else
    switch (t->mb.mb_part)
    {
    case MB_16x16:
        vec0 = t->mb.vec[0][0];
        vec1 = t->mb.vec[1][0];
        dst  = pred_u;
        if(vec0.refno > -1)
        {
            list_idx = 0;
            src = t->ref[list_idx][vec0.refno]->U + ((t->mb.mb_y << 3) + (vec0.y >> 3)) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec0.x >> 3);
            t->eighth_pixel_mc_u(src, t->edged_stride_uv, pred_u, vec0.x, vec0.y, 8, 8);            
        }
        if(vec1.refno > -1)
        {
            list_idx = 1;
            if(vec0.refno > -1)
                dst = pred_bi;            
            else
                dst = pred_u;
            src = t->ref[list_idx][vec1.refno]->U + ((t->mb.mb_y << 3) + (vec1.y >> 3)) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec1.x >> 3);
            t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec1.x, vec1.y, 8, 8);            
        }
        if(dst != pred_u)
        {
            t->pia[MB_8x8](dst,pred_u,8,8,pred_u,8);            
        }

        dst = pred_v;
        if(vec0.refno > -1)
        {
            list_idx = 0;
            src = t->ref[list_idx][vec0.refno]->V + ((t->mb.mb_y << 3) + (vec0.y >> 3)) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec0.x >> 3);
            t->eighth_pixel_mc_u(src, t->edged_stride_uv, pred_v, vec0.x, vec0.y, 8, 8);            
        }
        if(vec1.refno > -1)
        {
            list_idx = 1;
            if(vec0.refno > -1)
                dst = pred_bi;            
            else
                dst = pred_v;
            src = t->ref[list_idx][vec1.refno]->V + ((t->mb.mb_y << 3) + (vec1.y >> 3)) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec1.x >> 3);
            t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec1.x, vec1.y, 8, 8);            
        }
        if(dst != pred_v)
        {
            t->pia[MB_8x8](dst,pred_v,8,8,pred_v,8);            
        }
        break;
    case MB_16x8:
        vec0 = t->mb.vec[0][0];
        vec1 = t->mb.vec[1][0];
        
        dst  = pred_u;
        if(vec0.refno > -1)
        {
            list_idx = 0;
            src = t->ref[list_idx][vec0.refno]->U + ((t->mb.mb_y << 3) + (vec0.y >> 3)) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec0.x >> 3);
            t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec0.x, vec0.y, 8, 4);
        }
        if(vec1.refno > -1)
        {
            if(vec0.refno > -1)
                dst = pred_bi;
            else
                dst = pred_u;
            list_idx = 1;
            src = t->ref[list_idx][vec1.refno]->U + ((t->mb.mb_y << 3) + (vec1.y >> 3)) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec1.x >> 3);
            t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec1.x, vec1.y, 8, 4);
        }
        if(dst != pred_u)
        {
            t->pia[MB_8x4](dst,pred_u,8,8,pred_u,8);            
        }

        dst  = pred_v;
        if(vec0.refno > -1)
        {
            list_idx = 0;
            src = t->ref[list_idx][vec0.refno]->V + ((t->mb.mb_y << 3) + (vec0.y >> 3)) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec0.x >> 3);
            t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec0.x, vec0.y, 8, 4);
        }
        if(vec1.refno > -1)
        {
            if(vec0.refno > -1)
                dst = pred_bi;
            else
                dst = pred_v;
            list_idx = 1;
            src = t->ref[list_idx][vec1.refno]->V + ((t->mb.mb_y << 3) + (vec1.y >> 3)) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec1.x >> 3);
            t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec1.x, vec1.y, 8, 4);
        }
        if(dst != pred_v)
        {
            t->pia[MB_8x4](dst,pred_v,8,8,pred_v,8);            
        }

        //now for next MB16x8
        vec0 = t->mb.vec[0][luma_index[8]];
        vec1 = t->mb.vec[1][luma_index[8]];        
        dst  = pred_u + 4 * 8;
        if(vec0.refno > -1)
        {
            list_idx = 0;
            src = t->ref[list_idx][vec0.refno]->U + ((t->mb.mb_y << 3) + (vec0.y >> 3)) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec0.x >> 3) +
            4 * t->edged_stride_uv;
            t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec0.x, vec0.y, 8, 4);
        }
        if(vec1.refno > -1)
        {
            if(vec0.refno > -1)
                dst = pred_bi + 4 * 8;
            else
                dst = pred_u + 4 * 8;
            list_idx = 1;
            src = t->ref[list_idx][vec1.refno]->U + ((t->mb.mb_y << 3) + (vec1.y >> 3)) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec1.x >> 3) +
            4 * t->edged_stride_uv;
            t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec1.x, vec1.y, 8, 4);
        }
        if(dst != pred_u + 4 * 8)
        {
            t->pia[MB_8x4](dst,pred_u + 4 * 8,8,8,pred_u + 4 * 8,8);
        }

        //for v
        dst  = pred_v + 4 * 8;
        if(vec0.refno > -1)
        {
            list_idx = 0;
            src = t->ref[list_idx][vec0.refno]->V + ((t->mb.mb_y << 3) + (vec0.y >> 3)) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec0.x >> 3) + 
                4 * t->edged_stride_uv;        
            t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec0.x, vec0.y, 8, 4);
        }
        if(vec1.refno > -1)
        {
            if(vec0.refno > -1)
                dst = pred_bi + 4 * 8;
            else
                dst = pred_v + 4 * 8;
            list_idx = 1;
            src = t->ref[list_idx][vec1.refno]->V + ((t->mb.mb_y << 3) + (vec1.y >> 3)) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec1.x >> 3) + 
                4 * t->edged_stride_uv;        
            t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec1.x, vec1.y, 8, 4);
        }
        if(dst != pred_v + 4 * 8)
        {
            t->pia[MB_8x4](dst,pred_v + 4 * 8,8,8,pred_v + 4 * 8,8);            
        }
        break;

    case MB_8x16:
        vec0 = t->mb.vec[0][0];
        vec1 = t->mb.vec[1][0];
        
        dst  = pred_u;
        if(vec0.refno > -1)
        {
            list_idx = 0;
            src = t->ref[list_idx][vec0.refno]->U + ((t->mb.mb_y << 3) + (vec0.y >> 3)) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec0.x >> 3);
            t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec0.x, vec0.y, 4, 8);
        }
        if(vec1.refno > -1)
        {
            if(vec0.refno > -1)
                dst = pred_bi;
            else
                dst = pred_u;
            list_idx = 1;
            src = t->ref[list_idx][vec1.refno]->U + ((t->mb.mb_y << 3) + (vec1.y >> 3)) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec1.x >> 3);
            t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec1.x, vec1.y, 4, 8);
        }
        if(dst != pred_u)
        {
            t->pia[MB_4x8](dst,pred_u,8,8,pred_u,8);            
        }

        dst  = pred_v;
        if(vec0.refno > -1)
        {
            list_idx = 0;
            src = t->ref[list_idx][vec0.refno]->V + ((t->mb.mb_y << 3) + (vec0.y >> 3)) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec0.x >> 3);
            //dst = pred_v;
            t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec0.x, vec0.y, 4, 8);
        }
        if(vec1.refno > -1)
        {
            if(vec0.refno > -1)
                dst = pred_bi;
            else
                dst = pred_v;
            list_idx = 1;
            src = t->ref[list_idx][vec1.refno]->V + ((t->mb.mb_y << 3) + (vec1.y >> 3)) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec1.x >> 3);
            t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec1.x, vec1.y, 4, 8);
        }
        if(dst != pred_v)
        {
            t->pia[MB_4x8](dst,pred_v,8,8,pred_v,8);            
        }

        //now for next MB8x16
        vec0 = t->mb.vec[0][luma_index[4]];
        vec1 = t->mb.vec[1][luma_index[4]];        
        dst  = pred_u + 4;
        if(vec0.refno > -1)
        {
            list_idx = 0;
            src = t->ref[list_idx][vec0.refno]->U + ((t->mb.mb_y << 3) + (vec0.y >> 3)) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec0.x >> 3) + 4;
            t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec0.x, vec0.y, 4, 8);
        }
        if(vec1.refno > -1)
        {
            if(vec0.refno > -1)
                dst = pred_bi + 4;
            else
                dst = pred_u + 4;
            list_idx = 1;
            src = t->ref[list_idx][vec1.refno]->U + ((t->mb.mb_y << 3) + (vec1.y >> 3)) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec1.x >> 3) + 4;
            t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec1.x, vec1.y, 4, 8);
        }
        if(dst != pred_u + 4)
        {
            t->pia[MB_4x8](dst,pred_u + 4,8,8,pred_u + 4,8);            
        }

        //for v
        dst  = pred_v + 4;
        if(vec0.refno > -1)
        {
            list_idx = 0;
            src = t->ref[list_idx][vec0.refno]->V + ((t->mb.mb_y << 3) + (vec0.y >> 3)) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec0.x >> 3) + 4;        
            t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec0.x, vec0.y, 4, 8);
        }
        if(vec1.refno > -1)
        {
            if(vec0.refno > -1)
                dst = pred_bi + 4;
            else
                dst = pred_v + 4;
            list_idx = 1;
            src = t->ref[list_idx][vec1.refno]->V + ((t->mb.mb_y << 3) + (vec1.y >> 3)) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec1.x >> 3) + 4;        
            t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec1.x, vec1.y, 4, 8);
        }
        if(dst != pred_v + 4)
        {
            t->pia[MB_4x8](dst,pred_v + 4,8,8,pred_v + 4,8);            
        }
      
        break;

    case MB_8x8:
        for(i = 0 ; i < 4 ; i ++)
        {
            switch(t->mb.submb_part[luma_index[4 * i]])
            {
            case MB_8x8:
                vec0 = t->mb.vec[0][luma_index[4 * i]];
                vec1 = t->mb.vec[1][luma_index[4 * i]];
                dst = pred_u + i / 2 * 32 + i % 2 * 4;
                if(vec0.refno > -1)
                {
                    list_idx = 0;
                    src = t->ref[list_idx][vec0.refno]->U + ((t->mb.mb_y << 3) + (vec0.y >> 3) + i / 2 * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec0.x >> 3) + (i % 2 * 4);
                    t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec0.x, vec0.y, 4, 4);
                }
                if(vec1.refno > -1)
                {
                    if(vec0.refno > -1)
                        dst = pred_bi + i / 2 * 32 + i % 2 * 4;
                    list_idx = 1;
                    src = t->ref[list_idx][vec1.refno]->U + ((t->mb.mb_y << 3) + (vec1.y >> 3) + i / 2 * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec1.x >> 3) + (i % 2 * 4);
                    t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec1.x, vec1.y, 4, 4);
                }
                if(dst != pred_u + i / 2 * 32 + i % 2 * 4)
                    t->pia[MB_4x4](dst,pred_u + i / 2 * 32 + i % 2 * 4,8,8,pred_u + i / 2 * 32 + i % 2 * 4,8);  

                dst = pred_v + i / 2 * 32 + i % 2 * 4;
                if(vec0.refno > -1)
                {
                    list_idx = 0;
                    src = t->ref[list_idx][vec0.refno]->V + ((t->mb.mb_y << 3) + (vec0.y >> 3) + i / 2 * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec0.x >> 3) + (i % 2 * 4);
                    t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec0.x, vec0.y, 4, 4);
                }
                if(vec1.refno > -1)
                {
                    if(vec0.refno > -1)
                        dst = pred_bi + i / 2 * 32 + i % 2 * 4;
                    list_idx = 1;
                    src = t->ref[list_idx][vec1.refno]->V + ((t->mb.mb_y << 3) + (vec1.y >> 3) + i / 2 * 4) * t->edged_stride_uv + (t->mb.mb_x << 3) + (vec1.x >> 3) + (i % 2 * 4);
                    t->eighth_pixel_mc_u(src, t->edged_stride_uv, dst, vec1.x, vec1.y, 4, 4);
                }
                if(dst != pred_v + i / 2 * 32 + i % 2 * 4)
                    t->pia[MB_4x4](dst,pred_v + i / 2 * 32 + i % 2 * 4,8,8,pred_v + i / 2 * 32 + i % 2 * 4,8);  

                break;
            default:
                assert(0);
                break;
            }
        }
    default:
        break;
    }

    T264dec_mb_decode_uv(t, pred_u, pred_v);   
}
