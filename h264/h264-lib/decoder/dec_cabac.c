/*****************************************************************************
*
*  T264 AVC CODEC
*
*  Copyright(C) 2004-2005 joylife	<joylife_video@yahoo.com.cn>
*				2004-2005 tricro	<tricro@hotmail.com>
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
* cabac.c: h264 encoder library
*****************************************************************************
* Copyright (C) 2003 Laurent Aimar
*
* Authors: Laurent Aimar <fenrir@via.ecp.fr>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA.
*****************************************************************************/

/*Note: the CABAC routine is currently referenced from x264 temporarily, with adaptation to 
*the data structure of T264. It should be modified further in the near future.
*It's can support B slice, but now only the baseline options is tested
*/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "T264.h"
#include "cabac_engine.h"
#include "inter.h"
#include "utility.h"
#include "inter_b.h"

/* From ffmpeg
*/
#define T264_SCAN8_SIZE (6*8)
#define T264_SCAN8_0 (4+1*8)

static const int T264_scan8[16+2*4] =
{
	/* Luma */
	VEC_LUMA + 0, VEC_LUMA + 1, VEC_LUMA + 1*8 + 0, VEC_LUMA + 1*8 + 1,
		VEC_LUMA + 2, VEC_LUMA + 3, VEC_LUMA + 1*8 + 2, VEC_LUMA + 1*8 + 3,
		VEC_LUMA + 2*8 + 0, VEC_LUMA + 2*8 + 1, VEC_LUMA + 3*8 + 0, VEC_LUMA + 3*8 + 1,
		VEC_LUMA + 2*8 + 2, VEC_LUMA + 2*8 + 3, VEC_LUMA + 3*8 + 2, VEC_LUMA + 3*8 + 3,

		/* Cb */
		NNZ_CHROMA0 + 0, NNZ_CHROMA0 + 1,
		NNZ_CHROMA0 + 1*8 + 0, NNZ_CHROMA0 + 1*8 + 1,

		/* Cr */
		NNZ_CHROMA1 + 0, NNZ_CHROMA1 + 1,
		NNZ_CHROMA1 + 1*8 + 0, NNZ_CHROMA1 + 1*8 + 1,
};
static const uint8_t block_idx_xy[4][4] =
{
	{ 0, 2, 8,  10},
	{ 1, 3, 9,  11},
	{ 4, 6, 12, 14},
	{ 5, 7, 13, 15}
};

#define IS_INTRA(mode) (mode == I_4x4 || mode == I_16x16)
#define IS_SKIP(type)  ( (type) == P_SKIP || (type) == B_SKIP )
enum {
	INTRA_4x4           = 0,
	INTRA_16x16         = 1,
	INTRA_PCM           = 2,

	P_L0            = 3,
	P_8x81          = 4,
	P_SKIP1         = 5,

	B_DIRECT        = 6,
	B_L0_L0         = 7,
	B_L0_L1         = 8,
	B_L0_BI         = 9,
	B_L1_L0         = 10,
	B_L1_L1         = 11,
	B_L1_BI         = 12,
	B_BI_L0         = 13,
	B_BI_L1         = 14,
	B_BI_BI         = 15,
	B_8x81          = 16,
	B_SKIP1         = 17,
};

static const int T264_mb_type_list0_table[18][2] =
{
	{0,0}, {0,0}, {0,0},    /* INTRA */
	{1,1},                  /* P_L0 */
	{0,0},                  /* P_8x8 */
	{1,1},                  /* P_SKIP */
	{0,0},                  /* B_DIRECT */
	{1,1}, {1,0}, {1,1},    /* B_L0_* */
	{0,1}, {0,0}, {0,1},    /* B_L1_* */
	{1,1}, {1,0}, {1,1},    /* B_BI_* */
	{0,0},                  /* B_8x8 */
	{0,0}                   /* B_SKIP */
};
static const int T264_mb_type_list1_table[18][2] =
{
	{0,0}, {0,0}, {0,0},    /* INTRA */
	{0,0},                  /* P_L0 */
	{0,0},                  /* P_8x8 */
	{0,0},                  /* P_SKIP */
	{0,0},                  /* B_DIRECT */
	{0,0}, {0,1}, {0,1},    /* B_L0_* */
	{1,0}, {1,1}, {1,1},    /* B_L1_* */
	{1,0}, {1,1}, {1,1},    /* B_BI_* */
	{0,0},                  /* B_8x8 */
	{0,0}                   /* B_SKIP */
};
static const int T264_mb_partition_listX_table[][2] = 
{
	{0, 0}, //B_DIRECT_8x8 = 100,
	{1, 0}, //B_L0_8x8,
	{0, 1}, //B_L1_8x8,
	{1, 1}, //B_Bi_8x8,
	{1, 0}, //B_L0_8x4,
	{1, 0}, //B_L0_4x8,
	{0, 1}, //B_L1_8x4,
	{0, 1}, //B_L1_4x8,
	{1, 1}, //B_Bi_8x4,
	{1, 1}, //B_Bi_4x8,
	{1, 0}, //B_L0_4x4,
	{0, 1},	//B_L1_4x4,
	{1, 1}	//B_Bi_4x4
};

static const int T264_map_btype_mbpart[] = 
{
	MB_8x8, //B_DIRECT_8x8 = 100,
	MB_8x8, //B_L0_8x8,
	MB_8x8, //B_L1_8x8,
	MB_8x8, //B_Bi_8x8,
	MB_8x4, //B_L0_8x4,
	MB_4x8, //B_L0_4x8,
	MB_8x4, //B_L1_8x4,
	MB_4x8, //B_L1_4x8,
	MB_8x4, //B_Bi_8x4,
	MB_4x8, //B_Bi_4x8,
	MB_4x4, //B_L0_4x4,
	MB_4x4,	//B_L1_4x4,
	MB_4x4	//B_Bi_4x4
};

