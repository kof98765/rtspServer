#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "T264.h"
#include "cabac_engine.h"
#include "inter.h"
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

static void T264_cabac_mb_type( T264_t *t )
{
	T264_mb_context_t *mb_ctxs = &(t->rec->mb[0]);
    int32_t mb_mode = t->mb.mb_mode;

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

        if( mb_mode == I_4x4 )
        {
            T264_cabac_encode_decision( &t->cabac, 3 + ctx, 0 );
        }
        else if(mb_mode == I_16x16)   /* I_16x16 */
        {
            T264_cabac_encode_decision( &t->cabac, 3 + ctx, 1 );
            T264_cabac_encode_terminal( &t->cabac, 0 );

            T264_cabac_encode_decision( &t->cabac, 3 + 3, ( t->mb.cbp_y == 0 ? 0 : 1 ));
            if( t->mb.cbp_c == 0 )
            {
                T264_cabac_encode_decision( &t->cabac, 3 + 4, 0 );
            }
            else
            {
                T264_cabac_encode_decision( &t->cabac, 3 + 4, 1 );
                T264_cabac_encode_decision( &t->cabac, 3 + 5, ( t->mb.cbp_c == 1 ? 0 : 1 ) );
            }
            T264_cabac_encode_decision( &t->cabac, 3 + 6, ( (t->mb.mode_i16x16 / 2) ? 1 : 0 ));
            T264_cabac_encode_decision( &t->cabac, 3 + 7, ( (t->mb.mode_i16x16 % 2) ? 1 : 0 ));
        }
		else	/* I_PCM */
		{
			T264_cabac_encode_decision( &t->cabac, 3 + ctx, 1 );
			T264_cabac_encode_terminal( &t->cabac, 1 );
		}
    }
    else if( t->slice_type == SLICE_P )
    {
        /* prefix: 14, suffix: 17 */
        if( mb_mode == P_MODE )
        {
            if( t->mb.mb_part == MB_16x16 )
            {
                T264_cabac_encode_decision( &t->cabac, 14, 0 );
                T264_cabac_encode_decision( &t->cabac, 15, 0 );
                T264_cabac_encode_decision( &t->cabac, 16, 0 );
            }
            else if( t->mb.mb_part == MB_16x8 )
            {
                T264_cabac_encode_decision( &t->cabac, 14, 0 );
                T264_cabac_encode_decision( &t->cabac, 15, 1 );
                T264_cabac_encode_decision( &t->cabac, 17, 1 );
            }
            else if( t->mb.mb_part == MB_8x16 )
            {
                T264_cabac_encode_decision( &t->cabac, 14, 0 );
                T264_cabac_encode_decision( &t->cabac, 15, 1 );
                T264_cabac_encode_decision( &t->cabac, 17, 0 );
            }
			else /* P8x8 mode */
			{
				T264_cabac_encode_decision( &t->cabac, 14, 0 );
				T264_cabac_encode_decision( &t->cabac, 15, 0 );
				T264_cabac_encode_decision( &t->cabac, 16, 1 );
			}
        }
        else if( mb_mode == I_4x4 )
        {
            /* prefix */
            T264_cabac_encode_decision( &t->cabac, 14, 1 );

            T264_cabac_encode_decision( &t->cabac, 17, 0 );
        }
        else if(mb_mode == I_16x16) /* intra 16x16 */
        {
            /* prefix */
            T264_cabac_encode_decision( &t->cabac, 14, 1 );

            /* suffix */
            T264_cabac_encode_decision( &t->cabac, 17, 1 );
            T264_cabac_encode_terminal( &t->cabac, 0 ); /*ctxIdx == 276 */

            T264_cabac_encode_decision( &t->cabac, 17+1, ( t->mb.cbp_y == 0 ? 0 : 1 ));
            if( t->mb.cbp_c == 0 )
            {
                T264_cabac_encode_decision( &t->cabac, 17+2, 0 );
            }
            else
            {
                T264_cabac_encode_decision( &t->cabac, 17+2, 1 );
                T264_cabac_encode_decision( &t->cabac, 17+2, ( t->mb.cbp_c == 1 ? 0 : 1 ) );
            }
            T264_cabac_encode_decision( &t->cabac, 17+3, ( (t->mb.mode_i16x16 / 2) ? 1 : 0 ));
            T264_cabac_encode_decision( &t->cabac, 17+3, ( (t->mb.mode_i16x16 % 2) ? 1 : 0 ));
        }
		else /* I_PCM */
		{
			/* prefix */
			T264_cabac_encode_decision( &t->cabac, 14, 1 );

			T264_cabac_encode_decision( &t->cabac, 17, 1 );
			T264_cabac_encode_terminal( &t->cabac, 1 ); /*ctxIdx == 276 */
		}
    }
    else if( t->slice_type == SLICE_B )
    {
		int ctx = 0;
		if( t->mb.mb_x > 0 && mb_ctxs[t->mb.mb_xy-1].mb_mode != B_SKIP && !mb_ctxs[t->mb.mb_xy-1].is_copy )
		{
			ctx++;
		}
		if( t->mb.mb_y > 0 && mb_ctxs[t->mb.mb_xy - t->mb_stride].mb_mode != B_SKIP && ! mb_ctxs[t->mb.mb_xy - t->mb_stride].is_copy)
		{
			ctx++;
		}
        
        if( t->mb.is_copy)
        {
            T264_cabac_encode_decision( &t->cabac, 27+ctx, 0 );
        }
        else if( t->mb.mb_part == MB_8x8 )
        {
            T264_cabac_encode_decision( &t->cabac, 27+ctx, 1 );
            T264_cabac_encode_decision( &t->cabac, 27+3,   1 );
            T264_cabac_encode_decision( &t->cabac, 27+4,   1 );

            T264_cabac_encode_decision( &t->cabac, 27+5,   1 );
            T264_cabac_encode_decision( &t->cabac, 27+5,   1 );
            T264_cabac_encode_decision( &t->cabac, 27+5,   1 );
        }
        else if( IS_INTRA( mb_mode ) )
        {
            /* prefix */
            T264_cabac_encode_decision( &t->cabac, 27+ctx, 1 );
            T264_cabac_encode_decision( &t->cabac, 27+3,   1 );
            T264_cabac_encode_decision( &t->cabac, 27+4,   1 );

            T264_cabac_encode_decision( &t->cabac, 27+5,   1 );
            T264_cabac_encode_decision( &t->cabac, 27+5,   0 );
            T264_cabac_encode_decision( &t->cabac, 27+5,   1 );

            /* Suffix */
            if( mb_mode == I_4x4 )
            {
                T264_cabac_encode_decision( &t->cabac, 32, 0 );
            }
			else if(mb_mode == I_16x16)
			{
				T264_cabac_encode_decision( &t->cabac, 32, 1 );
				T264_cabac_encode_terminal( &t->cabac,     0 );

				/* TODO */
				T264_cabac_encode_decision( &t->cabac, 32+1, ( t->mb.cbp_y == 0 ? 0 : 1 ));
				if( t->mb.cbp_c == 0 )
				{
					T264_cabac_encode_decision( &t->cabac, 32+2, 0 );
				}
				else
				{
					T264_cabac_encode_decision( &t->cabac, 32+2, 1 );
					T264_cabac_encode_decision( &t->cabac, 32+2, ( t->mb.cbp_c == 1 ? 0 : 1 ) );
				}
				T264_cabac_encode_decision( &t->cabac, 32+3, ( (t->mb.mode_i16x16 / 2) ? 1 : 0 ));
				T264_cabac_encode_decision( &t->cabac, 32+3, ( (t->mb.mode_i16x16 % 2) ? 1 : 0 ));
			}
            else /* I_PCM */
            {
                T264_cabac_encode_decision( &t->cabac, 32, 1 );
                T264_cabac_encode_terminal( &t->cabac,     1 );
            }
            
        }
        else
        {
            static const int i_mb_len[21] =
            {
                3, 6, 6,    /* L0 L0 */
                3, 6, 6,    /* L1 L1 */
                6, 7, 7,    /* BI BI */

                6, 6,       /* L0 L1 */
                6, 6,       /* L1 L0 */
                7, 7,       /* L0 BI */
                7, 7,       /* L1 BI */
                7, 7,       /* BI L0 */
                7, 7,       /* BI L1 */
            };
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

            const int i_partition = t->mb.mb_part;
            int idx = 0;
            int i, b_part_mode, part_mode0, part_mode1;
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
            switch( b_part_mode )
            {
                /* D_16x16, D_16x8, D_8x16 */
                case B_BI_BI: idx += 3;
                case B_L1_L1: idx += 3;
                case B_L0_L0:
                    if( i_partition == MB_16x8 )
                        idx += 1;
                    else if( i_partition == MB_8x16 )
                        idx += 2;
                    break;

                /* D_16x8, D_8x16 */
                case B_BI_L1: idx += 2;
                case B_BI_L0: idx += 2;
                case B_L1_BI: idx += 2;
                case B_L0_BI: idx += 2;
                case B_L1_L0: idx += 2;
                case B_L0_L1:
                    idx += 3*3;
                    if( i_partition == MB_8x16 )
                        idx++;
                    break;
                default:
					return;
			}

            T264_cabac_encode_decision( &t->cabac, 27+ctx,                         i_mb_bits[idx][0] );
            T264_cabac_encode_decision( &t->cabac, 27+3,                           i_mb_bits[idx][1] );
            T264_cabac_encode_decision( &t->cabac, 27+(i_mb_bits[idx][1] != 0 ? 4 : 5), i_mb_bits[idx][2] );
            for( i = 3; i < i_mb_len[idx]; i++ )
            {
                T264_cabac_encode_decision( &t->cabac, 27+5,                       i_mb_bits[idx][i] );
            }
        }
    }
    else
    {
		//dummy here
    }
}

static void T264_cabac_mb_intra4x4_pred_mode( T264_t *t, int i_pred, int i_mode )
{
    if( i_pred == i_mode )
    {
        /* b_prev_intra4x4_pred_mode */
        T264_cabac_encode_decision( &t->cabac, 68, 1 );
    }
    else
    {
        /* b_prev_intra4x4_pred_mode */
        T264_cabac_encode_decision( &t->cabac, 68, 0 );
        if( i_mode > i_pred  )
        {
            i_mode--;
        }
        T264_cabac_encode_decision( &t->cabac, 69, (i_mode     )&0x01 );
        T264_cabac_encode_decision( &t->cabac, 69, (i_mode >> 1)&0x01 );
        T264_cabac_encode_decision( &t->cabac, 69, (i_mode >> 2)&0x01 );
    }
}

static void T264_cabac_mb_intra8x8_pred_mode( T264_t *t )
{
    const int i_mode  = t->mb.mb_mode_uv;
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
	
    if( i_mode == Intra_8x8_DC )
    {
        T264_cabac_encode_decision( &t->cabac, 64 + ctx, Intra_8x8_DC );
    }
    else
    {
        T264_cabac_encode_decision( &t->cabac, 64 + ctx, 1 );
        T264_cabac_encode_decision( &t->cabac, 64 + 3, ( i_mode == 1 ? 0 : 1 ) );
        if( i_mode > 1 )
        {
            T264_cabac_encode_decision( &t->cabac, 64 + 3, ( i_mode == 2 ? 0 : 1 ) );
        }
    }
}

static void T264_cabac_mb_cbp_luma( T264_t *t )
{
    /* TODO: clean up and optimize */
	T264_mb_context_t *mb_ctxs = &(t->rec->mb[0]);
    int i8x8;
    for( i8x8 = 0; i8x8 < 4; i8x8++ )
    {
        int i_mba_xy = -1;
        int i_mbb_xy = -1;
        int x = luma_inverse_x[4*i8x8];
        int y = luma_inverse_y[4*i8x8];
        int ctx = 0;

        if( x > 0 )
            i_mba_xy = t->mb.mb_xy;
        else if( t->mb.mb_x > 0 )
            i_mba_xy = t->mb.mb_xy - 1;

        if( y > 0 )
            i_mbb_xy = t->mb.mb_xy;
        else if( t->mb.mb_y > 0 )
            i_mbb_xy = t->mb.mb_xy - t->mb_stride;


        /* No need to test for PCM and SKIP */
        if( i_mba_xy >= 0 )
        {
            const int i8x8a = block_idx_xy[(x-1)&0x03][y]/4;
            if( ((mb_ctxs[i_mba_xy].cbp_y >> i8x8a)&0x01) == 0 )
            {
                ctx++;
            }
        }

        if( i_mbb_xy >= 0 )
        {
            const int i8x8b = block_idx_xy[x][(y-1)&0x03]/4;
            if( ((mb_ctxs[i_mbb_xy].cbp_y >> i8x8b)&0x01) == 0 )
            {
                ctx += 2;
            }
        }
															   
        T264_cabac_encode_decision( &t->cabac, 73 + ctx, (t->mb.cbp_y >> i8x8)&0x01 );
    }
}

