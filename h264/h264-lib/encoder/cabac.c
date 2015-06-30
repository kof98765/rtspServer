///*****************************************************************************
//*
//*  T264 AVC CODEC
//*
//*  Copyright(C) 2004-2005 joylife	<joylife_video@yahoo.com.cn>
//*				2004-2005 tricro	<tricro@hotmail.com>
//*
//*  This program is free software ; you can redistribute it and/or modify
//*  it under the terms of the GNU General Public License as published by
//*  the Free Software Foundation ; either version 2 of the License, or
//*  (at your option) any later version.
//*
//*  This program is distributed in the hope that it will be useful,
//*  but WITHOUT ANY WARRANTY ; without even the implied warranty of
//*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//*  GNU General Public License for more details.
//*
//*  You should have received a copy of the GNU General Public License
//*  along with this program ; if not, write to the Free Software
//*  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
//*
//****************************************************************************/
//
///*****************************************************************************
// * cabac.c: h264 encoder library
// *****************************************************************************
// * Copyright (C) 2003 Laurent Aimar
// *
// * Authors: Laurent Aimar <fenrir@via.ecp.fr>
// *
// * This program is free software; you can redistribute it and/or modify
// * it under the terms of the GNU General Public License as published by
// * the Free Software Foundation; either version 2 of the License, or
// * (at your option) any later version.
// *
// * This program is distributed in the hope that it will be useful,
// * but WITHOUT ANY WARRANTY; without even the implied warranty of
// * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// * GNU General Public License for more details.
// *
// * You should have received a copy of the GNU General Public License
// * along with this program; if not, write to the Free Software
// * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA.
// *****************************************************************************/
//
///*Note: the CABAC routine is currently referenced from x264 temporarily, with adaptation to 
// *the data structure of T264. It should be modified further in the near future.
// *It's can support B slice, but only with MB mode P16x16, P16x8, P8x16, Direct16x16, B_SKIP
// *ie., B8x8 is not support now
//*/
//
//#include <stdlib.h>
//#include <stdio.h>
//#include <string.h>
//#include <assert.h>
//#include "T264.h"
//#include "cabac_engine.h"
//#include "inter.h"
///* From ffmpeg
//*/
//#define T264_SCAN8_SIZE (6*8)
//#define T264_SCAN8_0 (4+1*8)
//
//static const int T264_scan8[16+2*4] =
//{
//	/* Luma */
//	VEC_LUMA + 0, VEC_LUMA + 1, VEC_LUMA + 1*8 + 0, VEC_LUMA + 1*8 + 1,
//	VEC_LUMA + 2, VEC_LUMA + 3, VEC_LUMA + 1*8 + 2, VEC_LUMA + 1*8 + 3,
//	VEC_LUMA + 2*8 + 0, VEC_LUMA + 2*8 + 1, VEC_LUMA + 3*8 + 0, VEC_LUMA + 3*8 + 1,
//	VEC_LUMA + 2*8 + 2, VEC_LUMA + 2*8 + 3, VEC_LUMA + 3*8 + 2, VEC_LUMA + 3*8 + 3,
//
//	/* Cb */
//	NNZ_CHROMA0 + 0, NNZ_CHROMA0 + 1,
//	NNZ_CHROMA0 + 1*8 + 0, NNZ_CHROMA0 + 1*8 + 1,
//
//	/* Cr */
//	NNZ_CHROMA1 + 0, NNZ_CHROMA1 + 1,
//	NNZ_CHROMA1 + 1*8 + 0, NNZ_CHROMA1 + 1*8 + 1,
//};
//static const uint8_t block_idx_xy[4][4] =
//{
//	{ 0, 2, 8,  10},
//	{ 1, 3, 9,  11},
//	{ 4, 6, 12, 14},
//	{ 5, 7, 13, 15}
//};
//
//#define IS_INTRA(mode) (mode == I_4x4 || mode == I_16x16)
//#define IS_SKIP(type)  ( (type) == P_SKIP || (type) == B_SKIP )
//enum {
//	INTRA_4x4           = 0,
//	INTRA_16x16         = 1,
//	INTRA_PCM           = 2,
//
//	P_L0            = 3,
//	P_8x81          = 4,
//	P_SKIP1         = 5,
//
//	B_DIRECT        = 6,
//	B_L0_L0         = 7,
//	B_L0_L1         = 8,
//	B_L0_BI         = 9,
//	B_L1_L0         = 10,
//	B_L1_L1         = 11,
//	B_L1_BI         = 12,
//	B_BI_L0         = 13,
//	B_BI_L1         = 14,
//	B_BI_BI         = 15,
//	B_8x81          = 16,
//	B_SKIP1         = 17,
//};
//
//static const int T264_mb_partition_listX_table[][2] = 
//{
//	{0, 0}, //B_DIRECT_8x8 = 100,
//	{1, 0}, //B_L0_8x8,
//	{0, 1}, //B_L1_8x8,
//	{1, 1}, //B_Bi_8x8,
//	{1, 0}, //B_L0_8x4,
//	{1, 0}, //B_L0_4x8,
//	{0, 1}, //B_L1_8x4,
//	{0, 1}, //B_L1_4x8,
//	{1, 1}, //B_Bi_8x4,
//	{1, 1}, //B_Bi_4x8,
//	{1, 0}, //B_L0_4x4,
//	{0, 1},	//B_L1_4x4,
//	{1, 1}	//B_Bi_4x4
//};
//
//static const int T264_mb_type_list0_table[18][2] =
//{
//	{0,0}, {0,0}, {0,0},    /* INTRA */
//	{1,1},                  /* P_L0 */
//	{0,0},                  /* P_8x8 */
//	{1,1},                  /* P_SKIP */
//	{0,0},                  /* B_DIRECT */
//	{1,1}, {1,0}, {1,1},    /* B_L0_* */
//	{0,1}, {0,0}, {0,1},    /* B_L1_* */
//	{1,1}, {1,0}, {1,1},    /* B_BI_* */
//	{0,0},                  /* B_8x8 */
//	{0,0}                   /* B_SKIP */
//};
//static const int T264_mb_type_list1_table[18][2] =
//{
//	{0,0}, {0,0}, {0,0},    /* INTRA */
//	{0,0},                  /* P_L0 */
//	{0,0},                  /* P_8x8 */
//	{0,0},                  /* P_SKIP */
//	{0,0},                  /* B_DIRECT */
//	{0,0}, {0,1}, {0,1},    /* B_L0_* */
//	{1,0}, {1,1}, {1,1},    /* B_L1_* */
//	{1,0}, {1,1}, {1,1},    /* B_BI_* */
//	{0,0},                  /* B_8x8 */
//	{0,0}                   /* B_SKIP */
//};
//
//static void T264_cabac_mb_type( T264_t *t )
//{
//	T264_mb_context_t *mb_ctxs = &(t->rec->mb[0]);
//    int32_t mb_mode = t->mb.mb_mode;
//
//    if( t->slice_type == SLICE_I )
//    {
//        int ctx = 0;
//        if( t->mb.mb_x > 0 && mb_ctxs[t->mb.mb_xy-1].mb_mode != I_4x4 )
//        {
//            ctx++;
//        }
//        if( t->mb.mb_y > 0 && mb_ctxs[t->mb.mb_xy - t->mb_stride].mb_mode != I_4x4 )
//        {
//            ctx++;
//        }
//
//        if( mb_mode == I_4x4 )
//        {
//            T264_cabac_encode_decision( &t->cabac, 3 + ctx, 0 );
//        }
//        else if(mb_mode == I_16x16)   /* I_16x16 */
//        {
//            T264_cabac_encode_decision( &t->cabac, 3 + ctx, 1 );
//            T264_cabac_encode_terminal( &t->cabac, 0 );
//
//            T264_cabac_encode_decision( &t->cabac, 3 + 3, ( t->mb.cbp_y == 0 ? 0 : 1 ));
//            if( t->mb.cbp_c == 0 )
//            {
//                T264_cabac_encode_decision( &t->cabac, 3 + 4, 0 );
//            }
//            else
//            {
//                T264_cabac_encode_decision( &t->cabac, 3 + 4, 1 );
//                T264_cabac_encode_decision( &t->cabac, 3 + 5, ( t->mb.cbp_c == 1 ? 0 : 1 ) );
//            }
//            T264_cabac_encode_decision( &t->cabac, 3 + 6, ( (t->mb.mode_i16x16 / 2) ? 1 : 0 ));
//            T264_cabac_encode_decision( &t->cabac, 3 + 7, ( (t->mb.mode_i16x16 % 2) ? 1 : 0 ));
//        }
//		else	/* I_PCM */
//		{
//			T264_cabac_encode_decision( &t->cabac, 3 + ctx, 1 );
//			T264_cabac_encode_terminal( &t->cabac, 1 );
//		}
//    }
//    else if( t->slice_type == SLICE_P )
//    {
//        /* prefix: 14, suffix: 17 */
//        if( mb_mode == P_MODE )
//        {
//            if( t->mb.mb_part == MB_16x16 )
//            {
//                T264_cabac_encode_decision( &t->cabac, 14, 0 );
//                T264_cabac_encode_decision( &t->cabac, 15, 0 );
//                T264_cabac_encode_decision( &t->cabac, 16, 0 );
//            }
//            else if( t->mb.mb_part == MB_16x8 )
//            {
//                T264_cabac_encode_decision( &t->cabac, 14, 0 );
//                T264_cabac_encode_decision( &t->cabac, 15, 1 );
//                T264_cabac_encode_decision( &t->cabac, 17, 1 );
//            }
//            else if( t->mb.mb_part == MB_8x16 )
//            {
//                T264_cabac_encode_decision( &t->cabac, 14, 0 );
//                T264_cabac_encode_decision( &t->cabac, 15, 1 );
//                T264_cabac_encode_decision( &t->cabac, 17, 0 );
//            }
//			else /* P8x8 mode */
//			{
//				T264_cabac_encode_decision( &t->cabac, 14, 0 );
//				T264_cabac_encode_decision( &t->cabac, 15, 0 );
//				T264_cabac_encode_decision( &t->cabac, 16, 1 );
//			}
//        }
//        else if( mb_mode == I_4x4 )
//        {
//            /* prefix */
//            T264_cabac_encode_decision( &t->cabac, 14, 1 );
//
//            T264_cabac_encode_decision( &t->cabac, 17, 0 );
//        }
//        else if(mb_mode == I_16x16) /* intra 16x16 */
//        {
//            /* prefix */
//            T264_cabac_encode_decision( &t->cabac, 14, 1 );
//
//            /* suffix */
//            T264_cabac_encode_decision( &t->cabac, 17, 1 );
//            T264_cabac_encode_terminal( &t->cabac, 0 ); /*ctxIdx == 276 */
//
//            T264_cabac_encode_decision( &t->cabac, 17+1, ( t->mb.cbp_y == 0 ? 0 : 1 ));
//            if( t->mb.cbp_c == 0 )
//            {
//                T264_cabac_encode_decision( &t->cabac, 17+2, 0 );
//            }
//            else
//            {
//                T264_cabac_encode_decision( &t->cabac, 17+2, 1 );
//                T264_cabac_encode_decision( &t->cabac, 17+2, ( t->mb.cbp_c == 1 ? 0 : 1 ) );
//            }
//            T264_cabac_encode_decision( &t->cabac, 17+3, ( (t->mb.mode_i16x16 / 2) ? 1 : 0 ));
//            T264_cabac_encode_decision( &t->cabac, 17+3, ( (t->mb.mode_i16x16 % 2) ? 1 : 0 ));
//        }
//		else /* I_PCM */
//		{
//			/* prefix */
//			T264_cabac_encode_decision( &t->cabac, 14, 1 );
//
//			T264_cabac_encode_decision( &t->cabac, 17, 1 );
//			T264_cabac_encode_terminal( &t->cabac, 1 ); /*ctxIdx == 276 */
//		}
//    }
//    else if( t->slice_type == SLICE_B )
//    {
//		int ctx = 0;
//		if( t->mb.mb_x > 0 && mb_ctxs[t->mb.mb_xy-1].mb_mode != B_SKIP && !mb_ctxs[t->mb.mb_xy-1].is_copy )
//		{
//			ctx++;
//		}
//		if( t->mb.mb_y > 0 && mb_ctxs[t->mb.mb_xy - t->mb_stride].mb_mode != B_SKIP && ! mb_ctxs[t->mb.mb_xy - t->mb_stride].is_copy)
//		{
//			ctx++;
//		}
//        
//        if( t->mb.is_copy)
//        {
//            T264_cabac_encode_decision( &t->cabac, 27+ctx, 0 );
//        }
//        else if( t->mb.mb_part == MB_8x8 )
//        {
//            T264_cabac_encode_decision( &t->cabac, 27+ctx, 1 );
//            T264_cabac_encode_decision( &t->cabac, 27+3,   1 );
//            T264_cabac_encode_decision( &t->cabac, 27+4,   1 );
//
//            T264_cabac_encode_decision( &t->cabac, 27+5,   1 );
//            T264_cabac_encode_decision( &t->cabac, 27+5,   1 );
//            T264_cabac_encode_decision( &t->cabac, 27+5,   1 );
//        }
//        else if( IS_INTRA( mb_mode ) )
//        {
//            /* prefix */
//            T264_cabac_encode_decision( &t->cabac, 27+ctx, 1 );
//            T264_cabac_encode_decision( &t->cabac, 27+3,   1 );
//            T264_cabac_encode_decision( &t->cabac, 27+4,   1 );
//
//            T264_cabac_encode_decision( &t->cabac, 27+5,   1 );
//            T264_cabac_encode_decision( &t->cabac, 27+5,   0 );
//            T264_cabac_encode_decision( &t->cabac, 27+5,   1 );
//
//            /* Suffix */
//            if( mb_mode == I_4x4 )
//            {
//                T264_cabac_encode_decision( &t->cabac, 32, 0 );
//            }
//			else if(mb_mode == I_16x16)
//			{
//				T264_cabac_encode_decision( &t->cabac, 32, 1 );
//				T264_cabac_encode_terminal( &t->cabac,     0 );
//
//				/* TODO */
//				T264_cabac_encode_decision( &t->cabac, 32+1, ( t->mb.cbp_y == 0 ? 0 : 1 ));
//				if( t->mb.cbp_c == 0 )
//				{
//					T264_cabac_encode_decision( &t->cabac, 32+2, 0 );
//				}
//				else
//				{
//					T264_cabac_encode_decision( &t->cabac, 32+2, 1 );
//					T264_cabac_encode_decision( &t->cabac, 32+2, ( t->mb.cbp_c == 1 ? 0 : 1 ) );
//				}
//				T264_cabac_encode_decision( &t->cabac, 32+3, ( (t->mb.mode_i16x16 / 2) ? 1 : 0 ));
//				T264_cabac_encode_decision( &t->cabac, 32+3, ( (t->mb.mode_i16x16 % 2) ? 1 : 0 ));
//			}
//            else /* I_PCM */
//            {
//                T264_cabac_encode_decision( &t->cabac, 32, 1 );
//                T264_cabac_encode_terminal( &t->cabac,     1 );
//            }
//            
//        }
//        else
//        {
//            static const int i_mb_len[21] =
//            {
//                3, 6, 6,    /* L0 L0 */
//                3, 6, 6,    /* L1 L1 */
//                6, 7, 7,    /* BI BI */
//
//                6, 6,       /* L0 L1 */
//                6, 6,       /* L1 L0 */
//                7, 7,       /* L0 BI */
//                7, 7,       /* L1 BI */
//                7, 7,       /* BI L0 */
//                7, 7,       /* BI L1 */
//            };
//            static const int i_mb_bits[21][7] =
//            {
//                { 1, 0, 0, },            { 1, 1, 0, 0, 0, 1, },    { 1, 1, 0, 0, 1, 0, },   /* L0 L0 */
//                { 1, 0, 1, },            { 1, 1, 0, 0, 1, 1, },    { 1, 1, 0, 1, 0, 0, },   /* L1 L1 */
//                { 1, 1, 0, 0, 0, 0 ,},   { 1, 1, 1, 1, 0, 0 , 0 }, { 1, 1, 1, 1, 0, 0 , 1 },/* BI BI */
//
//                { 1, 1, 0, 1, 0, 1, },   { 1, 1, 0, 1, 1, 0, },     /* L0 L1 */
//                { 1, 1, 0, 1, 1, 1, },   { 1, 1, 1, 1, 1, 0, },     /* L1 L0 */
//                { 1, 1, 1, 0, 0, 0, 0 }, { 1, 1, 1, 0, 0, 0, 1 },   /* L0 BI */
//                { 1, 1, 1, 0, 0, 1, 0 }, { 1, 1, 1, 0, 0, 1, 1 },   /* L1 BI */
//                { 1, 1, 1, 0, 1, 0, 0 }, { 1, 1, 1, 0, 1, 0, 1 },   /* BI L0 */
//                { 1, 1, 1, 0, 1, 1, 0 }, { 1, 1, 1, 0, 1, 1, 1 }    /* BI L1 */
//            };
//
//            const int i_partition = t->mb.mb_part;
//            int idx = 0;
//            int i, b_part_mode, part_mode0, part_mode1;
//			static const int b_part_mode_map[3][3] = {
//				{ B_L0_L0, B_L0_L1, B_L0_BI },
//				{ B_L1_L0, B_L1_L1, B_L1_BI },
//				{ B_BI_L0, B_BI_L1, B_BI_BI }
//			};
//
//			switch(t->mb.mb_part)
//			{
//			case MB_16x16:
//				part_mode0 = t->mb.mb_part2[0] - B_L0_16x16;
//				b_part_mode = b_part_mode_map[part_mode0][part_mode0];
//				break;
//			case MB_16x8:
//				part_mode0 = t->mb.mb_part2[0] - B_L0_16x8;
//				part_mode1 = t->mb.mb_part2[1] - B_L0_16x8;
//				b_part_mode = b_part_mode_map[part_mode0][part_mode1];
//				break;
//			case MB_8x16:
//				part_mode0 = t->mb.mb_part2[0] - B_L0_8x16;
//				part_mode1 = t->mb.mb_part2[1] - B_L0_8x16;
//				b_part_mode = b_part_mode_map[part_mode0][part_mode1];
//				break;
//			}
//            switch( b_part_mode )
//            {
//                /* D_16x16, D_16x8, D_8x16 */
//                case B_BI_BI: idx += 3;
//                case B_L1_L1: idx += 3;
//                case B_L0_L0:
//                    if( i_partition == MB_16x8 )
//                        idx += 1;
//                    else if( i_partition == MB_8x16 )
//                        idx += 2;
//                    break;
//
//                /* D_16x8, D_8x16 */
//                case B_BI_L1: idx += 2;
//                case B_BI_L0: idx += 2;
//                case B_L1_BI: idx += 2;
//                case B_L0_BI: idx += 2;
//                case B_L1_L0: idx += 2;
//                case B_L0_L1:
//                    idx += 3*3;
//                    if( i_partition == MB_8x16 )
//                        idx++;
//                    break;
//                default:
//					return;
//			}
//
//            T264_cabac_encode_decision( &t->cabac, 27+ctx,                         i_mb_bits[idx][0] );
//            T264_cabac_encode_decision( &t->cabac, 27+3,                           i_mb_bits[idx][1] );
//            T264_cabac_encode_decision( &t->cabac, 27+(i_mb_bits[idx][1] != 0 ? 4 : 5), i_mb_bits[idx][2] );
//            for( i = 3; i < i_mb_len[idx]; i++ )
//            {
//                T264_cabac_encode_decision( &t->cabac, 27+5,                       i_mb_bits[idx][i] );
//            }
//        }
//    }
//    else
//    {
//		//dummy here
//    }
//}
//
//static void T264_cabac_mb_intra4x4_pred_mode( T264_t *t, int i_pred, int i_mode )
//{
//    if( i_pred == i_mode )
//    {
//        /* b_prev_intra4x4_pred_mode */
//        T264_cabac_encode_decision( &t->cabac, 68, 1 );
//    }
//    else
//    {
//        /* b_prev_intra4x4_pred_mode */
//        T264_cabac_encode_decision( &t->cabac, 68, 0 );
//        if( i_mode > i_pred  )
//        {
//            i_mode--;
//        }
//        T264_cabac_encode_decision( &t->cabac, 69, (i_mode     )&0x01 );
//        T264_cabac_encode_decision( &t->cabac, 69, (i_mode >> 1)&0x01 );
//        T264_cabac_encode_decision( &t->cabac, 69, (i_mode >> 2)&0x01 );
//    }
//}
//
//static void T264_cabac_mb_intra8x8_pred_mode( T264_t *t )
//{
//    const int i_mode  = t->mb.mb_mode_uv;
//	T264_mb_context_t *mb_ctxs = &(t->rec->mb[0]);
//
//	int ctx = 0;
//	if( t->mb.mb_x > 0 && mb_ctxs[t->mb.mb_xy-1].mb_mode_uv != Intra_8x8_DC)
//	{
//		ctx++;
//	}
//	if( t->mb.mb_y > 0 && mb_ctxs[t->mb.mb_xy - t->mb_stride].mb_mode_uv != Intra_8x8_DC )
//	{
//		ctx++;
//	}
//	
//    if( i_mode == Intra_8x8_DC )
//    {
//        T264_cabac_encode_decision( &t->cabac, 64 + ctx, Intra_8x8_DC );
//    }
//    else
//    {
//        T264_cabac_encode_decision( &t->cabac, 64 + ctx, 1 );
//        T264_cabac_encode_decision( &t->cabac, 64 + 3, ( i_mode == 1 ? 0 : 1 ) );
//        if( i_mode > 1 )
//        {
//            T264_cabac_encode_decision( &t->cabac, 64 + 3, ( i_mode == 2 ? 0 : 1 ) );
//        }
//    }
//}
//
//static void T264_cabac_mb_cbp_luma( T264_t *t )
//{
//    /* TODO: clean up and optimize */
//	T264_mb_context_t *mb_ctxs = &(t->rec->mb[0]);
//    int i8x8;
//    for( i8x8 = 0; i8x8 < 4; i8x8++ )
//    {
//        int i_mba_xy = -1;
//        int i_mbb_xy = -1;
//        int x = luma_inverse_x[4*i8x8];
//        int y = luma_inverse_y[4*i8x8];
//        int ctx = 0;
//
//        if( x > 0 )
//            i_mba_xy = t->mb.mb_xy;
//        else if( t->mb.mb_x > 0 )
//            i_mba_xy = t->mb.mb_xy - 1;
//
//        if( y > 0 )
//            i_mbb_xy = t->mb.mb_xy;
//        else if( t->mb.mb_y > 0 )
//            i_mbb_xy = t->mb.mb_xy - t->mb_stride;
//
//
//        /* No need to test for PCM and SKIP */
//        if( i_mba_xy >= 0 )
//        {
//            const int i8x8a = block_idx_xy[(x-1)&0x03][y]/4;
//            if( ((mb_ctxs[i_mba_xy].cbp_y >> i8x8a)&0x01) == 0 )
//            {
//                ctx++;
//            }
//        }
//
//        if( i_mbb_xy >= 0 )
//        {
//            const int i8x8b = block_idx_xy[x][(y-1)&0x03]/4;
//            if( ((mb_ctxs[i_mbb_xy].cbp_y >> i8x8b)&0x01) == 0 )
//            {
//                ctx += 2;
//            }
//        }
//															   
//        T264_cabac_encode_decision( &t->cabac, 73 + ctx, (t->mb.cbp_y >> i8x8)&0x01 );
//    }
//}
//
//static void T264_cabac_mb_cbp_chroma( T264_t *t )
//{
//    int cbp_a = -1;
//    int cbp_b = -1;
//    int ctx;
//	T264_mb_context_t *mb_ctxs = &(t->rec->mb[0]);
//    /* No need to test for SKIP/PCM */
//    if( t->mb.mb_x > 0 )
//    {
//        cbp_a = (mb_ctxs[t->mb.mb_xy - 1].cbp_c)&0x3;
//    }
//
//    if( t->mb.mb_y > 0 )
//    {
//        cbp_b = (mb_ctxs[t->mb.mb_xy - t->mb_stride].cbp_c)&0x3;
//    }
//
//    ctx = 0;
//    if( cbp_a > 0 ) ctx++;
//    if( cbp_b > 0 ) ctx += 2;
//    if( t->mb.cbp_c == 0 )
//    {
//        T264_cabac_encode_decision( &t->cabac, 77 + ctx, 0 );
//    }
//    else
//    {
//        T264_cabac_encode_decision( &t->cabac, 77 + ctx, 1 );
//
//        ctx = 4;
//        if( cbp_a == 2 ) ctx++;
//        if( cbp_b == 2 ) ctx += 2;
//        T264_cabac_encode_decision( &t->cabac, 77 + ctx, t->mb.cbp_c > 1 ? 1 : 0 );
//    }
//}
//
///* TODO check it with != qp per mb */
//static void T264_cabac_mb_qp_delta( T264_t *t )
//{
//    int i_mbn_xy = t->mb.mb_xy - 1;
//    int i_dqp = t->mb.mb_qp_delta;
//    int val = i_dqp <= 0 ? (-2*i_dqp) : (2*i_dqp - 1);
//    int ctx;
//	T264_mb_context_t *mb_ctxs = &(t->rec->mb[0]);
//
//    /* No need to test for PCM / SKIP */
//    if( i_mbn_xy >= 0 && mb_ctxs[i_mbn_xy].mb_qp_delta != 0 &&
//        ( mb_ctxs[i_mbn_xy].mb_mode == I_16x16 || mb_ctxs[i_mbn_xy].cbp_y || mb_ctxs[i_mbn_xy].cbp_c) )
//        ctx = 1;
//    else
//        ctx = 0;
//
//    while( val > 0 )
//    {
//        T264_cabac_encode_decision( &t->cabac,  60 + ctx, 1 );
//        if( ctx < 2 )
//            ctx = 2;
//        else
//            ctx = 3;
//        val--;
//    }
//    T264_cabac_encode_decision( &t->cabac,  60 + ctx, 0 );
//}
//
//void T264_cabac_mb_skip( T264_t *t, int b_skip )
//{
//	T264_mb_context_t *mb_ctxs = &(t->rec->mb[0]);
//    int ctx = 0;
//
//    if( t->mb.mb_x > 0 && !IS_SKIP( mb_ctxs[t->mb.mb_xy -1].mb_mode) )
//    {
//        ctx++;
//    }
//    if( t->mb.mb_y > 0 && !IS_SKIP( mb_ctxs[t->mb.mb_xy - t->mb_stride].mb_mode) )
//    {
//        ctx++;
//    }
//
//    if( t->slice_type == SLICE_P )
//        T264_cabac_encode_decision( &t->cabac, 11 + ctx, b_skip ? 1 : 0 );
//    else /* SLICE_TYPE_B */
//        T264_cabac_encode_decision( &t->cabac, 24 + ctx, b_skip ? 1 : 0 );
//}
//
//static __inline  void T264_cabac_mb_sub_p_partition( T264_t *t, int i_sub )
//{
//    if( i_sub == MB_8x8 )
//    {
//            T264_cabac_encode_decision( &t->cabac, 21, 1 );
//    }
//    else if( i_sub == MB_8x4 )
//    {
//            T264_cabac_encode_decision( &t->cabac, 21, 0 );
//            T264_cabac_encode_decision( &t->cabac, 22, 0 );
//    }
//    else if( i_sub == MB_4x8 )
//    {
//            T264_cabac_encode_decision( &t->cabac, 21, 0 );
//            T264_cabac_encode_decision( &t->cabac, 22, 1 );
//            T264_cabac_encode_decision( &t->cabac, 23, 1 );
//    }
//    else if( i_sub == MB_4x4 )
//    {
//            T264_cabac_encode_decision( &t->cabac, 21, 0 );
//            T264_cabac_encode_decision( &t->cabac, 22, 1 );
//            T264_cabac_encode_decision( &t->cabac, 23, 0 );
//    }
//}
//
//static __inline  void T264_cabac_mb_sub_b_partition( T264_t *t, int i_sub )
//{
//    if( i_sub == B_DIRECT_8x8 )
//    {
//        T264_cabac_encode_decision( &t->cabac, 36, 0 );
//    }
//    else if( i_sub == B_L0_8x8 )
//    {
//        T264_cabac_encode_decision( &t->cabac, 36, 1 );
//        T264_cabac_encode_decision( &t->cabac, 37, 0 );
//        T264_cabac_encode_decision( &t->cabac, 39, 0 );
//    }
//    else if( i_sub == B_L1_8x8 )
//    {
//        T264_cabac_encode_decision( &t->cabac, 36, 1 );
//        T264_cabac_encode_decision( &t->cabac, 37, 0 );
//        T264_cabac_encode_decision( &t->cabac, 39, 1 );
//    }
//    else if( i_sub == B_Bi_8x8 )
//    {
//        T264_cabac_encode_decision( &t->cabac, 36, 1 );
//        T264_cabac_encode_decision( &t->cabac, 37, 1 );
//        T264_cabac_encode_decision( &t->cabac, 38, 0 );
//        T264_cabac_encode_decision( &t->cabac, 39, 0 );
//        T264_cabac_encode_decision( &t->cabac, 39, 0 );
//    }
//    else if( i_sub == B_L0_8x4 )
//    {
//        T264_cabac_encode_decision( &t->cabac, 36, 1 );
//        T264_cabac_encode_decision( &t->cabac, 37, 1 );
//        T264_cabac_encode_decision( &t->cabac, 38, 0 );
//        T264_cabac_encode_decision( &t->cabac, 39, 0 );
//        T264_cabac_encode_decision( &t->cabac, 39, 1 );
//    }
//    else if( i_sub == B_L0_4x8 )
//    {
//        T264_cabac_encode_decision( &t->cabac, 36, 1 );
//        T264_cabac_encode_decision( &t->cabac, 37, 1 );
//        T264_cabac_encode_decision( &t->cabac, 38, 0 );
//        T264_cabac_encode_decision( &t->cabac, 39, 1 );
//        T264_cabac_encode_decision( &t->cabac, 39, 0 );
//    }
//    else if( i_sub == B_L1_8x4 )
//    {
//        T264_cabac_encode_decision( &t->cabac, 36, 1 );
//        T264_cabac_encode_decision( &t->cabac, 37, 1 );
//        T264_cabac_encode_decision( &t->cabac, 38, 0 );
//        T264_cabac_encode_decision( &t->cabac, 39, 1 );
//        T264_cabac_encode_decision( &t->cabac, 39, 1 );
//    }
//    else if( i_sub == B_L1_4x8 )
//    {
//        T264_cabac_encode_decision( &t->cabac, 36, 1 );
//        T264_cabac_encode_decision( &t->cabac, 37, 1 );
//        T264_cabac_encode_decision( &t->cabac, 38, 1 );
//        T264_cabac_encode_decision( &t->cabac, 39, 0 );
//        T264_cabac_encode_decision( &t->cabac, 39, 0 );
//        T264_cabac_encode_decision( &t->cabac, 39, 0 );
//    }
//    else if( i_sub == B_Bi_8x4 )
//    {
//        T264_cabac_encode_decision( &t->cabac, 36, 1 );
//        T264_cabac_encode_decision( &t->cabac, 37, 1 );
//        T264_cabac_encode_decision( &t->cabac, 38, 1 );
//        T264_cabac_encode_decision( &t->cabac, 39, 0 );
//        T264_cabac_encode_decision( &t->cabac, 39, 0 );
//        T264_cabac_encode_decision( &t->cabac, 39, 1 );
//    }
//    else if( i_sub == B_Bi_4x8 )
//    {
//        T264_cabac_encode_decision( &t->cabac, 36, 1 );
//        T264_cabac_encode_decision( &t->cabac, 37, 1 );
//        T264_cabac_encode_decision( &t->cabac, 38, 1 );
//        T264_cabac_encode_decision( &t->cabac, 39, 0 );
//        T264_cabac_encode_decision( &t->cabac, 39, 1 );
//        T264_cabac_encode_decision( &t->cabac, 39, 0 );
//    }
//    else if( i_sub == B_L0_4x4 )
//    {
//        T264_cabac_encode_decision( &t->cabac, 36, 1 );
//        T264_cabac_encode_decision( &t->cabac, 37, 1 );
//        T264_cabac_encode_decision( &t->cabac, 38, 1 );
//        T264_cabac_encode_decision( &t->cabac, 39, 0 );
//        T264_cabac_encode_decision( &t->cabac, 39, 1 );
//        T264_cabac_encode_decision( &t->cabac, 39, 1 );
//    }
//    else if( i_sub == B_L1_4x4 )
//    {
//        T264_cabac_encode_decision( &t->cabac, 36, 1 );
//        T264_cabac_encode_decision( &t->cabac, 37, 1 );
//        T264_cabac_encode_decision( &t->cabac, 38, 1 );
//        T264_cabac_encode_decision( &t->cabac, 39, 1 );
//        T264_cabac_encode_decision( &t->cabac, 39, 0 );
//    }
//    else if( i_sub == B_Bi_4x4 )
//    {
//        T264_cabac_encode_decision( &t->cabac, 36, 1 );
//        T264_cabac_encode_decision( &t->cabac, 37, 1 );
//        T264_cabac_encode_decision( &t->cabac, 38, 1 );
//        T264_cabac_encode_decision( &t->cabac, 39, 1 );
//        T264_cabac_encode_decision( &t->cabac, 39, 1 );
//    }
//}
//
//
//static __inline  void T264_cabac_mb_ref( T264_t *t, int i_list, int idx )
//{
//	const int i8    = T264_scan8[idx];
//	T264_mb_context_t *mb_ctxs = &(t->rec->mb[0]);
//	const int i_refa = t->mb.vec_ref[i8 - 1].vec[i_list].refno;
//    const int i_refb = t->mb.vec_ref[i8 - 8].vec[i_list].refno;
//    int i_ref  = t->mb.vec_ref[i8].vec[i_list].refno;
//	int a_direct, b_direct;
//	int ctx  = 0;
//	int luma_idx = luma_index[idx];
//	if( t->slice_type==SLICE_B && t->mb.mb_x > 0 && (mb_ctxs[t->mb.mb_xy-1].mb_mode == B_SKIP||mb_ctxs[t->mb.mb_xy-1].is_copy ) && (luma_idx&0x03)==0)
//	{
//		a_direct = 1;
//	}
//	else
//		a_direct = 0;
//	if( t->slice_type==SLICE_B && t->mb.mb_y > 0 && (mb_ctxs[t->mb.mb_xy - t->mb_stride].mb_mode == B_SKIP||mb_ctxs[t->mb.mb_xy - t->mb_stride].is_copy) && luma_idx<4)
//	{
//		b_direct = 1;
//	}
//	else
//		b_direct = 0;
//
//    if( i_refa>0 && !a_direct)
//        ctx++;
//    if( i_refb>0 && !b_direct)
//        ctx += 2;
//
//    while( i_ref > 0 )
//    {
//        T264_cabac_encode_decision( &t->cabac, 54 + ctx, 1 );
//        if( ctx < 4 )
//            ctx = 4;
//        else
//            ctx = 5;
//
//        i_ref--;
//    }
//    T264_cabac_encode_decision( &t->cabac, 54 + ctx, 0 );
//}
//
//
//static __inline  void  T264_cabac_mb_mvd_cpn( T264_t *t, int i_list, int i8, int l, int mvd )
//{
//    const int amvd = abs( t->mb.mvd_ref[i_list][i8 - 1][l] ) +
//                     abs( t->mb.mvd_ref[i_list][i8 - 8][l] );
//    const int i_abs = abs( mvd );
//    const int i_prefix = T264_MIN( i_abs, 9 );
//    const int ctxbase = (l == 0 ? 40 : 47);
//    int ctx;
//    int i;
//
//
//    if( amvd < 3 )
//        ctx = 0;
//    else if( amvd > 32 )
//        ctx = 2;
//    else
//        ctx = 1;
//
//    for( i = 0; i < i_prefix; i++ )
//    {
//        T264_cabac_encode_decision( &t->cabac, ctxbase + ctx, 1 );
//        if( ctx < 3 )
//            ctx = 3;
//        else if( ctx < 6 )
//            ctx++;
//    }
//    if( i_prefix < 9 )
//    {
//        T264_cabac_encode_decision( &t->cabac, ctxbase + ctx, 0 );
//    }
//
//    if( i_prefix >= 9 )
//    {
//        int i_suffix = i_abs - 9;
//        int k = 3;
//
//        while( i_suffix >= (1<<k) )
//        {
//            T264_cabac_encode_bypass( &t->cabac, 1 );
//            i_suffix -= 1 << k;
//            k++;
//        }
//        T264_cabac_encode_bypass( &t->cabac, 0 );
//        while( k-- )
//        {
//            T264_cabac_encode_bypass( &t->cabac, (i_suffix >> k)&0x01 );
//        }
//    }
//
//    /* sign */
//    if( mvd > 0 )
//        T264_cabac_encode_bypass( &t->cabac, 0 );
//    else if( mvd < 0 )
//        T264_cabac_encode_bypass( &t->cabac, 1 );
//}
//
//static __inline  void  T264_cabac_mb_mvd( T264_t *t, int i_list, int idx, int width, int height )
//{
//    T264_vector_t mvp;
//    int mdx, mdy;
//	int i, j;
//	int i8    = T264_scan8[idx];
//	int luma_idx = luma_index[idx];
//    /* Calculate mvd */
//	mvp.refno = t->mb.vec_ref[i8].vec[i_list].refno;
//    T264_predict_mv( t, i_list, luma_idx, width, &mvp );
//	mdx = t->mb.vec_ref[i8].vec[i_list].x - mvp.x;
//	mdy = t->mb.vec_ref[i8].vec[i_list].y - mvp.y;
//    
//    /* encode */
//    T264_cabac_mb_mvd_cpn( t, i_list, i8, 0, mdx );
//    T264_cabac_mb_mvd_cpn( t, i_list, i8, 1, mdy );
//	/* save mvd value */
//	for(j=0; j<height; j++)
//	{
//		for(i=0; i<width; i++)
//		{
//			t->mb.mvd_ref[i_list][i8+i][0] = mdx;
//			t->mb.mvd_ref[i_list][i8+i][1] = mdy;
//			t->mb.mvd[i_list][luma_idx+i][0] = mdx;
//			t->mb.mvd[i_list][luma_idx+i][1] = mdy;
//		}
//		i8 += 8;
//		luma_idx += 4;
//	}
//}
//
//static __inline void T264_cabac_mb8x8_mvd( T264_t *t, int i_list )
//{
//	int i;
//	int sub_part;
//	for( i = 0; i < 4; i++ )
//	{
//		sub_part = t->mb.submb_part[luma_index[i<<2]];
//		if( T264_mb_partition_listX_table[sub_part-B_DIRECT_8x8][i_list] == 0 )
//		{
//			continue;
//		}
//
//		switch( sub_part )
//		{
//		case B_DIRECT_8x8:
//			assert(0);
//			break;
//		case B_L0_8x8:
//		case B_L1_8x8:
//		case B_Bi_8x8:
//			T264_cabac_mb_mvd( t, i_list, 4*i, 2, 2 );
//			break;
//		case B_L0_8x4:
//		case B_L1_8x4:
//		case B_Bi_8x4:
//			T264_cabac_mb_mvd( t, i_list, 4*i+0, 2, 1 );
//			T264_cabac_mb_mvd( t, i_list, 4*i+2, 2, 1 );
//			break;
//		case B_L0_4x8:
//		case B_L1_4x8:
//		case B_Bi_4x8:
//			T264_cabac_mb_mvd( t, i_list, 4*i+0, 1, 2 );
//			T264_cabac_mb_mvd( t, i_list, 4*i+1, 1, 2 );
//			break;
//		case B_L0_4x4:
//		case B_L1_4x4:
//		case B_Bi_4x4:
//			T264_cabac_mb_mvd( t, i_list, 4*i+0, 1, 1 );
//			T264_cabac_mb_mvd( t, i_list, 4*i+1, 1, 1 );
//			T264_cabac_mb_mvd( t, i_list, 4*i+2, 1, 1 );
//			T264_cabac_mb_mvd( t, i_list, 4*i+3, 1, 1 );
//			break;
//		}
//	}
//}
//
//static int T264_cabac_mb_cbf_ctxidxinc( T264_t *t, int i_cat, int i_idx )
//{
//    /* TODO: clean up/optimize */
//	T264_mb_context_t *mb_ctxs = &(t->rec->mb[0]);
//	T264_mb_context_t *mb_ctx;
//    int i_mba_xy = -1;
//    int i_mbb_xy = -1;
//    int i_nza = -1;
//    int i_nzb = -1;
//    int ctx = 0;
//	int cbp;
//
//    if( i_cat == 0 )
//    {
//        if( t->mb.mb_x > 0 )
//        {
//            i_mba_xy = t->mb.mb_xy -1;
//			mb_ctx = &(mb_ctxs[i_mba_xy]);
//            if( mb_ctx->mb_mode == I_16x16 )
//            {
//                i_nza = (mb_ctx->cbp & 0x100);
//            }
//        }
//        if( t->mb.mb_y > 0 )
//        {
//            i_mbb_xy = t->mb.mb_xy - t->mb_stride;
//			mb_ctx = &(mb_ctxs[i_mbb_xy]);
//            if( mb_ctx->mb_mode == I_16x16 )
//            {
//                i_nzb = (mb_ctx->cbp & 0x100);
//            }
//        }
//    }
//    else if( i_cat == 1 || i_cat == 2 )
//    {
//        int x = luma_inverse_x[i_idx];
//        int y = luma_inverse_y[i_idx];
//		int i8 = T264_scan8[i_idx];
//        if( x > 0 )
//            i_mba_xy = t->mb.mb_xy;
//        else if( t->mb.mb_x > 0 )
//            i_mba_xy = t->mb.mb_xy -1;
//
//        if( y > 0 )
//            i_mbb_xy = t->mb.mb_xy;
//        else if( t->mb.mb_y > 0 )
//            i_mbb_xy = t->mb.mb_xy - t->mb_stride;
//
//        /* no need to test for skip/pcm */
//        if( i_mba_xy >= 0 )
//        {
//            const int i8x8a = block_idx_xy[(x-1)&0x03][y]/4;
//            if( (mb_ctxs[i_mba_xy].cbp_y&0x0f)>> i8x8a )
//            {
//                i_nza = t->mb.nnz_ref[i8-1];
//            }
//        }
//        if( i_mbb_xy >= 0 )
//        {
//            const int i8x8b = block_idx_xy[x][(y-1)&0x03]/4;
//            if( (mb_ctxs[i_mbb_xy].cbp_y&0x0f)>> i8x8b )
//            {
//                i_nzb = t->mb.nnz_ref[i8 - 8];
//            }
//        }
//    }
//    else if( i_cat == 3 )
//    {
//        /* no need to test skip/pcm */
//        if( t->mb.mb_x > 0 )
//        {
//            i_mba_xy = t->mb.mb_xy -1;
//			cbp = mb_ctxs[i_mba_xy].cbp;
//            if( cbp&0x30 )
//            {
//                i_nza = cbp&( 0x02 << ( 8 + i_idx) );
//            }
//        }
//        if( t->mb.mb_y > 0 )
//        {
//            i_mbb_xy = t->mb.mb_xy - t->mb_stride;
//			cbp = mb_ctxs[i_mbb_xy].cbp;
//            if( cbp&0x30 )
//            {
//                i_nzb = cbp&( 0x02 << ( 8 + i_idx) );
//            }
//        }
//    }
//    else if( i_cat == 4 )
//    {
//        int idxc = i_idx% 4;
//
//        if( idxc == 1 || idxc == 3 )
//            i_mba_xy = t->mb.mb_xy;
//        else if( t->mb.mb_x > 0 )
//            i_mba_xy = t->mb.mb_xy - 1;
//
//        if( idxc == 2 || idxc == 3 )
//            i_mbb_xy = t->mb.mb_xy;
//        else if( t->mb.mb_y > 0 )
//            i_mbb_xy = t->mb.mb_xy - t->mb_stride;
//
//        /* no need to test skip/pcm */
//        if( i_mba_xy >= 0 && (mb_ctxs[i_mba_xy].cbp&0x30) == 0x20 )
//        {
//            i_nza = t->mb.nnz_ref[T264_scan8[16+i_idx] - 1];
//        }
//        if( i_mbb_xy >= 0 && (mb_ctxs[i_mbb_xy].cbp&0x30) == 0x20 )
//        {
//            i_nzb = t->mb.nnz_ref[T264_scan8[16+i_idx] - 8];
//        }
//    }
//
//    if( ( i_mba_xy < 0  && IS_INTRA( t->mb.mb_mode ) ) || i_nza > 0 )
//    {
//        ctx++;
//    }
//    if( ( i_mbb_xy < 0  && IS_INTRA( t->mb.mb_mode ) ) || i_nzb > 0 )
//    {
//        ctx += 2;
//    }
//
//    return 4 * i_cat + ctx;
//}
//
//
//static void block_residual_write_cabac( T264_t *t, int i_ctxBlockCat, int i_idx, int16_t *l, int i_count )
//{
//    static const int significant_coeff_flag_offset[5] = { 0, 15, 29, 44, 47 };
//    static const int last_significant_coeff_flag_offset[5] = { 0, 15, 29, 44, 47 };
//    static const int coeff_abs_level_m1_offset[5] = { 0, 10, 20, 30, 39 };
//
//    int i_coeff_abs_m1[16];
//    int i_coeff_sign[16];
//    int i_coeff = 0;
//    int i_last  = 0;
//
//    int i_abslevel1 = 0;
//    int i_abslevelgt1 = 0;
//
//    int i;
//
//    /* i_ctxBlockCat: 0-> DC 16x16  i_idx = 0
//     *                1-> AC 16x16  i_idx = luma4x4idx
//     *                2-> Luma4x4   i_idx = luma4x4idx
//     *                3-> DC Chroma i_idx = iCbCr
//     *                4-> AC Chroma i_idx = 4 * iCbCr + chroma4x4idx
//     */
//
//    //fprintf( stderr, "l[] = " );
//    for( i = 0; i < i_count; i++ )
//    {
//        //fprintf( stderr, "%d ", l[i] );
//        if( l[i] != 0 )
//        {
//            i_coeff_abs_m1[i_coeff] = abs( l[i] ) - 1;
//            i_coeff_sign[i_coeff]   = ( l[i] < 0 ? 1 : 0);
//            i_coeff++;
//
//            i_last = i;
//        }
//    }
//    //fprintf( stderr, "\n" );
//
//    if( i_coeff == 0 )
//    {
//        /* codec block flag */
//        T264_cabac_encode_decision( &t->cabac,  85 + T264_cabac_mb_cbf_ctxidxinc( t, i_ctxBlockCat, i_idx ), 0 );
//        return;
//    }
//
//    /* block coded */
//    T264_cabac_encode_decision( &t->cabac,  85 + T264_cabac_mb_cbf_ctxidxinc( t, i_ctxBlockCat, i_idx ), 1 );
//    for( i = 0; i < i_count - 1; i++ )
//    {
//        int i_ctxIdxInc;
//
//        i_ctxIdxInc = T264_MIN( i, i_count - 2 );
//
//        if( l[i] != 0 )
//        {
//            T264_cabac_encode_decision( &t->cabac, 105 + significant_coeff_flag_offset[i_ctxBlockCat] + i_ctxIdxInc, 1 );
//            T264_cabac_encode_decision( &t->cabac, 166 + last_significant_coeff_flag_offset[i_ctxBlockCat] + i_ctxIdxInc, i == i_last ? 1 : 0 );
//        }
//        else
//        {
//            T264_cabac_encode_decision( &t->cabac, 105 + significant_coeff_flag_offset[i_ctxBlockCat] + i_ctxIdxInc, 0 );
//        }
//        if( i == i_last )
//        {
//            break;
//        }
//    }
//
//    for( i = i_coeff - 1; i >= 0; i-- )
//    {
//        int i_prefix;
//        int i_ctxIdxInc;
//
//        /* write coeff_abs - 1 */
//
//        /* prefix */
//        i_prefix = T264_MIN( i_coeff_abs_m1[i], 14 );
//
//        i_ctxIdxInc = (i_abslevelgt1 != 0 ? 0 : T264_MIN( 4, i_abslevel1 + 1 )) + coeff_abs_level_m1_offset[i_ctxBlockCat];
//        if( i_prefix == 0 )
//        {
//            T264_cabac_encode_decision( &t->cabac,  227 + i_ctxIdxInc, 0 );
//        }
//        else
//        {
//            int j;
//            T264_cabac_encode_decision( &t->cabac,  227 + i_ctxIdxInc, 1 );
//            i_ctxIdxInc = 5 + T264_MIN( 4, i_abslevelgt1 ) + coeff_abs_level_m1_offset[i_ctxBlockCat];
//            for( j = 0; j < i_prefix - 1; j++ )
//            {
//                T264_cabac_encode_decision( &t->cabac,  227 + i_ctxIdxInc, 1 );
//            }
//            if( i_prefix < 14 )
//            {
//                T264_cabac_encode_decision( &t->cabac,  227 + i_ctxIdxInc, 0 );
//            }
//        }
//        /* suffix */
//        if( i_coeff_abs_m1[i] >= 14 )
//        {
//            int k = 0;
//            int i_suffix = i_coeff_abs_m1[i] - 14;
//
//            while( i_suffix >= (1<<k) )
//            {
//                T264_cabac_encode_bypass( &t->cabac, 1 );
//                i_suffix -= 1 << k;
//                k++;
//            }
//            T264_cabac_encode_bypass( &t->cabac, 0 );
//            while( k-- )
//            {
//                T264_cabac_encode_bypass( &t->cabac, (i_suffix >> k)&0x01 );
//            }
//        }
//
//        /* write sign */
//        T264_cabac_encode_bypass( &t->cabac, i_coeff_sign[i] );
//
//
//        if( i_coeff_abs_m1[i] == 0 )
//        {
//            i_abslevel1++;
//        }
//        else
//        {
//            i_abslevelgt1++;
//        }
//    }
//}
//
//
//static int8_t
//T264_mb_predict_intra4x4_mode(T264_t *t, int32_t idx)
//{
//	int32_t x, y;
//	int8_t nA, nB, pred_blk;
//
//	x = luma_inverse_x[idx];
//	y = luma_inverse_y[idx];
//
//	nA = t->mb.i4x4_pred_mode_ref[IPM_LUMA + x + y * 8 - 1];
//	nB = t->mb.i4x4_pred_mode_ref[IPM_LUMA + x + y * 8 - 8];
//
//	pred_blk  = T264_MIN(nA, nB);
//
//	if( pred_blk < 0 )
//		return Intra_4x4_DC;
//
//	return pred_blk;	
//}
//
//void T264_macroblock_write_cabac( T264_t *t, bs_t *s )
//{
//    const int i_mb_type = t->mb.mb_mode;
//    const int i_mb_pos_start = BitstreamPos( s );
//    int       i_mb_pos_tex;
//
//    int i;
//
//    /* Write the MB type */
//    T264_cabac_mb_type( t );
//
//    /* PCM special block type UNTESTED */
//	/* no PCM here*/
//	
//    if( IS_INTRA( i_mb_type ) )
//    {
//        /* Prediction */
//        if( i_mb_type == I_4x4 )
//        {
//            for( i = 0; i < 16; i++ )
//            {
//                const int i_pred = T264_mb_predict_intra4x4_mode( t, i );
//                const int i_mode = t->mb.i4x4_pred_mode_ref[T264_scan8[i]];
//                T264_cabac_mb_intra4x4_pred_mode( t, i_pred, i_mode );
//            }
//        }
//        T264_cabac_mb_intra8x8_pred_mode( t );
//    }
//    else if( i_mb_type == P_MODE )
//    {
//        if( t->mb.mb_part == MB_16x16 )
//        {
//            if( t->ps.num_ref_idx_l0_active_minus1 > 0 )
//            {
//                T264_cabac_mb_ref( t, 0, 0 );
//            }
//            T264_cabac_mb_mvd( t, 0, 0, 4, 4 );
//        }
//        else if( t->mb.mb_part == MB_16x8 )
//        {
//            if( t->ps.num_ref_idx_l0_active_minus1 > 0 )
//            {
//                T264_cabac_mb_ref( t, 0, 0 );
//                T264_cabac_mb_ref( t, 0, 8 );
//            }
//            T264_cabac_mb_mvd( t, 0, 0, 4, 2 );
//            T264_cabac_mb_mvd( t, 0, 8, 4, 2 );
//        }
//        else if( t->mb.mb_part == MB_8x16 )
//        {
//            if( t->ps.num_ref_idx_l0_active_minus1 > 0 )
//            {
//                T264_cabac_mb_ref( t, 0, 0 );
//                T264_cabac_mb_ref( t, 0, 4 );
//            }
//            T264_cabac_mb_mvd( t, 0, 0, 2, 4 );
//            T264_cabac_mb_mvd( t, 0, 4, 2, 4 );
//        }
//		else	/* 8x8 */
//		{
//			/* sub mb type */
//			T264_cabac_mb_sub_p_partition( t, t->mb.submb_part[0] );
//			T264_cabac_mb_sub_p_partition( t, t->mb.submb_part[2] );
//			T264_cabac_mb_sub_p_partition( t, t->mb.submb_part[8] );
//			T264_cabac_mb_sub_p_partition( t, t->mb.submb_part[10] );
//
//			/* ref 0 */
//			if( t->ps.num_ref_idx_l0_active_minus1 > 0 )
//			{
//				T264_cabac_mb_ref( t, 0, 0 );
//				T264_cabac_mb_ref( t, 0, 4 );
//				T264_cabac_mb_ref( t, 0, 8 );
//				T264_cabac_mb_ref( t, 0, 12 );
//			}
//
//			for( i = 0; i < 4; i++ )
//			{
//				switch( t->mb.submb_part[luma_index[i<<2]] )
//				{
//				case MB_8x8:
//					T264_cabac_mb_mvd( t, 0, 4*i, 2, 2 );
//					break;
//				case MB_8x4:
//					T264_cabac_mb_mvd( t, 0, 4*i+0, 2, 1 );
//					T264_cabac_mb_mvd( t, 0, 4*i+2, 2, 1 );
//					break;
//				case MB_4x8:
//					T264_cabac_mb_mvd( t, 0, 4*i+0, 1, 2 );
//					T264_cabac_mb_mvd( t, 0, 4*i+1, 1, 2 );
//					break;
//				case MB_4x4:
//					T264_cabac_mb_mvd( t, 0, 4*i+0, 1, 1 );
//					T264_cabac_mb_mvd( t, 0, 4*i+1, 1, 1 );
//					T264_cabac_mb_mvd( t, 0, 4*i+2, 1, 1 );
//					T264_cabac_mb_mvd( t, 0, 4*i+3, 1, 1 );
//					break;
//				}
//			}
//		}
//    }
//    else if( i_mb_type == B_MODE )
//    {
//		if((t->mb.mb_part==MB_16x16&&t->mb.is_copy!=1) || (t->mb.mb_part==MB_16x8) || (t->mb.mb_part==MB_8x16))
//		{
//			/* to be changed here*/
//			/* All B mode */
//			int i_list;
//			int b_list[2][2];
//			const int i_partition = t->mb.mb_part;
//			int b_part_mode, part_mode0, part_mode1;
//			static const int b_part_mode_map[3][3] = {
//				{ B_L0_L0, B_L0_L1, B_L0_BI },
//				{ B_L1_L0, B_L1_L1, B_L1_BI },
//				{ B_BI_L0, B_BI_L1, B_BI_BI }
//			};
//
//			switch(t->mb.mb_part)
//			{
//			case MB_16x16:
//				part_mode0 = t->mb.mb_part2[0] - B_L0_16x16;
//				b_part_mode = b_part_mode_map[part_mode0][part_mode0];
//				break;
//			case MB_16x8:
//				part_mode0 = t->mb.mb_part2[0] - B_L0_16x8;
//				part_mode1 = t->mb.mb_part2[1] - B_L0_16x8;
//				b_part_mode = b_part_mode_map[part_mode0][part_mode1];
//				break;
//			case MB_8x16:
//				part_mode0 = t->mb.mb_part2[0] - B_L0_8x16;
//				part_mode1 = t->mb.mb_part2[1] - B_L0_8x16;
//				b_part_mode = b_part_mode_map[part_mode0][part_mode1];
//				break;
//			}
//			/* init ref list utilisations */
//			for( i = 0; i < 2; i++ )
//			{
//				b_list[0][i] = T264_mb_type_list0_table[b_part_mode][i];
//				b_list[1][i] = T264_mb_type_list1_table[b_part_mode][i];
//			}
//			for( i_list = 0; i_list < 2; i_list++ )
//			{
//				const int i_ref_max = i_list == 0 ? t->ps.num_ref_idx_l0_active_minus1+1 : t->ps.num_ref_idx_l1_active_minus1+1;
//
//				if( i_ref_max > 1 )
//				{
//					if( t->mb.mb_part == MB_16x16 )
//					{
//						if( b_list[i_list][0] ) T264_cabac_mb_ref( t, i_list, 0 );
//					}
//					else if( t->mb.mb_part == MB_16x8 )
//					{
//						if( b_list[i_list][0] ) T264_cabac_mb_ref( t, i_list, 0 );
//						if( b_list[i_list][1] ) T264_cabac_mb_ref( t, i_list, 8 );
//					}
//					else if( t->mb.mb_part == MB_8x16 )
//					{
//						if( b_list[i_list][0] ) T264_cabac_mb_ref( t, i_list, 0 );
//						if( b_list[i_list][1] ) T264_cabac_mb_ref( t, i_list, 4 );
//					}
//				}
//			}
//			for( i_list = 0; i_list < 2; i_list++ )
//			{
//				if( t->mb.mb_part == MB_16x16 )
//				{
//					if( b_list[i_list][0] ) T264_cabac_mb_mvd( t, i_list, 0, 4, 4 );
//				}
//				else if( t->mb.mb_part == MB_16x8 )
//				{
//					if( b_list[i_list][0] ) T264_cabac_mb_mvd( t, i_list, 0, 4, 2 );
//					if( b_list[i_list][1] ) T264_cabac_mb_mvd( t, i_list, 8, 4, 2 );
//				}
//				else if( t->mb.mb_part == MB_8x16 )
//				{
//					if( b_list[i_list][0] ) T264_cabac_mb_mvd( t, i_list, 0, 2, 4 );
//					if( b_list[i_list][1] ) T264_cabac_mb_mvd( t, i_list, 4, 2, 4 );
//				}
//			}
//		}
//		else if(t->mb.mb_part==MB_16x16 && t->mb.is_copy)
//		{
//		}
//		else /* B8x8 */
//		{
//			/* TODO */
//			int i_list;
//			/* sub mb type */
//			T264_cabac_mb_sub_b_partition( t, t->mb.submb_part[0] );
//			T264_cabac_mb_sub_b_partition( t, t->mb.submb_part[2] );
//			T264_cabac_mb_sub_b_partition( t, t->mb.submb_part[8] );
//			T264_cabac_mb_sub_b_partition( t, t->mb.submb_part[10] );
//
//			/* ref */
//			for( i_list = 0; i_list < 2; i_list++ )
//			{
//				if( ( i_list ? t->ps.num_ref_idx_l1_active_minus1 : t->ps.num_ref_idx_l0_active_minus1 ) == 0 )
//					continue;
//				for( i = 0; i < 4; i++ )
//				{
//					int sub_part = t->mb.submb_part[luma_index[i<<2]]-B_DIRECT_8x8;
//					if( T264_mb_partition_listX_table[sub_part][i_list] == 1 )
//					{
//						T264_cabac_mb_ref( t, i_list, 4*i );
//					}
//				}
//			}
//			T264_cabac_mb8x8_mvd( t, 0 );
//			T264_cabac_mb8x8_mvd( t, 1 );
//		}
//    }
//    
//    i_mb_pos_tex = BitstreamPos( s );
//    
//    if( i_mb_type != I_16x16 )
//    {
//        T264_cabac_mb_cbp_luma( t );
//        T264_cabac_mb_cbp_chroma( t );
//    }
//	
//    if( t->mb.cbp_y > 0 || t->mb.cbp_c > 0 || i_mb_type == I_16x16 )
//    {
//        T264_cabac_mb_qp_delta( t );
//
//        /* write residual */
//        if( i_mb_type == I_16x16 )
//        {
//            /* DC Luma */
//            block_residual_write_cabac( t, 0, 0, t->mb.dc4x4_z, 16 );
//
//            if( t->mb.cbp_y != 0 )
//            {
//                /* AC Luma */
//                for( i = 0; i < 16; i++ )
//                {
//                    block_residual_write_cabac( t, 1, i, &(t->mb.dct_y_z[i][1]), 15 );
//                }
//            }
//        }
//        else
//        {
//			if(t->frame_num == 1)
//				t->frame_num = 1;
//            for( i = 0; i < 16; i++ )
//            {
//                if( t->mb.cbp_y & ( 1 << ( i / 4 ) ) )
//                {
//                    block_residual_write_cabac( t, 2, i, &(t->mb.dct_y_z[i][0]), 16 );
//                }
//            }
//        }
//
//        if( t->mb.cbp_c&0x03 )    /* Chroma DC residual present */
//        {
//            block_residual_write_cabac( t, 3, 0, &(t->mb.dc2x2_z[0][0]), 4 );
//            block_residual_write_cabac( t, 3, 1, &(t->mb.dc2x2_z[1][0]), 4 );
//        }
//        if( t->mb.cbp_c&0x02 ) /* Chroma AC residual present */
//        {
//            for( i = 0; i < 8; i++ )
//            {
//                block_residual_write_cabac( t, 4, i, &(t->mb.dct_uv_z[i>>2][i&0x03][1]), 15);
//            }
//        }
//    }
///*
//    if( IS_INTRA( i_mb_type ) )
//        t->stat.frame.i_itex_bits += bs_pos(s) - i_mb_pos_tex;
//    else
//        t->stat.frame.i_ptex_bits += bs_pos(s) - i_mb_pos_tex;
//*/
//}