static void T264_cabac_mb_type( T264_t *t )
{
	int act_sym, mode_sym, sym, sym1, sym2;
	T264_mb_context_t *mb_ctxs = &(t->rec->mb[0]);
	int32_t mb_mode;
	
	if( t->slice_type == SLICE_I )
	{
		int ctx = 0;
		if( t->mb.mb_x > 0 && mb_ctxs[t->mb.mb_xy-1].mb_mode != I_4x4 )
		{
			ctx++;
		}
		if( t->mb.mb_y > 0 && mb_ctxs[t->mb.mb_xy - t->mb_stride].mb_mode != I_4x4 )
		{
			ctx++;
		}

		act_sym = T264_cabac_decode_decision(&t->cabac, 3+ctx);
		if(act_sym == 0)
		{
			mb_mode = I_4x4;
		}
		else
		{
			mode_sym = T264_cabac_decode_terminal(&t->cabac);
			if(mode_sym == 0)	/*I_16x16*/
			{
				mb_mode = I_16x16;
				t->mb.cbp_y = T264_cabac_decode_decision(&t->cabac, 3+3);
				sym = T264_cabac_decode_decision(&t->cabac, 3+4);
				if(sym == 0)
				{
					t->mb.cbp_c = 0;
				}
				else
				{
					sym = T264_cabac_decode_decision(&t->cabac, 3+5);
					t->mb.cbp_c = (sym==0)?1:2;
				}
				sym1 = T264_cabac_decode_decision(&t->cabac, 3+6);
				sym2 = T264_cabac_decode_decision(&t->cabac, 3+7);
				t->mb.mode_i16x16 = (sym1<<1)|sym2;
			}
			else
			{
				//I_PCM
			}
		}
	}
	else if( t->slice_type == SLICE_P )
	{
		/* prefix: 14, suffix: 17 */
		int ctx = 0;
		act_sym = T264_cabac_decode_decision(&t->cabac, 14);
		if(act_sym == 0)
		{
			//P_MODE
			static int mb_part_map[] = {MB_16x16, MB_8x8, MB_8x16, MB_16x8};
			mb_mode = P_MODE;
			sym1 = T264_cabac_decode_decision(&t->cabac, 15);
			ctx = (sym1==0)?16:17;
			sym2 = T264_cabac_decode_decision(&t->cabac, ctx);
			sym = (sym1<<1)|sym2;
			t->mb.mb_part = mb_part_map[sym];

		}
		else
		{
			mode_sym = T264_cabac_decode_decision(&t->cabac, 17);
			if(mode_sym == 0)
			{
				mb_mode = I_4x4;
			}
			else
			{
				sym = T264_cabac_decode_terminal(&t->cabac);
				if(sym == 0)
				{
					//I_16x16
					mb_mode = I_16x16;
					t->mb.cbp_y = T264_cabac_decode_decision(&t->cabac, 17+1);
					sym1 = T264_cabac_decode_decision(&t->cabac, 17+2);
					if(sym1 == 0)
					{
						t->mb.cbp_c = 0;
					}
					else
					{
						sym2 = T264_cabac_decode_decision(&t->cabac, 17+2);
						t->mb.cbp_c = (sym2==0)?1:2;
					}
					sym1 = T264_cabac_decode_decision(&t->cabac, 17+3);
					sym2 = T264_cabac_decode_decision(&t->cabac, 17+3);
					t->mb.mode_i16x16 = (sym1<<1)|sym2;
				}
				else
				{
					//I_PCM
				}
			}
		}
	}
	else if( t->slice_type == SLICE_B )
	{
		int ctx = 0;
		int idx;
		if( t->mb.mb_x > 0 && mb_ctxs[t->mb.mb_xy-1].mb_mode != B_SKIP && !mb_ctxs[t->mb.mb_xy-1].is_copy )
		{
			ctx++;
		}
		if( t->mb.mb_y > 0 && mb_ctxs[t->mb.mb_xy - t->mb_stride].mb_mode != B_SKIP && ! mb_ctxs[t->mb.mb_xy - t->mb_stride].is_copy)
		{
			ctx++;
		}

		sym = T264_cabac_decode_decision(&t->cabac, 27+ctx);
		if(sym == 0)
		{
			t->mb.is_copy = 1;
			t->mb.mb_part = MB_16x16;
			mb_mode = B_MODE;
		}
		else
		{
			t->mb.is_copy = 0;
			sym1 = T264_cabac_decode_decision(&t->cabac, 27+3);
			if(sym1 == 0)
			{
				sym2 = T264_cabac_decode_decision(&t->cabac, 27+5);
				mb_mode = B_MODE;
				t->mb.mb_part = MB_16x16;
				t->mb.mb_part2[0] = (sym2==0)?B_L0_16x16:B_L1_16x16;
			}
			else
			{
				sym2 = T264_cabac_decode_decision(&t->cabac, 27+4);
				if(sym2 == 0)
				{
					static const int mb_bits_2_part0[] = {
						MB_16x16, MB_16x8, MB_8x16, MB_16x8, MB_8x16,
						MB_16x8, MB_8x16, MB_16x8
					};
					static const int mb_bits_2_part20[][2] = {
						{B_Bi_16x16, B_Bi_16x16}, 
						{B_L0_16x8, B_L0_16x8}, {B_L0_8x16, B_L0_8x16},
						{B_L1_16x8, B_L1_16x8}, {B_L1_8x16, B_L1_8x16},
						{B_L0_16x8, B_L1_16x8}, {B_L0_8x16, B_L1_8x16},
						{B_L1_16x8, B_L0_16x8}
					};
					sym = T264_cabac_decode_decision(&t->cabac, 27+5);
					idx = sym<<2;
					sym = T264_cabac_decode_decision(&t->cabac, 27+5);
					idx |= sym<<1;
					sym = T264_cabac_decode_decision(&t->cabac, 27+5);
					idx |= sym;
					mb_mode = B_MODE;
					t->mb.mb_part = mb_bits_2_part0[idx];
					t->mb.mb_part2[0] = mb_bits_2_part20[idx][0];
					t->mb.mb_part2[1] = mb_bits_2_part20[idx][1];
				}
				else
				{
					sym = T264_cabac_decode_decision(&t->cabac, 27+5);
					idx = sym<<2;
					sym = T264_cabac_decode_decision(&t->cabac, 27+5);
					idx |= sym<<1;
					sym = T264_cabac_decode_decision(&t->cabac, 27+5);
					idx |= sym;
					if(idx == 0x07)
					{
						mb_mode = B_MODE;
						t->mb.mb_part = MB_8x8;
					}
					else if(idx == 0x05)
					{
						sym = T264_cabac_decode_decision(&t->cabac, 32);
						if(sym == 0)
						{
							//I_4x4
							mb_mode = I_4x4;
						}
						else
						{
							sym = T264_cabac_decode_terminal(&t->cabac);
							if(sym == 0)
							{
								//I_16x16
								mb_mode = I_16x16;
								t->mb.cbp_y = T264_cabac_decode_decision(&t->cabac, 32+1);
								sym = T264_cabac_decode_decision(&t->cabac, 32+2);
								if(sym == 0)
								{
									t->mb.cbp_c = 0;
								}
								else 
								{
									sym = T264_cabac_decode_decision(&t->cabac, 32+2);
									t->mb.cbp_c = (sym==0)?1:2;
								}
								sym1 = T264_cabac_decode_decision(&t->cabac, 32+3);
								sym2 = T264_cabac_decode_decision(&t->cabac, 32+3);
								t->mb.mode_i16x16 = (sym1<<1)|sym2;
							}
							else
							{
								//I_PCM
							}
						}
					}
					else if(idx == 0x06)
					{
						mb_mode = B_MODE;
						t->mb.mb_part = MB_8x16;
						t->mb.mb_part2[0] = B_L1_8x16;
						t->mb.mb_part2[1] = B_L0_8x16;
					}
					else
					{
						static const int i_mb_bits[21][7] =
						{
							{ 1, 0, 0, },            { 1, 1, 0, 0, 0, 1, },    { 1, 1, 0, 0, 1, 0, },   /* L0 L0 */
							{ 1, 0, 1, },            { 1, 1, 0, 0, 1, 1, },    { 1, 1, 0, 1, 0, 0, },   /* L1 L1 */
							{ 1, 1, 0, 0, 0, 0 ,},   { 1, 1, 1, 1, 0, 0 , 0 }, { 1, 1, 1, 1, 0, 0 , 1 },/* BI BI */

							{ 1, 1, 0, 1, 0, 1, },   { 1, 1, 0, 1, 1, 0, },     /* L0 L1 */
							{ 1, 1, 0, 1, 1, 1, },   { 1, 1, 1, 1, 1, 0, },     /* L1 L0 */
							{ 1, 1, 1, 0, 0, 0, 0 }, { 1, 1, 1, 0, 0, 0, 1 },   /* L0 BI */
							{ 1, 1, 1, 0, 0, 1, 0 }, { 1, 1, 1, 0, 0, 1, 1 },   /* L1 BI */
							{ 1, 1, 1, 0, 1, 0, 0 }, { 1, 1, 1, 0, 1, 0, 1 },   /* BI L0 */
							{ 1, 1, 1, 0, 1, 1, 0 }, { 1, 1, 1, 0, 1, 1, 1 }    /* BI L1 */
						};
						
						static const int mb_bits_2_part1[] = {
							MB_16x8, MB_8x16, MB_16x8, MB_8x16, 
							MB_16x8, MB_8x16, MB_16x8, MB_8x16,
							MB_16x8, MB_8x16, MB_16x8, MB_8x16,
							MB_16x8, MB_8x16, MB_16x8, MB_8x16
						};
						static const int mb_bits_2_part21[][2] = {
							{B_L0_16x8, B_Bi_16x8}, {B_L0_8x16, B_Bi_8x16},
							{B_L1_16x8, B_Bi_16x8}, {B_L1_8x16, B_Bi_8x16},
							{B_Bi_16x8, B_L0_16x8}, {B_Bi_8x16, B_L0_8x16},
							{B_Bi_16x8, B_L1_16x8}, {B_Bi_8x16, B_L1_8x16},
							{B_Bi_16x8, B_Bi_16x8}, {B_Bi_8x16, B_Bi_8x16},
							{B_L0_16x8, B_L0_16x8}, {B_L0_8x16, B_L0_8x16},
							{B_L0_16x8, B_L0_16x8}, {B_L0_8x16, B_L0_8x16},
							{B_L0_16x8, B_L0_16x8}, {B_L0_8x16, B_L0_8x16}
						};
						idx <<= 1;
						sym = T264_cabac_decode_decision(&t->cabac, 27+5);
						idx |= sym;
						mb_mode = B_MODE;
						t->mb.mb_part = mb_bits_2_part1[idx];
						t->mb.mb_part2[0] = mb_bits_2_part21[idx][0];
						t->mb.mb_part2[1] = mb_bits_2_part21[idx][1];
					}
				}
			}
		}	
	}
	else
	{
		//dummy here
		mb_mode = t->mb.mb_mode;
	}
	t->mb.mb_mode = mb_mode;
}

