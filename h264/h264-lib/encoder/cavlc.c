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

 /*****************************************************************************
 * cavlc.c: from x 264
 *****************************************************************************/


//#include <stdlib.h>
//#include <string.h>
//#include <stdint.h>

#include <stdio.h>

#include "T264.h"
#include "vlc.h"
#include "bitstream.h"
#include "inter.h"
#include <assert.h>

static const uint8_t intra4x4_cbp_to_golomb[48]=
{
  3, 29, 30, 17, 31, 18, 37,  8, 32, 38, 19,  9, 20, 10, 11,  2,
 16, 33, 34, 21, 35, 22, 39,  4, 36, 40, 23,  5, 24,  6,  7,  1,
 41, 42, 43, 25, 44, 26, 46, 12, 45, 47, 27, 13, 28, 14, 15,  0
};
static const uint8_t inter_cbp_to_golomb[48]=
{
  0,  2,  3,  7,  4,  8, 17, 13,  5, 18,  9, 14, 10, 15, 16, 11,
  1, 32, 33, 36, 34, 37, 44, 40, 35, 45, 38, 41, 39, 42, 43, 19,
  6, 24, 25, 20, 26, 21, 46, 28, 27, 47, 22, 29, 23, 30, 31, 12
};

/*
static const uint8_t block_idx_x[16] =
{
    0, 1, 0, 1, 2, 3, 2, 3, 0, 1, 0, 1, 2, 3, 2, 3
};
static const uint8_t block_idx_y[16] =
{
    0, 0, 1, 1, 0, 0, 1, 1, 2, 2, 3, 3, 2, 2, 3, 3
};
static const uint8_t block_idx_xy[4][4] =
{
    { 0, 2, 8,  10},
    { 1, 3, 9,  11},
    { 4, 6, 12, 14},
    { 5, 7, 13, 15}
};
*/

#define IS_DIRECT(x) (x == B_DIRECT_16x16 || x == B_DIRECT_8x8)
#define BLOCK_INDEX_CHROMA_DC   (-1)
#define BLOCK_INDEX_LUMA_DC     (-2)

static __inline void eg_write_vlc(bs_t* bs, vlc_t v)
{
	eg_write_direct(bs, v.i_bits & ((uint32_t)~0 >> (uint32_t)(32 - v.i_size)), v.i_size);
}

int8_t
T264_mb_predict_intra4x4_mode(T264_t *t, int32_t idx)
{
	int32_t x, y;
	int8_t nA, nB, pred_blk;

    x = luma_inverse_x[idx];
    y = luma_inverse_y[idx];

    nA = t->mb.i4x4_pred_mode_ref[IPM_LUMA + x + y * 8 - 1];
    nB = t->mb.i4x4_pred_mode_ref[IPM_LUMA + x + y * 8 - 8];
	
    pred_blk  = T264_MIN(nA, nB);

    if( pred_blk < 0 )
        return Intra_4x4_DC;

    return pred_blk;	
}

uint8_t 
T264_mb_predict_non_zero_code(T264_t* t, int idx)
{	
    int32_t x, y;
    int32_t nA, nB;
	int16_t pred_blk;

	if(idx >= 0 && idx < 16)
	{
        x = luma_inverse_x[idx];
        y = luma_inverse_y[idx];
        nA = t->mb.nnz_ref[NNZ_LUMA + x + y * 8 - 1];
        nB = t->mb.nnz_ref[NNZ_LUMA + x + y * 8 - 8];
	}
	else if(idx < 20)
	{
		y = (idx - 16) / 2;
		x = (idx - 16) % 2;
        nA = t->mb.nnz_ref[NNZ_CHROMA0 + x + y * 8 - 1];
        nB = t->mb.nnz_ref[NNZ_CHROMA0 + x + y * 8 - 8];
	}
	else if(idx < 24)
	{
        y = (idx - 20) / 2;
        x = (idx - 20) % 2;
        nA = t->mb.nnz_ref[NNZ_CHROMA1 + x + y * 8 - 1];
        nB = t->mb.nnz_ref[NNZ_CHROMA1 + x + y * 8 - 8];
	}
	
    pred_blk = nA + nB;

    if( pred_blk < 0x80 )
    {
        pred_blk = ( pred_blk + 1 ) >> 1;
    }
    return pred_blk & 0x7f;
}

/****************************************************************************
 * block_residual_write_cavlc
 ****************************************************************************/