static void T264_cabac_mb_cbp_chroma( T264_t *t )
{
    int cbp_a = -1;
    int cbp_b = -1;
    int ctx;
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
    if( t->mb.cbp_c == 0 )
    {
        T264_cabac_encode_decision( &t->cabac, 77 + ctx, 0 );
    }
    else
    {
        T264_cabac_encode_decision( &t->cabac, 77 + ctx, 1 );

        ctx = 4;
        if( cbp_a == 2 ) ctx++;
        if( cbp_b == 2 ) ctx += 2;
        T264_cabac_encode_decision( &t->cabac, 77 + ctx, t->mb.cbp_c > 1 ? 1 : 0 );
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

    while( val > 0 )
    {
        T264_cabac_encode_decision( &t->cabac,  60 + ctx, 1 );
        if( ctx < 2 )
            ctx = 2;
        else
            ctx = 3;
        val--;
    }
    T264_cabac_encode_decision( &t->cabac,  60 + ctx, 0 );
}

void T264_cabac_mb_skip( T264_t *t, int b_skip )
{
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
        T264_cabac_encode_decision( &t->cabac, 11 + ctx, b_skip ? 1 : 0 );
    else /* SLICE_TYPE_B */
        T264_cabac_encode_decision( &t->cabac, 24 + ctx, b_skip ? 1 : 0 );
}

static __inline  void T264_cabac_mb_sub_p_partition( T264_t *t, int i_sub )
{
    if( i_sub == MB_8x8 )
    {
            T264_cabac_encode_decision( &t->cabac, 21, 1 );
    }
    else if( i_sub == MB_8x4 )
    {
            T264_cabac_encode_decision( &t->cabac, 21, 0 );
            T264_cabac_encode_decision( &t->cabac, 22, 0 );
    }
    else if( i_sub == MB_4x8 )
    {
            T264_cabac_encode_decision( &t->cabac, 21, 0 );
            T264_cabac_encode_decision( &t->cabac, 22, 1 );
            T264_cabac_encode_decision( &t->cabac, 23, 1 );
    }
    else if( i_sub == MB_4x4 )
    {
            T264_cabac_encode_decision( &t->cabac, 21, 0 );
            T264_cabac_encode_decision( &t->cabac, 22, 1 );
            T264_cabac_encode_decision( &t->cabac, 23, 0 );
    }
}

static __inline  void T264_cabac_mb_sub_b_partition( T264_t *t, int i_sub )
{
    if( i_sub == B_DIRECT_8x8 )
    {
        T264_cabac_encode_decision( &t->cabac, 36, 0 );
    }
    else if( i_sub == B_L0_8x8 )
    {
        T264_cabac_encode_decision( &t->cabac, 36, 1 );
        T264_cabac_encode_decision( &t->cabac, 37, 0 );
        T264_cabac_encode_decision( &t->cabac, 39, 0 );
    }
    else if( i_sub == B_L1_8x8 )
    {
        T264_cabac_encode_decision( &t->cabac, 36, 1 );
        T264_cabac_encode_decision( &t->cabac, 37, 0 );
        T264_cabac_encode_decision( &t->cabac, 39, 1 );
    }
    else if( i_sub == B_Bi_8x8 )
    {
        T264_cabac_encode_decision( &t->cabac, 36, 1 );
        T264_cabac_encode_decision( &t->cabac, 37, 1 );
        T264_cabac_encode_decision( &t->cabac, 38, 0 );
        T264_cabac_encode_decision( &t->cabac, 39, 0 );
        T264_cabac_encode_decision( &t->cabac, 39, 0 );
    }
    else if( i_sub == B_L0_8x4 )
    {
        T264_cabac_encode_decision( &t->cabac, 36, 1 );
        T264_cabac_encode_decision( &t->cabac, 37, 1 );
        T264_cabac_encode_decision( &t->cabac, 38, 0 );
        T264_cabac_encode_decision( &t->cabac, 39, 0 );
        T264_cabac_encode_decision( &t->cabac, 39, 1 );
    }
    else if( i_sub == B_L0_4x8 )
    {
        T264_cabac_encode_decision( &t->cabac, 36, 1 );
        T264_cabac_encode_decision( &t->cabac, 37, 1 );
        T264_cabac_encode_decision( &t->cabac, 38, 0 );
        T264_cabac_encode_decision( &t->cabac, 39, 1 );
        T264_cabac_encode_decision( &t->cabac, 39, 0 );
    }
    else if( i_sub == B_L1_8x4 )
    {
        T264_cabac_encode_decision( &t->cabac, 36, 1 );
        T264_cabac_encode_decision( &t->cabac, 37, 1 );
        T264_cabac_encode_decision( &t->cabac, 38, 0 );
        T264_cabac_encode_decision( &t->cabac, 39, 1 );
        T264_cabac_encode_decision( &t->cabac, 39, 1 );
    }
    else if( i_sub == B_L1_4x8 )
    {
        T264_cabac_encode_decision( &t->cabac, 36, 1 );
        T264_cabac_encode_decision( &t->cabac, 37, 1 );
        T264_cabac_encode_decision( &t->cabac, 38, 1 );
        T264_cabac_encode_decision( &t->cabac, 39, 0 );
        T264_cabac_encode_decision( &t->cabac, 39, 0 );
        T264_cabac_encode_decision( &t->cabac, 39, 0 );
    }
    else if( i_sub == B_Bi_8x4 )
    {
        T264_cabac_encode_decision( &t->cabac, 36, 1 );
        T264_cabac_encode_decision( &t->cabac, 37, 1 );
        T264_cabac_encode_decision( &t->cabac, 38, 1 );
        T264_cabac_encode_decision( &t->cabac, 39, 0 );
        T264_cabac_encode_decision( &t->cabac, 39, 0 );
        T264_cabac_encode_decision( &t->cabac, 39, 1 );
    }
    else if( i_sub == B_Bi_4x8 )
    {
        T264_cabac_encode_decision( &t->cabac, 36, 1 );
        T264_cabac_encode_decision( &t->cabac, 37, 1 );
        T264_cabac_encode_decision( &t->cabac, 38, 1 );
        T264_cabac_encode_decision( &t->cabac, 39, 0 );
        T264_cabac_encode_decision( &t->cabac, 39, 1 );
        T264_cabac_encode_decision( &t->cabac, 39, 0 );
    }
    else if( i_sub == B_L0_4x4 )
    {
        T264_cabac_encode_decision( &t->cabac, 36, 1 );
        T264_cabac_encode_decision( &t->cabac, 37, 1 );
        T264_cabac_encode_decision( &t->cabac, 38, 1 );
        T264_cabac_encode_decision( &t->cabac, 39, 0 );
        T264_cabac_encode_decision( &t->cabac, 39, 1 );
        T264_cabac_encode_decision( &t->cabac, 39, 1 );
    }
    else if( i_sub == B_L1_4x4 )
    {
        T264_cabac_encode_decision( &t->cabac, 36, 1 );
        T264_cabac_encode_decision( &t->cabac, 37, 1 );
        T264_cabac_encode_decision( &t->cabac, 38, 1 );
        T264_cabac_encode_decision( &t->cabac, 39, 1 );
        T264_cabac_encode_decision( &t->cabac, 39, 0 );
    }
    else if( i_sub == B_Bi_4x4 )
    {
        T264_cabac_encode_decision( &t->cabac, 36, 1 );
        T264_cabac_encode_decision( &t->cabac, 37, 1 );
        T264_cabac_encode_decision( &t->cabac, 38, 1 );
        T264_cabac_encode_decision( &t->cabac, 39, 1 );
        T264_cabac_encode_decision( &t->cabac, 39, 1 );
    }
}


static __inline  void T264_cabac_mb_ref( T264_t *t, int i_list, int idx )
{
	const int i8    = T264_scan8[idx];
	T264_mb_context_t *mb_ctxs = &(t->rec->mb[0]);
	const int i_refa = t->mb.vec_ref[i8 - 1].vec[i_list].refno;
    const int i_refb = t->mb.vec_ref[i8 - 8].vec[i_list].refno;
    int i_ref  = t->mb.vec_ref[i8].vec[i_list].refno;
	int a_direct, b_direct;
	int ctx  = 0;
	int luma_idx = luma_index[idx];
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

    while( i_ref > 0 )
    {
        T264_cabac_encode_decision( &t->cabac, 54 + ctx, 1 );
        if( ctx < 4 )
            ctx = 4;
        else
            ctx = 5;

        i_ref--;
    }
    T264_cabac_encode_decision( &t->cabac, 54 + ctx, 0 );
}


static __inline  void  T264_cabac_mb_mvd_cpn( T264_t *t, int i_list, int i8, int l, int mvd )
{
    const int amvd = abs( t->mb.mvd_ref[i_list][i8 - 1][l] ) +
                     abs( t->mb.mvd_ref[i_list][i8 - 8][l] );
    const int i_abs = abs( mvd );
    const int i_prefix = T264_MIN( i_abs, 9 );
    const int ctxbase = (l == 0 ? 40 : 47);
    int ctx;
    int i;


    if( amvd < 3 )
        ctx = 0;
    else if( amvd > 32 )
        ctx = 2;
    else
        ctx = 1;

    for( i = 0; i < i_prefix; i++ )
    {
        T264_cabac_encode_decision( &t->cabac, ctxbase + ctx, 1 );
        if( ctx < 3 )
            ctx = 3;
        else if( ctx < 6 )
            ctx++;
    }
    if( i_prefix < 9 )
    {
        T264_cabac_encode_decision( &t->cabac, ctxbase + ctx, 0 );
    }

    if( i_prefix >= 9 )
    {
        int i_suffix = i_abs - 9;
        int k = 3;

        while( i_suffix >= (1<<k) )
        {
            T264_cabac_encode_bypass( &t->cabac, 1 );
            i_suffix -= 1 << k;
            k++;
        }
        T264_cabac_encode_bypass( &t->cabac, 0 );
        while( k-- )
        {
            T264_cabac_encode_bypass( &t->cabac, (i_suffix >> k)&0x01 );
        }
    }

    /* sign */
    if( mvd > 0 )
        T264_cabac_encode_bypass( &t->cabac, 0 );
    else if( mvd < 0 )
        T264_cabac_encode_bypass( &t->cabac, 1 );
}

static __inline  void  T264_cabac_mb_mvd( T264_t *t, int i_list, int idx, int width, int height )
{
    T264_vector_t mvp;
    int mdx, mdy;
	int i, j;
	int i8    = T264_scan8[idx];
	int luma_idx = luma_index[idx];
    /* Calculate mvd */
	mvp.refno = t->mb.vec_ref[i8].vec[i_list].refno;
    T264_predict_mv( t, i_list, luma_idx, width, &mvp );
	mdx = t->mb.vec_ref[i8].vec[i_list].x - mvp.x;
	mdy = t->mb.vec_ref[i8].vec[i_list].y - mvp.y;
    
    /* encode */
    T264_cabac_mb_mvd_cpn( t, i_list, i8, 0, mdx );
    T264_cabac_mb_mvd_cpn( t, i_list, i8, 1, mdy );
	/* save mvd value */
	for(j=0; j<height; j++)
	{
		for(i=0; i<width; i++)
		{
			t->mb.mvd_ref[i_list][i8+i][0] = mdx;
			t->mb.mvd_ref[i_list][i8+i][1] = mdy;
			t->mb.mvd[i_list][luma_idx+i][0] = mdx;
			t->mb.mvd[i_list][luma_idx+i][1] = mdy;
		}
		i8 += 8;
		luma_idx += 4;
	}
}

static __inline void T264_cabac_mb8x8_mvd( T264_t *t, int i_list )
{
	int i;
	int sub_part;
	for( i = 0; i < 4; i++ )
	{
		sub_part = t->mb.submb_part[luma_index[i<<2]];
		if( T264_mb_partition_listX_table[sub_part-B_DIRECT_8x8][i_list] == 0 )
		{
			continue;
		}

		switch( sub_part )
		{
		case B_DIRECT_8x8:
			assert(0);
			break;
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
        if( x > 0 )
            i_mba_xy = t->mb.mb_xy;
        else if( t->mb.mb_x > 0 )
            i_mba_xy = t->mb.mb_xy -1;

        if( y > 0 )
            i_mbb_xy = t->mb.mb_xy;
        else if( t->mb.mb_y > 0 )
            i_mbb_xy = t->mb.mb_xy - t->mb_stride;

        /* no need to test for skip/pcm */
        if( i_mba_xy >= 0 )
        {
            const int i8x8a = block_idx_xy[(x-1)&0x03][y]/4;
            if( (mb_ctxs[i_mba_xy].cbp_y&0x0f)>> i8x8a )
            {
                i_nza = t->mb.nnz_ref[i8-1];
            }
        }
        if( i_mbb_xy >= 0 )
        {
            const int i8x8b = block_idx_xy[x][(y-1)&0x03]/4;
            if( (mb_ctxs[i_mbb_xy].cbp_y&0x0f)>> i8x8b )
            {
                i_nzb = t->mb.nnz_ref[i8 - 8];
            }
        }
    }
    else if( i_cat == 3 )
    {
        /* no need to test skip/pcm */
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
        int idxc = i_idx% 4;

        if( idxc == 1 || idxc == 3 )
            i_mba_xy = t->mb.mb_xy;
        else if( t->mb.mb_x > 0 )
            i_mba_xy = t->mb.mb_xy - 1;

        if( idxc == 2 || idxc == 3 )
            i_mbb_xy = t->mb.mb_xy;
        else if( t->mb.mb_y > 0 )
            i_mbb_xy = t->mb.mb_xy - t->mb_stride;

        /* no need to test skip/pcm */
        if( i_mba_xy >= 0 && (mb_ctxs[i_mba_xy].cbp&0x30) == 0x20 )
        {
            i_nza = t->mb.nnz_ref[T264_scan8[16+i_idx] - 1];
        }
        if( i_mbb_xy >= 0 && (mb_ctxs[i_mbb_xy].cbp&0x30) == 0x20 )
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


static void block_residual_write_cabac( T264_t *t, int i_ctxBlockCat, int i_idx, int16_t *l, int i_count )
{
    static const int significant_coeff_flag_offset[5] = { 0, 15, 29, 44, 47 };
    static const int last_significant_coeff_flag_offset[5] = { 0, 15, 29, 44, 47 };
    static const int coeff_abs_level_m1_offset[5] = { 0, 10, 20, 30, 39 };

    int i_coeff_abs_m1[16];
    int i_coeff_sign[16];
    int i_coeff = 0;
    int i_last  = 0;

    int i_abslevel1 = 0;
    int i_abslevelgt1 = 0;

    int i;

    /* i_ctxBlockCat: 0-> DC 16x16  i_idx = 0
     *                1-> AC 16x16  i_idx = luma4x4idx
     *                2-> Luma4x4   i_idx = luma4x4idx
     *                3-> DC Chroma i_idx = iCbCr
     *                4-> AC Chroma i_idx = 4 * iCbCr + chroma4x4idx
     */

    //fprintf( stderr, "l[] = " );
    for( i = 0; i < i_count; i++ )
    {
        //fprintf( stderr, "%d ", l[i] );
        if( l[i] != 0 )
        {
            i_coeff_abs_m1[i_coeff] = abs( l[i] ) - 1;
            i_coeff_sign[i_coeff]   = ( l[i] < 0 ? 1 : 0);
            i_coeff++;

            i_last = i;
        }
    }
    //fprintf( stderr, "\n" );

    if( i_coeff == 0 )
    {
        /* codec block flag */
        T264_cabac_encode_decision( &t->cabac,  85 + T264_cabac_mb_cbf_ctxidxinc( t, i_ctxBlockCat, i_idx ), 0 );
        return;
    }

    /* block coded */
    T264_cabac_encode_decision( &t->cabac,  85 + T264_cabac_mb_cbf_ctxidxinc( t, i_ctxBlockCat, i_idx ), 1 );
    for( i = 0; i < i_count - 1; i++ )
    {
        int i_ctxIdxInc;

        i_ctxIdxInc = T264_MIN( i, i_count - 2 );

        if( l[i] != 0 )
        {
            T264_cabac_encode_decision( &t->cabac, 105 + significant_coeff_flag_offset[i_ctxBlockCat] + i_ctxIdxInc, 1 );
            T264_cabac_encode_decision( &t->cabac, 166 + last_significant_coeff_flag_offset[i_ctxBlockCat] + i_ctxIdxInc, i == i_last ? 1 : 0 );
        }
        else
        {
            T264_cabac_encode_decision( &t->cabac, 105 + significant_coeff_flag_offset[i_ctxBlockCat] + i_ctxIdxInc, 0 );
        }
        if( i == i_last )
        {
            break;
        }
    }

    for( i = i_coeff - 1; i >= 0; i-- )
    {
        int i_prefix;
        int i_ctxIdxInc;

        /* write coeff_abs - 1 */

        /* prefix */
        i_prefix = T264_MIN( i_coeff_abs_m1[i], 14 );

        i_ctxIdxInc = (i_abslevelgt1 != 0 ? 0 : T264_MIN( 4, i_abslevel1 + 1 )) + coeff_abs_level_m1_offset[i_ctxBlockCat];
        if( i_prefix == 0 )
        {
            T264_cabac_encode_decision( &t->cabac,  227 + i_ctxIdxInc, 0 );
        }
        else
        {
            int j;
            T264_cabac_encode_decision( &t->cabac,  227 + i_ctxIdxInc, 1 );
            i_ctxIdxInc = 5 + T264_MIN( 4, i_abslevelgt1 ) + coeff_abs_level_m1_offset[i_ctxBlockCat];
            for( j = 0; j < i_prefix - 1; j++ )
            {
                T264_cabac_encode_decision( &t->cabac,  227 + i_ctxIdxInc, 1 );
            }
            if( i_prefix < 14 )
            {
                T264_cabac_encode_decision( &t->cabac,  227 + i_ctxIdxInc, 0 );
            }
        }
        /* suffix */
        if( i_coeff_abs_m1[i] >= 14 )
        {
            int k = 0;
            int i_suffix = i_coeff_abs_m1[i] - 14;

            while( i_suffix >= (1<<k) )
            {
                T264_cabac_encode_bypass( &t->cabac, 1 );
                i_suffix -= 1 << k;
                k++;
            }
            T264_cabac_encode_bypass( &t->cabac, 0 );
            while( k-- )
            {
                T264_cabac_encode_bypass( &t->cabac, (i_suffix >> k)&0x01 );
            }
        }

        /* write sign */
        T264_cabac_encode_bypass( &t->cabac, i_coeff_sign[i] );


        if( i_coeff_abs_m1[i] == 0 )
        {
            i_abslevel1++;
        }
        else
        {
            i_abslevelgt1++;
        }
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

void T264_macroblock_write_cabac( T264_t *t, bs_t *s )
{
    const int i_mb_type = t->mb.mb_mode;
    const int i_mb_pos_start = BitstreamPos( s );
    int       i_mb_pos_tex;

    int i;

    /* Write the MB type */
    T264_cabac_mb_type( t );

    /* PCM special block type UNTESTED */
	/* no PCM here*/
	
    if( IS_INTRA( i_mb_type ) )
    {
        /* Prediction */
        if( i_mb_type == I_4x4 )
        {
            for( i = 0; i < 16; i++ )
            {
                const int i_pred = T264_mb_predict_intra4x4_mode( t, i );
                const int i_mode = t->mb.i4x4_pred_mode_ref[T264_scan8[i]];
                T264_cabac_mb_intra4x4_pred_mode( t, i_pred, i_mode );
            }
        }
        T264_cabac_mb_intra8x8_pred_mode( t );
    }
    else if( i_mb_type == P_MODE )
    {
        if( t->mb.mb_part == MB_16x16 )
        {
            if( t->ps.num_ref_idx_l0_active_minus1 > 0 )
            {
                T264_cabac_mb_ref( t, 0, 0 );
            }
            T264_cabac_mb_mvd( t, 0, 0, 4, 4 );
        }
        else if( t->mb.mb_part == MB_16x8 )
        {
            if( t->ps.num_ref_idx_l0_active_minus1 > 0 )
            {
                T264_cabac_mb_ref( t, 0, 0 );
                T264_cabac_mb_ref( t, 0, 8 );
            }
            T264_cabac_mb_mvd( t, 0, 0, 4, 2 );
            T264_cabac_mb_mvd( t, 0, 8, 4, 2 );
        }
        else if( t->mb.mb_part == MB_8x16 )
        {
            if( t->ps.num_ref_idx_l0_active_minus1 > 0 )
            {
                T264_cabac_mb_ref( t, 0, 0 );
                T264_cabac_mb_ref( t, 0, 4 );
            }
            T264_cabac_mb_mvd( t, 0, 0, 2, 4 );
            T264_cabac_mb_mvd( t, 0, 4, 2, 4 );
        }
		else	/* 8x8 */
		{
			/* sub mb type */
			T264_cabac_mb_sub_p_partition( t, t->mb.submb_part[0] );
			T264_cabac_mb_sub_p_partition( t, t->mb.submb_part[2] );
			T264_cabac_mb_sub_p_partition( t, t->mb.submb_part[8] );
			T264_cabac_mb_sub_p_partition( t, t->mb.submb_part[10] );

			/* ref 0 */
			if( t->ps.num_ref_idx_l0_active_minus1 > 0 )
			{
				T264_cabac_mb_ref( t, 0, 0 );
				T264_cabac_mb_ref( t, 0, 4 );
				T264_cabac_mb_ref( t, 0, 8 );
				T264_cabac_mb_ref( t, 0, 12 );
			}

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
			for( i_list = 0; i_list < 2; i_list++ )
			{
				const int i_ref_max = i_list == 0 ? t->ps.num_ref_idx_l0_active_minus1+1 : t->ps.num_ref_idx_l1_active_minus1+1;

				if( i_ref_max > 1 )
				{
					if( t->mb.mb_part == MB_16x16 )
					{
						if( b_list[i_list][0] ) T264_cabac_mb_ref( t, i_list, 0 );
					}
					else if( t->mb.mb_part == MB_16x8 )
					{
						if( b_list[i_list][0] ) T264_cabac_mb_ref( t, i_list, 0 );
						if( b_list[i_list][1] ) T264_cabac_mb_ref( t, i_list, 8 );
					}
					else if( t->mb.mb_part == MB_8x16 )
					{
						if( b_list[i_list][0] ) T264_cabac_mb_ref( t, i_list, 0 );
						if( b_list[i_list][1] ) T264_cabac_mb_ref( t, i_list, 4 );
					}
				}
			}
			for( i_list = 0; i_list < 2; i_list++ )
			{
				if( t->mb.mb_part == MB_16x16 )
				{
					if( b_list[i_list][0] ) T264_cabac_mb_mvd( t, i_list, 0, 4, 4 );
				}
				else if( t->mb.mb_part == MB_16x8 )
				{
					if( b_list[i_list][0] ) T264_cabac_mb_mvd( t, i_list, 0, 4, 2 );
					if( b_list[i_list][1] ) T264_cabac_mb_mvd( t, i_list, 8, 4, 2 );
				}
				else if( t->mb.mb_part == MB_8x16 )
				{
					if( b_list[i_list][0] ) T264_cabac_mb_mvd( t, i_list, 0, 2, 4 );
					if( b_list[i_list][1] ) T264_cabac_mb_mvd( t, i_list, 4, 2, 4 );
				}
			}
		}
		else if(t->mb.mb_part==MB_16x16 && t->mb.is_copy)
		{
		}
		else /* B8x8 */
		{
			/* TODO */
			int i_list;
			/* sub mb type */
			T264_cabac_mb_sub_b_partition( t, t->mb.submb_part[0] );
			T264_cabac_mb_sub_b_partition( t, t->mb.submb_part[2] );
			T264_cabac_mb_sub_b_partition( t, t->mb.submb_part[8] );
			T264_cabac_mb_sub_b_partition( t, t->mb.submb_part[10] );

			/* ref */
			for( i_list = 0; i_list < 2; i_list++ )
			{
				if( ( i_list ? t->ps.num_ref_idx_l1_active_minus1 : t->ps.num_ref_idx_l0_active_minus1 ) == 0 )
					continue;
				for( i = 0; i < 4; i++ )
				{
					int sub_part = t->mb.submb_part[luma_index[i<<2]]-B_DIRECT_8x8;
					if( T264_mb_partition_listX_table[sub_part][i_list] == 1 )
					{
						T264_cabac_mb_ref( t, i_list, 4*i );
					}
				}
			}
			T264_cabac_mb8x8_mvd( t, 0 );
			T264_cabac_mb8x8_mvd( t, 1 );
		}
    }
    
    i_mb_pos_tex = BitstreamPos( s );
    
    if( i_mb_type != I_16x16 )
    {
        T264_cabac_mb_cbp_luma( t );
        T264_cabac_mb_cbp_chroma( t );
    }
	
    if( t->mb.cbp_y > 0 || t->mb.cbp_c > 0 || i_mb_type == I_16x16 )
    {
        T264_cabac_mb_qp_delta( t );

        /* write residual */
        if( i_mb_type == I_16x16 )
        {
            /* DC Luma */
            block_residual_write_cabac( t, 0, 0, t->mb.dc4x4_z, 16 );

            if( t->mb.cbp_y != 0 )
            {
                /* AC Luma */
                for( i = 0; i < 16; i++ )
                {
                    block_residual_write_cabac( t, 1, i, &(t->mb.dct_y_z[i][1]), 15 );
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
                    block_residual_write_cabac( t, 2, i, &(t->mb.dct_y_z[i][0]), 16 );
                }
            }
        }

        if( t->mb.cbp_c&0x03 )    /* Chroma DC residual present */
        {
            block_residual_write_cabac( t, 3, 0, &(t->mb.dc2x2_z[0][0]), 4 );
            block_residual_write_cabac( t, 3, 1, &(t->mb.dc2x2_z[1][0]), 4 );
        }
        if( t->mb.cbp_c&0x02 ) /* Chroma AC residual present */
        {
            for( i = 0; i < 8; i++ )
            {
                block_residual_write_cabac( t, 4, i, &(t->mb.dct_uv_z[i>>2][i&0x03][1]), 15);
            }
        }
    }
/*
    if( IS_INTRA( i_mb_type ) )
        t->stat.frame.i_itex_bits += bs_pos(s) - i_mb_pos_tex;
    else
        t->stat.frame.i_ptex_bits += bs_pos(s) - i_mb_pos_tex;
*/
}
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
#include "config.h"
#include "stdio.h"
#ifndef CHIP_DM642
#include "memory.h"
#endif

#include "T264.h"
#include "utility.h"
#include "intra.h"
#include "cavlc.h"
#include "inter.h"
#include "inter_b.h"
#include "interpolate.h"
#include "estimation.h"
#include "deblock.h"
#include "ratecontrol.h"
#include "stat.h"
#ifndef CHIP_DM642
#include "sse2.h"
#endif
#include "math.h"
#include "dct.h"
#include "predict.h"
#include "bitstream.h"
#include "rbsp.h"
#include "assert.h"
#include "typedecision.h"
//for CABAC
#include "cabac_engine.h"
//#include "cabac.h"



const int32_t chroma_qp[] =
{
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 
    11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 
    21, 22, 23, 24, 25, 26, 27, 28, 29,
    29, 30, 31, 32, 32, 33, 34, 34, 35, 35,
    36, 36, 37, 37, 37, 38, 38, 38, 39, 39, 39, 39
};

//! convert from H.263 QP to H.26L quant given by: quant=pow(2,QP/6)
static const int32_t qp_cost[52]=
{
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 2, 2, 2, 2,
    3, 3, 3, 4, 4, 4, 5, 6,
    6, 7, 8, 9,10,11,13,14,
    16,18,20,23,25,29,32,36,
    40,45,51,57,64,72,81,91
};

void
T264_mb_load_context(T264_t* t, int32_t mb_y, int32_t mb_x)
{
    int32_t qpc;
    int32_t i, j;

    t->mb.mb_x = mb_x;
    t->mb.mb_y = mb_y;
    t->mb.mb_xy = t->mb.mb_y * t->mb_stride + t->mb.mb_x;
    t->mb.mb_neighbour = 0;
    if (mb_x != 0)
        t->mb.mb_neighbour |= MB_LEFT;
    if (mb_y != 0)
    {
        t->mb.mb_neighbour |= MB_TOP;
        //if (mb_x != t->mb_stride - 1) //just tell MB is not the top-most
            t->mb.mb_neighbour |= MB_TOPRIGHT;
    }
    t->mb.src_y = t->cur.Y[0]  + (mb_y << 4) * t->stride    + (mb_x << 4);
    t->mb.dst_y  = t->rec->Y[0] + (mb_y << 4) * t->edged_stride    + (mb_x << 4);
    t->mb.src_u = t->cur.U     + (mb_y << 3) * t->stride_uv + (mb_x << 3);
    t->mb.dst_u = t->rec->U + (mb_y << 3) * t->edged_stride_uv + (mb_x << 3);
    t->mb.src_v = t->cur.V     + (mb_y << 3) * t->stride_uv + (mb_x << 3);
    t->mb.dst_v = t->rec->V + (mb_y << 3) * t->edged_stride_uv + (mb_x << 3);

    t->mb.mb_qp_delta = 0;
    /* t->ps.chroma_qp_index_offset maybe modify in ratecontrol */
    qpc = clip3(t->ps.chroma_qp_index_offset + t->qp_y, 0, 51);
    t->qp_uv = chroma_qp[qpc];
    t->mb.lambda = qp_cost[t->qp_y];

    t->mb.context = &t->rec->mb[t->mb.mb_xy];

	/*for CABAC, init the mvd_ref */
	memset(&(t->mb.mvd_ref[0][0][0]), 0, sizeof(t->mb.mvd_ref));
#define INITINVALIDVEC(vec) vec.refno = -2; vec.x = vec.y = 0;
    INITINVALIDVEC(t->mb.vec_ref[0].vec[0]);
    INITINVALIDVEC(t->mb.vec_ref[IPM_LUMA - 8 + 4].vec[0]);
    INITINVALIDVEC(t->mb.vec_ref[IPM_LUMA - 8 + 4 + 8].vec[0]);
    INITINVALIDVEC(t->mb.vec_ref[IPM_LUMA - 8 + 4 + 16].vec[0]);
    INITINVALIDVEC(t->mb.vec_ref[IPM_LUMA - 8 + 4 + 24].vec[0]);
    INITINVALIDVEC(t->mb.vec_ref[0].vec[1]);
    INITINVALIDVEC(t->mb.vec_ref[IPM_LUMA - 8 + 4].vec[1]);
    INITINVALIDVEC(t->mb.vec_ref[IPM_LUMA - 8 + 4 + 8].vec[1]);
    INITINVALIDVEC(t->mb.vec_ref[IPM_LUMA - 8 + 4 + 16].vec[1]);
    INITINVALIDVEC(t->mb.vec_ref[IPM_LUMA - 8 + 4 + 24].vec[1]);
    
    t->mb.vec_ref[0].part = -1;
    t->mb.vec_ref[IPM_LUMA - 8 + 4].part      = -1;
    t->mb.vec_ref[IPM_LUMA - 8 + 4 + 8].part  = -1;
    t->mb.vec_ref[IPM_LUMA - 8 + 4 + 16].part = -1;
    t->mb.vec_ref[IPM_LUMA - 8 + 4 + 24].part = -1;
    
    t->mb.vec_ref[0].subpart = -1;
    t->mb.vec_ref[IPM_LUMA - 8 + 4].subpart      = -1;
    t->mb.vec_ref[IPM_LUMA - 8 + 4 + 8].subpart  = -1;
    t->mb.vec_ref[IPM_LUMA - 8 + 4 + 16].subpart = -1;
    t->mb.vec_ref[IPM_LUMA - 8 + 4 + 24].subpart = -1;
	
    memset(t->mb.submb_part, -1, sizeof(uint8_t) * 16);//t->mb.submb_part));
    t->mb.mb_part = -1;
    for(i = 0 ; i < 2 ; i ++)
    {
        for(j = 0 ; j < 16 ; j ++)
        {
            INITINVALIDVEC(t->mb.vec[i][j]);
        }
    }
    t->mb.sad_ref[0] = t->mb.sad_ref[1] = t->mb.sad_ref[2] = 0;

    //intra_4x4 prediction modes and non-zero counts
	if( mb_y > 0 )
    {
        int16_t top_xy  = t->mb.mb_xy - t->mb_stride;
        /* intra 4x4 pred mode layout
         	? x x x x
            x
            x
            x
            x
         */        
        t->mb.i4x4_pred_mode_ref[IPM_LUMA - 8 + 0] = t->rec->mb[top_xy].mode_i4x4[10];
        t->mb.i4x4_pred_mode_ref[IPM_LUMA - 8 + 1] = t->rec->mb[top_xy].mode_i4x4[11];
        t->mb.i4x4_pred_mode_ref[IPM_LUMA - 8 + 2] = t->rec->mb[top_xy].mode_i4x4[14];
        t->mb.i4x4_pred_mode_ref[IPM_LUMA - 8 + 3] = t->rec->mb[top_xy].mode_i4x4[15];

        t->mb.vec_ref[IPM_LUMA - 8 + 0].vec[0] = t->rec->mb[top_xy].vec[0][12];
        t->mb.vec_ref[IPM_LUMA - 8 + 1].vec[0] = t->rec->mb[top_xy].vec[0][13];
        t->mb.vec_ref[IPM_LUMA - 8 + 2].vec[0] = t->rec->mb[top_xy].vec[0][14];
        t->mb.vec_ref[IPM_LUMA - 8 + 3].vec[0] = t->rec->mb[top_xy].vec[0][15];
        t->mb.vec_ref[IPM_LUMA - 8 + 0].vec[1] = t->rec->mb[top_xy].vec[1][12];
        t->mb.vec_ref[IPM_LUMA - 8 + 1].vec[1] = t->rec->mb[top_xy].vec[1][13];
        t->mb.vec_ref[IPM_LUMA - 8 + 2].vec[1] = t->rec->mb[top_xy].vec[1][14];
        t->mb.vec_ref[IPM_LUMA - 8 + 3].vec[1] = t->rec->mb[top_xy].vec[1][15];

        t->mb.vec_ref[IPM_LUMA - 8 + 0].part = 
        t->mb.vec_ref[IPM_LUMA - 8 + 1].part = 
        t->mb.vec_ref[IPM_LUMA - 8 + 2].part = 
        t->mb.vec_ref[IPM_LUMA - 8 + 3].part = t->rec->mb[top_xy].mb_part;

        t->mb.vec_ref[IPM_LUMA - 8 + 0].subpart = t->rec->mb[top_xy].submb_part[12];
        t->mb.vec_ref[IPM_LUMA - 8 + 1].subpart = t->rec->mb[top_xy].submb_part[13];
        t->mb.vec_ref[IPM_LUMA - 8 + 2].subpart = t->rec->mb[top_xy].submb_part[14];
        t->mb.vec_ref[IPM_LUMA - 8 + 3].subpart = t->rec->mb[top_xy].submb_part[15];

        t->mb.sad_ref[1] = t->rec->mb[top_xy].sad;

		//for CABAC, load mvd
		t->mb.mvd_ref[0][IPM_LUMA - 8 + 0][0] = t->rec->mb[top_xy].mvd[0][12][0];
		t->mb.mvd_ref[0][IPM_LUMA - 8 + 0][1] = t->rec->mb[top_xy].mvd[0][12][1];

		t->mb.mvd_ref[0][IPM_LUMA - 8 + 1][0] = t->rec->mb[top_xy].mvd[0][13][0];
		t->mb.mvd_ref[0][IPM_LUMA - 8 + 1][1] = t->rec->mb[top_xy].mvd[0][13][1];

		t->mb.mvd_ref[0][IPM_LUMA - 8 + 2][0] = t->rec->mb[top_xy].mvd[0][14][0];
		t->mb.mvd_ref[0][IPM_LUMA - 8 + 2][1] = t->rec->mb[top_xy].mvd[0][14][1];

		t->mb.mvd_ref[0][IPM_LUMA - 8 + 3][0] = t->rec->mb[top_xy].mvd[0][15][0];
		t->mb.mvd_ref[0][IPM_LUMA - 8 + 3][1] = t->rec->mb[top_xy].mvd[0][15][1];

		t->mb.mvd_ref[1][IPM_LUMA - 8 + 0][0] = t->rec->mb[top_xy].mvd[1][12][0];
		t->mb.mvd_ref[1][IPM_LUMA - 8 + 0][1] = t->rec->mb[top_xy].mvd[1][12][1];

		t->mb.mvd_ref[1][IPM_LUMA - 8 + 1][0] = t->rec->mb[top_xy].mvd[1][13][0];
		t->mb.mvd_ref[1][IPM_LUMA - 8 + 1][1] = t->rec->mb[top_xy].mvd[1][13][1];

		t->mb.mvd_ref[1][IPM_LUMA - 8 + 2][0] = t->rec->mb[top_xy].mvd[1][14][0];
		t->mb.mvd_ref[1][IPM_LUMA - 8 + 2][1] = t->rec->mb[top_xy].mvd[1][14][1];

		t->mb.mvd_ref[1][IPM_LUMA - 8 + 3][0] = t->rec->mb[top_xy].mvd[1][15][0];
		t->mb.mvd_ref[1][IPM_LUMA - 8 + 3][1] = t->rec->mb[top_xy].mvd[1][15][1];

        if (mb_x != t->mb_stride - 1)
        {
            int32_t righttop_xy = top_xy + 1;
            t->mb.vec_ref[IPM_LUMA - 8 + 4].vec[0]     = t->rec->mb[righttop_xy].vec[0][12];
            t->mb.vec_ref[IPM_LUMA - 8 + 4].vec[1]     = t->rec->mb[righttop_xy].vec[1][12];
            t->mb.vec_ref[IPM_LUMA - 8 + 4].part    = t->rec->mb[righttop_xy].mb_part;
            t->mb.vec_ref[IPM_LUMA - 8 + 4].subpart = t->rec->mb[righttop_xy].submb_part[12];
            t->mb.sad_ref[2] = t->rec->mb[righttop_xy].sad;
        }
        /* nnz layout:
          ? x x x x ? x x
          x         x
          x         x
          x         ? x x
          x         x
                    x
         */
        t->mb.nnz_ref[NNZ_LUMA - 8 + 0] = t->rec->mb[top_xy].nnz[12];
        t->mb.nnz_ref[NNZ_LUMA - 8 + 1] = t->rec->mb[top_xy].nnz[13];
        t->mb.nnz_ref[NNZ_LUMA - 8 + 2] = t->rec->mb[top_xy].nnz[14];
        t->mb.nnz_ref[NNZ_LUMA - 8 + 3] = t->rec->mb[top_xy].nnz[15];

        t->mb.nnz_ref[NNZ_CHROMA0 - 8 + 0] = t->rec->mb[top_xy].nnz[18];
        t->mb.nnz_ref[NNZ_CHROMA0 - 8 + 1] = t->rec->mb[top_xy].nnz[19];
        t->mb.nnz_ref[NNZ_CHROMA1 - 8 + 0] = t->rec->mb[top_xy].nnz[22];
        t->mb.nnz_ref[NNZ_CHROMA1 - 8 + 1] = t->rec->mb[top_xy].nnz[23];
    }
    else
    {
        /* load intra4x4 */
        t->mb.i4x4_pred_mode_ref[IPM_LUMA - 8 + 0] = 
        t->mb.i4x4_pred_mode_ref[IPM_LUMA - 8 + 1] = 
        t->mb.i4x4_pred_mode_ref[IPM_LUMA - 8 + 2] = 
        t->mb.i4x4_pred_mode_ref[IPM_LUMA - 8 + 3] = -1;

        INITINVALIDVEC(t->mb.vec_ref[IPM_LUMA - 8 + 0].vec[0]);
        INITINVALIDVEC(t->mb.vec_ref[IPM_LUMA - 8 + 1].vec[0]);
        INITINVALIDVEC(t->mb.vec_ref[IPM_LUMA - 8 + 2].vec[0]);
        INITINVALIDVEC(t->mb.vec_ref[IPM_LUMA - 8 + 3].vec[0]);
        INITINVALIDVEC(t->mb.vec_ref[IPM_LUMA - 8 + 0].vec[1]);
        INITINVALIDVEC(t->mb.vec_ref[IPM_LUMA - 8 + 1].vec[1]);
        INITINVALIDVEC(t->mb.vec_ref[IPM_LUMA - 8 + 2].vec[1]);
        INITINVALIDVEC(t->mb.vec_ref[IPM_LUMA - 8 + 3].vec[1]);

        t->mb.vec_ref[IPM_LUMA - 8 + 0].part = 
        t->mb.vec_ref[IPM_LUMA - 8 + 1].part = 
        t->mb.vec_ref[IPM_LUMA - 8 + 2].part = 
        t->mb.vec_ref[IPM_LUMA - 8 + 3].part = -1;

        t->mb.vec_ref[IPM_LUMA - 8 + 0].subpart = 
        t->mb.vec_ref[IPM_LUMA - 8 + 1].subpart = 
        t->mb.vec_ref[IPM_LUMA - 8 + 2].subpart = 
        t->mb.vec_ref[IPM_LUMA - 8 + 3].subpart = -1;

		//for CABAC, load mvd
		t->mb.mvd_ref[0][IPM_LUMA - 8 + 0][0] = 0;
		t->mb.mvd_ref[0][IPM_LUMA - 8 + 0][1] = 0;

		t->mb.mvd_ref[0][IPM_LUMA - 8 + 1][0] = 0;
		t->mb.mvd_ref[0][IPM_LUMA - 8 + 1][1] = 0;

		t->mb.mvd_ref[0][IPM_LUMA - 8 + 2][0] = 0;
		t->mb.mvd_ref[0][IPM_LUMA - 8 + 2][1] = 0;

		t->mb.mvd_ref[0][IPM_LUMA - 8 + 3][0] = 0;
		t->mb.mvd_ref[0][IPM_LUMA - 8 + 3][1] = 0;

		t->mb.mvd_ref[1][IPM_LUMA - 8 + 0][0] = 0;
		t->mb.mvd_ref[1][IPM_LUMA - 8 + 0][1] = 0;

		t->mb.mvd_ref[1][IPM_LUMA - 8 + 1][0] = 0;
		t->mb.mvd_ref[1][IPM_LUMA - 8 + 1][1] = 0;

		t->mb.mvd_ref[1][IPM_LUMA - 8 + 2][0] = 0;
		t->mb.mvd_ref[1][IPM_LUMA - 8 + 2][1] = 0;

		t->mb.mvd_ref[1][IPM_LUMA - 8 + 3][0] = 0;
		t->mb.mvd_ref[1][IPM_LUMA - 8 + 3][1] = 0;

        t->mb.nnz_ref[NNZ_LUMA - 8 + 0] =
        t->mb.nnz_ref[NNZ_LUMA - 8 + 1] =
        t->mb.nnz_ref[NNZ_LUMA - 8 + 2] =
        t->mb.nnz_ref[NNZ_LUMA - 8 + 3] = 0x80;

        t->mb.nnz_ref[NNZ_CHROMA0 - 8 + 0] =
        t->mb.nnz_ref[NNZ_CHROMA0 - 8 + 1] =
        t->mb.nnz_ref[NNZ_CHROMA1 - 8 + 0] =
        t->mb.nnz_ref[NNZ_CHROMA1 - 8 + 1] = 0x80;
    }

    if( mb_x > 0 )
    {
        int16_t left_xy  = t->mb.mb_xy - 1;

        /* load intra4x4 */
        t->mb.i4x4_pred_mode_ref[IPM_LUMA - 1 + 0] = t->rec->mb[left_xy].mode_i4x4[5];
        t->mb.i4x4_pred_mode_ref[IPM_LUMA - 1 + 8] = t->rec->mb[left_xy].mode_i4x4[7];
        t->mb.i4x4_pred_mode_ref[IPM_LUMA - 1 + 16] = t->rec->mb[left_xy].mode_i4x4[13];
        t->mb.i4x4_pred_mode_ref[IPM_LUMA - 1 + 24] = t->rec->mb[left_xy].mode_i4x4[15];

        t->mb.vec_ref[IPM_LUMA - 1 + 0].vec[0] = t->rec->mb[left_xy].vec[0][3];
        t->mb.vec_ref[IPM_LUMA - 1 + 8].vec[0] = t->rec->mb[left_xy].vec[0][7];
        t->mb.vec_ref[IPM_LUMA - 1 + 16].vec[0] = t->rec->mb[left_xy].vec[0][11];
        t->mb.vec_ref[IPM_LUMA - 1 + 24].vec[0] = t->rec->mb[left_xy].vec[0][15];
        t->mb.vec_ref[IPM_LUMA - 1 + 0].vec[1] = t->rec->mb[left_xy].vec[1][3];
        t->mb.vec_ref[IPM_LUMA - 1 + 8].vec[1] = t->rec->mb[left_xy].vec[1][7];
        t->mb.vec_ref[IPM_LUMA - 1 + 16].vec[1] = t->rec->mb[left_xy].vec[1][11];
        t->mb.vec_ref[IPM_LUMA - 1 + 24].vec[1] = t->rec->mb[left_xy].vec[1][15];

        t->mb.vec_ref[IPM_LUMA - 1 + 0].part = 
        t->mb.vec_ref[IPM_LUMA - 1 + 8].part = 
        t->mb.vec_ref[IPM_LUMA - 1 + 16].part =
        t->mb.vec_ref[IPM_LUMA - 1 + 24].part = t->rec->mb[left_xy].mb_part;

        t->mb.vec_ref[IPM_LUMA - 8 + 0].subpart = t->rec->mb[left_xy].submb_part[3];
        t->mb.vec_ref[IPM_LUMA - 8 + 8].subpart = t->rec->mb[left_xy].submb_part[7];
        t->mb.vec_ref[IPM_LUMA - 8 + 16].subpart = t->rec->mb[left_xy].submb_part[11];
        t->mb.vec_ref[IPM_LUMA - 8 + 24].subpart = t->rec->mb[left_xy].submb_part[15];

        t->mb.sad_ref[0] = t->rec->mb[left_xy].sad;

		//for CABAC, load mvd
		t->mb.mvd_ref[0][IPM_LUMA - 1 + 0][0] = t->rec->mb[left_xy].mvd[0][3][0];
		t->mb.mvd_ref[0][IPM_LUMA - 1 + 0][1] = t->rec->mb[left_xy].mvd[0][3][1];

		t->mb.mvd_ref[0][IPM_LUMA - 1 + 8][0] = t->rec->mb[left_xy].mvd[0][7][0];
		t->mb.mvd_ref[0][IPM_LUMA - 1 + 8][1] = t->rec->mb[left_xy].mvd[0][7][1];
		
		t->mb.mvd_ref[0][IPM_LUMA - 1 + 16][0] = t->rec->mb[left_xy].mvd[0][11][0];
		t->mb.mvd_ref[0][IPM_LUMA - 1 + 16][1] = t->rec->mb[left_xy].mvd[0][11][1];

		t->mb.mvd_ref[0][IPM_LUMA - 1 + 24][0] = t->rec->mb[left_xy].mvd[0][15][0];
		t->mb.mvd_ref[0][IPM_LUMA - 1 + 24][1] = t->rec->mb[left_xy].mvd[0][15][1];

		t->mb.mvd_ref[1][IPM_LUMA - 1 + 0][0] = t->rec->mb[left_xy].mvd[1][3][0];
		t->mb.mvd_ref[1][IPM_LUMA - 1 + 0][1] = t->rec->mb[left_xy].mvd[1][3][1];

		t->mb.mvd_ref[1][IPM_LUMA - 1 + 8][0] = t->rec->mb[left_xy].mvd[1][7][0];
		t->mb.mvd_ref[1][IPM_LUMA - 1 + 8][1] = t->rec->mb[left_xy].mvd[1][7][1];

		t->mb.mvd_ref[1][IPM_LUMA - 1 + 16][0] = t->rec->mb[left_xy].mvd[1][11][0];
		t->mb.mvd_ref[1][IPM_LUMA - 1 + 16][1] = t->rec->mb[left_xy].mvd[1][11][1];

		t->mb.mvd_ref[1][IPM_LUMA - 1 + 24][0] = t->rec->mb[left_xy].mvd[1][15][0];
		t->mb.mvd_ref[1][IPM_LUMA - 1 + 24][1] = t->rec->mb[left_xy].mvd[1][15][1];


        /* load non_zero_count */
        t->mb.nnz_ref[NNZ_LUMA - 1 + 0] = t->rec->mb[left_xy].nnz[3];
        t->mb.nnz_ref[NNZ_LUMA - 1 + 8] = t->rec->mb[left_xy].nnz[7];
        t->mb.nnz_ref[NNZ_LUMA - 1 + 16] = t->rec->mb[left_xy].nnz[11];
        t->mb.nnz_ref[NNZ_LUMA - 1 + 24] = t->rec->mb[left_xy].nnz[15];

        t->mb.nnz_ref[NNZ_CHROMA0 - 1 + 0] = t->rec->mb[left_xy].nnz[17];
        t->mb.nnz_ref[NNZ_CHROMA0 - 1 + 8] = t->rec->mb[left_xy].nnz[19];
        t->mb.nnz_ref[NNZ_CHROMA1 - 1 + 0] = t->rec->mb[left_xy].nnz[21];
        t->mb.nnz_ref[NNZ_CHROMA1 - 1 + 8] = t->rec->mb[left_xy].nnz[23];
    }
    else
    {
        t->mb.i4x4_pred_mode_ref[IPM_LUMA - 1 + 0]  = 
        t->mb.i4x4_pred_mode_ref[IPM_LUMA - 1 + 8]  = 
        t->mb.i4x4_pred_mode_ref[IPM_LUMA - 1 + 16] =
        t->mb.i4x4_pred_mode_ref[IPM_LUMA - 1 + 24] = -1;

        INITINVALIDVEC(t->mb.vec_ref[IPM_LUMA - 1 + 0].vec[0]);
        INITINVALIDVEC(t->mb.vec_ref[IPM_LUMA - 1 + 8].vec[0]);
        INITINVALIDVEC(t->mb.vec_ref[IPM_LUMA - 1 + 16].vec[0]);
        INITINVALIDVEC(t->mb.vec_ref[IPM_LUMA - 1 + 24].vec[0]);
        INITINVALIDVEC(t->mb.vec_ref[IPM_LUMA - 1 + 0].vec[1]);
        INITINVALIDVEC(t->mb.vec_ref[IPM_LUMA - 1 + 8].vec[1]);
        INITINVALIDVEC(t->mb.vec_ref[IPM_LUMA - 1 + 16].vec[1]);
        INITINVALIDVEC(t->mb.vec_ref[IPM_LUMA - 1 + 24].vec[1]);

        t->mb.vec_ref[IPM_LUMA - 1 + 0].part  = 
        t->mb.vec_ref[IPM_LUMA - 1 + 8].part  = 
        t->mb.vec_ref[IPM_LUMA - 1 + 16].part =
        t->mb.vec_ref[IPM_LUMA - 1 + 24].part = -1;

        t->mb.vec_ref[IPM_LUMA - 1 + 0].subpart  = 
        t->mb.vec_ref[IPM_LUMA - 1 + 8].subpart  = 
        t->mb.vec_ref[IPM_LUMA - 1 + 16].subpart =
        t->mb.vec_ref[IPM_LUMA - 1 + 24].subpart = -1;

		//for CABAC, load mvd
		t->mb.mvd_ref[0][IPM_LUMA - 1 + 0][0] = 0;
		t->mb.mvd_ref[0][IPM_LUMA - 1 + 0][1] = 0;

		t->mb.mvd_ref[0][IPM_LUMA - 1 + 8][0] = 0;
		t->mb.mvd_ref[0][IPM_LUMA - 1 + 8][1] = 0;

		t->mb.mvd_ref[0][IPM_LUMA - 1 + 16][0] = 0;
		t->mb.mvd_ref[0][IPM_LUMA - 1 + 16][1] = 0;

		t->mb.mvd_ref[0][IPM_LUMA - 1 + 24][0] = 0;
		t->mb.mvd_ref[0][IPM_LUMA - 1 + 24][1] = 0;

		t->mb.mvd_ref[1][IPM_LUMA - 1 + 0][0] = 0;
		t->mb.mvd_ref[1][IPM_LUMA - 1 + 0][1] = 0;

		t->mb.mvd_ref[1][IPM_LUMA - 1 + 8][0] = 0;
		t->mb.mvd_ref[1][IPM_LUMA - 1 + 8][1] = 0;

		t->mb.mvd_ref[1][IPM_LUMA - 1 + 16][0] = 0;
		t->mb.mvd_ref[1][IPM_LUMA - 1 + 16][1] = 0;

		t->mb.mvd_ref[1][IPM_LUMA - 1 + 24][0] = 0;
		t->mb.mvd_ref[1][IPM_LUMA - 1 + 24][1] = 0;

        t->mb.nnz_ref[NNZ_LUMA - 1 + 0]  =
        t->mb.nnz_ref[NNZ_LUMA - 1 + 8]  =
        t->mb.nnz_ref[NNZ_LUMA - 1 + 16] =
        t->mb.nnz_ref[NNZ_LUMA - 1 + 24] = 0x80;

        t->mb.nnz_ref[NNZ_CHROMA0 - 1 + 0] =
        t->mb.nnz_ref[NNZ_CHROMA0 - 1 + 8] =
        t->mb.nnz_ref[NNZ_CHROMA1 - 1 + 0] =
        t->mb.nnz_ref[NNZ_CHROMA1 - 1 + 8] = 0x80;
    }
    if (mb_x > 0 && mb_y > 0)
    {
        int32_t lefttop_xy = t->mb.mb_xy - t->mb_stride - 1;
        t->mb.vec_ref[0].vec[0] = t->rec->mb[lefttop_xy].vec[0][15];
        t->mb.vec_ref[0].vec[1] = t->rec->mb[lefttop_xy].vec[1][15];
        t->mb.vec_ref[0].subpart = t->rec->mb[lefttop_xy].submb_part[15];
        t->mb.vec_ref[0].part = t->rec->mb[lefttop_xy].mb_part;
    }
	//for CABAC
	t->mb.mb_mode_uv = Intra_8x8_DC;
#undef INITINVALIDVEC
}

void
T264_mb_save_context(T264_t* t)
{
    memcpy(t->mb.context, &t->mb, sizeof(*t->mb.context));
}

static void
T264_reset_ref(T264_t* t)
{
    int32_t i;

    for(i = 1 ; i < MAX_REFFRAMES ; i ++)
    {
        t->refn[i].poc = -1;
    }
    t->rec = &t->refn[0];
    t->refn[0].poc = 0;
}

static void
T264_load_ref(T264_t* t)
{
    int32_t i;

    t->refl0_num = 0;
    t->refl1_num = 0;
    if (t->slice_type == SLICE_P)
    {
        for(i = 0 ; i < t->param.ref_num ; i ++)
        {
            if (t->refn[i + 1].poc >= 0)
            {
                t->ref[0][t->refl0_num ++] = &t->refn[i + 1];
            }
        }
    }
    else if (t->slice_type == SLICE_B)
    {
        for(i = 0 ; i < t->param.ref_num; i ++)
        {
            if (t->refn[i + 1].poc < t->cur.poc)
            {
                if (t->refn[i + 1].poc >= 0)
                    t->ref[0][t->refl0_num ++] = &t->refn[i + 1];
            }
            else
            {
                // yes, t->refn[i].poc already > 0
                t->ref[1][t->refl1_num ++] = &t->refn[i + 1];
            }
        }
    }
}

void
T264_extend_border(T264_t* t, T264_frame_t* f)
{
    int32_t i;
    uint8_t* py0;
    uint8_t* pu;
    uint8_t* pv;
    uint8_t* tmpy0;
    uint8_t* tmpu;
    uint8_t* tmpv;

    // top, top-left, top-right
    py0 = f->Y[0] - t->edged_stride;
    pu = f->U - t->edged_stride_uv;
    pv = f->V - t->edged_stride_uv;
    for(i = 0 ; i < (EDGED_HEIGHT >> 1) ; i ++)
    {
        // y
        memcpy(py0, f->Y[0], t->stride);
        memset(py0 - EDGED_WIDTH, f->Y[0][0], EDGED_WIDTH);
        memset(py0 + t->stride, f->Y[0][t->stride - 1], EDGED_WIDTH);
        py0 -= t->edged_stride;

        memcpy(py0, f->Y[0], t->stride);
        memset(py0 - EDGED_WIDTH, f->Y[0][0], EDGED_WIDTH);
        memset(py0 + t->stride, f->Y[0][t->stride - 1], EDGED_WIDTH);
        py0 -= t->edged_stride;

        // u
        memcpy(pu, f->U, t->stride_uv);
        memset(pu - (EDGED_WIDTH >> 1), f->U[0], EDGED_WIDTH >> 1);
        memset(pu + t->stride_uv, f->U[t->stride_uv - 1], EDGED_WIDTH >> 1);
        pu -= t->edged_stride_uv;

        // V
        memcpy(pv, f->V, t->stride_uv);
        memset(pv - (EDGED_WIDTH >> 1), f->V[0], EDGED_WIDTH >> 1);
        memset(pv + t->stride_uv, f->V[t->stride_uv - 1], EDGED_WIDTH >> 1);
        pv -= t->edged_stride_uv;
    }

    // left & right
    py0 = f->Y[0] - EDGED_WIDTH;
    pu = f->U - (EDGED_WIDTH >> 1);
    pv = f->V - (EDGED_WIDTH >> 1);
    for(i = 0 ; i < (t->height >> 1) ; i ++)
    {
        // left
        memset(py0, py0[EDGED_WIDTH], EDGED_WIDTH);
        // right
        memset(&py0[t->stride + EDGED_WIDTH], py0[t->stride + EDGED_WIDTH - 1], EDGED_WIDTH);
        py0 += t->edged_stride;

        memset(py0, py0[EDGED_WIDTH], EDGED_WIDTH);
        memset(&py0[t->stride + EDGED_WIDTH], py0[t->stride + EDGED_WIDTH - 1], EDGED_WIDTH);
        py0 += t->edged_stride;

        // u
        memset(pu, pu[EDGED_WIDTH >> 1], EDGED_WIDTH >> 1);
        memset(&pu[t->stride_uv + (EDGED_WIDTH >> 1)], pu[t->stride_uv + (EDGED_WIDTH >> 1) - 1], EDGED_WIDTH >> 1);
        pu += t->edged_stride_uv;

        // v
        memset(pv, pv[EDGED_WIDTH >> 1], EDGED_WIDTH >> 1);
        memset(&pv[t->stride_uv + (EDGED_WIDTH >> 1)], pv[t->stride_uv + (EDGED_WIDTH >> 1) - 1], EDGED_WIDTH >> 1);
        pv += t->edged_stride_uv;
    }

    // bottom, left-bottom,right-bottom
    py0 = f->Y[0] + t->edged_stride * t->height;
    tmpy0 = f->Y[0] + t->edged_stride * (t->height - 1);
    pu = f->U + t->edged_stride_uv * (t->height >> 1);
    tmpu = f->U + t->edged_stride_uv * ((t->height >> 1) - 1);
    pv = f->V + t->edged_stride_uv * (t->height >> 1);
    tmpv = f->V + t->edged_stride_uv * ((t->height >> 1)- 1);
    for(i = 0 ; i < (EDGED_HEIGHT >> 1) ; i ++)
    {
        // y
        memcpy(py0, tmpy0, t->stride);
        memset(py0 - EDGED_WIDTH, tmpy0[0], EDGED_WIDTH);
        memset(py0 + t->stride, tmpy0[t->stride - 1], EDGED_WIDTH);
        py0 += t->edged_stride;

        memcpy(py0, tmpy0, t->stride);
        memset(py0 - EDGED_WIDTH, tmpy0[0], EDGED_WIDTH);
        memset(py0 + t->stride, tmpy0[t->stride - 1], EDGED_WIDTH);
        py0 += t->edged_stride;

        // u
        memcpy(pu, tmpu, t->stride_uv);
        memset(pu - (EDGED_WIDTH >> 1), tmpu[0], EDGED_WIDTH >> 1);
        memset(pu + t->stride_uv, tmpu[t->stride_uv - 1], EDGED_WIDTH >> 1);
        pu += t->edged_stride_uv;

        // v
        memcpy(pv, tmpv, t->stride_uv);
        memset(pv - (EDGED_WIDTH >> 1), tmpv[0], EDGED_WIDTH >> 1);
        memset(pv + t->stride_uv, tmpv[t->stride_uv - 1], EDGED_WIDTH >> 1);
        pv += t->edged_stride_uv;
    }
}

void
T264_interpolate_halfpel(T264_t* t, T264_frame_t* f)
{
    int32_t src_offset;
    int32_t width, height;

    if (t->flags & (USE_HALFPEL| USE_QUARTPEL))
    {
        src_offset = - 3;
        width      = t->width + 3 + 2;
        height     = t->height;
        t->interpolate_halfpel_h(f->Y[0] + src_offset, t->edged_stride, f->Y[1] + src_offset, t->edged_stride, width, height);
        // extend border
        {
            uint8_t* src, *dst;
            int32_t i;
            // left & right
            dst = f->Y[1] - EDGED_WIDTH;
            src = f->Y[1] - 3;
            for(i = 0 ; i < t->height ; i ++)
            {
                // left
                memset(dst, src[0], EDGED_WIDTH - 3);
                // right
                memset(&dst[t->stride + EDGED_WIDTH + 2], src[t->stride - 1 + 3 + 2], EDGED_WIDTH - 2);
                dst += t->edged_stride;
                src += t->edged_stride;
            }
            // top
            dst = f->Y[1] - EDGED_HEIGHT * t->edged_stride - EDGED_WIDTH;
            src = f->Y[1] - EDGED_WIDTH;
            for(i = 0 ; i < EDGED_HEIGHT ; i ++)
            {
                memcpy(dst, src, t->edged_stride);
                dst += t->edged_stride;
            }
            // bottom
            src = f->Y[1] + (t->height - 1) * t->edged_stride - EDGED_WIDTH;
            dst = src + t->edged_stride;
            for(i = 0 ; i < EDGED_HEIGHT ; i ++)
            {
                memcpy(dst, src, t->edged_stride);
                dst += t->edged_stride;
            }
        }
        src_offset = - 3 * t->edged_stride;
        width      = t->width;
        height     = t->height + 3 + 2;
        t->interpolate_halfpel_v(f->Y[0] + src_offset, t->edged_stride, f->Y[2] + src_offset, t->edged_stride, width, height);
        // extend border
        {
            uint8_t* src, *dst;
            int32_t i;
            // left & right
            dst = f->Y[2] - 3 * t->edged_stride - EDGED_WIDTH;
            src = f->Y[2] - 3 * t->edged_stride;
            for(i = 0 ; i < t->height + 3 + 2 ; i ++)
            {
                // left
                memset(dst, src[0], EDGED_WIDTH);
                // right
                memset(&dst[t->stride + EDGED_WIDTH], src[t->stride - 1], EDGED_WIDTH);
                dst += t->edged_stride;
                src += t->edged_stride;
            }
            // top
            dst = f->Y[2] - EDGED_HEIGHT * t->edged_stride - EDGED_WIDTH;
            src = f->Y[2] - 3 * t->edged_stride - EDGED_WIDTH;
            for(i = 0 ; i < EDGED_HEIGHT - 3 ; i ++)
            {
                memcpy(dst, src, t->edged_stride);
                dst += t->edged_stride;
            }
            // bottom
            src = f->Y[2] + (t->height + 2 - 1) * t->edged_stride - EDGED_WIDTH;
            dst = src + t->edged_stride;
            for(i = 0 ; i < EDGED_HEIGHT - 2 ; i ++)
            {
                memcpy(dst, src, t->edged_stride);
                dst += t->edged_stride;
            }
        }
        if (t->flags & USE_FASTINTERPOLATE)
        {
            // NOTE: here just offset -3 is not enough to complete reverting the origin implemention
            //   in Y[2] its border is already extend 3 + 2, the idea border in Y[3] is -3 -3 2 2
            // If u use USE_FASTINTERPOLATE we prefer the speed is the first condition we do not extend the border further
            src_offset = - 3;
            width      = t->width + 3 + 2;
            height     = t->height;
            t->interpolate_halfpel_h(f->Y[2] + src_offset, t->edged_stride, f->Y[3] + src_offset, t->edged_stride, width, height);
            // extend border
            {
                uint8_t* src, *dst;
                int32_t i;
                // left & right
                dst = f->Y[3] - EDGED_WIDTH;
                src = f->Y[3] - 3;
                for(i = 0 ; i < t->height ; i ++)
                {
                    // left
                    memset(dst, src[0], EDGED_WIDTH - 3);
                    // right
                    memset(&dst[t->stride + EDGED_WIDTH + 2], src[t->stride - 1 + 3 + 2], EDGED_WIDTH - 2);
                    dst += t->edged_stride;
                    src += t->edged_stride;
                }
                // top
                dst = f->Y[3] - EDGED_HEIGHT * t->edged_stride - EDGED_WIDTH;
                src = f->Y[3] - EDGED_WIDTH;
                for(i = 0 ; i < EDGED_HEIGHT ; i ++)
                {
                    memcpy(dst, src, t->edged_stride);
                    dst += t->edged_stride;
                }
                // bottom
                src = f->Y[3] + (t->height - 1) * t->edged_stride - EDGED_WIDTH;
                dst = src + t->edged_stride;
                for(i = 0 ; i < EDGED_HEIGHT ; i ++)
                {
                    memcpy(dst, src, t->edged_stride);
                    dst += t->edged_stride;
                }
            }
        }
        else
        {
            src_offset = - 3 * t->edged_stride - 3;
            width      = t->width + 3 + 2;
            height     = t->height + 3 + 2;
            t->interpolate_halfpel_hv(f->Y[0] + src_offset, t->edged_stride, f->Y[3] + src_offset, t->edged_stride, width, height);
            // extend border
            {
                uint8_t* src, *dst;
                int32_t i;
                // left & right
                dst = f->Y[3] - 3 * t->edged_stride - EDGED_WIDTH;
                src = f->Y[3] - 3 * t->edged_stride - 3;
                for(i = 0 ; i < t->height + 3 + 2 ; i ++)
                {
                    // left
                    memset(dst, src[0], EDGED_WIDTH - 3);
                    // right
                    memset(&dst[t->stride + EDGED_WIDTH + 2], src[t->stride - 1 + 3 + 2], EDGED_WIDTH - 2);
                    dst += t->edged_stride;
                    src += t->edged_stride;
                }
                // top
                dst = f->Y[3] - EDGED_HEIGHT * t->edged_stride - EDGED_WIDTH;
                src = f->Y[3] - 3 * t->edged_stride - EDGED_WIDTH;
                for(i = 0 ; i < EDGED_HEIGHT - 3 ; i ++)
                {
                    memcpy(dst, src, t->edged_stride);
                    dst += t->edged_stride;
                }
                // bottom
                src = f->Y[3] + (t->height + 2 - 1) * t->edged_stride - EDGED_WIDTH;
                dst = src + t->edged_stride;
                for(i = 0 ; i < EDGED_HEIGHT - 2 ; i ++)
                {
                    memcpy(dst, src, t->edged_stride);
                    dst += t->edged_stride;
                }
            }
        }
    }
}

static void
T264_save_ref(T264_t* t)
{
    int32_t i;
    T264_frame_t tmp;
    /* deblock filter exec here */
    if (t->param.disable_filter == 0)
        T264_deblock_frame(t, t->rec);
    /* current only del with i,p */
    T264_extend_border(t, t->rec);
    T264_interpolate_halfpel(t, t->rec);

    tmp = t->refn[t->param.ref_num];
    t->refn[0].poc = t->poc;
    for(i = t->param.ref_num ; i >= 1 ; i --)
    {
        t->refn[i] = t->refn[i - 1];
    }

    t->refn[0] = tmp;
    t->rec = &t->refn[0];
}

void
T264_mb_mode_decision(T264_t* t)
{
    if(t->slice_type == SLICE_P)
    {
        T264_mode_decision_interp_y(t);
    }
    else if(t->slice_type == SLICE_B)
    {
        T264_mode_decision_interb_y(t);
    }
    else if (t->slice_type == SLICE_I)
    {
        T264_mode_decision_intra_y(t);
    }
}

void
T264_mb_encode(T264_t* t)
{
    if(t->mb.mb_mode == P_MODE)
    {
        T264_encode_inter_y(t);
        T264_encode_inter_uv(t);

        t->stat.p_block_num[t->mb.mb_part] ++;
    }
    else if(t->mb.mb_mode == P_SKIP)
    {
        t->stat.skip_block_num++;
    }
    else if (t->mb.mb_mode == B_MODE)
    {
        T264_encode_inter_y(t);
        T264_encode_interb_uv(t);

        t->stat.p_block_num[0] ++;
    }
    else if (t->mb.mb_mode == I_4x4 || t->mb.mb_mode == I_16x16)
    {
        T264_encode_intra_y(t);

        //
        // Chroma
        //
        T264_mode_decision_intra_uv(t);
        T264_encode_intra_uv(t);

        t->stat.i_block_num[t->mb.mb_mode] ++;
    }
}

static void
T264_emms_c()
{
}

void
T264_init_cpu(T264_t* t)
{
#ifndef CHIP_DM642
    if ((t->param.cpu & T264_CPU_FORCE) != T264_CPU_FORCE)
    {
        t->param.cpu = T264_detect_cpu(); 
    }
#endif
    t->pred16x16[Intra_16x16_TOP]    = T264_predict_16x16_mode_0_c;
    t->pred16x16[Intra_16x16_LEFT]   = T264_predict_16x16_mode_1_c;
    t->pred16x16[Intra_16x16_DC]     = T264_predict_16x16_mode_2_c;
    t->pred16x16[Intra_16x16_PLANE]  = T264_predict_16x16_mode_3_c;
    t->pred16x16[Intra_16x16_DCTOP]  = T264_predict_16x16_mode_20_c;
    t->pred16x16[Intra_16x16_DCLEFT] = T264_predict_16x16_mode_21_c;
    t->pred16x16[Intra_16x16_DC128]  = T264_predict_16x16_mode_22_c;
    
    t->pred8x8[Intra_8x8_TOP]    = T264_predict_8x8_mode_0_c;
    t->pred8x8[Intra_8x8_LEFT]   = T264_predict_8x8_mode_1_c;
    t->pred8x8[Intra_8x8_DC]     = T264_predict_8x8_mode_2_c;
    t->pred8x8[Intra_8x8_PLANE]  = T264_predict_8x8_mode_3_c;
    t->pred8x8[Intra_8x8_DCTOP]  = T264_predict_8x8_mode_20_c;
    t->pred8x8[Intra_8x8_DCLEFT] = T264_predict_8x8_mode_21_c;
    t->pred8x8[Intra_8x8_DC128]  = T264_predict_8x8_mode_22_c;

    t->pred4x4[Intra_4x4_TOP]    = T264_predict_4x4_mode_0_c;
    t->pred4x4[Intra_4x4_LEFT]   = T264_predict_4x4_mode_1_c;
    t->pred4x4[Intra_4x4_DC]     = T264_predict_4x4_mode_2_c;
    t->pred4x4[Intra_4x4_DCTOP]  = T264_predict_4x4_mode_20_c;
    t->pred4x4[Intra_4x4_DCLEFT] = T264_predict_4x4_mode_21_c;
    t->pred4x4[Intra_4x4_DC128]  = T264_predict_4x4_mode_22_c;

    t->pred4x4[Intra_4x4_DIAGONAL_DOWNLEFT]  = T264_predict_4x4_mode_3_c;
    t->pred4x4[Intra_4x4_DIAGONAL_DOWNRIGHT]  = T264_predict_4x4_mode_4_c;
    t->pred4x4[Intra_4x4_VERTICAL_RIGHT]  = T264_predict_4x4_mode_5_c;
    t->pred4x4[Intra_4x4_HORIZONTAL_DOWN]  = T264_predict_4x4_mode_6_c;
    t->pred4x4[Intra_4x4_VERTICAL_LEFT]  = T264_predict_4x4_mode_7_c;
    t->pred4x4[Intra_4x4_HORIZONTAL_UP]  = T264_predict_4x4_mode_8_c;

    if (t->flags & USE_SAD)
    {
        t->cmp[MB_16x16] = T264_sad_u_16x16_c;
        t->cmp[MB_16x8]  = T264_sad_u_16x8_c;
        t->cmp[MB_8x16]  = T264_sad_u_8x16_c;
        t->cmp[MB_8x8]   = T264_sad_u_8x8_c;
        t->cmp[MB_8x4]   = T264_sad_u_8x4_c;
        t->cmp[MB_4x8]   = T264_sad_u_4x8_c;
        t->cmp[MB_4x4]   = T264_sad_u_4x4_c;
    }
    else
    {
        t->cmp[MB_16x16] = T264_satd_u_16x16_c;
        t->cmp[MB_16x8]  = T264_satd_u_16x8_c;
        t->cmp[MB_8x16]  = T264_satd_u_8x16_c;
        t->cmp[MB_8x8]   = T264_satd_u_8x8_c;
        t->cmp[MB_8x4]   = T264_satd_u_8x4_c;
        t->cmp[MB_4x8]   = T264_satd_u_4x8_c;
        t->cmp[MB_4x4]   = T264_satd_u_4x4_c;
    }

    t->sad[MB_16x16] = T264_sad_u_16x16_c;
    t->sad[MB_16x8]  = T264_sad_u_16x8_c;
    t->sad[MB_8x16]  = T264_sad_u_8x16_c;
    t->sad[MB_8x8]   = T264_sad_u_8x8_c;
    t->sad[MB_8x4]   = T264_sad_u_8x4_c;
    t->sad[MB_4x8]   = T264_sad_u_4x8_c;
    t->sad[MB_4x4]   = T264_sad_u_4x4_c;
    t->fdct4x4   = dct4x4_c;
    t->fdct4x4dc = dct4x4dc_c;
    t->fdct2x2dc = dct2x2dc_c;
    t->idct4x4   = idct4x4_c;
    t->idct4x4dc = idct4x4dc_c;
    t->idct2x2dc = idct2x2dc_c;

    t->quant4x4    = quant4x4_c;
    t->quant4x4dc  = quant4x4dc_c;
    t->quant2x2dc  = quant2x2dc_c;
    t->iquant4x4   = iquant4x4_c;
    t->iquant4x4dc = iquant4x4dc_c;
    t->iquant2x2dc = iquant2x2dc_c;

    t->expand8to16   = expand8to16_c;
    t->contract16to8 = contract16to8_c;
    t->contract16to8add = contract16to8add_c;
    t->expand8to16sub   = expand8to16sub_c;
    t->memcpy_stride_u = memcpy_stride_u_c;
    t->eighth_pixel_mc_u = T264_eighth_pixel_mc_u_c;

    t->interpolate_halfpel_h = interpolate_halfpel_h_c;
    t->interpolate_halfpel_v = interpolate_halfpel_v_c;
    t->interpolate_halfpel_hv = interpolate_halfpel_hv_c;

    //t->pixel_avg = T264_pixel_avg_c;  //modify by wushangyun for pia optimization
    t->pia[MB_16x16] = T264_pia_u_16x16_c;
    t->pia[MB_16x8]  = T264_pia_u_16x8_c;
    t->pia[MB_8x16]  = T264_pia_u_8x16_c;
    t->pia[MB_8x8]   = T264_pia_u_8x8_c;
    t->pia[MB_8x4]   = T264_pia_u_8x4_c;
    t->pia[MB_4x8]   = T264_pia_u_4x8_c;
    t->pia[MB_4x4]   = T264_pia_u_4x4_c;
    t->pia[MB_2x2]   = T264_pia_u_2x2_c;

    t->T264_satd_16x16_u = T264_satd_i16x16_u_c;
    t->emms = T264_emms_c;
    
    // flags relative
    if (t->flags & USE_FULLSEARCH)
        t->search = T264_spiral_search_full;
    else if (t->flags & USE_DIAMONDSEACH)
        t->search = T264_search;
    else
        t->search = T264_search_full;

#ifndef CHIP_DM642
    if (t->param.cpu & T264_CPU_MMX)
    {
        t->emms = T264_emms_mmx;
        t->fdct4x4 = dct4x4_mmx;
        t->fdct4x4dc = dct4x4dc_mmx;
        t->idct4x4 = idct4x4_mmx;
        t->idct4x4dc = idct4x4dc_mmx;
	    t->contract16to8add = contract16to8add_mmx;
	    t->expand8to16sub   = expand8to16sub_mmx;
    
        t->pia[MB_4x8]   = T264_pia_u_4x8_mmx;
        t->pia[MB_4x4]   = T264_pia_u_4x4_mmx;
    }
    if (t->param.cpu & T264_CPU_SSE)
    {
        if (t->flags & USE_SAD)
        {
            t->cmp[MB_8x16]  = T264_sad_u_8x16_sse;
            t->cmp[MB_8x8]   = T264_sad_u_8x8_sse;
            t->cmp[MB_8x4]   = T264_sad_u_8x4_sse;
            t->cmp[MB_4x8]   = T264_sad_u_4x8_sse;
            t->cmp[MB_4x4]   = T264_sad_u_4x4_sse;
        }

        t->pia[MB_16x16] = T264_pia_u_16x16_sse;
        t->pia[MB_16x8]  = T264_pia_u_16x8_sse;
        t->pia[MB_8x16]  = T264_pia_u_8x16_sse;
        t->pia[MB_8x8]   = T264_pia_u_8x8_sse;
        t->pia[MB_8x4]   = T264_pia_u_8x4_sse;

        t->sad[MB_8x16]  = T264_sad_u_8x16_sse;
        t->sad[MB_8x8]   = T264_sad_u_8x8_sse;
        t->sad[MB_8x4]   = T264_sad_u_8x4_sse;
        t->sad[MB_4x8]   = T264_sad_u_4x8_sse;
        t->sad[MB_4x4]   = T264_sad_u_4x4_sse;
    }
    if (t->param.cpu & T264_CPU_SSE2)
    {
        t->quant4x4 = quant4x4_sse2;
        t->iquant4x4 = iquant4x4_sse2;
        if (t->flags & USE_SAD)
        {
            t->cmp[MB_16x16] = T264_sad_u_16x16_sse2;
            t->cmp[MB_16x8]  = T264_sad_u_16x8_sse2;
        }

        t->sad[MB_16x16] = T264_sad_u_16x16_sse2;
        t->sad[MB_16x8]  = T264_sad_u_16x8_sse2;
        t->interpolate_halfpel_h = interpolate_halfpel_h_sse2;
        t->interpolate_halfpel_v = interpolate_halfpel_v_sse2;
        t->pia[MB_16x16] = T264_pia_u_16x16_sse2;
        t->pia[MB_16x8]  = T264_pia_u_16x8_sse2;
    }
#endif    
}

static void __inline
T264_init_frame(T264_t* t, uint8_t* src, T264_frame_t* f, int32_t poc)
{
    f->Y[0] = src;
    f->U = f->Y[0] + t->width * t->height;
    f->V = f->U + (t->width * t->height >> 2);
    f->poc = poc;
}

static void __inline
T264_pending_bframe(T264_t* t, uint8_t* src, int32_t poc)
{
    T264_frame_t* f = &t->pending_bframes[t->pending_bframes_num ++];

    memcpy(f->Y[0], src, t->height * t->width + (t->height * t->width >> 1));

    f->poc = poc;
}

// get non zero count & cbp
void
T264_mb_encode_post(T264_t* t)
{
    int32_t i, j;
	//for CABAC
	int32_t dc_nz, dc_nz0, dc_nz1, cbp_dc;
	cbp_dc = 0;

    if (t->mb.mb_mode == I_16x16)
    {
        t->mb.cbp_y = 0;
        for(i = 0; i < 16 ; i ++)
        {
            int32_t x, y;
            const int32_t nz = array_non_zero_count(&(t->mb.dct_y_z[i][1]), 15);
            x = luma_inverse_x[i];
            y = luma_inverse_y[i];
            t->mb.nnz[luma_index[i]] = nz;
            t->mb.nnz_ref[NNZ_LUMA + y * 8 + x] = nz;
            if( nz > 0 )
            {
                t->mb.cbp_y = 0x0f;
            }
        }
		//for CABAC, record the DC non_zero
		dc_nz = array_non_zero_count(&(t->mb.dc4x4_z[0]), 16);
		if(dc_nz != 0)
		{
			cbp_dc = 1;
		}
    }	
    else
    {
        t->mb.cbp_y = 0;
        for(i = 0; i < 16; i ++)
        {
            int32_t x, y;
            const int32_t nz = array_non_zero_count(t->mb.dct_y_z[i], 16);
            x = luma_inverse_x[i];
            y = luma_inverse_y[i];
            t->mb.nnz[luma_index[i]] = nz;
            t->mb.nnz_ref[NNZ_LUMA + y * 8 + x] = nz;
            if( nz > 0 )
            {
                t->mb.cbp_y |= 1 << (i / 4);
            }
        }
    }

    /* Calculate the chroma patern */
    t->mb.cbp_c = 0;
    for(i = 0; i < 8; i ++)
    {
        int32_t x, y;
        const int nz = array_non_zero_count(&(t->mb.dct_uv_z[i / 4][i % 4][1]), 15);
        t->mb.nnz[i + 16] = nz;
        if (i < 4)
        {
            x = i % 2;
            y = i / 2;
            t->mb.nnz_ref[NNZ_CHROMA0 + y * 8 + x] = nz;
        }
        else
        {
            int32_t j = i - 4;
            x = j % 2;
            y = j / 2;
            t->mb.nnz_ref[NNZ_CHROMA1 + y * 8 + x] = nz;
        }
        if( nz > 0 )
        {
            t->mb.cbp_c = 0x02;    /* dc+ac */
        }
    }
	//for CABAC, chroma dc pattern
	dc_nz0 = array_non_zero_count(t->mb.dc2x2_z[0], 4) > 0;
	dc_nz1 = array_non_zero_count(t->mb.dc2x2_z[1], 4) > 0;
    if(t->mb.cbp_c == 0x00 &&
       (dc_nz0 || dc_nz1))
    {
        t->mb.cbp_c = 0x01;    /* dc only */
    }
	if(dc_nz0)
		cbp_dc |= 0x02;
	if(dc_nz1)
		cbp_dc |= 0x04;

    // really decide SKIP mode
    if(t->slice_type == SLICE_P)
    {
        if (t->mb.mb_part == MB_16x16 && t->mb.cbp_y == 0 && t->mb.cbp_c == 0 && t->mb.vec[0][0].refno == 0)
        {
            T264_vector_t vec;
            T264_predict_mv_skip(t, 0, &vec);
            if (vec.x == t->mb.vec[0][0].x &&
                vec.y == t->mb.vec[0][0].y)
            {
                t->mb.mb_part = MB_16x16;
                t->mb.mb_mode = P_SKIP;
            }
        }
    }
    else if (t->slice_type == SLICE_B)
    {
        if (t->mb.is_copy && t->mb.cbp_y == 0 && t->mb.cbp_c == 0)
        {
            t->mb.mb_mode = B_SKIP;
        }
    }

    if (t->mb.mb_mode == I_4x4)
    {
        int8_t* p = t->mb.i4x4_pred_mode_ref;
        for(i = 0; i < 16 ; i ++)
        {
            int32_t x, y;
            x = luma_inverse_x[i];
            y = luma_inverse_y[i];
            p[IPM_LUMA + y * 8 + x] = t->mb.mode_i4x4[i];
        }
    }
    else
    {
        memset(t->mb.mode_i4x4, Intra_4x4_DC, 16 * sizeof(uint8_t));
    }

    if (t->mb.mb_mode != I_4x4 && t->mb.mb_mode != I_16x16)
    {
        for(i = 0 ; i < 16 ; i ++)
        {
            int32_t x, y;
            x = i % 4;
            y = i / 4;
            t->mb.vec_ref[VEC_LUMA + y * 8 + x].vec[0]     = t->mb.vec[0][i];
            t->mb.vec_ref[VEC_LUMA + y * 8 + x].vec[1]     = t->mb.vec[1][i];
            t->mb.vec_ref[VEC_LUMA + y * 8 + x].part    = t->mb.mb_part;
            t->mb.vec_ref[VEC_LUMA + y * 8 + x].subpart = t->mb.submb_part[i];
        }
    }
    else
    {
        memset(t->mb.submb_part, -1, sizeof(uint8_t) * 16);//t->mb.submb_part));
        t->mb.mb_part = -1;
#define INITINVALIDVEC(vec) vec.refno = -1; vec.x = vec.y = 0;
        for(i = 0 ; i < 2 ; i ++)
        {
            for(j = 0 ; j < 16 ; j ++)
            {
                INITINVALIDVEC(t->mb.vec[i][j]);
            }
        }
    }
#undef INITINVALIDVEC
	//for CABAC, cbp
	t->mb.cbp = t->mb.cbp_y | (t->mb.cbp_c<<4) | (cbp_dc << 8);
}

static uint32_t
write_dst(uint8_t* src, int32_t nal_pos[4], int32_t nal_num, uint8_t* dst, int32_t dst_size)
{
    int32_t i, j, n;
    int32_t count;
    int32_t nal_len;

    n = 0;
    for(i = 0 ; i < nal_num - 1; i ++)
    {
        nal_len = nal_pos[i + 1] - nal_pos[i];
        
        // start code 00 00 00 01
        dst[n ++] = src[0];
        dst[n ++] = src[1];
        dst[n ++] = src[2];
        dst[n ++] = src[3];
        count = 0;
        for(j = 4 ; j < nal_len - 1; j ++)
        {
            if (src[j] == 0)
            {
                count ++;
                if (count >= 2 && src[j + 1] <= 3)
                {
                    dst[n ++] = 0;
                    dst[n ++] = 3;
                    count = 0;
                    continue;
                }
            }
            else
            {
                count = 0;
            }
            dst[n ++] = src[j];
        }
        dst[n ++] = src[j];
        src += nal_len;
    }

    return n;
}

///////////////////////////////////////////////////////////
// interface
T264_t*
T264_open(T264_param_t* para)
{
    T264_t* t;
    int32_t i;

    //
    // TODO: here check the input param if it is valid
    //
    if (para->flags & USE_FORCEBLOCKSIZE)
        para->flags |= USE_SUBBLOCK;
    if (para->flags & USE_QUARTPEL)
        para->flags |= USE_HALFPEL;

    t = T264_malloc(sizeof(T264_t), CACHE_SIZE);
    memset(t, 0, sizeof(T264_t));

    t->mb_width  = para->width >> 4;
    t->mb_height = para->height >> 4;
    t->mb_stride = t->mb_width;
    t->width  = t->mb_width << 4;
    t->height = t->mb_height << 4;
    t->edged_width = t->width + 2 * EDGED_WIDTH;
    t->edged_height = t->height + 2 * EDGED_HEIGHT;
    t->qp_y   = para->qp;
    t->flags  = para->flags;

    t->stride    = t->width;
    t->stride_uv = t->width >> 1;
    t->edged_stride = t->edged_width;
    t->edged_stride_uv = t->edged_width >> 1;

    t->bs = T264_malloc(sizeof(bs_t), CACHE_SIZE);
    t->bs_buf = T264_malloc(t->width * t->height << 1, CACHE_SIZE);
    para->direct_flag = 1;  /* force direct mode */
    if (para->b_num)
        para->ref_num ++;

    for(i = 0 ; i < para->ref_num + 1 ; i ++)
    {
        uint8_t* p = T264_malloc(t->edged_width * t->edged_height + (t->edged_width * t->edged_height >> 1), CACHE_SIZE);
        t->refn[i].Y[0] = p + EDGED_HEIGHT * t->edged_width + EDGED_WIDTH;
        t->refn[i].U = p + t->edged_width * t->edged_height + (t->edged_width * EDGED_HEIGHT >> 2) + (EDGED_WIDTH >> 1);
        t->refn[i].V = p + t->edged_width * t->edged_height + (t->edged_width * t->edged_height >> 2) + (t->edged_width * EDGED_HEIGHT >> 2) + (EDGED_WIDTH >> 1);
        t->refn[i].mb = T264_malloc(t->mb_height * t->mb_width * sizeof(T264_mb_context_t), CACHE_SIZE);
        p = T264_malloc(t->edged_width * t->edged_height * 3, CACHE_SIZE);
        t->refn[i].Y[1] = p + EDGED_HEIGHT * t->edged_width + EDGED_WIDTH;
        t->refn[i].Y[2] = t->refn[i].Y[1] + t->edged_width * t->edged_height;
        t->refn[i].Y[3] = t->refn[i].Y[2] + t->edged_width * t->edged_height;
    }
    for(i = 0 ; i < para->b_num ; i ++)
    {
        t->pending_bframes[i].Y[0] = T264_malloc(t->width * t->height + (t->width * t->height >> 1), CACHE_SIZE);
    }

    t->param = *para;
    t->idr_pic_id = -1;
    t->frame_id = 0;
    t->last_i_frame_id = 0;

    T264_init_cpu(t);

    if (para->enable_rc)
    {
        rc_create(t, &t->plugins[t->plug_num]);
        t->plugins[t->plug_num].proc(t, t->plugins[t->plug_num].handle, STATE_BEFORESEQ);
        t->plug_num ++;
    }
    if (para->enable_stat)
    {
        stat_create(t, &t->plugins[t->plug_num]);
        t->plugins[t->plug_num].proc(t, t->plugins[t->plug_num].handle, STATE_BEFORESEQ);
        t->plug_num ++;
    }
    if (t->flags & USE_EXTRASUBPELSEARCH)
        t->subpel_pts = 8;
    else
        t->subpel_pts = 4;

    return t;
}

void
T264_close(T264_t* t)
{
    int32_t i;

    for(i = 0 ; i < t->param.ref_num + 1 ; i ++)
    {
        T264_free(t->refn[i].Y[0] - (EDGED_HEIGHT * t->edged_width + EDGED_WIDTH));
        T264_free(t->refn[i].mb);
        T264_free(t->refn[i].Y[1] - (EDGED_HEIGHT * t->edged_width + EDGED_WIDTH));
    }

    for(i = 0 ; i < t->plug_num ; i ++)
    {
        t->plugins[i].close(t, &t->plugins[i]);
    }

    for(i = 0 ; i < t->param.b_num ; i ++)
    {
        T264_free(t->pending_bframes[i].Y[0]);
    }
    T264_free(t->bs_buf);
    T264_free(t->bs);

    T264_free(t);	
}

static void __inline
T264_encode_frame(T264_t* t)
{
    int32_t i, j;

	//for test, to be cleaned
	//extern int frame_cabac, mb_cabac, slice_type_cabac;
    T264_load_ref(t);

    slice_header_init(t, &t->slice);
    slice_header_write(t, &t->slice);

    t->header_bits = eg_len(t->bs) * 8;
    t->sad_all = 0;
	//for CABAC
	if( t->param.cabac )
	{
		/* alignment needed */
		BitstreamPadOne( t->bs );

		/* init cabac */
		T264_cabac_context_init( &t->cabac, t->slice_type, t->ps.pic_init_qp_minus26+26+t->slice.slice_qp_delta, t->slice.cabac_init_idc );
		T264_cabac_encode_init ( &t->cabac, t->bs );
	}
    for(i = 0 ; i < t->mb_height ; i ++)
    {
        for(j = 0 ; j < t->mb_width ; j ++)
        {
            T264_mb_load_context(t, i, j);

            T264_mb_mode_decision(t);

            T264_mb_encode(t);

            T264_mb_encode_post(t);

			if(t->param.cabac)
			{
				T264_mb_save_context(t);
			}
			//SKIP
            if(t->mb.mb_mode == P_SKIP || t->mb.mb_mode == B_SKIP)
            {
				if( t->param.cabac )
				{
					if( t->mb.mb_xy > 0 )
					{
						/* not end_of_slice_flag */
						T264_cabac_encode_terminal( &t->cabac, 0 );
					}

					T264_cabac_mb_skip( t, 1 );
					//for CABAC, set MVD to zero
					memset(&(t->mb.context->mvd[0][0][0]), 0, sizeof(t->mb.context->mvd));
				}
				else
				{
                    t->skip ++;
                }
            }
            else
            {
				if( t->param.cabac )
				{
					//for CABAC, set MVD to zero
					memset(&(t->mb.mvd[0][0][0]), 0, sizeof(t->mb.mvd));
					if( t->mb.mb_xy > 0 )
					{
						/* not end_of_slice_flag */
						T264_cabac_encode_terminal( &t->cabac, 0 );
					}
					if( t->slice_type != SLICE_I )
					{
						T264_cabac_mb_skip( t, 0 );
					}
					T264_macroblock_write_cabac( t, t->bs );
					memcpy(&(t->mb.context->mvd[0][0][0]), &(t->mb.mvd[0][0][0]), sizeof(t->mb.context->mvd));
				}
                else
                {
                    T264_macroblock_write_cavlc(t);				
                }
            }
			if(!t->param.cabac)
			{
                T264_mb_save_context(t);
			}
            t->sad_all += t->mb.sad;
        }
    }

	if( t->param.cabac )
	{
		/* end of slice */
		T264_cabac_encode_terminal( &t->cabac, 1 );
	}
	else if (t->skip > 0)
    {
        eg_write_ue(t->bs, t->skip);
        t->skip = 0;
    }

	if( t->param.cabac )
	{
		int i_cabac_word;
		int pos;
		T264_cabac_encode_flush( &t->cabac );
		/* TODO cabac stuffing things (p209) */
		pos = BitstreamPos( t->bs)/8;
		i_cabac_word = (((3 * t->cabac.i_sym_cnt - 3 * 96 * t->mb_width * t->mb_height)/32) - pos)/3;

		while( i_cabac_word > 0 )
		{
			BitstreamPutBits( t->bs, 16, 0x0000 );
			i_cabac_word--;
		}
	}
	
    /* update current pic */
    if (t->slice_type != SLICE_B)
    {
        T264_save_ref(t);
        t->frame_num = (t->frame_num + 1) % (1 << (t->ss.log2_max_frame_num_minus4 + 4));
        t->poc += 2;
    }

    rbsp_trailing_bits(t);
    eg_flush(t->bs);
	//for CABAC
	if( t->param.cabac )
	{
		T264_cabac_model_update( &t->cabac, t->slice_type, t->ps.pic_init_qp_minus26+26 + t->slice.slice_qp_delta );
	}
}

static int32_t __inline
T264_flush_frames(T264_t* t, uint8_t* dst, int32_t dst_size)
{
    int32_t i, j = 0;
    int32_t nal_pos[4];     // remember each nal start pos
    int32_t nal_num;
    int32_t len = 0;
    int32_t old_poc = t->poc;

    // b frames
    while (t->pending_bframes_num)
    {
        t->pending_bframes_num --;
        t->poc = t->pending_bframes[j].poc;

        eg_init(t->bs, t->bs_buf, dst_size);
        T264_init_frame(t, t->pending_bframes[j].Y[0], &t->cur, t->poc);
        nal_num = 0;

        nal_pos[nal_num ++] = eg_len(t->bs);
        nal_unit_init(&t->nal, 0, NAL_SLICE_NOPART);
        nal_unit_write(t, &t->nal);

        t->slice_type        = SLICE_B;

        for(i = 0 ; i < t->plug_num ; i ++)
        {
            t->plugins[i].proc(t, t->plugins[i].handle, STATE_BEFOREPIC);
        }

		T264_encode_frame(t);
        nal_pos[nal_num ++] = eg_len(t->bs);

        len += write_dst(t->bs_buf, nal_pos, nal_num, dst + len, dst_size - len);

        t->emms();
 		
        t->frame_bits = len * 8;		

        for(i = 0 ; i < t->plug_num ; i ++)
        {
            t->plugins[i].proc(t, t->plugins[i].handle, STATE_AFTERPIC);
        }

        j ++;
    }

    t->poc = old_poc;

    return len;
}

int32_t
T264_encode(T264_t* t, uint8_t* src, uint8_t* dst, int32_t dst_size)
{
    int32_t i;
    int32_t nal_pos[5];     // remember each nal start pos
    int32_t nal_num = 0;
    int32_t len;

    eg_init(t->bs, t->bs_buf, dst_size);
    T264_init_frame(t, src, &t->cur, t->poc);

    t->slice_type = decision_slice_type(t);
    switch (t->slice_type)
    {
    case SLICE_P:
        nal_pos[nal_num ++] = eg_len(t->bs);
        nal_unit_init(&t->nal, 1, NAL_SLICE_NOPART);
        nal_unit_write(t, &t->nal);

        break;
    case SLICE_B:
        // buffer b frame
        T264_pending_bframe(t, src, t->poc);
        t->poc += 2;
        t->frame_id ++;
        t->frame_no ++;

        return 0;
    case SLICE_I:
        nal_pos[nal_num ++] = eg_len(t->bs);
        nal_unit_init(&t->nal, 1, NAL_SLICE_NOPART);
        nal_unit_write(t, &t->nal);

        /* reset when begin a new gop */
        t->frame_no = 0;
        t->last_i_frame_id = t->frame_id;

        for(i = 0 ; i < t->plug_num ; i ++)
        {
            t->plugins[i].proc(t, t->plugins[i].handle, STATE_BEFOREGOP);
        }

        break;
    case SLICE_IDR:
        // fixed slice type
        t->slice_type = SLICE_I;
        if (t->frame_id == 0)
        {
            /* only write once */
            T264_custom_set_t set;
            nal_pos[nal_num ++] = eg_len(t->bs);
            nal_unit_init(&t->nal, 1, NAL_CUSTOM_SET);
            nal_unit_write(t, &t->nal);
            custom_set_init(t, &set);
            custom_set_write(t, &set);
        }
        // first flush b frames
        len = T264_flush_frames(t, dst, dst_size);
        dst += len;
        dst_size -= len;
        len = 0;

        nal_pos[nal_num ++] = eg_len(t->bs);
        nal_unit_init(&t->nal, 1, NAL_SEQ_SET);
        nal_unit_write(t, &t->nal);

        seq_set_init(t, &t->ss);
        seq_set_write(t, &t->ss);

        nal_pos[nal_num ++] = eg_len(t->bs);

        nal_unit_init(&t->nal, 1, NAL_PIC_SET);
        nal_unit_write(t, &t->nal);

        pic_set_init(t, &t->ps);
        pic_set_write(t, &t->ps);

        nal_pos[nal_num ++] = eg_len(t->bs);

        nal_unit_init(&t->nal, 1, NAL_SLICE_IDR);
        nal_unit_write(t, &t->nal);

        t->idr_pic_id = (t->idr_pic_id + 1) % 65535;
        t->frame_num = 0;
        t->last_i_frame_id = t->frame_id;

        /* reset when begin a new gop */
        t->frame_no = 0;
        t->poc = 0;

        T264_reset_ref(t);

        for(i = 0 ; i < t->plug_num ; i ++)
        {
            t->plugins[i].proc(t, t->plugins[i].handle, STATE_BEFOREGOP);
        }
        break;
    default:
        assert(0);
        break;
    }

    t->frame_id ++;
    t->frame_no ++;

    for(i = 0 ; i < t->plug_num ; i ++)
    {
        t->plugins[i].proc(t, t->plugins[i].handle, STATE_BEFOREPIC);
    }

    // i or p frame
    T264_encode_frame(t);
    nal_pos[nal_num ++] = eg_len(t->bs);

    len = write_dst(t->bs_buf, nal_pos, nal_num, dst, dst_size);

    t->emms();

    t->frame_bits = len * 8;
    for(i = 0 ; i < t->plug_num ; i ++)
    {
        t->plugins[i].proc(t, t->plugins[i].handle, STATE_AFTERPIC);
    }

    len += T264_flush_frames(t, dst + len, dst_size - len);

    return len;
}