static int T264_cabac_mb_intra4x4_pred_mode( T264_t *t, int i_pred)
{
	int i_mode, sym;
	sym = T264_cabac_decode_decision(&t->cabac, 68);
	if(sym == 1)
	{
		i_mode = i_pred;
	}
	else
	{
		i_mode = T264_cabac_decode_decision(&t->cabac, 69);
		sym = T264_cabac_decode_decision(&t->cabac, 69);
		i_mode |= (sym<<1);
		sym = T264_cabac_decode_decision(&t->cabac, 69);
		i_mode |= (sym<<2);
		if(i_mode >= i_pred)
			i_mode ++;
	}
	return i_mode;
}

static void T264_cabac_mb_intra8x8_pred_mode( T264_t *t )
{
	int i_mode, sym;
	T264_mb_context_t *mb_ctxs = &(t->rec->mb[0]);

	int ctx = 0;
	if( t->mb.mb_x > 0 && mb_ctxs[t->mb.mb_xy-1].mb_mode_uv != Intra_8x8_DC)
	{
		ctx++;
	}
	if( t->mb.mb_y > 0 && mb_ctxs[t->mb.mb_xy - t->mb_stride].mb_mode_uv != Intra_8x8_DC )
	{
		ctx++;
	}

	sym = T264_cabac_decode_decision(&t->cabac, 64+ctx);
	if(sym == 0)
	{
		i_mode = Intra_8x8_DC;
	}
	else
	{
		sym = T264_cabac_decode_decision(&t->cabac, 64+3);
		if(sym == 0)
		{
			i_mode = 1;
		}
		else
		{
			sym = T264_cabac_decode_decision(&t->cabac, 64+3);
			i_mode = (sym==0)?2:3;
		}
	}
	t->mb.mb_mode_uv = i_mode;
}

static void T264_cabac_mb_cbp_luma( T264_t *t )
{
	/* TODO: clean up and optimize */
	T264_mb_context_t *mb_ctxs = &(t->rec->mb[0]);
	int i8x8, sym, cbp_y, cbp_ya, cbp_yb;
	cbp_y = 0;
	for( i8x8 = 0; i8x8 < 4; i8x8++ )
	{
		int i_mba_xy = -1;
		int i_mbb_xy = -1;
		int x = luma_inverse_x[4*i8x8];
		int y = luma_inverse_y[4*i8x8];
		int ctx = 0;

		if( x > 0 )
		{
			i_mba_xy = t->mb.mb_xy;
			cbp_ya = cbp_y;
		}
		else if( t->mb.mb_x > 0 )
		{
			i_mba_xy = t->mb.mb_xy - 1;
			cbp_ya = mb_ctxs[i_mba_xy].cbp_y;
		}

		if( y > 0 )
		{
			i_mbb_xy = t->mb.mb_xy;
			cbp_yb = cbp_y;
		}
		else if( t->mb.mb_y > 0 )
		{
			i_mbb_xy = t->mb.mb_xy - t->mb_stride;
			cbp_yb = mb_ctxs[i_mbb_xy].cbp_y;
		}

		/* No need to test for PCM and SKIP */
		if( i_mba_xy >= 0 )
		{
			const int i8x8a = block_idx_xy[(x-1)&0x03][y]/4;
			if( ((cbp_ya >> i8x8a)&0x01) == 0 )
			{
				ctx++;
			}
		}

		if( i_mbb_xy >= 0 )
		{
			const int i8x8b = block_idx_xy[x][(y-1)&0x03]/4;
			if( ((cbp_yb >> i8x8b)&0x01) == 0 )
			{
				ctx += 2;
			}
		}
		sym = T264_cabac_decode_decision(&t->cabac, 73+ctx);
		cbp_y |= (sym<<i8x8);
	}
	t->mb.cbp_y = cbp_y;
}

static void T264_cabac_mb_cbp_chroma( T264_t *t )
{
	int cbp_a = -1;
	int cbp_b = -1;
	int ctx, sym;
	T264_mb_context_t *mb_ctxs = &(t->rec->mb[0]);
	/* No need to test for SKIP/PCM */
	if( t->mb.mb_x > 0 )
	{
		cbp_a = (mb_ctxs[t->mb.mb_xy - 1].cbp_c)&0x3;
	}

	if( t->mb.mb_y > 0 )
	{
		cbp_b = (mb_ctxs[t->mb.mb_xy - t->mb_stride].cbp_c)&0x3;
	}

	ctx = 0;
	if( cbp_a > 0 ) ctx++;
	if( cbp_b > 0 ) ctx += 2;
	sym = T264_cabac_decode_decision(&t->cabac, 77+ctx);
	if(sym == 0)
	{
		t->mb.cbp_c = 0;
	}
	else
	{
		ctx = 4;
		if( cbp_a == 2 ) ctx++;
		if( cbp_b == 2 ) ctx += 2;
		sym = T264_cabac_decode_decision(&t->cabac, 77+ctx);
		t->mb.cbp_c = (sym==0)?1:2;
	}
}

/* TODO check it with != qp per mb */
static void T264_cabac_mb_qp_delta( T264_t *t )
{
	int i_mbn_xy = t->mb.mb_xy - 1;
	int i_dqp = t->mb.mb_qp_delta;
	int val = i_dqp <= 0 ? (-2*i_dqp) : (2*i_dqp - 1);
	int ctx;
	T264_mb_context_t *mb_ctxs = &(t->rec->mb[0]);

	/* No need to test for PCM / SKIP */
	if( i_mbn_xy >= 0 && mb_ctxs[i_mbn_xy].mb_qp_delta != 0 &&
		( mb_ctxs[i_mbn_xy].mb_mode == I_16x16 || mb_ctxs[i_mbn_xy].cbp_y || mb_ctxs[i_mbn_xy].cbp_c) )
		ctx = 1;
	else
		ctx = 0;

	val = 0;
	while(T264_cabac_decode_decision(&t->cabac, 60+ctx) != 0)
	{
		val ++;
		if(ctx < 2)
			ctx = 2;
		else
			ctx = 3;
	}
	t->mb.mb_qp_delta = ((val&0x01)==0)?(-(val>>1)):((val+1)>>1);
}