static void block_residual_write_cavlc( T264_t *h, int32_t i_idx, int16_t *l, int32_t i_count )
{
    int level[16], run[16];
    int i_total, i_trailing;
    int i_total_zero;
    int i_last;
    unsigned int i_sign;

    int i;
    int i_zero_left;
    int i_suffix_length;

    /* first find i_last */
    i_last = i_count - 1;
    while( i_last >= 0 && l[i_last] == 0 )
    {
        i_last--;
    }

    i_sign = 0;
    i_total = 0;
    i_trailing = 0;
    i_total_zero = 0;

    if( i_last >= 0 )
    {
        int b_trailing = 1;
        int idx = 0;

        /* level and run and total */
        while( i_last >= 0 )
        {
            level[idx] = l[i_last--];

            run[idx] = 0;
            while( i_last >= 0 && l[i_last] == 0 )
            {
                run[idx]++;
                i_last--;
            }

            i_total++;
            i_total_zero += run[idx];

            if( b_trailing && ABS( level[idx] ) == 1 && i_trailing < 3 )
            {
                i_sign <<= 1;
                if( level[idx] < 0 )
                {
                    i_sign |= 0x01;
                }

                i_trailing++;
            }
            else
            {
                b_trailing = 0;
            }

            idx++;
        }
    }

    /* total/trailing represented by Coeff_token (5 tables)*/
    if( i_idx == BLOCK_INDEX_CHROMA_DC )
    {
		eg_write_vlc(h->bs, x264_coeff_token[4][i_total*4+i_trailing]);
    }
    else
    {
        /* T264_mb_predict_non_zero_code return 0 <-> (16+16+1)>>1 = 16 */
        static const int ct_index[17] = {0,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3,3 };
        int nC = 0;

        if( i_idx == BLOCK_INDEX_LUMA_DC )
        {
			// predict nC = (nA + nB) / 2;
            nC = T264_mb_predict_non_zero_code(h, 0);
        }
        else
        {
			// predict nC = (nA + nB) / 2;
            nC = T264_mb_predict_non_zero_code(h, i_idx);
        }

		eg_write_vlc(h->bs, x264_coeff_token[ct_index[nC]][i_total*4+i_trailing]);
    }

    if( i_total <= 0 )
    {
        return;
    }

	//encoding trailing '1's (max = 3)
    i_suffix_length = i_total > 10 && i_trailing < 3 ? 1 : 0;
    if( i_trailing > 0 )
    {
		eg_write_direct(h->bs, i_sign & ((uint32_t)~0 >> (uint32_t)(32 - i_trailing)), i_trailing);
    }

	//encoding remain levels of non-zero coefficients (prefix + suffix)
    for( i = i_trailing; i < i_total; i++ )
    {
        int i_level_code;

        /* calculate level code */
        if( level[i] < 0 )
        {
            i_level_code = -2*level[i] - 1;
        }
        else /* if( level[i] > 0 ) */
        {
            i_level_code = 2 * level[i] - 2;
        }
        if( i == i_trailing && i_trailing < 3 )
        {
            i_level_code -=2; /* as level[i] can't be 1 for the first one if i_trailing < 3 */
        }

        if( ( i_level_code >> i_suffix_length ) < 14 )
        {
            eg_write_vlc(h->bs, x264_level_prefix[i_level_code >> i_suffix_length]);
            if( i_suffix_length > 0 )
            {
				eg_write_direct(h->bs, (uint32_t)i_level_code & ((uint32_t)~0 >> (uint32_t)(32 - i_suffix_length)), i_suffix_length);
            }
        }
        else if( i_suffix_length == 0 && i_level_code < 30 )
        {
            eg_write_vlc(h->bs, x264_level_prefix[14]);
            eg_write_direct(h->bs, (i_level_code - 14) & ((uint32_t)~0 >> (uint32_t)(32 - 4)), 4);
        }
        else if( i_suffix_length > 0 && ( i_level_code >> i_suffix_length ) == 14 )
        {
            eg_write_vlc(h->bs, x264_level_prefix[14]);
            eg_write_direct(h->bs, i_level_code & ((uint32_t)~0 >> (uint32_t)(32 - i_suffix_length)), i_suffix_length);
        }
        else
        {
            eg_write_vlc(h->bs, x264_level_prefix[15]);
            i_level_code -= 15 << i_suffix_length;
            if( i_suffix_length == 0 )
            {
                i_level_code -= 15;
            }

            if( i_level_code >= ( 1 << 12 ) || i_level_code < 0 )
            {
                fprintf( stderr, "OVERFLOW levelcode=%d\n", i_level_code );
            }

			eg_write_direct(h->bs, i_level_code & ((uint32_t)~0 >> (uint32_t)(32 - 12)), 12);
        }

        if( i_suffix_length == 0 )
        {
            i_suffix_length++;
        }
        if( ABS( level[i] ) > ( 3 << ( i_suffix_length - 1 ) ) && i_suffix_length < 6 )
        {
            i_suffix_length++;
        }
    }

	//encode total zeros [i_total-1][i_total_zero]
    if( i_total < i_count )
    {
        if( i_idx == BLOCK_INDEX_CHROMA_DC )
        {
            eg_write_vlc(h->bs, x264_total_zeros_dc[i_total-1][i_total_zero]);
        }
        else
        {
            eg_write_vlc(h->bs, x264_total_zeros[i_total-1][i_total_zero]);
        }
    }

	//encode each run of zeros
    for( i = 0, i_zero_left = i_total_zero; i < i_total - 1; i++ )
    {
        int i_zl;

        if( i_zero_left <= 0 )
        {
            break;
        }

        i_zl = T264_MIN( i_zero_left - 1, 6 );

		eg_write_vlc(h->bs, x264_run_before[i_zl][run[i]]);

        i_zero_left -= run[i];
    }
}