int T264_cabac_dec_mb_skip( T264_t *t)
{
	int b_skip;
	T264_mb_context_t *mb_ctxs = &(t->rec->mb[0]);
	int ctx = 0;

	if( t->mb.mb_x > 0 && !IS_SKIP( mb_ctxs[t->mb.mb_xy -1].mb_mode) )
	{
		ctx++;
	}
	if( t->mb.mb_y > 0 && !IS_SKIP( mb_ctxs[t->mb.mb_xy - t->mb_stride].mb_mode) )
	{
		ctx++;
	}

	if( t->slice_type == SLICE_P )
		b_skip = T264_cabac_decode_decision(&t->cabac, 11+ctx);
	else /* SLICE_TYPE_B */
		b_skip = T264_cabac_decode_decision(&t->cabac, 24+ctx);
	return b_skip;
}

static __inline  int T264_cabac_mb_sub_p_partition( T264_t *t)
{
	int i_sub, sym;
	sym = T264_cabac_decode_decision(&t->cabac, 21);
	if(sym == 1)
	{
		i_sub = MB_8x8;
	}
	else
	{
		sym = T264_cabac_decode_decision(&t->cabac, 22);
		if(sym == 0)
		{
			i_sub = MB_8x4;
		}
		else
		{
			sym = T264_cabac_decode_decision(&t->cabac, 23);
			i_sub = (sym==0)?MB_4x4:MB_4x8;
		}
	}
	return i_sub;
}

static __inline  int T264_cabac_mb_sub_b_partition( T264_t *t)
{
	int i_sub, sym, sym1;
	sym = T264_cabac_decode_decision(&t->cabac, 36);
	if(sym == 0)
	{
		i_sub = B_DIRECT_8x8;
	}
	else
	{
		sym = T264_cabac_decode_decision(&t->cabac, 37);
		if(sym == 0)
		{
			sym1 = T264_cabac_decode_decision(&t->cabac, 39);
			i_sub = (sym1==0)?B_L0_8x8:B_L1_8x8;
		}
		else
		{
			int idx;
			sym = T264_cabac_decode_decision(&t->cabac, 38);
			if(sym == 0)
			{
				static const int idx_2_sub0[] = {B_Bi_8x8, B_L0_8x4, B_L0_4x8, B_L1_8x4};
				sym1 = T264_cabac_decode_decision(&t->cabac, 39);
				idx = sym1<<1;
				idx |= T264_cabac_decode_decision(&t->cabac, 39);
				i_sub = idx_2_sub0[idx];
			}
			else
			{
				sym = T264_cabac_decode_decision(&t->cabac, 39);
				if(sym == 0)
				{
					static const int idx_2_sub1[] = {B_L1_4x8, B_Bi_8x4, B_Bi_4x8, B_L0_4x4};
					sym1 = T264_cabac_decode_decision(&t->cabac, 39);
					idx = sym1<<1;
					idx |= T264_cabac_decode_decision(&t->cabac, 39);
					i_sub = idx_2_sub1[idx];
				}
				else
				{
					sym1 = T264_cabac_decode_decision(&t->cabac, 39);
					i_sub = (sym1==0)?B_L1_4x4:B_Bi_4x4;
				}
			}
		}
	}
	return i_sub;
}


static __inline  void T264_cabac_mb_ref( T264_t *t, int i_list, int idx, int width, int height, int i_ref_max )
{
	int i8    = T264_scan8[idx];
	
	int i_ref, i, j;
	int luma_idx = luma_index[idx];
	if( i_ref_max <= 1 )
	{
		i_ref = 0;
	}
	else
	{
		T264_mb_context_t *mb_ctxs = &(t->rec->mb[0]);
		const int i_refa = t->mb.vec_ref[i8 - 1].vec[i_list].refno;
		const int i_refb = t->mb.vec_ref[i8 - 8].vec[i_list].refno;
		int a_direct, b_direct;
		int ctx  = 0;
		if( t->slice_type==SLICE_B && t->mb.mb_x > 0 && (mb_ctxs[t->mb.mb_xy-1].mb_mode == B_SKIP||mb_ctxs[t->mb.mb_xy-1].is_copy ) && (luma_idx&0x03)==0)
		{
			a_direct = 1;
		}
		else
			a_direct = 0;
		if( t->slice_type==SLICE_B && t->mb.mb_y > 0 && (mb_ctxs[t->mb.mb_xy - t->mb_stride].mb_mode == B_SKIP||mb_ctxs[t->mb.mb_xy - t->mb_stride].is_copy) && luma_idx<4)
		{
			b_direct = 1;
		}
		else
			b_direct = 0;

		if( i_refa>0 && !a_direct)
			ctx++;
		if( i_refb>0 && !b_direct)
			ctx += 2;
		i_ref = 0;
		while(T264_cabac_decode_decision(&t->cabac, 54+ctx) != 0)
		{
			i_ref ++;
			if(ctx < 4)
				ctx = 4;
			else
				ctx = 5;
		}
	}
	/* save ref value */
	for(j=0; j<height; j++)
	{
		for(i=0; i<width; i++)
		{
			t->mb.vec_ref[i8+i].vec[i_list].refno = i_ref;
			t->mb.vec[i_list][luma_idx+i].refno = i_ref;
		}
		i8 += 8;
		luma_idx += 4;
	}
}


static __inline  int  T264_cabac_mb_mvd_cpn( T264_t *t, int i_list, int i8, int l)
{
	int i_abs, i_prefix, i_suffix;
	const int amvd = abs( t->mb.mvd_ref[i_list][i8 - 1][l] ) +
		abs( t->mb.mvd_ref[i_list][i8 - 8][l] );
	const int ctxbase = (l == 0 ? 40 : 47);
	
	int ctx;

	if( amvd < 3 )
		ctx = 0;
	else if( amvd > 32 )
		ctx = 2;
	else
		ctx = 1;

	i_prefix = 0;
	while(i_prefix<9 && T264_cabac_decode_decision(&t->cabac, ctxbase+ctx)!=0)
	{
		i_prefix ++;
		if(ctx < 3)
			ctx = 3;
		else if(ctx < 6)
			ctx ++;
	}
	if(i_prefix >= 9)
	{
		int k = 3;
		i_suffix = 0;
		while(T264_cabac_decode_bypass(&t->cabac) != 0)
		{
			i_suffix += 1<<k;
			k++;
		}
		while(k--)
		{
			i_suffix += T264_cabac_decode_bypass(&t->cabac)<<k;
		}
		i_abs = 9 + i_suffix; 
	}
	else
		i_abs = i_prefix;
	
	/* sign */
	if(i_abs != 0)
	{
		if(T264_cabac_decode_bypass(&t->cabac) != 0)
			i_abs = -i_abs;
	}
	return i_abs;
}