void 
T264_macroblock_write_cavlc(T264_t *t)
{
    int32_t mb_mode = t->mb.mb_mode;
    int32_t i;
    int32_t offset;

    if (t->slice_type == SLICE_I)
    {
        offset = 0;
    }
    else if (t->slice_type == SLICE_P)
    {
        offset = 5;
    }
    else if (t->slice_type == SLICE_B)
    {
        offset = 23;
    }

    if(t->slice_type != SLICE_I)
    {
        eg_write_ue(t->bs, t->skip);  /* skip run */
        t->skip = 0;
    }

    if (mb_mode == I_4x4)
    {
        // mb_type
        eg_write_ue(t->bs, offset + 0);
        /* Prediction: Luma */
        for (i = 0 ; i < 16; i ++) /* i : Inverse raster scan */
        {
            int8_t pred = T264_mb_predict_intra4x4_mode(t, i);
            int8_t mode = t->mb.mode_i4x4[i];

            if( pred == mode)
            {
                eg_write_direct1(t->bs, 1);	/* b_prev_intra4x4_pred_mode */
            }
            else
            {
                eg_write_direct1(t->bs, 0);	/* b_prev_intra4x4_pred_mode */
                if( mode < pred )
                {
                    eg_write_direct(t->bs, mode, 3);  /* rem_intra4x4_pred_mode */
                }
                else
                {
                    eg_write_direct(t->bs, mode - 1, 3);  /* rem_intra4x4_pred_mode */
                }
            }
        }
        //intra_chroma_pred_mode
        eg_write_ue(t->bs, t->mb.mb_mode_uv);

        // coded_block_pattern
        eg_write_ue(t->bs, intra4x4_cbp_to_golomb[( t->mb.cbp_c << 4 ) | t->mb.cbp_y]);

        //delta_qp
        if (t->mb.cbp_y > 0 || t->mb.cbp_c > 0)
        {
            eg_write_se(t->bs, t->mb.mb_qp_delta);	/* 0 = no change on qp */

            for (i = 0; i < 16 ; i ++)
            {
                if(t->mb.cbp_y & (1 << ( i / 4 )))
                {
                    block_residual_write_cavlc(t, i, t->mb.dct_y_z[i], 16);
                }
            }
        }
    }
    else if (mb_mode == I_16x16)
    {
        // mb_type
        eg_write_ue(t->bs, offset + 1 + t->mb.mode_i16x16 + 
            t->mb.cbp_c * 4 + (t->mb.cbp_y == 0 ? 0 : 12));
        // intra chroma pred mode
        eg_write_ue(t->bs, t->mb.mb_mode_uv);

        // delta qp
        eg_write_se(t->bs, t->mb.mb_qp_delta);

        // dc luma
        block_residual_write_cavlc(t, BLOCK_INDEX_LUMA_DC, t->mb.dc4x4_z, 16);

        if (t->mb.cbp_y != 0)
        {
            for(i = 0 ; i < 16 ; i ++)
            {
                if (t->mb.cbp_y & (1 << (i / 4)))
                {
                    block_residual_write_cavlc(t, i, &(t->mb.dct_y_z[i][1]), 15);
                }
            }
        }
    }
    else
    {
        T264_vector_t vec;
        if (t->slice_type == SLICE_P)
        {
            switch (t->mb.mb_part) 
            {
            case MB_16x16:
                eg_write_ue(t->bs, MB_16x16);
                if (t->ps.num_ref_idx_l0_active_minus1 > 0)
                {
                    eg_write_te(t->bs, t->ps.num_ref_idx_l0_active_minus1, t->mb.vec[0][0].refno);
                }
                vec = t->mb.vec[0][0];
                T264_predict_mv(t, 0, 0, 4, &vec);
                eg_write_se(t->bs, t->mb.vec[0][0].x - vec.x);
                eg_write_se(t->bs, t->mb.vec[0][0].y - vec.y);
        	    break;
            case MB_16x8:
                eg_write_ue(t->bs, MB_16x8);
                if (t->ps.num_ref_idx_l0_active_minus1 > 0)
                {
                    eg_write_te(t->bs, t->ps.num_ref_idx_l0_active_minus1, t->mb.vec[0][0].refno);
                    eg_write_te(t->bs, t->ps.num_ref_idx_l0_active_minus1, t->mb.vec[0][8].refno);
                }
                vec = t->mb.vec[0][0];
                T264_predict_mv(t, 0, 0, 4, &vec);
                eg_write_se(t->bs, t->mb.vec[0][0].x - vec.x);
                eg_write_se(t->bs, t->mb.vec[0][0].y - vec.y);

                vec = t->mb.vec[0][8];
                T264_predict_mv(t, 0, 8, 4, &vec);
                eg_write_se(t->bs, t->mb.vec[0][8].x - vec.x);
                eg_write_se(t->bs, t->mb.vec[0][8].y - vec.y);
        	    break;
            case MB_8x16:
                eg_write_ue(t->bs, MB_8x16);
                if (t->ps.num_ref_idx_l0_active_minus1 > 0)
                {
                    eg_write_te(t->bs, t->ps.num_ref_idx_l0_active_minus1, t->mb.vec[0][0].refno);
                    eg_write_te(t->bs, t->ps.num_ref_idx_l0_active_minus1, t->mb.vec[0][luma_index[4]].refno);
                }
                vec = t->mb.vec[0][0];
                T264_predict_mv(t, 0, 0, 2, &vec);
                eg_write_se(t->bs, t->mb.vec[0][0].x - vec.x);
                eg_write_se(t->bs, t->mb.vec[0][0].y - vec.y);

                vec = t->mb.vec[0][luma_index[4]];
                T264_predict_mv(t, 0, luma_index[4], 2, &vec);
                eg_write_se(t->bs, t->mb.vec[0][luma_index[4]].x - vec.x);
                eg_write_se(t->bs, t->mb.vec[0][luma_index[4]].y - vec.y);
                break;
            case MB_8x8:
            case MB_8x8ref0:
                if (t->mb.vec[0][luma_index[0]].refno == 0 &&
                    t->mb.vec[0][luma_index[4]].refno == 0 &&
                    t->mb.vec[0][luma_index[8]].refno == 0 &&
                    t->mb.vec[0][luma_index[12]].refno == 0)
                {
                    eg_write_ue(t->bs, MB_8x8ref0);
                    for (i = 0 ; i < 4 ; i ++)
                    {
                        switch (t->mb.submb_part[luma_index[4 * i]]) 
                        {
                        case MB_8x8:
                            eg_write_ue(t->bs, 0);
                    	    break;
                        case MB_8x4:
                            eg_write_ue(t->bs, 1);
                            break;
                        case MB_4x8:
                            eg_write_ue(t->bs, 2);
                            break;
                        case MB_4x4:
                            eg_write_ue(t->bs, 3);
                            break;
                        default:
                            break;
                        }
                    }
                }
                else
                {
                    eg_write_ue(t->bs, MB_8x8);
                    for (i = 0 ; i < 4 ; i ++)
                    {
                        switch (t->mb.submb_part[luma_index[4 * i]]) 
                        {
                        case MB_8x8:
                            eg_write_ue(t->bs, 0);
                            break;
                        case MB_8x4:
                            eg_write_ue(t->bs, 1);
                            break;
                        case MB_4x8:
                            eg_write_ue(t->bs, 2);
                            break;
                        case MB_4x4:
                            eg_write_ue(t->bs, 3);
                            break;
                        default:
                            break;
                        }
                    }
                    
                    if (t->ps.num_ref_idx_l0_active_minus1 > 0)
                    {
                        eg_write_te(t->bs, t->ps.num_ref_idx_l0_active_minus1, t->mb.vec[0][0].refno);
                        eg_write_te(t->bs, t->ps.num_ref_idx_l0_active_minus1, t->mb.vec[0][luma_index[4]].refno);
                        eg_write_te(t->bs, t->ps.num_ref_idx_l0_active_minus1, t->mb.vec[0][luma_index[8]].refno);
                        eg_write_te(t->bs, t->ps.num_ref_idx_l0_active_minus1, t->mb.vec[0][luma_index[12]].refno);
                    }
                }
                for(i = 0 ; i < 4 ; i ++)
                {
                    switch(t->mb.submb_part[luma_index[4 * i]]) 
                    {
                    case MB_8x8:
                        vec = t->mb.vec[0][luma_index[4 * i]];
                        T264_predict_mv(t, 0, luma_index[4 * i], 2, &vec);
                        eg_write_se(t->bs, t->mb.vec[0][luma_index[4 * i]].x - vec.x);
                        eg_write_se(t->bs, t->mb.vec[0][luma_index[4 * i]].y - vec.y);
                	    break;
                    case MB_8x4:
                        vec = t->mb.vec[0][luma_index[4 * i]];
                        T264_predict_mv(t, 0, luma_index[4 * i], 2, &vec);
                        eg_write_se(t->bs, t->mb.vec[0][luma_index[4 * i]].x - vec.x);
                        eg_write_se(t->bs, t->mb.vec[0][luma_index[4 * i]].y - vec.y);

                        vec = t->mb.vec[0][luma_index[4 * i + 2]];
                        T264_predict_mv(t, 0, luma_index[4 * i + 2], 2, &vec);
                        eg_write_se(t->bs, t->mb.vec[0][luma_index[4 * i + 2]].x - vec.x);
                        eg_write_se(t->bs, t->mb.vec[0][luma_index[4 * i + 2]].y - vec.y);
                	    break;
                    case MB_4x8:
                        vec = t->mb.vec[0][luma_index[4 * i]];
                        T264_predict_mv(t, 0, luma_index[4 * i], 1, &vec);
                        eg_write_se(t->bs, t->mb.vec[0][luma_index[4 * i]].x - vec.x);
                        eg_write_se(t->bs, t->mb.vec[0][luma_index[4 * i]].y - vec.y);

                        vec = t->mb.vec[0][luma_index[4 * i + 1]];
                        T264_predict_mv(t, 0, luma_index[4 * i + 1], 1, &vec);
                        eg_write_se(t->bs, t->mb.vec[0][luma_index[4 * i + 1]].x - vec.x);
                        eg_write_se(t->bs, t->mb.vec[0][luma_index[4 * i + 1]].y - vec.y);
                        break;
                    case MB_4x4:
                        vec = t->mb.vec[0][luma_index[4 * i]];
                        T264_predict_mv(t, 0, luma_index[4 * i], 1, &vec);
                        eg_write_se(t->bs, t->mb.vec[0][luma_index[4 * i]].x - vec.x);
                        eg_write_se(t->bs, t->mb.vec[0][luma_index[4 * i]].y - vec.y);

                        vec = t->mb.vec[0][luma_index[4 * i + 1]];
                        T264_predict_mv(t, 0, luma_index[4 * i + 1], 1, &vec);
                        eg_write_se(t->bs, t->mb.vec[0][luma_index[4 * i + 1]].x - vec.x);
                        eg_write_se(t->bs, t->mb.vec[0][luma_index[4 * i + 1]].y - vec.y);

                        vec = t->mb.vec[0][luma_index[4 * i + 2]];
                        T264_predict_mv(t, 0, luma_index[4 * i + 2], 1, &vec);
                        eg_write_se(t->bs, t->mb.vec[0][luma_index[4 * i + 2]].x - vec.x);
                        eg_write_se(t->bs, t->mb.vec[0][luma_index[4 * i + 2]].y - vec.y);

                        vec = t->mb.vec[0][luma_index[4 * i + 3]];
                        T264_predict_mv(t, 0, luma_index[4 * i + 3], 1, &vec);
                        eg_write_se(t->bs, t->mb.vec[0][luma_index[4 * i + 3]].x - vec.x);
                        eg_write_se(t->bs, t->mb.vec[0][luma_index[4 * i + 3]].y - vec.y);
                        break;
                    }
                }
                break;
            default:
                break;
            }
        }
        else
        {
            switch (t->mb.mb_part)
            {
            case MB_16x16:
                 if (t->mb.is_copy != 1)
                 {
                    switch (t->mb.mb_part2[0])
                    {
                    case B_L0_16x16:
                        eg_write_ue(t->bs, 1);
                        if (t->ps.num_ref_idx_l0_active_minus1 > 0)
                        {
                            eg_write_te(t->bs, t->ps.num_ref_idx_l0_active_minus1, t->mb.vec[0][0].refno);
                        }
                        vec = t->mb.vec[0][0];
                        T264_predict_mv(t, 0, 0, 4, &vec);
                        eg_write_se(t->bs, t->mb.vec[0][0].x - vec.x);
                        eg_write_se(t->bs, t->mb.vec[0][0].y - vec.y);
                        break;
                    case B_L1_16x16:
                        eg_write_ue(t->bs, 2);
                        if (t->ps.num_ref_idx_l1_active_minus1 > 0)
                        {
                            eg_write_te(t->bs, t->ps.num_ref_idx_l1_active_minus1, t->mb.vec[1][0].refno);
                        }
                        vec = t->mb.vec[1][0];
                        T264_predict_mv(t, 1, 0, 4, &vec);
                        eg_write_se(t->bs, t->mb.vec[1][0].x - vec.x);
                        eg_write_se(t->bs, t->mb.vec[1][0].y - vec.y);
                        break;
                    case B_Bi_16x16:
                        eg_write_ue(t->bs, 3);
                        if (t->ps.num_ref_idx_l0_active_minus1 > 0)
                        {
                            eg_write_te(t->bs, t->ps.num_ref_idx_l0_active_minus1, t->mb.vec[0][0].refno);
                        }
                        if (t->ps.num_ref_idx_l1_active_minus1 > 0)
                        {
                            eg_write_te(t->bs, t->ps.num_ref_idx_l1_active_minus1, t->mb.vec[1][0].refno);
                        }
                        vec = t->mb.vec[0][0];
                        T264_predict_mv(t, 0, 0, 4, &vec);
                        eg_write_se(t->bs, t->mb.vec[0][0].x - vec.x);
                        eg_write_se(t->bs, t->mb.vec[0][0].y - vec.y);
                        vec = t->mb.vec[1][0];
                        T264_predict_mv(t, 1, 0, 4, &vec);
                        eg_write_se(t->bs, t->mb.vec[1][0].x - vec.x);
                        eg_write_se(t->bs, t->mb.vec[1][0].y - vec.y);
                        break;
                    }
                }
                else
                {
                    eg_write_ue(t->bs, 0);
                }
                break;
            case MB_16x8:
                {
                    static int8_t mode_16x8[3][3] = 
                    {
                        B_L0_L0_16x8, B_L0_L1_16x8, B_L0_Bi_16x8,
                        B_L1_L0_16x8, B_L1_L1_16x8, B_L1_Bi_16x8,
                        B_Bi_L0_16x8, B_Bi_L1_16x8, B_Bi_Bi_16x8
                    };
                    eg_write_ue(t->bs, mode_16x8[t->mb.mb_part2[0] - B_L0_16x8][t->mb.mb_part2[1] - B_L0_16x8]);
                    if (t->ps.num_ref_idx_l0_active_minus1 > 0)
                    {
                        if (t->mb.mb_part2[0] != B_L1_16x8)
                            eg_write_te(t->bs, t->ps.num_ref_idx_l0_active_minus1, t->mb.vec[0][0].refno);
                        if (t->mb.mb_part2[1] != B_L1_16x8)
                            eg_write_te(t->bs, t->ps.num_ref_idx_l0_active_minus1, t->mb.vec[0][8].refno);
                    }
                    if (t->ps.num_ref_idx_l1_active_minus1 > 0)
                    {
                        if (t->mb.mb_part2[0] != B_L0_16x8)
                            eg_write_te(t->bs, t->ps.num_ref_idx_l1_active_minus1, t->mb.vec[1][0].refno);
                        if (t->mb.mb_part2[1] != B_L0_16x8)
                            eg_write_te(t->bs, t->ps.num_ref_idx_l1_active_minus1, t->mb.vec[1][8].refno);
                    }
                    // l0
                    for (i = 0 ; i < 2 ; i ++)
                    {
                        switch(t->mb.mb_part2[i]) 
                        {
                        case B_L0_16x8:
                            vec = t->mb.vec[0][8 * i];
                            T264_predict_mv(t, 0, luma_index[8 * i], 4, &vec);
                            eg_write_se(t->bs, t->mb.vec[0][luma_index[8 * i]].x - vec.x);
                            eg_write_se(t->bs, t->mb.vec[0][luma_index[8 * i]].y - vec.y);
                            break;
                        case B_Bi_16x8:
                            vec = t->mb.vec[0][luma_index[8 * i]];
                            T264_predict_mv(t, 0, luma_index[8 * i], 4, &vec);
                            eg_write_se(t->bs, t->mb.vec[0][luma_index[8 * i]].x - vec.x);
                            eg_write_se(t->bs, t->mb.vec[0][luma_index[8 * i]].y - vec.y);
                        }
                    }
                    for (i = 0 ; i < 2 ; i ++)
                    {
                        switch(t->mb.mb_part2[i]) 
                        {
                        case B_L1_16x8:
                            vec = t->mb.vec[1][luma_index[8 * i]];
                            T264_predict_mv(t, 1, luma_index[8 * i], 4, &vec);
                            eg_write_se(t->bs, t->mb.vec[1][luma_index[8 * i]].x - vec.x);
                            eg_write_se(t->bs, t->mb.vec[1][luma_index[8 * i]].y - vec.y);
                            break;
                        case B_Bi_16x8:
                            vec = t->mb.vec[1][luma_index[8 * i]];
                            T264_predict_mv(t, 1, luma_index[8 * i], 4, &vec);
                            eg_write_se(t->bs, t->mb.vec[1][luma_index[8 * i]].x - vec.x);
                            eg_write_se(t->bs, t->mb.vec[1][luma_index[8 * i]].y - vec.y);
                            break;
                        }
                    }
                }
                break;
            case MB_8x16:
                {
                    static int8_t mode_8x16[3][3] = 
                    {
                        B_L0_L0_8x16, B_L0_L1_8x16, B_L0_Bi_8x16,
                        B_L1_L0_8x16, B_L1_L1_8x16, B_L1_Bi_8x16,
                        B_Bi_L0_8x16, B_Bi_L1_8x16, B_Bi_Bi_8x16
                    };
                    eg_write_ue(t->bs, mode_8x16[t->mb.mb_part2[0] - B_L0_8x16][t->mb.mb_part2[1] - B_L0_8x16]);
                    if (t->ps.num_ref_idx_l0_active_minus1 > 0)
                    {
                        if (t->mb.mb_part2[0] != B_L1_8x16)
                            eg_write_te(t->bs, t->ps.num_ref_idx_l0_active_minus1, t->mb.vec[0][0].refno);
                        if (t->mb.mb_part2[1] != B_L1_8x16)
                            eg_write_te(t->bs, t->ps.num_ref_idx_l0_active_minus1, t->mb.vec[0][2].refno);
                    }
                    if (t->ps.num_ref_idx_l1_active_minus1 > 0)
                    {
                        if (t->mb.mb_part2[0] != B_L0_8x16)
                            eg_write_te(t->bs, t->ps.num_ref_idx_l1_active_minus1, t->mb.vec[1][0].refno);
                        if (t->mb.mb_part2[1] != B_L0_8x16)
                            eg_write_te(t->bs, t->ps.num_ref_idx_l1_active_minus1, t->mb.vec[1][2].refno);
                    }
                    // l0
                    for (i = 0 ; i < 2 ; i ++)
                    {
                        switch(t->mb.mb_part2[i]) 
                        {
                        case B_L0_8x16:
                            vec = t->mb.vec[0][2 * i];
                            T264_predict_mv(t, 0, luma_index[4 * i], 2, &vec);
                            eg_write_se(t->bs, t->mb.vec[0][2 * i].x - vec.x);
                            eg_write_se(t->bs, t->mb.vec[0][2 * i].y - vec.y);
                            break;
                        case B_Bi_8x16:
                            vec = t->mb.vec[0][2 * i];
                            T264_predict_mv(t, 0, luma_index[4 * i], 2, &vec);
                            eg_write_se(t->bs, t->mb.vec[0][2 * i].x - vec.x);
                            eg_write_se(t->bs, t->mb.vec[0][2 * i].y - vec.y);
                        }
                    }
                    for (i = 0 ; i < 2 ; i ++)
                    {
                        switch(t->mb.mb_part2[i]) 
                        {
                        case B_L1_8x16:
                            vec = t->mb.vec[1][2 * i];
                            T264_predict_mv(t, 1, luma_index[4 * i], 2, &vec);
                            eg_write_se(t->bs, t->mb.vec[1][2 * i].x - vec.x);
                            eg_write_se(t->bs, t->mb.vec[1][2 * i].y - vec.y);
                            break;
                        case B_Bi_8x16:
                            vec = t->mb.vec[1][2 * i];
                            T264_predict_mv(t, 1, luma_index[4 * i], 2, &vec);
                            eg_write_se(t->bs, t->mb.vec[1][2 * i].x - vec.x);
                            eg_write_se(t->bs, t->mb.vec[1][2 * i].y - vec.y);
                            break;
                        }
                    }
                }
                break;
            case MB_8x8:
                {
                    eg_write_ue(t->bs, 22);
                    for (i = 0 ; i < 4 ; i ++)
                    {
                        if (t->mb.sub_is_copy[i] != 1)
                        {
                            switch (t->mb.submb_part[luma_index[4 * i]]) 
                            {
                            case B_L0_8x8:
                                eg_write_ue(t->bs, 1);
                                break;
                            case B_L1_8x8:
                                eg_write_ue(t->bs, 2);
                                break;
                            case B_Bi_8x8:
                                eg_write_ue(t->bs, 3);
                                break;
                            default:
                                assert(0);
                                break;
                            }
                        }
                        else
                        {
                            eg_write_ue(t->bs, 0);
                            break;
                        }
                    }
                    if (t->ps.num_ref_idx_l0_active_minus1 > 0)
                    {
                        for (i = 0 ; i < 4 ; i ++)
                        {
                            if (t->mb.submb_part[luma_index[4 * i]] != 2 && t->mb.submb_part[luma_index[4 * i]] != 0)
                                eg_write_te(t->bs, t->ps.num_ref_idx_l0_active_minus1, t->mb.vec[0][4 * i].refno);
                        }
                    }
                    if (t->ps.num_ref_idx_l1_active_minus1 > 0)
                    {
                        for (i = 0 ; i < 4 ; i ++)
                        {
                            if (t->mb.submb_part[luma_index[4 * i]] != 1 && t->mb.submb_part[luma_index[4 * i]] != 0)
                                eg_write_te(t->bs, t->ps.num_ref_idx_l1_active_minus1, t->mb.vec[1][4 * i].refno);
                        }
                    }
                    // l0
                    for (i = 0 ; i < 4 ; i ++)
                    {
                        switch(t->mb.submb_part[luma_index[4 * i]]) 
                        {
                        case B_L0_8x8:
                        case B_Bi_8x8:
                            vec = t->mb.vec[0][luma_index[4 * i]];
                            T264_predict_mv(t, 0, luma_index[4 * i], 2, &vec);
                            eg_write_se(t->bs, t->mb.vec[0][luma_index[4 * i]].x - vec.x);
                            eg_write_se(t->bs, t->mb.vec[0][luma_index[4 * i]].y - vec.y);
                            break;
                        }
                    }
                    for (i = 0 ; i < 4 ; i ++)
                    {
                        switch(t->mb.submb_part[luma_index[4 * i]]) 
                        {
                        case B_L1_8x8:
                        case B_Bi_8x8:
                            vec = t->mb.vec[1][luma_index[4 * i]];
                            T264_predict_mv(t, 1, luma_index[4 * i], 2, &vec);
                            eg_write_se(t->bs, t->mb.vec[1][luma_index[4 * i]].x - vec.x);
                            eg_write_se(t->bs, t->mb.vec[1][luma_index[4 * i]].y - vec.y);
                            break;
                        }
                    }
                }
                break;
            }
        }
        eg_write_ue(t->bs, inter_cbp_to_golomb[(t->mb.cbp_c << 4)| t->mb.cbp_y]);
        //delta_qp
        if (t->mb.cbp_y > 0 || t->mb.cbp_c > 0)
        {
            eg_write_se(t->bs, t->mb.mb_qp_delta);	/* 0 = no change on qp */

            for (i = 0; i < 16 ; i ++)
            {
                if(t->mb.cbp_y & (1 << ( i / 4 )))
                {
                    block_residual_write_cavlc(t, i, t->mb.dct_y_z[i], 16);
                }
            }
        }
    }

    if (t->mb.cbp_c != 0)
    {
        block_residual_write_cavlc(t, BLOCK_INDEX_CHROMA_DC, t->mb.dc2x2_z[0], 4);
        block_residual_write_cavlc(t, BLOCK_INDEX_CHROMA_DC, t->mb.dc2x2_z[1], 4);
        if (t->mb.cbp_c & 0x2)
        {
            for(i = 0 ; i < 8 ; i ++)
            {
                block_residual_write_cavlc(t, 16 + i, &(t->mb.dct_uv_z[i / 4][i % 4][1]), 15);
            }
        }
    }
}