static __inline  void  T264_cabac_mb_mvd( T264_t *t, int i_list, int idx, int width, int height )
{
	T264_vector_t mvp;
	int mdx, mdy, mvx, mvy;
	int i, j;
	int i8    = T264_scan8[idx];
	int luma_idx = luma_index[idx];
	/* Calculate mvd */

	mvp.refno = t->mb.vec_ref[i8].vec[i_list].refno;
	T264_predict_mv( t, i_list, luma_idx, width, &mvp );
	/* decode */
	mdx = T264_cabac_mb_mvd_cpn( t, i_list, i8, 0);
	mdy = T264_cabac_mb_mvd_cpn( t, i_list, i8, 1);
	/* save mvd value */
	mvx = mdx + mvp.x;
	mvy = mdy + mvp.y;
	for(j=0; j<height; j++)
	{
		for(i=0; i<width; i++)
		{
			t->mb.mvd_ref[i_list][i8+i][0] = mdx;
			t->mb.mvd_ref[i_list][i8+i][1] = mdy;
			t->mb.mvd[i_list][luma_idx+i][0] = mdx;
			t->mb.mvd[i_list][luma_idx+i][1] = mdy;
			t->mb.vec_ref[i8+i].vec[i_list].x = mvx;
			t->mb.vec_ref[i8+i].vec[i_list].y = mvy;
			t->mb.vec[i_list][luma_idx+i].x = mvx;
			t->mb.vec[i_list][luma_idx+i].y = mvy;			
		}
		i8 += 8;
		luma_idx += 4;
	}
}
static __inline void T264_cabac_mb8x8_mvd( T264_t *t, int i_list )
{
	int i;
	int sub_part, luma_idx;
	for( i = 0; i < 4; i++ )
	{
		luma_idx = luma_index[i<<2];
		sub_part = t->mb.submb_part[luma_idx];
		if( T264_mb_partition_listX_table[sub_part-B_DIRECT_8x8][i_list] == 0 )
		{
			continue;
		}

		switch( sub_part )
		{
		case B_L0_8x8:
		case B_L1_8x8:
		case B_Bi_8x8:
			T264_cabac_mb_mvd( t, i_list, 4*i, 2, 2 );
			break;
		case B_L0_8x4:
		case B_L1_8x4:
		case B_Bi_8x4:
			T264_cabac_mb_mvd( t, i_list, 4*i+0, 2, 1 );
			T264_cabac_mb_mvd( t, i_list, 4*i+2, 2, 1 );
			break;
		case B_L0_4x8:
		case B_L1_4x8:
		case B_Bi_4x8:
			T264_cabac_mb_mvd( t, i_list, 4*i+0, 1, 2 );
			T264_cabac_mb_mvd( t, i_list, 4*i+1, 1, 2 );
			break;
		case B_L0_4x4:
		case B_L1_4x4:
		case B_Bi_4x4:
			T264_cabac_mb_mvd( t, i_list, 4*i+0, 1, 1 );
			T264_cabac_mb_mvd( t, i_list, 4*i+1, 1, 1 );
			T264_cabac_mb_mvd( t, i_list, 4*i+2, 1, 1 );
			T264_cabac_mb_mvd( t, i_list, 4*i+3, 1, 1 );
			break;
		}
	}
}
static int T264_cabac_mb_cbf_ctxidxinc( T264_t *t, int i_cat, int i_idx )
{
	/* TODO: clean up/optimize */
	T264_mb_context_t *mb_ctxs = &(t->rec->mb[0]);
	T264_mb_context_t *mb_ctx;
	int i_mba_xy = -1;
	int i_mbb_xy = -1;
	int i_nza = -1;
	int i_nzb = -1;
	int ctx = 0;
	int cbp;

	if( i_cat == 0 )
	{
		if( t->mb.mb_x > 0 )
		{
			i_mba_xy = t->mb.mb_xy -1;
			mb_ctx = &(mb_ctxs[i_mba_xy]);
			if( mb_ctx->mb_mode == I_16x16 )
			{
				i_nza = (mb_ctx->cbp & 0x100);
			}
		}
		if( t->mb.mb_y > 0 )
		{
			i_mbb_xy = t->mb.mb_xy - t->mb_stride;
			mb_ctx = &(mb_ctxs[i_mbb_xy]);
			if( mb_ctx->mb_mode == I_16x16 )
			{
				i_nzb = (mb_ctx->cbp & 0x100);
			}
		}
	}
	else if( i_cat == 1 || i_cat == 2 )
	{
		int x = luma_inverse_x[i_idx];
		int y = luma_inverse_y[i_idx];
		int i8 = T264_scan8[i_idx];
		int cbp_ya, cbp_yb;
		if( x > 0 )
		{
			i_mba_xy = t->mb.mb_xy;
			cbp_ya = t->mb.cbp_y;
		}
		else if( t->mb.mb_x > 0 )
		{
			i_mba_xy = t->mb.mb_xy -1;
			cbp_ya = mb_ctxs[i_mba_xy].cbp_y;
		}

		if( y > 0 )
		{
			i_mbb_xy = t->mb.mb_xy;
			cbp_yb = t->mb.cbp_y;
		}
		else if( t->mb.mb_y > 0 )
		{
			i_mbb_xy = t->mb.mb_xy - t->mb_stride;
			cbp_yb = mb_ctxs[i_mbb_xy].cbp_y;
		}

		/* no need to test for skip/pcm */
		if( i_mba_xy >= 0 )
		{
			const int i8x8a = block_idx_xy[(x-1)&0x03][y]/4;
			if( (cbp_ya&0x0f)>> i8x8a )
			{
				i_nza = t->mb.nnz_ref[i8-1];
			}
		}
		if( i_mbb_xy >= 0 )
		{
			const int i8x8b = block_idx_xy[x][(y-1)&0x03]/4;
			if( (cbp_yb&0x0f)>> i8x8b )
			{
				i_nzb = t->mb.nnz_ref[i8 - 8];
			}
		}
	}
	else if( i_cat == 3 )
	{
		/* no need to test skip/pcm */
		//only test other MB's cbp, so we do not care about it
		if( t->mb.mb_x > 0 )
		{
			i_mba_xy = t->mb.mb_xy -1;
			cbp = mb_ctxs[i_mba_xy].cbp;
			if( cbp&0x30 )
			{
				i_nza = cbp&( 0x02 << ( 8 + i_idx) );
			}
		}
		if( t->mb.mb_y > 0 )
		{
			i_mbb_xy = t->mb.mb_xy - t->mb_stride;
			cbp = mb_ctxs[i_mbb_xy].cbp;
			if( cbp&0x30 )
			{
				i_nzb = cbp&( 0x02 << ( 8 + i_idx) );
			}
		}
	}
	else if( i_cat == 4 )
	{
		int cbp_ca, cbp_cb;
		int idxc = i_idx% 4;

		if( idxc == 1 || idxc == 3 )
		{
			cbp_ca = t->mb.cbp_c;
			i_mba_xy = t->mb.mb_xy;
		}
		else if( t->mb.mb_x > 0 )
		{
			i_mba_xy = t->mb.mb_xy - 1;
			cbp_ca = mb_ctxs[i_mba_xy].cbp_c;
		}

		if( idxc == 2 || idxc == 3 )
		{
			i_mbb_xy = t->mb.mb_xy;
			cbp_cb = t->mb.cbp_c;
		}
		else if( t->mb.mb_y > 0 )
		{
			i_mbb_xy = t->mb.mb_xy - t->mb_stride;
			cbp_cb = mb_ctxs[i_mbb_xy].cbp_c;
		}
		/* no need to test skip/pcm */
		if( i_mba_xy >= 0 && (cbp_ca&0x03) == 0x02 )
		{
			i_nza = t->mb.nnz_ref[T264_scan8[16+i_idx] - 1];
		}
		if( i_mbb_xy >= 0 && (cbp_cb&0x03) == 0x02 )
		{
			i_nzb = t->mb.nnz_ref[T264_scan8[16+i_idx] - 8];
		}
	}

	if( ( i_mba_xy < 0  && IS_INTRA( t->mb.mb_mode ) ) || i_nza > 0 )
	{
		ctx++;
	}
	if( ( i_mbb_xy < 0  && IS_INTRA( t->mb.mb_mode ) ) || i_nzb > 0 )
	{
		ctx += 2;
	}

	return 4 * i_cat + ctx;
}


static void block_residual_read_cabac( T264_t *t, int i_ctxBlockCat, int i_idx, int16_t *l, int i_count )
{
	static const int significant_coeff_flag_offset[5] = { 0, 15, 29, 44, 47 };
	static const int last_significant_coeff_flag_offset[5] = { 0, 15, 29, 44, 47 };
	static const int coeff_abs_level_m1_offset[5] = { 0, 10, 20, 30, 39 };

	int i_coeff_sig_map[16];
	int i_coeff = 0;
	int i_last  = 0;

	int i_abslevel1 = 0;
	int i_abslevelgt1 = 0;

	int i, i1, sym, i_abs, x, y;

	/* i_ctxBlockCat: 0-> DC 16x16  i_idx = 0
	*                1-> AC 16x16  i_idx = luma4x4idx
	*                2-> Luma4x4   i_idx = luma4x4idx
	*                3-> DC Chroma i_idx = iCbCr
	*                4-> AC Chroma i_idx = 4 * iCbCr + chroma4x4idx
	*/

	memset(l, 0, sizeof(int16_t)*i_count);
	sym = T264_cabac_decode_decision(&t->cabac, 85 + T264_cabac_mb_cbf_ctxidxinc( t, i_ctxBlockCat, i_idx ));
	if(sym == 0)
	{
		//the block is not coded
		return;
	}
	for(i=0; i<i_count-1; i++)
	{
		int i_ctxIdxInc;
		i_ctxIdxInc = T264_MIN(i, i_count-2);
		sym = T264_cabac_decode_decision(&t->cabac, 105+significant_coeff_flag_offset[i_ctxBlockCat] + i_ctxIdxInc);
		if(sym != 0)
		{
			i_coeff_sig_map[i] = 1;
			i_coeff ++;
			//--- read last coefficient symbol ---
			if(T264_cabac_decode_decision(&t->cabac, 166 + last_significant_coeff_flag_offset[i_ctxBlockCat] + i_ctxIdxInc))
			{
				for(i++; i<i_count; i++)
					i_coeff_sig_map[i] = 0;
			}
		}
		else
		{
			i_coeff_sig_map[i] = 0;
		}
	}
	//--- last coefficient must be significant if no last symbol was received ---
	if (i<i_count)
	{
		i_coeff_sig_map[i] = 1;
		i_coeff ++;
	}
	i1 = i_count - 1;
	for( i = i_coeff - 1; i >= 0; i-- )
	{
		int i_prefix;
		int i_ctxIdxInc;

		i_ctxIdxInc = (i_abslevelgt1 != 0 ? 0 : T264_MIN( 4, i_abslevel1 + 1 )) + coeff_abs_level_m1_offset[i_ctxBlockCat];
		sym = T264_cabac_decode_decision(&t->cabac, 227+i_ctxIdxInc);
		if(sym == 0)
		{
			i_abs = 0;
		}
		else
		{
			i_prefix = 1;
			i_ctxIdxInc = 5 + T264_MIN( 4, i_abslevelgt1 ) + coeff_abs_level_m1_offset[i_ctxBlockCat];
			while(i_prefix<14 && T264_cabac_decode_decision(&t->cabac, 227+i_ctxIdxInc)!=0)
			{
				i_prefix ++;
			}
			/* suffix */
			if(i_prefix >= 14)
			{
				int k = 0;
				int i_suffix = 0;
				while(T264_cabac_decode_bypass(&t->cabac) != 0)
				{
					i_suffix += 1<<k;
					k++;
				}
				while(k--)
				{
					i_suffix += T264_cabac_decode_bypass(&t->cabac)<<k;
				}
				i_abs = i_prefix + i_suffix;
			}
			else
			{
				i_abs = i_prefix;
			}
		}
		/* read the sign */
		sym = T264_cabac_decode_bypass(&t->cabac);
		while(i_coeff_sig_map[i1]==0)
		{
			i1 --;
		}
		l[i1] = (sym==0)?(i_abs+1):(-(i_abs+1));
		i1 --;
		if(i_abs == 0)
		{
			i_abslevel1 ++;
		}
		else
		{
			i_abslevelgt1 ++;
		}
	}
	
	if (i_ctxBlockCat==1 || i_ctxBlockCat==2)
	{
		x = luma_inverse_x[i_idx];
		y = luma_inverse_y[i_idx];
		t->mb.nnz[luma_index[i_idx]] = i_coeff;
		t->mb.nnz_ref[NNZ_LUMA + y * 8 + x] = i_coeff;
	}
	else if (i_ctxBlockCat==4 && i_idx<4)
	{
		int idx = i_idx + 16;
		t->mb.nnz[idx] = i_coeff;
		x = (idx - 16) % 2;
		y = (idx - 16) / 2;
		t->mb.nnz_ref[NNZ_CHROMA0 + y * 8 + x] = i_coeff;
	}
	else if (i_ctxBlockCat==4 && i_idx<8)
	{
		int idx = i_idx + 16;
		t->mb.nnz[idx] = i_coeff;
		x = (idx - 20) % 2;
		y = (idx - 20) / 2;
		t->mb.nnz_ref[NNZ_CHROMA1 + y * 8 + x] = i_coeff;
	}
}


static int8_t
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
static void __inline mb_get_directMB16x16_mv_cabac(T264_t* t)
{
	int				i, j, i8, k;

	//to be revised. This has a problem
	T264_get_direct_mv(t, t->mb.vec);
	for(k=0; k<16; k++)
	{
		i8 = luma_index[k];
		i = luma_inverse_x[k];
		j = luma_inverse_y[k];
		t->mb.vec_ref[VEC_LUMA+(j<<3)+i].vec[0] = t->mb.vec[0][k];
		t->mb.vec_ref[VEC_LUMA+(j<<3)+i].vec[1] = t->mb.vec[1][k];
	}
	i8 = VEC_LUMA;
	for(j=0; j<4; j++)
	{
		for(i=0; i<4; i++)
		{
			t->mb.mvd_ref[0][i8+i][0] = 0;
			t->mb.mvd_ref[0][i8+i][1] = 0;
			t->mb.mvd_ref[1][i8+i][0] = 0;
			t->mb.mvd_ref[1][i8+i][1] = 0;
		}
		i8 += 8;
	}
	memset(&(t->mb.mvd[0][0][0]), 0, sizeof(t->mb.mvd));
}
void T264_macroblock_read_cabac( T264_t *t, bs_t *s )
{
	int i_mb_type;
	const int i_mb_pos_start = BitstreamPos( s );
	int       i_mb_pos_tex;

	int i, j, cbp_dc;

	/* Write the MB type */
	T264_cabac_mb_type( t );

	/* PCM special block type UNTESTED */
	/* no PCM here*/
	i_mb_type = t->mb.mb_mode;
	if( IS_INTRA( i_mb_type ) )
	{
		/* Prediction */
		if( i_mb_type == I_4x4 )
		{
			for( i = 0; i < 16; i++ )
			{
				const int i_pred = T264_mb_predict_intra4x4_mode( t, i );
				const int i_mode = T264_cabac_mb_intra4x4_pred_mode( t, i_pred);
				t->mb.i4x4_pred_mode_ref[T264_scan8[i]] = i_mode;
				t->mb.mode_i4x4[i] = i_mode;
			}
		}
		T264_cabac_mb_intra8x8_pred_mode( t );
		/* save ref */
		memset(t->mb.submb_part, -1, sizeof(t->mb.submb_part));
		t->mb.mb_part = -1;
#define INITINVALIDVEC(vec) vec.refno = -1; vec.x = vec.y = 0;
		for(i = 0 ; i < 2 ; i ++)
		{
			for(j = 0 ; j < 16 ; j ++)
			{
				INITINVALIDVEC(t->mb.vec[i][j]);
			}
		}
#undef INITINVALIDVEC
	}
	else if( i_mb_type == P_MODE )
	{
		int i_ref_max = t->refl0_num;
		if( t->mb.mb_part == MB_16x16 )
		{
			T264_cabac_mb_ref( t, 0, 0, 4, 4, i_ref_max);
			T264_cabac_mb_mvd( t, 0, 0, 4, 4 );
		}
		else if( t->mb.mb_part == MB_16x8 )
		{
			T264_cabac_mb_ref( t, 0, 0, 4, 2, i_ref_max );
			T264_cabac_mb_ref( t, 0, 8, 4, 2, i_ref_max );
			
			T264_cabac_mb_mvd( t, 0, 0, 4, 2 );
			T264_cabac_mb_mvd( t, 0, 8, 4, 2 );
		}
		else if( t->mb.mb_part == MB_8x16 )
		{
			T264_cabac_mb_ref( t, 0, 0, 2, 4, i_ref_max );
			T264_cabac_mb_ref( t, 0, 4, 2, 4, i_ref_max );
			
			T264_cabac_mb_mvd( t, 0, 0, 2, 4 );
			T264_cabac_mb_mvd( t, 0, 4, 2, 4 );
		}
		else	/* 8x8 */
		{
			/* sub mb type */
			t->mb.submb_part[0] = T264_cabac_mb_sub_p_partition( t );
			t->mb.submb_part[2] = T264_cabac_mb_sub_p_partition( t );
			t->mb.submb_part[8] = T264_cabac_mb_sub_p_partition( t );
			t->mb.submb_part[10] = T264_cabac_mb_sub_p_partition( t );

			/* ref 0 */
			T264_cabac_mb_ref( t, 0, 0, 2, 2, i_ref_max );
			T264_cabac_mb_ref( t, 0, 4, 2, 2, i_ref_max );
			T264_cabac_mb_ref( t, 0, 8, 2, 2, i_ref_max );
			T264_cabac_mb_ref( t, 0, 12, 2, 2, i_ref_max);
			
			for( i = 0; i < 4; i++ )
			{
				switch( t->mb.submb_part[luma_index[i<<2]] )
				{
				case MB_8x8:
					T264_cabac_mb_mvd( t, 0, 4*i, 2, 2 );
					break;
				case MB_8x4:
					T264_cabac_mb_mvd( t, 0, 4*i+0, 2, 1 );
					T264_cabac_mb_mvd( t, 0, 4*i+2, 2, 1 );
					break;
				case MB_4x8:
					T264_cabac_mb_mvd( t, 0, 4*i+0, 1, 2 );
					T264_cabac_mb_mvd( t, 0, 4*i+1, 1, 2 );
					break;
				case MB_4x4:
					T264_cabac_mb_mvd( t, 0, 4*i+0, 1, 1 );
					T264_cabac_mb_mvd( t, 0, 4*i+1, 1, 1 );
					T264_cabac_mb_mvd( t, 0, 4*i+2, 1, 1 );
					T264_cabac_mb_mvd( t, 0, 4*i+3, 1, 1 );
					break;
				}
			}
		}
	}
	else if( i_mb_type == B_MODE )
	{
		if((t->mb.mb_part==MB_16x16&&t->mb.is_copy!=1) || (t->mb.mb_part==MB_16x8) || (t->mb.mb_part==MB_8x16))
		{
			/* to be changed here*/
			/* All B mode */
			int i_list;
			int b_list[2][2];
			const int i_partition = t->mb.mb_part;
			int b_part_mode, part_mode0, part_mode1;
			static const int b_part_mode_map[3][3] = {
				{ B_L0_L0, B_L0_L1, B_L0_BI },
				{ B_L1_L0, B_L1_L1, B_L1_BI },
				{ B_BI_L0, B_BI_L1, B_BI_BI }
			};

			switch(t->mb.mb_part)
			{
			case MB_16x16:
				part_mode0 = t->mb.mb_part2[0] - B_L0_16x16;
				b_part_mode = b_part_mode_map[part_mode0][part_mode0];
				break;
			case MB_16x8:
				part_mode0 = t->mb.mb_part2[0] - B_L0_16x8;
				part_mode1 = t->mb.mb_part2[1] - B_L0_16x8;
				b_part_mode = b_part_mode_map[part_mode0][part_mode1];
				break;
			case MB_8x16:
				part_mode0 = t->mb.mb_part2[0] - B_L0_8x16;
				part_mode1 = t->mb.mb_part2[1] - B_L0_8x16;
				b_part_mode = b_part_mode_map[part_mode0][part_mode1];
				break;
			}
			/* init ref list utilisations */
			for( i = 0; i < 2; i++ )
			{
				b_list[0][i] = T264_mb_type_list0_table[b_part_mode][i];
				b_list[1][i] = T264_mb_type_list1_table[b_part_mode][i];
			}
#define INITINVALIDVEC(vec) vec.refno = -1; vec.x = vec.y = 0;
			for( i_list = 0; i_list < 2; i_list++ )
			{
				const int i_ref_max = i_list == 0 ? t->refl0_num:t->refl1_num;//t->ps.num_ref_idx_l0_active_minus1+1 : t->ps.num_ref_idx_l1_active_minus1+1;
				if( t->mb.mb_part == MB_16x16 )
				{
					if( b_list[i_list][0] ) 
						T264_cabac_mb_ref( t, i_list, 0, 4, 4, i_ref_max );
					else
					{
						INITINVALIDVEC(t->mb.vec[i_list][0]);
						copy_nvec(&t->mb.vec[i_list][0], &t->mb.vec[i_list][0], 4, 4, 4);
					}
				}
				else if( t->mb.mb_part == MB_16x8 )
				{
					if( b_list[i_list][0] ) 
						T264_cabac_mb_ref( t, i_list, 0, 4, 2, i_ref_max );
					else
					{
						INITINVALIDVEC(t->mb.vec[i_list][0]);
						copy_nvec(&t->mb.vec[i_list][0], &t->mb.vec[i_list][0], 4, 2, 4);
						INITINVALIDVEC(t->mb.vec_ref[VEC_LUMA+8].vec[i_list]);
					}
					if( b_list[i_list][1] ) 
						T264_cabac_mb_ref( t, i_list, 8, 4, 2, i_ref_max );
					else
					{
						INITINVALIDVEC(t->mb.vec[i_list][8]);
						copy_nvec(&t->mb.vec[i_list][8], &t->mb.vec[i_list][8], 4, 2, 4);
					}

				}
				else if( t->mb.mb_part == MB_8x16 )
				{
					if( b_list[i_list][0] )
						T264_cabac_mb_ref( t, i_list, 0, 2, 4, i_ref_max );
					else
					{
						INITINVALIDVEC(t->mb.vec[i_list][0]);
						copy_nvec(&t->mb.vec[i_list][0], &t->mb.vec[i_list][0], 2, 4, 4);
						INITINVALIDVEC(t->mb.vec_ref[VEC_LUMA+1].vec[i_list]);
					}
					if( b_list[i_list][1] )
						T264_cabac_mb_ref( t, i_list, 4, 2, 4, i_ref_max );
					else
					{
						INITINVALIDVEC(t->mb.vec[i_list][2]);
						copy_nvec(&t->mb.vec[i_list][2], &t->mb.vec[i_list][2], 2, 4, 4);
					}
				}
			}
#undef INITINVALIDVEC
			for( i_list = 0; i_list < 2; i_list++ )
			{
				if( t->mb.mb_part == MB_16x16 )
				{
					if( b_list[i_list][0] ) 
						T264_cabac_mb_mvd( t, i_list, 0, 4, 4 );
				}
				else if( t->mb.mb_part == MB_16x8 )
				{
					if( b_list[i_list][0] )
						T264_cabac_mb_mvd( t, i_list, 0, 4, 2 );
					if( b_list[i_list][1] ) 
						T264_cabac_mb_mvd( t, i_list, 8, 4, 2 );
				}
				else if( t->mb.mb_part == MB_8x16 )
				{
					if( b_list[i_list][0] ) 
						T264_cabac_mb_mvd( t, i_list, 0, 2, 4 );
					if( b_list[i_list][1] ) 
						T264_cabac_mb_mvd( t, i_list, 4, 2, 4 );
				}
			}
		}
		else if(t->mb.mb_part==MB_16x16 && t->mb.is_copy)
		{
			mb_get_directMB16x16_mv_cabac(t);
		}
		else /* B8x8 */
		{
			/* TODO */
			int i_list;
			/* sub mb type */
			t->mb.submb_part[0] = T264_cabac_mb_sub_b_partition( t );
			t->mb.submb_part[2] = T264_cabac_mb_sub_b_partition( t );
			t->mb.submb_part[8] = T264_cabac_mb_sub_b_partition( t );
			t->mb.submb_part[10] = T264_cabac_mb_sub_b_partition( t );

			/* ref */
			for( i_list = 0; i_list < 2; i_list++ )
			{
				int i_ref_max = (i_list==0)?t->refl0_num:t->refl1_num;
				for( i = 0; i < 4; i++ )
				{
					int luma_idx = luma_index[i<<2];
					int sub_part = t->mb.submb_part[luma_idx]-B_DIRECT_8x8;
					if( T264_mb_partition_listX_table[sub_part][i_list] == 1 )
					{
						T264_cabac_mb_ref( t, i_list, 4*i, 2, 2, i_ref_max );
					}
					else
					{
						int i8 = i / 2 * 16 + i % 2 * 2;
#define INITINVALIDVEC(vec) vec.refno = -1; vec.x = vec.y = 0;
						INITINVALIDVEC(t->mb.vec[i_list][luma_idx]);
						copy_nvec(&t->mb.vec[i_list][luma_idx], &t->mb.vec[i_list][luma_idx], 2, 2, 4);
						t->mb.vec_ref[VEC_LUMA + i8 + 0].vec[i_list] =
							t->mb.vec_ref[VEC_LUMA + i8 + 1].vec[i_list] =
							t->mb.vec_ref[VEC_LUMA + i8 + 8].vec[i_list] =
							t->mb.vec_ref[VEC_LUMA + i8 + 9].vec[i_list] = t->mb.vec[i_list][luma_idx];
#undef INITINVALIDVEC
					}
				}
			}
			T264_cabac_mb8x8_mvd( t, 0 );
			T264_cabac_mb8x8_mvd( t, 1 );
			for(i=0; i<4; i++)
			{
				int i_part = luma_index[i<<2];
				int sub_part = t->mb.submb_part[i_part] - B_DIRECT_8x8;
				t->mb.submb_part[i_part] = T264_map_btype_mbpart[sub_part];
			}
		}
	}

	i_mb_pos_tex = BitstreamPos( s );

	if( i_mb_type != I_16x16 )
	{
		T264_cabac_mb_cbp_luma( t );
		T264_cabac_mb_cbp_chroma( t );
	}

	cbp_dc = 0;
	if( t->mb.cbp_y > 0 || t->mb.cbp_c > 0 || i_mb_type == I_16x16 )
	{
		T264_cabac_mb_qp_delta( t );

		/* read residual */
		if( i_mb_type == I_16x16 )
		{
			/* DC Luma */
			int dc_nz;
			block_residual_read_cabac( t, 0, 0, t->mb.dc4x4_z, 16 );
			//for CABAC, record the DC non_zero
			dc_nz = array_non_zero_count(&(t->mb.dc4x4_z[0]), 16);
			if(dc_nz != 0)
			{
				cbp_dc = 1;
			}

			if( t->mb.cbp_y != 0 )
			{
				/* AC Luma */
				for( i = 0; i < 16; i++ )
				{
					block_residual_read_cabac( t, 1, i, &(t->mb.dct_y_z[i][1]), 15 );
					t->mb.dct_y_z[i][0] = t->mb.dc4x4_z[i];
				}
			}
		}
		else
		{
			if(t->frame_num == 1)
				t->frame_num = 1;
			for( i = 0; i < 16; i++ )
			{
				if( t->mb.cbp_y & ( 1 << ( i / 4 ) ) )
				{
					block_residual_read_cabac( t, 2, i, &(t->mb.dct_y_z[i][0]), 16 );
				}
			}
		}

		if( t->mb.cbp_c&0x03 )    /* Chroma DC residual present */
		{
			int dc_nz0, dc_nz1;
			block_residual_read_cabac( t, 3, 0, &(t->mb.dc2x2_z[0][0]), 4 );
			block_residual_read_cabac( t, 3, 1, &(t->mb.dc2x2_z[1][0]), 4 );
			//for CABAC, chroma dc pattern
			dc_nz0 = array_non_zero_count(t->mb.dc2x2_z[0], 4) > 0;
			dc_nz1 = array_non_zero_count(t->mb.dc2x2_z[1], 4) > 0;
			if(dc_nz0)
				cbp_dc |= 0x02;
			if(dc_nz1)
				cbp_dc |= 0x04;
		}
		if( t->mb.cbp_c&0x02 ) /* Chroma AC residual present */
		{
			for( i = 0; i < 8; i++ )
			{
				block_residual_read_cabac( t, 4, i, &(t->mb.dct_uv_z[i>>2][i&0x03][1]), 15);
				t->mb.dct_uv_z[i>>2][i&0x03][0] = t->mb.dc2x2_z[i>>2][i&0x03];
			}
		}
		else
		{
			for(i = 0 ; i < 4 ; i ++)
			{
				t->mb.dct_uv_z[0][i][0] = t->mb.dc2x2_z[0][i];
				t->mb.dct_uv_z[1][i][0] = t->mb.dc2x2_z[1][i];
			}
		}
	}
	//for CABAC, cbp
	t->mb.cbp = t->mb.cbp_y | (t->mb.cbp_c<<4) | (cbp_dc << 8);
	/*
	if( IS_INTRA( i_mb_type ) )
	t->stat.frame.i_itex_bits += bs_pos(s) - i_mb_pos_tex;
	else
	t->stat.frame.i_ptex_bits += bs_pos(s) - i_mb_pos_tex;
	*/
}
int T264dec_mb_read_cabac(T264_t *t)
{
	int skip;
	//for dec CABAC, set MVD to zero
	memset(&(t->mb.mvd[0][0][0]), 0, sizeof(t->mb.mvd));
	t->mb.cbp = t->mb.cbp_y = t->mb.cbp_c = t->mb.mb_qp_delta = 0;
	if (t->slice_type != SLICE_I)
		skip = T264_cabac_dec_mb_skip(t);
	else
		skip = 0;
	if(skip)
	{
		/* skip mb block */
		if (t->slice_type == SLICE_P)
		{
			T264_predict_mv_skip(t, 0, &t->mb.vec[0][0]);
			copy_nvec(&t->mb.vec[0][0], &t->mb.vec[0][0], 4, 4, 4);
			t->mb.mb_mode = P_MODE;     /* decode as MB_16x16 */
			t->mb.mb_part = MB_16x16;
		}
		else if(t->slice_type == SLICE_B)
		{
			mb_get_directMB16x16_mv_cabac(t);
            t->mb.is_copy = 1;
			t->mb.mb_mode = B_MODE;     /* decode as MB_16x16 */                
			t->mb.mb_part = MB_16x16;
		}
		else
		{
			assert(0);
		}				
	}
	else
	{
		T264_macroblock_read_cabac(t, t->bs);					
	}
	return skip;
}