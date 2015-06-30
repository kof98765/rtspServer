/*****************************************************************************
*
*  T264 AVC CODEC
*
*  Copyright(C) 2004-2005 llcc <lcgate1@yahoo.com.cn>
*               2004-2005 visionany <visionany@yahoo.com.cn>
*	2005.3.2 CloudWu<sywu@sohu.com>	added support for B-frame MB16x8 and MB8x16,MB8x8 support
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
#include "bitstream.h"
#include "utility.h"
#ifndef CHIP_DM642
#include "memory.h"
#endif
#include "assert.h"
#include "cavlc.h"
#include "inter.h"
#include "inter_b.h"

#define BLOCK_INDEX_CHROMA_DC   (-1)
#define BLOCK_INDEX_LUMA_DC     (-2)
#define INITINVALIDVEC(vec) vec.refno = -1; vec.x = vec.y = 0;

typedef struct  
{
    uint8_t len;
    uint8_t trailing_ones;
    uint8_t total_coeff;
} vlc_coeff_token_t;
#define VLC(a, b, c) {a, b, c}
#define VLC2(a, b, c) VLC(a, b, c), VLC(a, b, c)
#define VLC4(a, b, c) VLC2(a, b, c), VLC2(a, b, c)

static const uint8_t i16x16_eg_to_cbp[26][3] = 
{
    0, 0, 0, 0, 0, 0, 1, 0, 0, 2, 0, 0, 3, 0, 0,
    0, 1, 0, 1, 1, 0, 2, 1, 0, 3, 1, 0, 0, 2, 0, 
    1, 2, 0, 2, 2, 0, 3, 2, 0, 0, 0, 15, 1, 0, 15, 
    2, 0, 15, 3, 0, 15, 0, 1, 15, 1, 1, 15, 2, 1, 15, 
    3, 1, 15, 0, 2, 15, 1, 2, 15, 2, 2, 15, 3, 2, 15
};

static const uint8_t i4x4_eg_to_cbp[48] =
{
    47, 31, 15, 0, 23, 27, 29, 30, 7, 11, 13, 14, 39, 43, 45,
    46, 16, 3, 5, 10, 12, 19, 21, 26, 28, 35, 37, 42, 44, 1,
    2, 4, 8, 17, 18, 20, 24, 6, 9, 22, 25, 32, 33, 34, 36, 40,
    38, 41
};

static const uint8_t inter_eg_to_cbp[48] = 
{
    0, 16, 1, 2, 4, 8, 32, 3, 5, 10, 12, 15, 47, 7, 11, 13, 14,
    6, 9, 31, 35, 37, 42, 44, 33, 34, 36, 40, 39, 43, 45, 46, 17,
    18, 20, 24, 19, 21, 26, 28, 23, 27, 29, 30, 22, 25, 38, 41
};

/* ++ cavlc tables ++ */
static const vlc_coeff_token_t coeff4_0[] = 
{
    VLC(6, 0, 2),   /* 0001 00 */
    VLC(6, 3, 3),   /* 0001 01 */
    VLC(6, 1, 2),   /* 0001 10 */
    VLC(6, 0, 1),   /* 0001 11 */
};

static const vlc_coeff_token_t coeff4_1[] = 
{
    VLC2(7, 3, 4),   /* 0000 000(0) */
    VLC(8, 2, 4),   /* 0000 0010 */
    VLC(8, 1, 4),   /* 0000 0011 */
    VLC2(7, 2, 3),   /* 0000 010(0) */
    VLC2(7, 1, 3),   /* 0000 011(0) */
    VLC4(6, 0, 4),   /* 0000 10(00) */
    VLC4(6, 0, 3),   /* 0000 11(00) */
};

static const vlc_coeff_token_t coeff3_0[] =
{
    VLC(6, 0, 1),    /* 0000 00 */ 
    VLC(6, 1, 1),    /* 0000 01 */ 
    VLC(-1, -1, -1), /* 0000 10 */ 
    VLC(6, 0, 0),    /* 0000 11 */
    VLC(6, 0, 2),    /* 0001 00 */
    VLC(6, 1, 2),    /* 0001 01 */
    VLC(6, 2, 2),    /* 0001 10 */
    VLC(-1, -1, -1), /* 0001 11 */
    VLC(6, 0, 3),    /* 0010 00 */
    VLC(6, 1, 3),    /* 0010 01 */
    VLC(6, 2, 3),    /* 0010 10 */
    VLC(6, 3, 3),    /* 0010 11 */
    VLC(6, 0, 4),    /* 0011 00 */
    VLC(6, 1, 4),    /* 0011 01 */
    VLC(6, 2, 4),    /* 0011 10 */
    VLC(6, 3, 4),    /* 0011 11 */
    VLC(6, 0, 5),    /* 0100 00 */
    VLC(6, 1, 5),    /* 0100 01 */
    VLC(6, 2, 5),    /* 0100 10 */
    VLC(6, 3, 5),    /* 0100 11 */
    VLC(6, 0, 6),    /* 0101 00 */
    VLC(6, 1, 6),    /* 0101 01 */
    VLC(6, 2, 6),    /* 0101 10 */
    VLC(6, 3, 6),    /* 0101 11 */
    VLC(6, 0, 7),    /* 0110 00 */
    VLC(6, 1, 7),    /* 0110 01 */
    VLC(6, 2, 7),    /* 0110 10 */
    VLC(6, 3, 7),    /* 0110 11 */
    VLC(6, 0, 8),
    VLC(6, 1, 8),
    VLC(6, 2, 8),
    VLC(6, 3, 8),
    VLC(6, 0, 9),
    VLC(6, 1, 9),
    VLC(6, 2, 9),
    VLC(6, 3, 9),
    VLC(6, 0, 10),
    VLC(6, 1, 10),
    VLC(6, 2, 10),
    VLC(6, 3, 10),
    VLC(6, 0, 11),
    VLC(6, 1, 11),
    VLC(6, 2, 11),
    VLC(6, 3, 11),
    VLC(6, 0, 12),
    VLC(6, 1, 12),
    VLC(6, 2, 12),
    VLC(6, 3, 12),
    VLC(6, 0, 13),
    VLC(6, 1, 13),
    VLC(6, 2, 13),
    VLC(6, 3, 13),
    VLC(6, 0, 14),
    VLC(6, 1, 14),
    VLC(6, 2, 14),
    VLC(6, 3, 14),
    VLC(6, 0, 15),
    VLC(6, 1, 15),
    VLC(6, 2, 15),
    VLC(6, 3, 15),
    VLC(6, 0, 16),
    VLC(6, 1, 16),
    VLC(6, 2, 16),
    VLC(6, 3, 16)
};

static const vlc_coeff_token_t coeff2_0[] = 
{
    VLC(4, 3, 7),   /* 1000 */
    VLC(4, 3, 6),   /* 1001 */
    VLC(4, 3, 5),   /* 1010 */
    VLC(4, 3, 4),   /* 1011 */
    VLC(4, 3, 3),   /* 1100 */
    VLC(4, 2, 2),   /* 1101 */
    VLC(4, 1, 1),   /* 1110 */
    VLC(4, 0, 0),   /* 1111 */
};

static const vlc_coeff_token_t coeff2_1[] = 
{
    VLC(5, 1, 5),   /* 0100 0 */
    VLC(5, 2, 5),
    VLC(5, 1, 4),
    VLC(5, 2, 4),
    VLC(5, 1, 3),
    VLC(5, 3, 8),
    VLC(5, 2, 3),
    VLC(5, 1, 2),
};

static const vlc_coeff_token_t coeff2_2[] = 
{
    VLC(6, 0, 3),   /* 0010 00 */
    VLC(6, 2, 7),
    VLC(6, 1, 7),
    VLC(6, 0, 2),
    VLC(6, 3, 9),
    VLC(6, 2, 6),
    VLC(6, 1, 6),
    VLC(6, 0, 1),
};

static const vlc_coeff_token_t coeff2_3[] = 
{
    VLC(7, 0, 7),   /* 0001 000 */
    VLC(7, 0, 6),
    VLC(7, 2, 9),
    VLC(7, 0, 5),
    VLC(7, 3, 10),
    VLC(7, 2, 8),
    VLC(7, 1, 8),
    VLC(7, 0, 4),
};

static const vlc_coeff_token_t coeff2_4[] = 
{
    VLC(8, 3, 12),   /* 0000 1000 */
    VLC(8, 2, 11),
    VLC(8, 1, 10),
    VLC(8, 0, 9),
    VLC(8, 3, 11),
    VLC(8, 2, 10),
    VLC(8, 1, 9),
    VLC(8, 0, 8),
};

static const vlc_coeff_token_t coeff2_5[] = 
{
    VLC(9, 0, 12),   /* 0000 0100 0 */
    VLC(9, 2, 13),
    VLC(9, 1, 12),
    VLC(9, 0, 11),
    VLC(9, 3, 13),
    VLC(9, 2, 12),
    VLC(9, 1, 11),
    VLC(9, 0, 10),
};

static const vlc_coeff_token_t coeff2_6[] = 
{
    VLC(-1, -1, -1),   /* 0000 0000 00 */
    VLC(10, 0, 16),    /* 0000 0000 01 */
    VLC(10, 3, 16),    /* 0000 0000 10 */
    VLC(10, 2, 16),    /* 0000 0000 11 */
    VLC(10, 1, 16),    /* 0000 0001 00 */
    VLC(10, 0, 15),    /* 0000 0001 01 */
    VLC(10, 3, 15),    /* 0000 0001 10 */
    VLC(10, 2, 15),    /* 0000 0001 11 */
    VLC(10, 1, 15),    /* 0000 0010 00 */
    VLC(10, 0, 14),
    VLC(10, 3, 14),
    VLC(10, 2, 14),
    VLC(10, 1, 14),
    VLC(10, 0, 13),
    VLC2(9, 1, 13),    /* 0000 0011 1(0) */
};

static const vlc_coeff_token_t coeff1_0[] = 
{
    VLC(4, 3, 4),  /* 0100 */
    VLC(4, 3, 3),  /* 0101 */
    VLC2(3, 2, 2), /* 011(0) */
    VLC4(2, 1, 1), /* 10 */
    VLC4(2, 0, 0), /* 11 */
};

static const vlc_coeff_token_t coeff1_1[] = 
{
    VLC(6, 3, 7),   /* 0001 00 */
    VLC(6, 2, 4),   /* 0001 01 */
    VLC(6, 1, 4),   /* 0001 10 */
    VLC(6, 0, 2),   /* 0001 11 */
    VLC(6, 3, 6),   /* 0010 00 */
    VLC(6, 2, 3),   /* 0010 01 */
    VLC(6, 1, 3),   /* 0010 10 */
    VLC(6, 0, 1),   /* 0010 11*/
    VLC2(5, 3, 5),   /* 0011 0(0)*/
    VLC2(5, 1, 2),   /* 0011 1(0)*/
};

static const vlc_coeff_token_t coeff1_2[] = 
{
    VLC(9, 3, 9),   /* 0000 0010 0 */
    VLC(9, 2, 7),   /* 0000 0010 1 */
    VLC(9, 1, 7),   /* 0000 0011 0 */
    VLC(9, 0, 6),   /* 0000 0011 1 */

    VLC2(8, 0, 5),   /* 0000 0100 */
    VLC2(8, 2, 6),   /* 0000 0101 */
    VLC2(8, 1, 6),   /* 0000 0110 */
    VLC2(8, 0, 4),   /* 0000 0111 */

    VLC4(7, 3, 8),    /* 0000 100 */
    VLC4(7, 2, 5),    /* 0000 101 */
    VLC4(7, 1, 5),    /* 0000 110 */
    VLC4(7, 0, 3),    /* 0000 111 */
};

static const vlc_coeff_token_t coeff1_3[] = 
{
    VLC(11, 3, 11),   /* 0000 0001 000 */
    VLC(11, 2, 9),    /* 0000 0001 001 */
    VLC(11, 1, 9),    /* 0000 0001 010 */
    VLC(11, 0, 8),    /* 0000 0001 011 */
    VLC(11, 3, 10),   /* 0000 0001 100 */
    VLC(11, 2, 8),    /* 0000 0001 101 */
    VLC(11, 1, 8),    /* 0000 0001 110 */
    VLC(11, 0, 7),    /* 0000 0001 111 */
};

static const vlc_coeff_token_t coeff1_4[] = 
{
    VLC(12, 0, 11),   /* 0000 0000 1000 */
    VLC(12, 2, 11),   /* 0000 0000 1001 */
    VLC(12, 1, 11),   /* 0000 0000 1010 */
    VLC(12, 0, 10),   /* 0000 0000 1011 */
    VLC(12, 3, 12),   /* 0000 0000 1100 */
    VLC(12, 2, 10),   /* 0000 0000 1101 */
    VLC(12, 1, 10),   /* 0000 0000 1110 */
    VLC(12, 0, 9),    /* 0000 0000 1111 */
};

static const vlc_coeff_token_t coeff1_5[] = 
{
    VLC(13, 3, 14),   /* 0000 0000 0100 0 */
    VLC(13, 2, 13),   /* 0000 0000 0100 1 */
    VLC(13, 1, 13),   /* 0000 0000 0101 0 */
    VLC(13, 0, 13),   /* 0000 0000 0101 1 */
    VLC(13, 3, 13),   /* 0000 0000 0110 0 */
    VLC(13, 2, 12),   /* 0000 0000 0110 1 */
    VLC(13, 1, 12),   /* 0000 0000 0111 0 */
    VLC(13, 0, 12),   /* 0000 0000 0111 1 */
};

static const vlc_coeff_token_t coeff1_6[] = 
{
    VLC2(-1, -1, -1),  /* 0000 0000 0000 00 */
    VLC2(13, 3, 15),   /* 0000 0000 0000 1(0) */
    VLC(14, 3, 16),   /* 0000 0000 0001 00 */
    VLC(14, 2, 16),   /* 0000 0000 0001 01 */
    VLC(14, 1, 16),   /* 0000 0000 0001 10 */
    VLC(14, 0, 16),   /* 0000 0000 0001 11 */

    VLC(14, 1, 15),   /* 0000 0000 0010 00 */
    VLC(14, 0, 15),   /* 0000 0000 0010 01 */
    VLC(14, 2, 15),   /* 0000 0000 0010 10 */
    VLC(14, 1, 14),   /* 0000 0000 0010 11 */
    VLC2(13, 2, 14),   /* 0000 0000 0011 0(0) */
    VLC2(13, 0, 14),   /* 0000 0000 0011 1(0) */
};

static const vlc_coeff_token_t coeff0_0[] = 
{
    VLC2(-1, -1, -1), /* 0000 0000 0000 000(0) */
    VLC2(15, 1, 13),  /* 0000 0000 0000 001(0) */
    VLC(16, 0, 16),   /* 0000 0000 0000 0100 */
    VLC(16, 2, 16),   
    VLC(16, 1, 16),
    VLC(16, 0, 15),
    VLC(16, 3, 16),
    VLC(16, 2, 15),
    VLC(16, 1, 15),
    VLC(16, 0, 14),
    VLC(16, 3, 15),
    VLC(16, 2, 14),
    VLC(16, 1, 14),
    VLC(16, 0, 13),   /* 0000 0000 0000 1111 */
    VLC2(15, 3, 14),  /* 0000 0000 0001 000(0) */
    VLC2(15, 2, 13),
    VLC2(15, 1, 12),
    VLC2(15, 0, 12),
    VLC2(15, 3, 13),
    VLC2(15, 2, 12),
    VLC2(15, 1, 11),
    VLC2(15, 0, 11),  /* 0000 0000 0001 111(0) */
    VLC4(14, 3, 12),  /* 0000 0000 0010 00(00) */
    VLC4(14, 2, 11),
    VLC4(14, 1, 10),
    VLC4(14, 0, 10),
    VLC4(14, 3, 11),
    VLC4(14, 2, 10),
    VLC4(14, 1, 9),
    VLC4(14, 0, 9),  /* 0000 0000 0011 11(00) */
};

static const vlc_coeff_token_t coeff0_1[] = 
{
    VLC(13, 0, 8),   /* 0000 0000 0100 0 */
    VLC(13, 2, 9),
    VLC(13, 1, 8),
    VLC(13, 0, 7),
    VLC(13, 3, 10),
    VLC(13, 2, 8),
    VLC(13, 1, 7),
    VLC(13, 0, 6),  /* 0000 0000 0111 1 */
};

static const vlc_coeff_token_t coeff0_2[] = 
{
    VLC(11, 3, 9),   /* 0000 0000 100 */
    VLC(11, 2, 7),
    VLC(11, 1, 6),
    VLC(11, 0, 5),   /* 0000 0000 111 */
    VLC2(10, 3, 8),  /* 0000 0001 00(0) */
    VLC2(10, 2, 6),
    VLC2(10, 1, 5),
    VLC2(10, 0, 4),  /* 0000 0001 11(0) */
    VLC4(9, 3, 7),  /* 0000 0010 0(0) */
    VLC4(9, 2, 5),
    VLC4(9, 1, 4),
    VLC4(9, 0, 3),  /* 0000 0011 1(0) */
};

static const vlc_coeff_token_t coeff0_3[] = 
{
    VLC(8, 3, 6),   /* 0000 0100 */
    VLC(8, 2, 4),
    VLC(8, 1, 3),
    VLC(8, 0, 2),
    VLC2(7, 3, 5),  /* 0000 100 */
    VLC2(7, 2, 3),
    VLC4(6, 3, 4),  /* 0000 11 */
};

static const vlc_coeff_token_t coeff0_4[] = 
{
    VLC(6, 1, 2),    /* 0001 00 */
    VLC(6, 0, 1),    /* 0001 01 */
    VLC2(5, 3, 3)    /* 0001 1 */
};

static const vlc_coeff_token_t coeff0_5[] = 
{
    VLC(-1, -1, -1),   /* 000 */
    VLC(3, 2, 2),      /* 001 */
    VLC2(2, 1, 1),     /* 01 */
    VLC4(1, 0, 0)      /* 1 */
};

static const uint8_t prefix_table0[] = 
{
    -1,
    3,
    2, 2,
    1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 0
};

static const uint8_t prefix_table1[] = 
{
    -1,
    7,
    6, 6,
    5, 5, 5, 5,
    4, 4, 4, 4, 4, 4, 4, 4
};

static const uint8_t prefix_table2[] =
{
    -1,
    11,
    10, 10,
    9, 9, 9, 9,
    8, 8, 8, 8, 8, 8, 8, 8
};

static const uint8_t prefix_table3[] = 
{
    -1,
    15,
    14, 14,
    13, 13, 13, 13,
    12, 12, 12, 12, 12, 12, 12, 12
};

#undef VLC
#undef VLC2
#undef VLC4
#define VLC(a, b) {a, b}
#define VLC2(a, b) VLC(a, b), VLC(a, b)
#define VLC4(a, b) VLC2(a, b), VLC2(a, b)
#define VLC8(a, b) VLC4(a, b), VLC4(a, b)

typedef struct  
{
    uint8_t num;
    uint8_t len;
} zero_count_t;

static const zero_count_t total_zero_table1_0[] = 
{
    VLC(-1, -1),
    VLC(15, 9), /* 0000 0000 1 */
    VLC(14, 9),
    VLC(13, 9), /* 0000 0001 1 */
    VLC2(12, 8),/* 0000 0010 */
    VLC2(11, 8),/* 0000 0011 */
    VLC4(10, 7),/* 0000 010 */
    VLC4(9, 7), /* 0000 011 */
    VLC8(8, 6), /* 0000 10 */
    VLC8(7, 6), /* 0000 11 */
};

static const zero_count_t total_zero_table1_1[] = 
{
    VLC2(-1, -1),
    VLC(6, 5), /* 0001 0 */
    VLC(5, 5), /* 0001 1 */
    VLC2(4, 4),/* 0010 */
    VLC2(3, 4),/* 0011 */
    VLC4(2, 3),/* 010 */
    VLC4(1, 3),/* 011 */
    VLC8(0, 1), /*1 */
    VLC8(0, 1), /*1 */
};

static const zero_count_t total_zero_table2_0[] = 
{
    VLC(14, 6), /* 0000 00 */
    VLC(13, 6),
    VLC(12, 6),
    VLC(11, 6),
    VLC2(10, 5),/* 0001 0 */
    VLC2(9, 5),
};

static const zero_count_t total_zero_table2_1[] = 
{
    VLC2(-1, -1),
    VLC(8, 4), /* 0010 */
    VLC(7, 4), /* 0011 */
    VLC(6, 4),
    VLC(5, 4),
    VLC2(4, 3),/* 011 */
    VLC2(3, 3),/* 100 */
    VLC2(2, 3), /*101 */
    VLC2(1, 3), /*110 */
    VLC2(0, 3), /*111 */
};

static const zero_count_t total_zero_table3_0[] = 
{
    VLC(13, 6), /* 0000 00 */
    VLC(11, 6),
    VLC2(12, 5),/* 0000 1 */
    VLC2(10, 5),/* 0001 0 */
    VLC2(9, 5), /* 0001 1 */
};

static const zero_count_t total_zero_table3_1[] = 
{
    VLC2(-1, -1),
    VLC(8, 4), /* 0010 */
    VLC(5, 4), /* 0011 */
    VLC(4, 4),
    VLC(0, 4),
    VLC2(7, 3),/* 011 */
    VLC2(6, 3),/* 100 */
    VLC2(3, 3), /*101 */
    VLC2(2, 3), /*110 */
    VLC2(1, 3), /*111 */
};

static const zero_count_t total_zero_table6_0[] = 
{
    VLC(10, 6), /* 0000 00 */
    VLC(0, 6),
    VLC2(1, 5),/* 0000 1 */
    VLC4(8, 4),/* 0000 1 */
};

static const zero_count_t total_zero_table6_1[] = 
{
    VLC(-1, -1),
    VLC(9, 3), /* 001 */
    VLC(7, 3), /* 010 */
    VLC(6, 3),
    VLC(5, 3),
    VLC(4, 3),
    VLC(3, 3),
    VLC(2, 3)
};

static const zero_count_t total_zero_table7_0[] = 
{
    VLC(9, 6), /* 0000 00 */
    VLC(0, 6),
    VLC2(1, 5),/* 0000 1 */
    VLC4(7, 4),/* 0001 */
};

static const zero_count_t total_zero_table7_1[] = 
{
    VLC(-1, -1),
    VLC(8, 3), /* 001 */
    VLC(6, 3), /* 010 */
    VLC(4, 3),
    VLC(3, 3),
    VLC(2, 3),
    VLC2(5, 2)
};

static const zero_count_t total_zero_table8_0[] = 
{
    VLC(8, 6), /* 0000 00 */
    VLC(0, 6),
    VLC2(2, 5),/* 0000 1 */
    VLC4(1, 4),/* 0001 */
};

static const zero_count_t total_zero_table8_1[] = 
{
    VLC(-1, -1),
    VLC(7, 3), /* 001 */
    VLC(6, 3), /* 010 */
    VLC(3, 3),
    VLC2(5, 2),
    VLC2(4, 2)
};

static const zero_count_t total_zero_table9_0[] = 
{
    VLC(1, 6), /* 0000 00 */
    VLC(0, 6),
    VLC2(7, 5),/* 0000 1 */
    VLC4(2, 4),/* 0001 */
};

static const zero_count_t total_zero_table9_1[] = 
{
    VLC(-1, -1),
    VLC(5, 3), /* 001 */
    VLC2(6, 2), /* 01 */
    VLC2(4, 2),
    VLC2(3, 2),
};

static const zero_count_t total_zero_table4_0[] = 
{
    VLC(12, 5), /* 0000 0 */
    VLC(11, 5),
    VLC(10, 5), /* 0000 1 */
    VLC(0, 5),  /* 0001 1 */
    VLC2(9, 4), /* 0010 */
    VLC2(7, 4),
    VLC2(3, 4),
    VLC2(2, 4), /* 0101 */
    VLC4(8, 3), /* 011 */
};

static const zero_count_t total_zero_table4_1[] = 
{
    VLC(6, 3),   /* 100 */
    VLC(5, 3),   /* 101 */
    VLC(4, 3),   /* 110 */
    VLC(1, 3)    /* 111 */
};

static const zero_count_t total_zero_table5_0[] = 
{
    VLC(11, 5),  /* 0000 0 */
    VLC(9, 5),
    VLC2(10, 4), /* 0000 1 */
    VLC2(8, 4),  /* 0010 */
    VLC2(2, 4),
    VLC2(1, 4),
    VLC2(0, 4),
    VLC4(7, 3)
};

static const zero_count_t total_zero_table5_1[] = 
{
    VLC(6, 3), /* 100 */
    VLC(5, 3),
    VLC(4, 3),
    VLC(3, 3)
};

static const zero_count_t total_zero_table10_0[] = 
{
    VLC(1, 5), /* 0000 0 */
    VLC(0, 5),
    VLC2(6, 4), /* 0000 1 */
};

static const zero_count_t total_zero_table10_1[] = 
{
    VLC(-1, -1),
    VLC(2, 3), /* 001 */
    VLC2(5, 2), /* 01 */
    VLC2(4, 2),
    VLC2(3, 2),
};

static const zero_count_t total_zero_table11_0[] = 
{
    VLC(0, 4), /* 0000 */
    VLC(1, 4),
    VLC2(2, 3), /* 010 */
    VLC2(3, 3),
    VLC2(5, 3),
    VLC8(4, 1)
};

static const zero_count_t total_zero_table12_0[] = 
{
    VLC(0, 4), /* 0000 */
    VLC(1, 4),
    VLC2(4, 3), /* 010 */
    VLC4(2, 2),
    VLC8(3, 1)
};

static const zero_count_t total_zero_table13_0[] = 
{
    VLC(0, 3), /* 000 */
    VLC(1, 3),
    VLC2(3, 2), /* 01 */
    VLC4(2, 1),
};

static const zero_count_t total_zero_table14_0[] = 
{
    VLC(0, 2), 
    VLC(1, 2),
    VLC2(2, 1),
};

static const zero_count_t total_zero_table_chroma[3][8] = 
{
    {
        VLC(3, 3), 
        VLC(2, 3),
        VLC2(1, 2),
        VLC4(0, 1)
    },
    {
        VLC2(2, 2),
        VLC2(1, 2),
        VLC4(0, 1)
    },
    {
        VLC4(1, 1),
        VLC4(0, 1)
    }
};

static const zero_count_t run_before_table_0[7][8] = 
{
    {
        VLC4(1, 1),
        VLC4(0, 1)
    },
    {
        VLC2(2, 2),
        VLC2(1, 2),
        VLC4(0, 1)
    },
    {
        VLC2(3, 2),
        VLC2(2, 2),
        VLC2(1, 2),
        VLC2(0, 2)
    },
    {
        VLC(4, 3),
        VLC(3, 3),
        VLC2(2, 2),
        VLC2(1, 2),
        VLC2(0, 2)
    },
    {
        VLC(5, 3),
        VLC(4, 3),
        VLC(3, 3),
        VLC(2, 3),
        VLC2(1, 2),
        VLC2(0, 2),
    },
    {
        VLC(1, 3),
        VLC(2, 3),
        VLC(4, 3),
        VLC(3, 3),
        VLC(6, 3),
        VLC(5, 3),
        VLC2(0, 2)
    },
    {
        VLC(-1, -1),
        VLC(6, 3),
        VLC(5, 3),
        VLC(4, 3),
        VLC(3, 3),
        VLC(2, 3),
        VLC(1, 3),
        VLC(0, 3)
    }
};

static const uint8_t run_before_table_1[] =
{
    -1,
    10,
    9, 9,
    8, 8, 8, 8,
    7, 7, 7, 7, 7, 7, 7, 7
};

static const uint8_t run_before_table_2[] =
{
    -1,
    14,
    13, 13,
    12, 12, 12, 12,
    11, 11, 11, 11, 11, 11, 11, 11
};
/* -- cavlc tables -- */

static void __inline
T264_mb_read_cavlc_i4x4_mode(T264_t* t)
{
    int32_t i, j;
    int32_t x, y;
    int8_t* p = t->mb.i4x4_pred_mode_ref;

    for(i = 0 ; i < 16 ; i ++)
    {
        int8_t pred;
        j = luma_index[i];

        pred = T264_mb_predict_intra4x4_mode(t, i);
        /* prev_intra4x4_pred_mode_flag */
        if (eg_read_direct1(t->bs))
        {
            t->mb.mode_i4x4[i] = pred;
        }
        else
        {
            int32_t mode = eg_read_direct(t->bs, 3);
            if (mode < pred)
            {
                t->mb.mode_i4x4[i] = mode;
            }
            else
            {
                t->mb.mode_i4x4[i] = mode + 1;
            }
        }

        x = j % 4;
        y = j / 4;
        p[IPM_LUMA + y * 8 + x] = t->mb.mode_i4x4[i];
    }
}

/* nC == -1 */
void
T264dec_mb_read_coff_token_t4(T264_t* t, uint8_t* trailing_ones, uint8_t* total_coff)
{
    int32_t code;

    code = eg_show(t->bs, 8);
    if (code >= 16)
    {
        if (code >= 128)
        {
            /* 1 */
            *trailing_ones = 1;
            *total_coff = 1;
            eg_read_skip(t->bs, 1);
        }
        else if (code >= 64)
        {
            /* 01 */
            *trailing_ones = 0;
            *total_coff = 0;
            eg_read_skip(t->bs, 2);
        }
        else if (code >= 32)
        {
            /* 001 */
            *trailing_ones = 2;
            *total_coff = 2;
            eg_read_skip(t->bs, 3);
        }
        else
        {
            code = (code >> 2) - 4;

            *trailing_ones = coeff4_0[code].trailing_ones;
            *total_coff = coeff4_0[code].total_coeff;
            eg_read_skip(t->bs, 6);
        }
    }
    else
    {
        *trailing_ones = coeff4_1[code].trailing_ones;
        *total_coff = coeff4_1[code].total_coeff;
        eg_read_skip(t->bs, coeff4_1[code].len);
    }
}

/* nC >= 8 */
void
T264dec_mb_read_coff_token_t3(T264_t* t, uint8_t* trailing_ones, uint8_t* total_coff)
{
    int32_t code;

    code = eg_read_direct(t->bs, 6);

    *trailing_ones = coeff3_0[code].trailing_ones;
    *total_coff = coeff3_0[code].total_coeff;
}

/* 8 > nC >= 4 */
void
T264dec_mb_read_coff_token_t2(T264_t* t, uint8_t* trailing_ones, uint8_t* total_coff)
{
    int32_t code;
    const vlc_coeff_token_t* table;

    code = eg_show(t->bs, 10);
    if (code >= 512)
    {
        table = coeff2_0;
        code = (code >> 6) - 8;
    }
    else if (code >= 256)
    {
        table = coeff2_1;
        code = (code >> 5) - 8;
    }
    else if (code >= 128)
    {
        table = coeff2_2;
        code = (code >> 4) - 8;
    }
    else if (code >= 64)
    {
        table = coeff2_3;
        code = (code >> 3) - 8;
    }
    else if (code >= 32)
    {
        table = coeff2_4;
        code = (code >> 2) - 8;
    }
    else if (code >= 16)
    {
        table = coeff2_5;
        code = (code >> 1) - 8;
    }
    else
    {
        table = coeff2_6;
    }

    *trailing_ones = table[code].trailing_ones;
    *total_coff = table[code].total_coeff;
    eg_read_skip(t->bs, table[code].len);
}

/* 4 > nC >= 2 */
void
T264dec_mb_read_coff_token_t1(T264_t* t, uint8_t* trailing_ones, uint8_t* total_coff)
{
    int32_t code;
    const vlc_coeff_token_t* table;

    code = eg_show(t->bs, 14);
    if (code >= 4096)
    {
        table = coeff1_0;
        code = (code >> 10) - 4;
    }
    else if (code >= 1024)
    {
        table = coeff1_1;
        code = (code >> 8) - 4;
    }
    else if (code >= 128)
    {
        table = coeff1_2;
        code = (code >> 5) - 4;
    }
    else if (code >= 64)
    {
        table = coeff1_3;
        code = (code >> 3) - 8;
    }
    else if (code >= 32)
    {
        table = coeff1_4;
        code = (code >> 2) - 8;
    }
    else if (code >= 16)
    {
        table = coeff1_5;
        code = (code >> 1) - 8;
    }
    else
    {
        table = coeff1_6;
    }

    *trailing_ones = table[code].trailing_ones;
    *total_coff = table[code].total_coeff;
    eg_read_skip(t->bs, table[code].len);
}

/* 2 > nC >= 0 */
void
T264dec_mb_read_coff_token_t0(T264_t* t, uint8_t* trailing_ones, uint8_t* total_coff)
{
    int32_t code;
    const vlc_coeff_token_t* table;

    code = eg_show(t->bs, 16);
    if (code >= 8192)
    {
        table = coeff0_5;
        code >>= 13;
    }
    else if (code >= 4096)
    {
        table = coeff0_4;
        code = (code >> 10) - 4;
    }
    else if (code >= 1024)
    {
        table = coeff0_3;
        code = (code >> 8) - 4;
    }
    else if (code >= 128)
    {
        table = coeff0_2;
        code = (code >> 5) - 4;
    }
    else if (code >= 64)
    {
        table = coeff0_1;
        code = (code >> 3) - 8;
    }
    else
    {
        table = coeff0_0;
    }

    *trailing_ones = table[code].trailing_ones;
    *total_coff = table[code].total_coeff;
    eg_read_skip(t->bs, table[code].len);
}

uint8_t
T264dec_mb_read_level_prefix(T264_t* t)
{
    uint8_t prefix;
    int32_t code;

    code = eg_show(t->bs, 16);
    if (code >= 4096)
    {
        prefix = prefix_table0[code >> 12];
    }
    else if (code >= 256)
    {
        prefix = prefix_table1[code >> 8];
    }
    else if (code >= 16)
    {
        prefix = prefix_table2[code >> 4];
    }
    else
    {
        prefix = prefix_table3[code];
    }

    eg_read_skip(t->bs, prefix + 1);

    return prefix;
}

uint8_t
T264dec_mb_read_total_zero1(T264_t* t)
{
    uint8_t total_zero;
    int32_t code;

    code = eg_show(t->bs, 9);
    if (code >= 32)
    {
        code >>= 4;
        total_zero = total_zero_table1_1[code].num;
        eg_read_skip(t->bs, total_zero_table1_1[code].len);
    }
    else
    {
        total_zero = total_zero_table1_0[code].num;
        eg_read_skip(t->bs, total_zero_table1_0[code].len);
    }

    assert(total_zero != 255);

    return total_zero;
}

uint8_t
T264dec_mb_read_total_zero2(T264_t* t)
{
    uint8_t total_zero;
    int32_t code;

    code = eg_show(t->bs, 6);
    if (code >= 8)
    {
        code >>= 2;
        total_zero = total_zero_table2_1[code].num;
        eg_read_skip(t->bs, total_zero_table2_1[code].len);
    }
    else
    {
        total_zero = total_zero_table2_0[code].num;
        eg_read_skip(t->bs, total_zero_table2_0[code].len);
    }

    assert(total_zero != 255);

    return total_zero;
}

uint8_t
T264dec_mb_read_total_zero3(T264_t* t)
{
    uint8_t total_zero;
    int32_t code;

    code = eg_show(t->bs, 6);
    if (code >= 8)
    {
        code >>= 2;
        total_zero = total_zero_table3_1[code].num;
        eg_read_skip(t->bs, total_zero_table3_1[code].len);
    }
    else
    {
        total_zero = total_zero_table3_0[code].num;
        eg_read_skip(t->bs, total_zero_table3_0[code].len);
    }

    assert(total_zero != 255);

    return total_zero;
}

uint8_t
T264dec_mb_read_total_zero6(T264_t* t)
{
    uint8_t total_zero;
    int32_t code;

    code = eg_show(t->bs, 6);
    if (code >= 8)
    {
        code >>= 3;
        total_zero = total_zero_table6_1[code].num;
        eg_read_skip(t->bs, total_zero_table6_1[code].len);
    }
    else
    {
        total_zero = total_zero_table6_0[code].num;
        eg_read_skip(t->bs, total_zero_table6_0[code].len);
    }

    assert(total_zero != 255);

    return total_zero;
}

uint8_t
T264dec_mb_read_total_zero7(T264_t* t)
{
    uint8_t total_zero;
    int32_t code;

    code = eg_show(t->bs, 6);
    if (code >= 8)
    {
        code >>= 3;
        total_zero = total_zero_table7_1[code].num;
        eg_read_skip(t->bs, total_zero_table7_1[code].len);
    }
    else
    {
        total_zero = total_zero_table7_0[code].num;
        eg_read_skip(t->bs, total_zero_table7_0[code].len);
    }

    assert(total_zero != 255);

    return total_zero;
}

uint8_t
T264dec_mb_read_total_zero8(T264_t* t)
{
    uint8_t total_zero;
    int32_t code;

    code = eg_show(t->bs, 6);
    if (code >= 8)
    {
        code >>= 3;
        total_zero = total_zero_table8_1[code].num;
        eg_read_skip(t->bs, total_zero_table8_1[code].len);
    }
    else
    {
        total_zero = total_zero_table8_0[code].num;
        eg_read_skip(t->bs, total_zero_table8_0[code].len);
    }

    assert(total_zero != 255);

    return total_zero;
}

uint8_t
T264dec_mb_read_total_zero9(T264_t* t)
{
    uint8_t total_zero;
    int32_t code;

    code = eg_show(t->bs, 6);
    if (code >= 8)
    {
        code >>= 3;
        total_zero = total_zero_table9_1[code].num;
        eg_read_skip(t->bs, total_zero_table9_1[code].len);
    }
    else
    {
        total_zero = total_zero_table9_0[code].num;
        eg_read_skip(t->bs, total_zero_table9_0[code].len);
    }

    assert(total_zero != 255);

    return total_zero;
}

uint8_t
T264dec_mb_read_total_zero4(T264_t* t)
{
    uint8_t total_zero;
    int32_t code;

    code = eg_show(t->bs, 5);
    if (code >= 16)
    {
        code = (code >> 2) - 4;
        total_zero = total_zero_table4_1[code].num;
        eg_read_skip(t->bs, total_zero_table4_1[code].len);
    }
    else
    {
        total_zero = total_zero_table4_0[code].num;
        eg_read_skip(t->bs, total_zero_table4_0[code].len);
    }

    assert(total_zero != 255);

    return total_zero;
}

uint8_t
T264dec_mb_read_total_zero5(T264_t* t)
{
    uint8_t total_zero;
    int32_t code;

    code = eg_show(t->bs, 5);
    if (code >= 16)
    {
        code = (code >> 2) - 4;
        total_zero = total_zero_table5_1[code].num;
        eg_read_skip(t->bs, total_zero_table5_1[code].len);
    }
    else
    {
        total_zero = total_zero_table5_0[code].num;
        eg_read_skip(t->bs, total_zero_table5_0[code].len);
    }

    assert(total_zero != 255);

    return total_zero;
}

uint8_t
T264dec_mb_read_total_zero10(T264_t* t)
{
    uint8_t total_zero;
    int32_t code;

    code = eg_show(t->bs, 5);
    if (code >= 4)
    {
        code >>= 2;
        total_zero = total_zero_table10_1[code].num;
        eg_read_skip(t->bs, total_zero_table10_1[code].len);
    }
    else
    {
        total_zero = total_zero_table10_0[code].num;
        eg_read_skip(t->bs, total_zero_table10_0[code].len);
    }

    assert(total_zero != 255);

    return total_zero;
}

uint8_t
T264dec_mb_read_total_zero11(T264_t* t)
{
    uint8_t total_zero;
    int32_t code;

    code = eg_show(t->bs, 4);
    total_zero = total_zero_table11_0[code].num;
    eg_read_skip(t->bs, total_zero_table11_0[code].len);

    assert(total_zero != 255);

    return total_zero;
}

uint8_t
T264dec_mb_read_total_zero12(T264_t* t)
{
    uint8_t total_zero;
    int32_t code;

    code = eg_show(t->bs, 4);
    total_zero = total_zero_table12_0[code].num;
    eg_read_skip(t->bs, total_zero_table12_0[code].len);

    return total_zero;
}

uint8_t
T264dec_mb_read_total_zero13(T264_t* t)
{
    uint8_t total_zero;
    int32_t code;

    code = eg_show(t->bs, 3);
    total_zero = total_zero_table13_0[code].num;
    eg_read_skip(t->bs, total_zero_table13_0[code].len);

    return total_zero;
}

uint8_t
T264dec_mb_read_total_zero14(T264_t* t)
{
    uint8_t total_zero;
    int32_t code;

    code = eg_show(t->bs, 2);
    total_zero = total_zero_table14_0[code].num;
    eg_read_skip(t->bs, total_zero_table14_0[code].len);

    return total_zero;
}

uint8_t
T264dec_mb_read_total_zero15(T264_t* t)
{
    return eg_read_direct1(t->bs);
}

uint8_t
T264dec_mb_read_total_zero_chroma(T264_t* t, uint8_t total_coeff)
{
    uint8_t total_zero;
    int32_t code;

    code = eg_show(t->bs, 3);
    total_zero = total_zero_table_chroma[total_coeff - 1][code].num;
    eg_read_skip(t->bs, total_zero_table_chroma[total_coeff - 1][code].len);

    assert(total_zero != 255);

    return total_zero;
}

uint8_t
T264dec_mb_read_run_before(T264_t* t, uint8_t zero_left)
{
    int32_t code;
    uint8_t run_before;

    assert(zero_left != 255);

    code = eg_show(t->bs, 3);
    if (zero_left <= 6)
    {
        run_before = run_before_table_0[zero_left - 1][code].num;
        eg_read_skip(t->bs, run_before_table_0[zero_left - 1][code].len);
    }
    else
    {
        eg_read_skip(t->bs, 3);
        if (code > 0)
        {
            run_before = run_before_table_0[6][code].num;
        }
        else
        {
            code = eg_show(t->bs, 4);
            if (code > 0)
            {
                run_before = run_before_table_1[code];
                eg_read_skip(t->bs, run_before - 6);
            }
            else
            {
                eg_read_skip(t->bs, 4);
                code = eg_show(t->bs, 4);
                run_before = run_before_table_2[code];
                eg_read_skip(t->bs, run_before - 10);
            }
        }
    }

    assert(run_before >= 0 && run_before <= 14);

    return run_before;
}

void
T264dec_mb_read_cavlc_residual(T264_t* t, int32_t idx, int16_t* z, int32_t count)
{
    uint8_t trailing_ones, total_coeff;
    int32_t i, j;
    int32_t zero_left = 0;
    int16_t level[16];
    uint8_t run[16];
    int32_t x, y;

    if(idx == BLOCK_INDEX_CHROMA_DC)
    {
        T264dec_mb_read_coff_token_t4(t, &trailing_ones, &total_coeff);
    }
    else
    {
        /* T264_mb_predict_non_zero_code return 0 <-> (16+16+1)>>1 = 16 */
        int32_t nC = 0;
        typedef void (*T264dec_mb_read_coff_token_t)(T264_t* t, uint8_t* trailing_ones, uint8_t* total_coff);
        static const T264dec_mb_read_coff_token_t read_coeff[17] = 
        {
            T264dec_mb_read_coff_token_t0, T264dec_mb_read_coff_token_t0,
            T264dec_mb_read_coff_token_t1, T264dec_mb_read_coff_token_t1,
            T264dec_mb_read_coff_token_t2, T264dec_mb_read_coff_token_t2,
            T264dec_mb_read_coff_token_t2, T264dec_mb_read_coff_token_t2,
            T264dec_mb_read_coff_token_t3, T264dec_mb_read_coff_token_t3,
            T264dec_mb_read_coff_token_t3, T264dec_mb_read_coff_token_t3,
            T264dec_mb_read_coff_token_t3, T264dec_mb_read_coff_token_t3,
            T264dec_mb_read_coff_token_t3, T264dec_mb_read_coff_token_t3,
            T264dec_mb_read_coff_token_t3
        };

        if(idx == BLOCK_INDEX_LUMA_DC)
        {
            // predict nC = (nA + nB) / 2;
            nC = T264_mb_predict_non_zero_code(t, 0);

            read_coeff[nC](t, &trailing_ones, &total_coeff);
        }
        else
        {
            // predict nC = (nA + nB) / 2;
            nC = T264_mb_predict_non_zero_code(t, idx);

            read_coeff[nC](t, &trailing_ones, &total_coeff);

            assert(total_coeff != 255);
            assert(trailing_ones != 255);

            if (idx < 16)
            {
                x = luma_inverse_x[idx];
                y = luma_inverse_y[idx];
                t->mb.nnz[luma_index[idx]] = total_coeff;
                t->mb.nnz_ref[NNZ_LUMA + y * 8 + x] = total_coeff;
            }
            else if (idx < 20)
            {
                t->mb.nnz[idx] = total_coeff;
                x = (idx - 16) % 2;
                y = (idx - 16) / 2;
                t->mb.nnz_ref[NNZ_CHROMA0 + y * 8 + x] = total_coeff;
            }
            else
            {
                t->mb.nnz[idx] = total_coeff;
                x = (idx - 20) % 2;
                y = (idx - 20) / 2;
                t->mb.nnz_ref[NNZ_CHROMA1 + y * 8 + x] = total_coeff;
            }
        }
    }

    if (total_coeff > 0)
    {
        uint8_t suffix_length = 0;
        int32_t level_code;

        if (total_coeff > 10 && trailing_ones < 3)
            suffix_length = 1;

        for(i = 0 ; i < trailing_ones ; i ++)
        {
            level[i] = 1 - 2 * eg_read_direct1(t->bs);
        }

        for( ; i < total_coeff ; i ++)
        {
            uint32_t level_suffixsize;
            uint32_t level_suffix;
            uint8_t level_prefix = T264dec_mb_read_level_prefix(t);

            level_suffixsize = suffix_length;
            if (suffix_length == 0 && level_prefix == 14)
                level_suffixsize = 4;
            else if (level_prefix == 15)
                level_suffixsize = 12;
            if (level_suffixsize > 0)
                level_suffix = eg_read_direct(t->bs, level_suffixsize);
            else
                level_suffix = 0;
            level_code = (level_prefix << suffix_length) + level_suffix;
            if (level_prefix == 15 && suffix_length == 0)
            {
                level_code += 15;
            }
            if (i == trailing_ones && trailing_ones < 3)
            {
                level_code += 2;
            }
            if (level_code % 2 == 0)
            {
                level[i] = (level_code + 2) >> 1;
            }
            else
            {
                level[i] = (-level_code - 1) >> 1;
            }

            if (suffix_length == 0)
                suffix_length = 1;

            if (ABS(level[i]) > (3 << (suffix_length - 1)) &&
                suffix_length < 6)
            {
                suffix_length ++;
            }
        }

        if (total_coeff < count)
        {
            typedef uint8_t (*T264dec_mb_read_total_zero_t)(T264_t* t);
            static T264dec_mb_read_total_zero_t total_zero_f[] =
            {
                T264dec_mb_read_total_zero1, T264dec_mb_read_total_zero2, T264dec_mb_read_total_zero3, T264dec_mb_read_total_zero4,
                T264dec_mb_read_total_zero5, T264dec_mb_read_total_zero6, T264dec_mb_read_total_zero7, T264dec_mb_read_total_zero8,
                T264dec_mb_read_total_zero9, T264dec_mb_read_total_zero10, T264dec_mb_read_total_zero11, T264dec_mb_read_total_zero12,
                T264dec_mb_read_total_zero13, T264dec_mb_read_total_zero14, T264dec_mb_read_total_zero15
            };

            if(idx != BLOCK_INDEX_CHROMA_DC)
                zero_left = total_zero_f[total_coeff - 1](t);
            else
                zero_left = T264dec_mb_read_total_zero_chroma(t, total_coeff);
        }

        for(i = 0 ; i < total_coeff - 1 ; i ++)
        {
            if (zero_left > 0)
            {
                run[i] = T264dec_mb_read_run_before(t, zero_left);
            }
            else
            {
                run[i] = 0;
            }
            zero_left -= run[i];
        }

        run[total_coeff - 1] = zero_left;

        j = -1;
        for(i = total_coeff - 1 ; i >= 0 ; i --)
        {
            j +=run[i] + 1;
            z[j] = level[i];
        }
    }
}

static void __inline
T264dec_mb_read_intra_cavlc(T264_t* t)
{
    int32_t i;

    if (t->mb.mb_part == I_4x4)
    {
        int32_t cbp;

        T264_mb_read_cavlc_i4x4_mode(t);

        t->mb.mb_mode_uv = eg_read_ue(t->bs);
        assert(t->mb.mb_mode_uv <= Intra_8x8_DC128);

        cbp = i4x4_eg_to_cbp[eg_read_ue(t->bs)];
        t->mb.cbp_y = cbp % 16;
        t->mb.cbp_c = cbp / 16;

        if (cbp > 0)
        {
            t->mb.mb_qp_delta = eg_read_se(t->bs);

            for(i = 0 ; i < 16 ; i ++)
            {
                if (t->mb.cbp_y & (1 << (i / 4)))
                {
                    T264dec_mb_read_cavlc_residual(t, i, t->mb.dct_y_z[i], 16);
                }
            }
        }

        t->mb.mb_mode = I_4x4;
    }
    else
    {
        t->mb.mode_i16x16 = i16x16_eg_to_cbp[t->mb.mb_part][0];
        t->mb.cbp_y = i16x16_eg_to_cbp[t->mb.mb_part][2];
        t->mb.cbp_c = i16x16_eg_to_cbp[t->mb.mb_part][1];

        t->mb.mb_mode_uv = eg_read_ue(t->bs);
        assert(t->mb.mb_mode_uv <= Intra_8x8_DC128);

        t->mb.mb_qp_delta = eg_read_se(t->bs);

        // dc luma
        T264dec_mb_read_cavlc_residual(t, BLOCK_INDEX_LUMA_DC, t->mb.dc4x4_z, 16);

        if (t->mb.cbp_y != 0)
        {
            for(i = 0 ; i < 16 ; i ++)
            {
                if (t->mb.cbp_y & (1 << (i / 4)))
                {
                    T264dec_mb_read_cavlc_residual(t, i, &(t->mb.dct_y_z[i][1]), 15);
                }
                t->mb.dct_y_z[i][0] = t->mb.dc4x4_z[i];
            }
        }

        t->mb.mb_mode = I_16x16;
    }
}

void __inline
mb_get_directMB16x16_mv(T264_t* t)
{
    T264_get_direct_mv(t, t->mb.vec);
}

void
T264dec_mb_read_cavlc(T264_t* t)
{
    int32_t mb_type;
    int32_t i, j;

    if (t->slice_type != SLICE_I)
    {
        if (t->skip == -1)
        {
            t->skip = eg_read_ue(t->bs);
        }
        if (t->skip -- > 0)
        {
            /* skip mb block, return */
            if (t->slice_type == SLICE_P)
            {
                T264_predict_mv_skip(t, 0, &t->mb.vec[0][0]);
                copy_nvec(&t->mb.vec[0][0], &t->mb.vec[0][0], 4, 4, 4);
                t->mb.mb_mode = P_MODE;     /* decode as MB_16x16 */
                t->mb.mb_part = MB_16x16;
                return;
            }
            else if (t->slice_type == SLICE_B)
            {
				T264_get_direct_mv(t,t->mb.vec);
                t->mb.mb_mode = B_MODE;     /* decode as MB_16x16 */                
                t->mb.mb_part = MB_16x16;
                t->mb.is_copy = 1;

                return;                
            }
            else
            {
                assert(0);
            }
        }
    }

    mb_type = eg_read_ue(t->bs);

    if (t->slice_type == SLICE_P)
    {
        T264_vector_t vec, vec1;

        t->mb.mb_part = mb_type;
        mb_type = -1;   /* ugly way: prevent break to i slice code */
        vec.refno = 0;
        vec1.refno = 0;

        t->mb.mb_mode = P_MODE;
        switch (t->mb.mb_part) 
        {
        case MB_16x16:
            if (t->refl0_num - 1 > 0)
            {
                vec.refno = eg_read_te(t->bs, t->refl0_num - 1);
            }
            T264_predict_mv(t, 0, 0, 4, &vec);
            t->mb.vec[0][0].x = eg_read_se(t->bs) + vec.x;
            t->mb.vec[0][0].y = eg_read_se(t->bs) + vec.y;
            t->mb.vec[0][0].refno = vec.refno;
            copy_nvec(&t->mb.vec[0][0], &t->mb.vec[0][0], 4, 4, 4);
            break;
        case MB_16x8:
            if (t->refl0_num - 1 > 0)
            {
                vec.refno = eg_read_te(t->bs, t->refl0_num - 1);
                vec1.refno = eg_read_te(t->bs, t->refl0_num - 1);
            }
            T264_predict_mv(t, 0, 0, 4, &vec);
            t->mb.vec[0][0].x = eg_read_se(t->bs) + vec.x;
            t->mb.vec[0][0].y = eg_read_se(t->bs) + vec.y;
            t->mb.vec[0][0].refno = vec.refno;
            copy_nvec(&t->mb.vec[0][0], &t->mb.vec[0][0], 4, 2, 4);
            t->mb.vec_ref[VEC_LUMA + 8].vec[0] = t->mb.vec[0][0];

            T264_predict_mv(t, 0, 8, 4, &vec1);
            t->mb.vec[0][8].x = eg_read_se(t->bs) + vec1.x;
            t->mb.vec[0][8].y = eg_read_se(t->bs) + vec1.y;
            t->mb.vec[0][8].refno = vec1.refno;
            copy_nvec(&t->mb.vec[0][8], &t->mb.vec[0][8], 4, 2, 4);
            break;
        case MB_8x16:
            if (t->refl0_num - 1 > 0)
            {
                vec.refno = eg_read_te(t->bs, t->refl0_num - 1);
                vec1.refno = eg_read_te(t->bs, t->refl0_num - 1);
            }
            T264_predict_mv(t, 0, 0, 2, &vec);
            t->mb.vec[0][0].x = eg_read_se(t->bs) + vec.x;
            t->mb.vec[0][0].y = eg_read_se(t->bs) + vec.y;
            t->mb.vec[0][0].refno = vec.refno;
            copy_nvec(&t->mb.vec[0][0], &t->mb.vec[0][0], 2, 4, 4);
            t->mb.vec_ref[VEC_LUMA + 1].vec[0] = t->mb.vec[0][0];

            T264_predict_mv(t, 0, luma_index[4], 2, &vec1);
            t->mb.vec[0][luma_index[4]].x = eg_read_se(t->bs) + vec1.x;
            t->mb.vec[0][luma_index[4]].y = eg_read_se(t->bs) + vec1.y;
            t->mb.vec[0][luma_index[4]].refno = vec1.refno;
            copy_nvec(&t->mb.vec[0][2], &t->mb.vec[0][2], 2, 4, 4);
            break;
        case MB_8x8:
        case MB_8x8ref0:
            t->mb.submb_part[luma_index[4 * 0]] = eg_read_ue(t->bs);
            t->mb.submb_part[luma_index[4 * 1]] = eg_read_ue(t->bs);
            t->mb.submb_part[luma_index[4 * 2]] = eg_read_ue(t->bs);
            t->mb.submb_part[luma_index[4 * 3]] = eg_read_ue(t->bs);
            if (t->mb.mb_part != MB_8x8ref0 && t->refl0_num - 1 > 0)
            {
                t->mb.vec[0][0].refno = eg_read_te(t->bs, t->refl0_num - 1);
                t->mb.vec[0][luma_index[4]].refno = eg_read_te(t->bs, t->refl0_num - 1);
                t->mb.vec[0][luma_index[8]].refno = eg_read_te(t->bs, t->refl0_num - 1);
                t->mb.vec[0][luma_index[12]].refno = eg_read_te(t->bs, t->refl0_num - 1);
            }
            else
            {
                t->mb.vec[0][0].refno = 0;
                t->mb.vec[0][luma_index[4]].refno = 0;
                t->mb.vec[0][luma_index[8]].refno = 0;
                t->mb.vec[0][luma_index[12]].refno = 0;
            }
            for(i = 0 ; i < 4 ; i ++)
            {
                switch(t->mb.submb_part[luma_index[4 * i]]) 
                {
                case 0: /* P_L0_8x8 */
                    t->mb.submb_part[luma_index[4 * i]] = MB_8x8;
                    vec = t->mb.vec[0][luma_index[4 * i]];
                    T264_predict_mv(t, 0, luma_index[4 * i], 2, &vec);
                    t->mb.vec[0][luma_index[4 * i]].x = eg_read_se(t->bs) + vec.x;
                    t->mb.vec[0][luma_index[4 * i]].y = eg_read_se(t->bs) + vec.y;

                    t->mb.vec[0][i / 2 * 8 + i % 2 * 2 + 1] = 
                    t->mb.vec[0][i / 2 * 8 + i % 2 * 2 + 4] = 
                    t->mb.vec[0][i / 2 * 8 + i % 2 * 2 + 5] = t->mb.vec[0][i / 2 * 8 + i % 2 * 2 + 0];
                    t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 0].vec[0] =
                    t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 1].vec[0] =
                    t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 8].vec[0] =
                    t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 9].vec[0] = t->mb.vec[0][i / 2 * 8 + i % 2 * 2 + 0];
                    break;
                case MB_8x4 - 4:    /* P_L0_8x4 */
                    t->mb.submb_part[luma_index[4 * i]] = MB_8x4;
                    vec.refno = t->mb.vec[0][luma_index[4 * i]].refno;
                    T264_predict_mv(t, 0, luma_index[4 * i], 2, &vec);
                    t->mb.vec[0][luma_index[4 * i]].x = eg_read_se(t->bs) + vec.x;
                    t->mb.vec[0][luma_index[4 * i]].y = eg_read_se(t->bs) + vec.y;
                    t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 0].vec[0] =
                    t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 1].vec[0] =
                    t->mb.vec[0][i / 2 * 8 + i % 2 * 2 + 1] = t->mb.vec[0][i / 2 * 8 + i % 2 * 2 + 0];

                    T264_predict_mv(t, 0, luma_index[4 * i + 2], 2, &vec);
                    t->mb.vec[0][luma_index[4 * i + 2]].x = eg_read_se(t->bs) + vec.x;
                    t->mb.vec[0][luma_index[4 * i + 2]].y = eg_read_se(t->bs) + vec.y;
                    t->mb.vec[0][luma_index[4 * i + 2]].refno = vec.refno;
                    t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 8].vec[0] =
                    t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 9].vec[0] = 
                    t->mb.vec[0][i / 2 * 8 + i % 2 * 2 + 5] = t->mb.vec[0][i / 2 * 8 + i % 2 * 2 + 4];
                    break;
                case MB_4x8 - 4:    /* P_L0_4x8 */
                    t->mb.submb_part[luma_index[4 * i]] = MB_4x8;
                    vec.refno = t->mb.vec[0][luma_index[4 * i]].refno;
                    T264_predict_mv(t, 0, luma_index[4 * i], 1, &vec);
                    t->mb.vec[0][luma_index[4 * i]].x = eg_read_se(t->bs) + vec.x;
                    t->mb.vec[0][luma_index[4 * i]].y = eg_read_se(t->bs) + vec.y;
                    t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 0].vec[0] =
                    t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 8].vec[0] =
                    t->mb.vec[0][i / 2 * 8 + i % 2 * 2 + 4] = t->mb.vec[0][i / 2 * 8 + i % 2 * 2 + 0];

                    T264_predict_mv(t, 0, luma_index[4 * i + 1], 1, &vec);
                    t->mb.vec[0][luma_index[4 * i + 1]].x = eg_read_se(t->bs) + vec.x;
                    t->mb.vec[0][luma_index[4 * i + 1]].y = eg_read_se(t->bs) + vec.y;
                    t->mb.vec[0][luma_index[4 * i + 1]].refno = vec.refno;
                    t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 1].vec[0] =
                    t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 9].vec[0] =
                    t->mb.vec[0][i / 2 * 8 + i % 2 * 2 + 5] = t->mb.vec[0][i / 2 * 8 + i % 2 * 2 + 1];
                    break;
                case MB_4x4 - 4:        /* P_L0_4x4 */
                    t->mb.submb_part[luma_index[4 * i]] = MB_4x4;
                    vec = t->mb.vec[0][luma_index[4 * i]];
                    T264_predict_mv(t, 0, luma_index[4 * i], 1, &vec);
                    t->mb.vec[0][luma_index[4 * i]].x = eg_read_se(t->bs) + vec.x;
                    t->mb.vec[0][luma_index[4 * i]].y = eg_read_se(t->bs) + vec.y;
                    t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 0].vec[0] = t->mb.vec[0][luma_index[4 * i]];

                    T264_predict_mv(t, 0, luma_index[4 * i + 1], 1, &vec);
                    t->mb.vec[0][luma_index[4 * i + 1]].x = eg_read_se(t->bs) + vec.x;
                    t->mb.vec[0][luma_index[4 * i + 1]].y = eg_read_se(t->bs) + vec.y;
                    t->mb.vec[0][luma_index[4 * i + 1]].refno = vec.refno;
                    t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 1].vec[0] = t->mb.vec[0][luma_index[4 * i + 1]];

                    T264_predict_mv(t, 0, luma_index[4 * i + 2], 1, &vec);
                    t->mb.vec[0][luma_index[4 * i + 2]].x = eg_read_se(t->bs) + vec.x;
                    t->mb.vec[0][luma_index[4 * i + 2]].y = eg_read_se(t->bs) + vec.y;
                    t->mb.vec[0][luma_index[4 * i + 2]].refno = vec.refno;
                    t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 8].vec[0] = t->mb.vec[0][luma_index[4 * i + 2]];

                    T264_predict_mv(t, 0, luma_index[4 * i + 3], 1, &vec);
                    t->mb.vec[0][luma_index[4 * i + 3]].x = eg_read_se(t->bs) + vec.x;
                    t->mb.vec[0][luma_index[4 * i + 3]].y = eg_read_se(t->bs) + vec.y;
                    t->mb.vec[0][luma_index[4 * i + 3]].refno = vec.refno;
                    t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 9].vec[0] = t->mb.vec[0][luma_index[4 * i + 3]];
                    break;
                }
            }
            break;
        default:
            t->mb.mb_part -= 5;
            T264dec_mb_read_intra_cavlc(t);

            /* save ref */
            memset(t->mb.submb_part, -1, sizeof(t->mb.submb_part));
            t->mb.mb_part = -1;
            for(i = 0 ; i < 2 ; i ++)
            {
                for(j = 0 ; j < 16 ; j ++)
                {
                    INITINVALIDVEC(t->mb.vec[i][j]);
                }
            }
            break;
        }
       INITINVALIDVEC(t->mb.vec[1][0]);
       copy_nvec(&t->mb.vec[1][0], &t->mb.vec[1][0], 4, 4, 4);
    }
    else if (t->slice_type == SLICE_B)
    {
        T264_vector_t vecPred0, vecPred1;
		T264_vector_t vec1[2];	//save the mv info of second Partition
        vecPred0.refno = 0;	//default reference index
        vecPred1.refno = 0;
		vec1[0].refno = vec1[1].refno = 0;

		t->mb.mb_part = mb_type;
        t->mb.mb_mode = B_MODE;
		t->mb.mb_part2[0] = mb_type;
		t->mb.mb_part2[1] = mb_type;
        switch (mb_type) 
        {
        case 0:	//B_Direct_16x16
            mb_get_directMB16x16_mv(t);
			t->mb.mb_part = MB_16x16;
            t->mb.is_copy = 1;
			break;
        case 1:	//B_L0_16x16
			t->mb.mb_part = MB_16x16;
            if (t->refl0_num - 1 > 0)
            {
                vecPred0.refno = eg_read_te(t->bs, t->refl0_num - 1);
            }
            T264_predict_mv(t, 0, 0, 4, &vecPred0);
            t->mb.vec[0][0].x = eg_read_se(t->bs) + vecPred0.x;
            t->mb.vec[0][0].y = eg_read_se(t->bs) + vecPred0.y;
            t->mb.vec[0][0].refno = vecPred0.refno;
            copy_nvec(&t->mb.vec[0][0], &t->mb.vec[0][0], 4, 4, 4);

            INITINVALIDVEC(t->mb.vec[1][0]);
            copy_nvec(&t->mb.vec[1][0], &t->mb.vec[1][0], 4, 4, 4);

            break;
        case 2:	//B_L1_16x16
			t->mb.mb_part = MB_16x16;
			if (t->refl1_num - 1 > 0)
            {
                vecPred1.refno = eg_read_te(t->bs, t->refl1_num - 1);
            }
            T264_predict_mv(t, 1, 0, 4, &vecPred1);
            t->mb.vec[1][0].x = eg_read_se(t->bs) + vecPred1.x;
            t->mb.vec[1][0].y = eg_read_se(t->bs) + vecPred1.y;
            t->mb.vec[1][0].refno = vecPred1.refno;
            copy_nvec(&t->mb.vec[1][0], &t->mb.vec[1][0], 4, 4, 4);

            INITINVALIDVEC(t->mb.vec[0][0]);
            copy_nvec(&t->mb.vec[0][0], &t->mb.vec[0][0], 4, 4, 4);
            break;
        case 3:	//B_Bi_16x16
			t->mb.mb_part = MB_16x16;
            if (t->refl0_num - 1 > 0)
            {
                vecPred0.refno = eg_read_te(t->bs, t->refl0_num - 1);
            }
			if (t->refl1_num - 1 > 0)
            {
                vecPred1.refno = eg_read_te(t->bs, t->refl1_num - 1);
            }
            T264_predict_mv(t, 0, 0, 4, &vecPred0);
            T264_predict_mv(t, 1, 0, 4, &vecPred1);
            t->mb.vec[0][0].x = eg_read_se(t->bs) + vecPred0.x;
            t->mb.vec[0][0].y = eg_read_se(t->bs) + vecPred0.y;
            t->mb.vec[0][0].refno = vecPred0.refno;
            t->mb.vec[1][0].x = eg_read_se(t->bs) + vecPred1.x;
            t->mb.vec[1][0].y = eg_read_se(t->bs) + vecPred1.y;
            t->mb.vec[1][0].refno = vecPred1.refno;
            copy_nvec(&t->mb.vec[0][0], &t->mb.vec[0][0], 4, 4, 4);
            copy_nvec(&t->mb.vec[1][0], &t->mb.vec[1][0], 4, 4, 4);
            break;
		case 4:	//B_L0_L0_16x8
			t->mb.mb_part = MB_16x8;
            if (t->refl0_num - 1 > 0)
            {
                vecPred0.refno = eg_read_te(t->bs, t->refl0_num - 1);
                vec1[0].refno = eg_read_te(t->bs, t->refl0_num - 1);	
            }
            T264_predict_mv(t, 0, 0, 4, &vecPred0);
            t->mb.vec[0][0].x = eg_read_se(t->bs) + vecPred0.x;
            t->mb.vec[0][0].y = eg_read_se(t->bs) + vecPred0.y;
            t->mb.vec[0][0].refno = vecPred0.refno;
            copy_nvec(&t->mb.vec[0][0], &t->mb.vec[0][0], 4, 2, 4);
            t->mb.vec_ref[VEC_LUMA + 8].vec[0] = t->mb.vec[0][0];	

            T264_predict_mv(t, 0, 8, 4, &vec1[0]);
            t->mb.vec[0][8].x = eg_read_se(t->bs) + vec1[0].x;
            t->mb.vec[0][8].y = eg_read_se(t->bs) + vec1[0].y;
            t->mb.vec[0][8].refno = vec1[0].refno;
            copy_nvec(&t->mb.vec[0][8], &t->mb.vec[0][8], 4, 2, 4);

            INITINVALIDVEC(t->mb.vec[1][0]);
            copy_nvec(&t->mb.vec[1][0], &t->mb.vec[1][0], 4, 4, 4);

			break;
        case 5:	//B_L0_L0_8x16
			t->mb.mb_part = MB_8x16;
            if (t->refl0_num - 1 > 0)
            {
                vecPred0.refno = eg_read_te(t->bs, t->refl0_num - 1);
                vec1[0].refno = eg_read_te(t->bs, t->refl0_num - 1);
            }
            T264_predict_mv(t, 0, 0, 2, &vecPred0);
            t->mb.vec[0][0].x = eg_read_se(t->bs) + vecPred0.x;
            t->mb.vec[0][0].y = eg_read_se(t->bs) + vecPred0.y;
            t->mb.vec[0][0].refno = vecPred0.refno;
            copy_nvec(&t->mb.vec[0][0], &t->mb.vec[0][0], 2, 4, 4);
            t->mb.vec_ref[VEC_LUMA + 1].vec[0] = t->mb.vec[0][0];

            T264_predict_mv(t, 0, luma_index[4], 2, &vec1[0]);
            t->mb.vec[0][luma_index[4]].x = eg_read_se(t->bs) + vec1[0].x;
            t->mb.vec[0][luma_index[4]].y = eg_read_se(t->bs) + vec1[0].y;
            t->mb.vec[0][luma_index[4]].refno = vec1[0].refno;
            copy_nvec(&t->mb.vec[0][2], &t->mb.vec[0][2], 2, 4, 4);

			INITINVALIDVEC(t->mb.vec[1][0]);
            copy_nvec(&t->mb.vec[1][0], &t->mb.vec[1][0], 4, 4, 4);

            break;

		case 6: //B_L1_L1_16x8
			t->mb.mb_part = MB_16x8;
            if (t->refl1_num - 1 > 0)
            {
                vecPred1.refno = eg_read_te(t->bs, t->refl1_num - 1);
                vec1[1].refno = eg_read_te(t->bs, t->refl1_num - 1);	
            }
            T264_predict_mv(t, 1, 0, 4, &vecPred1);
            t->mb.vec[1][0].x = eg_read_se(t->bs) + vecPred1.x;
            t->mb.vec[1][0].y = eg_read_se(t->bs) + vecPred1.y;
            t->mb.vec[1][0].refno = vecPred1.refno;
            copy_nvec(&t->mb.vec[1][0], &t->mb.vec[1][0], 4, 2, 4);
            t->mb.vec_ref[VEC_LUMA + 8].vec[1] = t->mb.vec[1][0];	

            T264_predict_mv(t, 1, 8, 4, &vec1[1]);
            t->mb.vec[1][8].x = eg_read_se(t->bs) + vec1[1].x;
            t->mb.vec[1][8].y = eg_read_se(t->bs) + vec1[1].y;
            t->mb.vec[1][8].refno = vec1[1].refno;
            copy_nvec(&t->mb.vec[1][8], &t->mb.vec[1][8], 4, 2, 4);

            INITINVALIDVEC(t->mb.vec[0][0]);
            copy_nvec(&t->mb.vec[0][0], &t->mb.vec[0][0], 4, 4, 4);

			break;

        case 7:	//B_L1_L1_8x16
			t->mb.mb_part = MB_8x16;
            if (t->refl1_num - 1 > 0)
            {
                vecPred1.refno = eg_read_te(t->bs, t->refl1_num - 1);
                vec1[1].refno = eg_read_te(t->bs, t->refl1_num - 1);
            }
            T264_predict_mv(t, 1, 0, 2, &vecPred1);
            t->mb.vec[1][0].x = eg_read_se(t->bs) + vecPred1.x;
            t->mb.vec[1][0].y = eg_read_se(t->bs) + vecPred1.y;
            t->mb.vec[1][0].refno = vecPred1.refno;
            copy_nvec(&t->mb.vec[1][0], &t->mb.vec[1][0], 2, 4, 4);
            t->mb.vec_ref[VEC_LUMA + 1].vec[1] = t->mb.vec[1][0];

            T264_predict_mv(t, 1, luma_index[4], 2, &vec1[1]);
            t->mb.vec[1][luma_index[4]].x = eg_read_se(t->bs) + vec1[1].x;
            t->mb.vec[1][luma_index[4]].y = eg_read_se(t->bs) + vec1[1].y;
            t->mb.vec[1][luma_index[4]].refno = vec1[1].refno;
            copy_nvec(&t->mb.vec[1][2], &t->mb.vec[1][2], 2, 4, 4);

			INITINVALIDVEC(t->mb.vec[0][0]);
            copy_nvec(&t->mb.vec[0][0], &t->mb.vec[0][0], 4, 4, 4);

            break;

		case 8://B_L0_L1_16x8
			t->mb.mb_part = MB_16x8;
			if (t->refl0_num - 1 > 0)
            {
                vecPred0.refno = eg_read_te(t->bs, t->refl0_num - 1);
            }
			if (t->refl1_num - 1 > 0)
            {
                vecPred1.refno = eg_read_te(t->bs, t->refl1_num - 1);
            }
            T264_predict_mv(t, 0, 0, 4, &vecPred0);
            t->mb.vec[0][0].x = eg_read_se(t->bs) + vecPred0.x;
            t->mb.vec[0][0].y = eg_read_se(t->bs) + vecPred0.y;
            t->mb.vec[0][0].refno = vecPred0.refno;
            copy_nvec(&t->mb.vec[0][0], &t->mb.vec[0][0], 4, 2, 4);

            INITINVALIDVEC(t->mb.vec[0][8]);
            copy_nvec(&t->mb.vec[0][8], &t->mb.vec[0][8], 4, 2, 4);

            INITINVALIDVEC(t->mb.vec_ref[VEC_LUMA + 8].vec[1]);
            INITINVALIDVEC(t->mb.vec[1][0]);
            copy_nvec(&t->mb.vec[1][0], &t->mb.vec[1][0], 4, 2, 4);

            T264_predict_mv(t, 1, 8, 4, &vecPred1);
            t->mb.vec[1][8].x = eg_read_se(t->bs) + vecPred1.x;
            t->mb.vec[1][8].y = eg_read_se(t->bs) + vecPred1.y;
            t->mb.vec[1][8].refno = vecPred1.refno;
            copy_nvec(&t->mb.vec[1][8], &t->mb.vec[1][8], 4, 2, 4);

			break;

        case 9:	//B_L0_L1_8x16
			t->mb.mb_part = MB_8x16;
            if (t->refl0_num - 1 > 0)
            {
                vecPred0.refno = eg_read_te(t->bs, t->refl0_num - 1);
            }
			if (t->refl1_num - 1 > 0)
            {
                vecPred1.refno = eg_read_te(t->bs, t->refl1_num - 1);
            }
            T264_predict_mv(t, 0, 0, 2, &vecPred0);
            t->mb.vec[0][0].x = eg_read_se(t->bs) + vecPred0.x;
            t->mb.vec[0][0].y = eg_read_se(t->bs) + vecPred0.y;
            t->mb.vec[0][0].refno = vecPred0.refno;
            copy_nvec(&t->mb.vec[0][0], &t->mb.vec[0][0], 2, 4, 4);
			
			INITINVALIDVEC(t->mb.vec[0][2]);
            copy_nvec(&t->mb.vec[0][2], &t->mb.vec[0][2], 2, 4, 4);

			INITINVALIDVEC(t->mb.vec[1][0]);
            copy_nvec(&t->mb.vec[1][0], &t->mb.vec[1][0], 2, 4, 4);
			INITINVALIDVEC(t->mb.vec_ref[VEC_LUMA + 1].vec[1]);

            T264_predict_mv(t, 1, luma_index[4], 2, &vecPred1);
            t->mb.vec[1][luma_index[4]].x = eg_read_se(t->bs) + vecPred1.x;
            t->mb.vec[1][luma_index[4]].y = eg_read_se(t->bs) + vecPred1.y;
            t->mb.vec[1][luma_index[4]].refno = vecPred1.refno;
            copy_nvec(&t->mb.vec[1][2], &t->mb.vec[1][2], 2, 4, 4);
            break;

		case 10:	//B_L1_L0_16x8:
			t->mb.mb_part = MB_16x8;
			if (t->refl0_num - 1 > 0)
            {
                vec1[0].refno = eg_read_te(t->bs, t->refl0_num - 1);
            }
			if (t->refl1_num - 1 > 0)
            {
                vecPred1.refno = eg_read_te(t->bs, t->refl1_num - 1);
            }
            INITINVALIDVEC(t->mb.vec[0][0]);
            copy_nvec(&t->mb.vec[0][0], &t->mb.vec[0][0], 4, 2, 4);
			INITINVALIDVEC(t->mb.vec_ref[VEC_LUMA + 8].vec[0]);

            T264_predict_mv(t, 0, 8, 4, &vec1[0]);
            t->mb.vec[0][8].x = eg_read_se(t->bs) + vec1[0].x;
            t->mb.vec[0][8].y = eg_read_se(t->bs) + vec1[0].y;
            t->mb.vec[0][8].refno = vecPred1.refno;
            copy_nvec(&t->mb.vec[0][8], &t->mb.vec[0][8], 4, 2, 4);

            T264_predict_mv(t, 1, 0, 4, &vecPred1);
            t->mb.vec[1][0].x = eg_read_se(t->bs) + vecPred1.x;
            t->mb.vec[1][0].y = eg_read_se(t->bs) + vecPred1.y;
            t->mb.vec[1][0].refno = vecPred1.refno;
            copy_nvec(&t->mb.vec[1][0], &t->mb.vec[1][0], 4, 2, 4);

            INITINVALIDVEC(t->mb.vec[1][8]);
            copy_nvec(&t->mb.vec[1][8], &t->mb.vec[1][8], 4, 2, 4);

			break;

		case 11://B_L1_L0_8x16
			t->mb.mb_part = MB_8x16;
			if (t->refl0_num - 1 > 0)
            {
                vec1[0].refno = eg_read_te(t->bs, t->refl0_num - 1);
            }
			if (t->refl1_num - 1 > 0)
            {
                vecPred1.refno = eg_read_te(t->bs, t->refl1_num - 1);
            }
			INITINVALIDVEC(t->mb.vec[0][0]);
            copy_nvec(&t->mb.vec[0][0], &t->mb.vec[0][0], 2, 4, 4);            
			INITINVALIDVEC(t->mb.vec_ref[VEC_LUMA + 1].vec[0]);

			T264_predict_mv(t, 0, luma_index[4], 2, &vec1[0]);
            t->mb.vec[0][luma_index[4]].x = eg_read_se(t->bs) + vec1[0].x;
            t->mb.vec[0][luma_index[4]].y = eg_read_se(t->bs) + vec1[0].y;
            t->mb.vec[0][luma_index[4]].refno = vec1[0].refno;
            copy_nvec(&t->mb.vec[0][2], &t->mb.vec[0][2], 2, 4, 4);

            T264_predict_mv(t, 1, 0, 2, &vecPred1);
            t->mb.vec[1][0].x = eg_read_se(t->bs) + vecPred1.x;
            t->mb.vec[1][0].y = eg_read_se(t->bs) + vecPred1.y;
            t->mb.vec[1][0].refno = vecPred1.refno;
            copy_nvec(&t->mb.vec[1][0], &t->mb.vec[1][0], 2, 4, 4);
			
			INITINVALIDVEC(t->mb.vec[1][2]);
            copy_nvec(&t->mb.vec[1][2], &t->mb.vec[1][2], 2, 4, 4);
            break;

		case 12:	//B_L0_Bi_16x8
			t->mb.mb_part = MB_16x8;
			if (t->refl0_num - 1 > 0)
            {
                vecPred0.refno = eg_read_te(t->bs, t->refl0_num - 1);
				vec1[0].refno = eg_read_te(t->bs, t->refl0_num - 1);
            }
			if (t->refl1_num - 1 > 0)
            {
                vecPred1.refno = eg_read_te(t->bs, t->refl1_num - 1);
            }
            T264_predict_mv(t, 0, 0, 4, &vecPred0);
            t->mb.vec[0][0].x = eg_read_se(t->bs) + vecPred0.x;
            t->mb.vec[0][0].y = eg_read_se(t->bs) + vecPred0.y;
            t->mb.vec[0][0].refno = vecPred0.refno;
            copy_nvec(&t->mb.vec[0][0], &t->mb.vec[0][0], 4, 2, 4);
            t->mb.vec_ref[VEC_LUMA + 8].vec[0] = t->mb.vec[0][0];	

            T264_predict_mv(t, 0, 8, 4, &vec1[0]);
			t->mb.vec[0][8].x = eg_read_se(t->bs) + vec1[0].x;
            t->mb.vec[0][8].y = eg_read_se(t->bs) + vec1[0].y;
            t->mb.vec[0][8].refno = vec1[0].refno;
            copy_nvec(&t->mb.vec[0][8], &t->mb.vec[0][8], 4, 2, 4);

            INITINVALIDVEC(t->mb.vec[1][0]);
            copy_nvec(&t->mb.vec[1][0], &t->mb.vec[1][0], 4, 2, 4);
			INITINVALIDVEC(t->mb.vec_ref[VEC_LUMA + 8].vec[1]);

            T264_predict_mv(t, 1, 8, 4, &vecPred1);
            t->mb.vec[1][8].x = eg_read_se(t->bs) + vecPred1.x;
            t->mb.vec[1][8].y = eg_read_se(t->bs) + vecPred1.y;
            t->mb.vec[1][8].refno = vecPred1.refno;
            copy_nvec(&t->mb.vec[1][8], &t->mb.vec[1][8], 4, 2, 4);

			break;
		case 13://B_L0_Bi_8x16
			t->mb.mb_part = MB_8x16;
			if (t->refl0_num - 1 > 0)
            {
                vecPred0.refno = eg_read_te(t->bs, t->refl0_num - 1);
				vec1[0].refno = eg_read_te(t->bs, t->refl0_num - 1);
            }
			if (t->refl1_num - 1 > 0)
            {
                vec1[1].refno = eg_read_te(t->bs, t->refl1_num - 1);
            }

            T264_predict_mv(t, 0, 0, 2, &vecPred0);
            t->mb.vec[0][0].x = eg_read_se(t->bs) + vecPred0.x;
            t->mb.vec[0][0].y = eg_read_se(t->bs) + vecPred0.y;
            t->mb.vec[0][0].refno = vecPred0.refno;
            copy_nvec(&t->mb.vec[0][0], &t->mb.vec[0][0], 2, 4, 4);
            t->mb.vec_ref[VEC_LUMA + 1].vec[0] = t->mb.vec[0][0];

            T264_predict_mv(t, 0, luma_index[4], 2, &vec1[0]);
            t->mb.vec[0][luma_index[4]].x = eg_read_se(t->bs) + vec1[0].x;
            t->mb.vec[0][luma_index[4]].y = eg_read_se(t->bs) + vec1[0].y;
            t->mb.vec[0][luma_index[4]].refno = vec1[0].refno;
            copy_nvec(&t->mb.vec[0][2], &t->mb.vec[0][2], 2, 4, 4);

			INITINVALIDVEC(t->mb.vec[1][0]);
            copy_nvec(&t->mb.vec[1][0], &t->mb.vec[1][0], 2, 4, 4);
			INITINVALIDVEC(t->mb.vec_ref[VEC_LUMA + 1].vec[1]);

            T264_predict_mv(t, 1, luma_index[4], 2, &vec1[1]);
            t->mb.vec[1][luma_index[4]].x = eg_read_se(t->bs) + vec1[1].x;
            t->mb.vec[1][luma_index[4]].y = eg_read_se(t->bs) + vec1[1].y;
            t->mb.vec[1][luma_index[4]].refno = vec1[1].refno;
            copy_nvec(&t->mb.vec[1][2], &t->mb.vec[1][2], 2, 4, 4);			

            break;

		case 14://B_L1_Bi_16x8
			t->mb.mb_part = MB_16x8;
			if (t->refl0_num - 1 > 0)
            {
				vec1[0].refno = eg_read_te(t->bs, t->refl0_num - 1);
            }
			if (t->refl1_num - 1 > 0)
            {
                vecPred1.refno = eg_read_te(t->bs, t->refl0_num - 1);
                vec1[1].refno = eg_read_te(t->bs, t->refl1_num - 1);
            }
            INITINVALIDVEC(t->mb.vec[0][0]);
            copy_nvec(&t->mb.vec[0][0], &t->mb.vec[0][0], 4, 2, 4);

            INITINVALIDVEC(t->mb.vec_ref[VEC_LUMA + 8].vec[0]);

            T264_predict_mv(t, 0, 8, 4, &vec1[0]);
			t->mb.vec[0][8].x = eg_read_se(t->bs) + vec1[0].x;
            t->mb.vec[0][8].y = eg_read_se(t->bs) + vec1[0].y;
            t->mb.vec[0][8].refno = vec1[0].refno;
            copy_nvec(&t->mb.vec[0][8], &t->mb.vec[0][8], 4, 2, 4);

            T264_predict_mv(t, 1, 0, 4, &vecPred1);
            t->mb.vec[1][0].x = eg_read_se(t->bs) + vecPred1.x;
            t->mb.vec[1][0].y = eg_read_se(t->bs) + vecPred1.y;
            t->mb.vec[1][0].refno = vecPred1.refno;
            copy_nvec(&t->mb.vec[1][0], &t->mb.vec[1][0], 4, 2, 4);
            t->mb.vec_ref[VEC_LUMA + 8].vec[1] = t->mb.vec[1][0];	

            T264_predict_mv(t, 1, 8, 4, &vec1[1]);
            t->mb.vec[1][8].x = eg_read_se(t->bs) + vec1[1].x;
            t->mb.vec[1][8].y = eg_read_se(t->bs) + vec1[1].y;
            t->mb.vec[1][8].refno = vec1[1].refno;
            copy_nvec(&t->mb.vec[1][8], &t->mb.vec[1][8], 4, 2, 4);
			break;

		case 15:	//B_L1_Bi_8x16
			t->mb.mb_part = MB_8x16;
			if (t->refl0_num - 1 > 0)
            {
                vec1[0].refno = eg_read_te(t->bs, t->refl0_num - 1);
            }
			if (t->refl1_num - 1 > 0)
            {
                vecPred1.refno = eg_read_te(t->bs, t->refl1_num - 1);
                vec1[1].refno = eg_read_te(t->bs, t->refl0_num - 1);
            }
			INITINVALIDVEC(t->mb.vec_ref[VEC_LUMA + 1].vec[0]);

			INITINVALIDVEC(t->mb.vec[0][0]);
            copy_nvec(&t->mb.vec[0][0], &t->mb.vec[0][0], 2, 4, 4);

            T264_predict_mv(t, 0, luma_index[4], 2, &vec1[0]);
            t->mb.vec[0][luma_index[4]].x = eg_read_se(t->bs) + vec1[0].x;
            t->mb.vec[0][luma_index[4]].y = eg_read_se(t->bs) + vec1[0].y;
            t->mb.vec[0][luma_index[4]].refno = vec1[0].refno;
            copy_nvec(&t->mb.vec[0][2], &t->mb.vec[0][2], 2, 4, 4);

            T264_predict_mv(t, 1, 0, 2, &vecPred1);
            t->mb.vec[1][0].x = eg_read_se(t->bs) + vecPred1.x;
            t->mb.vec[1][0].y = eg_read_se(t->bs) + vecPred1.y;
            t->mb.vec[1][0].refno = vecPred1.refno;
            copy_nvec(&t->mb.vec[1][0], &t->mb.vec[1][0], 2, 4, 4);
            t->mb.vec_ref[VEC_LUMA + 1].vec[1] = t->mb.vec[1][0];

            T264_predict_mv(t, 1, luma_index[4], 2, &vec1[1]);
            t->mb.vec[1][luma_index[4]].x = eg_read_se(t->bs) + vec1[1].x;
            t->mb.vec[1][luma_index[4]].y = eg_read_se(t->bs) + vec1[1].y;
            t->mb.vec[1][luma_index[4]].refno = vec1[1].refno;
            copy_nvec(&t->mb.vec[1][2], &t->mb.vec[1][2], 2, 4, 4);

            break;

		case 16:	//B_Bi_L0_16x8
			t->mb.mb_part = MB_16x8;
			if (t->refl0_num - 1 > 0)
            {
                vecPred0.refno = eg_read_te(t->bs, t->refl1_num - 1);
                vec1[0].refno = eg_read_te(t->bs, t->refl0_num - 1);
            }
			if (t->refl1_num - 1 > 0)
            {
                vecPred1.refno = eg_read_te(t->bs, t->refl1_num - 1);
            }

			T264_predict_mv(t, 0, 0, 4, &vecPred0);
            t->mb.vec[0][0].x = eg_read_se(t->bs) + vecPred0.x;
            t->mb.vec[0][0].y = eg_read_se(t->bs) + vecPred0.y;
            t->mb.vec[0][0].refno = vecPred0.refno;
            copy_nvec(&t->mb.vec[0][0], &t->mb.vec[0][0], 4, 2, 4);
            t->mb.vec_ref[VEC_LUMA + 8].vec[0] = t->mb.vec[0][0];	

            T264_predict_mv(t, 0, 8, 4, &vec1[0]);
            t->mb.vec[0][8].x = eg_read_se(t->bs) + vec1[0].x;
            t->mb.vec[0][8].y = eg_read_se(t->bs) + vec1[0].y;
            t->mb.vec[0][8].refno = vecPred1.refno;
            copy_nvec(&t->mb.vec[0][8], &t->mb.vec[0][8], 4, 2, 4);

            T264_predict_mv(t, 1, 0, 4, &vecPred1);
            t->mb.vec[1][0].x = eg_read_se(t->bs) + vecPred1.x;
            t->mb.vec[1][0].y = eg_read_se(t->bs) + vecPred1.y;
            t->mb.vec[1][0].refno = vecPred1.refno;
            copy_nvec(&t->mb.vec[1][0], &t->mb.vec[1][0], 4, 2, 4);
            t->mb.vec_ref[VEC_LUMA + 8].vec[1] = t->mb.vec[1][0];	

            INITINVALIDVEC(t->mb.vec[1][8]);
            copy_nvec(&t->mb.vec[1][8], &t->mb.vec[1][8], 4, 2, 4);
			
			break;
		case 17:	//B_Bi_L0_8x16
			t->mb.mb_part = MB_8x16;
			if (t->refl0_num - 1 > 0)
            {
                vecPred0.refno = eg_read_te(t->bs, t->refl1_num - 1);
                vec1[0].refno = eg_read_te(t->bs, t->refl0_num - 1);
            }
			if (t->refl1_num - 1 > 0)
            {
                vecPred1.refno = eg_read_te(t->bs, t->refl1_num - 1);
            }

			T264_predict_mv(t, 0, 0, 2, &vecPred0);
            t->mb.vec[0][0].x = eg_read_se(t->bs) + vecPred0.x;
            t->mb.vec[0][0].y = eg_read_se(t->bs) + vecPred0.y;
            t->mb.vec[0][0].refno = vecPred0.refno;
            copy_nvec(&t->mb.vec[0][0], &t->mb.vec[0][0], 2, 4, 4);
            t->mb.vec_ref[VEC_LUMA + 1].vec[0] = t->mb.vec[0][0];	

			T264_predict_mv(t, 0, 2, 2, &vec1[0]);
            t->mb.vec[0][2].x = eg_read_se(t->bs) + vec1[0].x;
            t->mb.vec[0][2].y = eg_read_se(t->bs) + vec1[0].y;
            t->mb.vec[0][2].refno = vecPred1.refno;
            copy_nvec(&t->mb.vec[0][2], &t->mb.vec[0][2], 2, 4, 4);

            T264_predict_mv(t, 1, 0, 2, &vecPred1);
            t->mb.vec[1][0].x = eg_read_se(t->bs) + vecPred1.x;
            t->mb.vec[1][0].y = eg_read_se(t->bs) + vecPred1.y;
            t->mb.vec[1][0].refno = vecPred1.refno;
            copy_nvec(&t->mb.vec[1][0], &t->mb.vec[1][0], 2, 4, 4);
            t->mb.vec_ref[VEC_LUMA + 1].vec[1] = t->mb.vec[1][0];	
			
            INITINVALIDVEC(t->mb.vec[1][2]);
            copy_nvec(&t->mb.vec[1][2], &t->mb.vec[1][2], 2, 4, 4);

			break;
		case 18://B_Bi_L1_16x8
			t->mb.mb_part = MB_16x8;
			if (t->refl0_num - 1 > 0)
            {
                vecPred0.refno = eg_read_te(t->bs, t->refl1_num - 1);
            }
			if (t->refl1_num - 1 > 0)
            {
                vecPred1.refno = eg_read_te(t->bs, t->refl1_num - 1);
                vec1[1].refno = eg_read_te(t->bs, t->refl0_num - 1);
            }

			T264_predict_mv(t, 0, 0, 4, &vecPred0);
            t->mb.vec[0][0].x = eg_read_se(t->bs) + vecPred0.x;
            t->mb.vec[0][0].y = eg_read_se(t->bs) + vecPred0.y;
            t->mb.vec[0][0].refno = vecPred0.refno;
            copy_nvec(&t->mb.vec[0][0], &t->mb.vec[0][0], 4, 2, 4);
            t->mb.vec_ref[VEC_LUMA + 8].vec[0] = t->mb.vec[0][0];	

            T264_predict_mv(t, 1, 0, 4, &vecPred1);
            t->mb.vec[1][0].x = eg_read_se(t->bs) + vecPred1.x;
            t->mb.vec[1][0].y = eg_read_se(t->bs) + vecPred1.y;
            t->mb.vec[1][0].refno = vecPred1.refno;
            copy_nvec(&t->mb.vec[1][0], &t->mb.vec[1][0], 4, 2, 4);
            t->mb.vec_ref[VEC_LUMA + 8].vec[1] = t->mb.vec[1][0];	

            T264_predict_mv(t, 1, 8, 4, &vec1[1]);
            t->mb.vec[1][8].x = eg_read_se(t->bs) + vec1[1].x;
            t->mb.vec[1][8].y = eg_read_se(t->bs) + vec1[1].y;
            t->mb.vec[1][8].refno = vecPred1.refno;
            copy_nvec(&t->mb.vec[1][8], &t->mb.vec[1][8], 4, 2, 4);

            INITINVALIDVEC(t->mb.vec[0][8]);
            copy_nvec(&t->mb.vec[0][8], &t->mb.vec[0][8], 4, 2, 4);

			break;

		case 19://B_Bi_L1_8x16
			t->mb.mb_part = MB_8x16;
			if (t->refl0_num - 1 > 0)
            {
                vecPred0.refno = eg_read_te(t->bs, t->refl1_num - 1);
            }
			if (t->refl1_num - 1 > 0)
            {
                vecPred1.refno = eg_read_te(t->bs, t->refl1_num - 1);
                vec1[1].refno = eg_read_te(t->bs, t->refl0_num - 1);
            }

			T264_predict_mv(t, 0, 0, 2, &vecPred0);
            t->mb.vec[0][0].x = eg_read_se(t->bs) + vecPred0.x;
            t->mb.vec[0][0].y = eg_read_se(t->bs) + vecPred0.y;
            t->mb.vec[0][0].refno = vecPred0.refno;
            copy_nvec(&t->mb.vec[0][0], &t->mb.vec[0][0], 2, 4, 4);
            t->mb.vec_ref[VEC_LUMA + 1].vec[0] = t->mb.vec[0][0];	

            T264_predict_mv(t, 1, 0, 2, &vecPred1);
            t->mb.vec[1][0].x = eg_read_se(t->bs) + vecPred1.x;
            t->mb.vec[1][0].y = eg_read_se(t->bs) + vecPred1.y;
            t->mb.vec[1][0].refno = vecPred1.refno;
            copy_nvec(&t->mb.vec[1][0], &t->mb.vec[1][0], 2, 4, 4);
            t->mb.vec_ref[VEC_LUMA + 1].vec[1] = t->mb.vec[1][0];	
			
            T264_predict_mv(t, 1, luma_index[4], 2, &vec1[1]);
            t->mb.vec[1][2].x = eg_read_se(t->bs) + vec1[1].x;
            t->mb.vec[1][2].y = eg_read_se(t->bs) + vec1[1].y;
            t->mb.vec[1][2].refno = vec1[1].refno;
            copy_nvec(&t->mb.vec[1][2], &t->mb.vec[1][2], 2, 4, 4);

            INITINVALIDVEC(t->mb.vec[0][2]);
            copy_nvec(&t->mb.vec[0][2], &t->mb.vec[0][2], 2, 4, 4);

			break;

		case 20:	//B_Bi_Bi_16x8
			t->mb.mb_part = MB_16x8;
			if (t->refl0_num - 1 > 0)
            {
                vecPred0.refno = eg_read_te(t->bs, t->refl1_num - 1);
                vec1[0].refno = eg_read_te(t->bs, t->refl0_num - 1);
            }
			if (t->refl1_num - 1 > 0)
            {
                vecPred1.refno = eg_read_te(t->bs, t->refl1_num - 1);
                vec1[1].refno = eg_read_te(t->bs, t->refl0_num - 1);
            }

			T264_predict_mv(t, 0, 0, 4, &vecPred0);
            t->mb.vec[0][0].x = eg_read_se(t->bs) + vecPred0.x;
            t->mb.vec[0][0].y = eg_read_se(t->bs) + vecPred0.y;
            t->mb.vec[0][0].refno = vecPred0.refno;
            copy_nvec(&t->mb.vec[0][0], &t->mb.vec[0][0], 4, 2, 4);
            t->mb.vec_ref[VEC_LUMA + 8].vec[0] = t->mb.vec[0][0];	

            T264_predict_mv(t, 0, 8, 4, &vec1[0]);
            t->mb.vec[0][8].x = eg_read_se(t->bs) + vec1[0].x;
            t->mb.vec[0][8].y = eg_read_se(t->bs) + vec1[0].y;
            t->mb.vec[0][8].refno = vec1[0].refno;
            copy_nvec(&t->mb.vec[0][8], &t->mb.vec[0][8], 4, 2, 4);

            T264_predict_mv(t, 1, 0, 4, &vecPred1);
            t->mb.vec[1][0].x = eg_read_se(t->bs) + vecPred1.x;
            t->mb.vec[1][0].y = eg_read_se(t->bs) + vecPred1.y;
            t->mb.vec[1][0].refno = vecPred1.refno;
            copy_nvec(&t->mb.vec[1][0], &t->mb.vec[1][0], 4, 2, 4);
            t->mb.vec_ref[VEC_LUMA + 8].vec[1] = t->mb.vec[1][0];	

            T264_predict_mv(t, 1, 8, 4, &vec1[1]);
            t->mb.vec[1][8].x = eg_read_se(t->bs) + vec1[1].x;
            t->mb.vec[1][8].y = eg_read_se(t->bs) + vec1[1].y;
            t->mb.vec[1][8].refno = vecPred1.refno;
            copy_nvec(&t->mb.vec[1][8], &t->mb.vec[1][8], 4, 2, 4);

			break;
		case 21://B_Bi_Bi_8x16
			t->mb.mb_part = MB_8x16;
			if (t->refl0_num - 1 > 0)
            {
                vecPred0.refno = eg_read_te(t->bs, t->refl1_num - 1);
                vec1[0].refno = eg_read_te(t->bs, t->refl0_num - 1);
            }
			if (t->refl1_num - 1 > 0)
            {
                vecPred1.refno = eg_read_te(t->bs, t->refl1_num - 1);
                vec1[1].refno = eg_read_te(t->bs, t->refl0_num - 1);
            }

			T264_predict_mv(t, 0, 0, 2, &vecPred0);
            t->mb.vec[0][0].x = eg_read_se(t->bs) + vecPred0.x;
            t->mb.vec[0][0].y = eg_read_se(t->bs) + vecPred0.y;
            t->mb.vec[0][0].refno = vecPred0.refno;
            copy_nvec(&t->mb.vec[0][0], &t->mb.vec[0][0], 2, 4, 4);
            t->mb.vec_ref[VEC_LUMA + 1].vec[0] = t->mb.vec[0][0];	

            T264_predict_mv(t, 0, 2, 2, &vec1[0]);
            t->mb.vec[0][2].x = eg_read_se(t->bs) + vec1[0].x;
            t->mb.vec[0][2].y = eg_read_se(t->bs) + vec1[0].y;
            t->mb.vec[0][2].refno = vec1[1].refno;
            copy_nvec(&t->mb.vec[0][2], &t->mb.vec[0][2], 2, 4, 4);

            T264_predict_mv(t, 1, 0, 2, &vecPred1);
            t->mb.vec[1][0].x = eg_read_se(t->bs) + vecPred1.x;
            t->mb.vec[1][0].y = eg_read_se(t->bs) + vecPred1.y;
            t->mb.vec[1][0].refno = vecPred1.refno;
            copy_nvec(&t->mb.vec[1][0], &t->mb.vec[1][0], 2, 4, 4);
            t->mb.vec_ref[VEC_LUMA + 1].vec[1] = t->mb.vec[1][0];	
			
            T264_predict_mv(t, 1, 2, 2, &vec1[1]);
            t->mb.vec[1][2].x = eg_read_se(t->bs) + vec1[1].x;
            t->mb.vec[1][2].y = eg_read_se(t->bs) + vec1[1].y;
            t->mb.vec[1][2].refno = vec1[1].refno;
            copy_nvec(&t->mb.vec[1][2], &t->mb.vec[1][2], 2, 4, 4);

			break;			

		case 22:	//B_8x8
			t->mb.submb_part[luma_index[4 * 0]] = eg_read_ue(t->bs);
            t->mb.submb_part[luma_index[4 * 1]] = eg_read_ue(t->bs);
            t->mb.submb_part[luma_index[4 * 2]] = eg_read_ue(t->bs);
            t->mb.submb_part[luma_index[4 * 3]] = eg_read_ue(t->bs);
			t->mb.mb_part = MB_8x8;
			if (t->refl0_num - 1 > 0)
			{
				for(i = 0;i < 4; ++i)
					if(t->mb.submb_part[luma_index[4 * i]] != B_DIRECT_8x8 - 100)
					{
						if (t->mb.submb_part[luma_index[4 * i]] != B_L1_8x8)
						{
							t->mb.vec[0][luma_index[4 * i]].refno = eg_read_te(t->bs, t->refl0_num - 1);
						}
						
					}
			}else
			{
				t->mb.vec[0][luma_index[0]].refno = 0;	
				t->mb.vec[0][luma_index[4]].refno = 0;	
				t->mb.vec[0][luma_index[8]].refno = 0;	
				t->mb.vec[0][luma_index[12]].refno = 0;	
			}
			if (t->refl1_num - 1 > 0)
			{
				for(i = 0;i < 4; ++i)
				if(t->mb.submb_part[luma_index[4 * i]] != B_DIRECT_8x8 - 100)
				{
					if (t->mb.submb_part[luma_index[4 * i]] != B_L0_8x8)
					{
						t->mb.vec[1][luma_index[4 * i]].refno = eg_read_te(t->bs, t->refl1_num - 1);
					}				
				}
			}else	
			{
				t->mb.vec[1][luma_index[0]].refno = 0;	
				t->mb.vec[1][luma_index[4]].refno = 0;	
				t->mb.vec[1][luma_index[8]].refno = 0;	
				t->mb.vec[1][luma_index[12]].refno = 0;	
			}

			for(i = 0 ; i < 4 ; i ++)
            {
                switch(t->mb.submb_part[luma_index[4 * i]]) 
                {
				case B_DIRECT_8x8 - 100:
//					submb_get_direct8x8_mv(t,i);
					assert(0);
					break;
				case B_L1_8x8 - 100:
					INITINVALIDVEC(t->mb.vec[0][luma_index[4 * i]]);
					copy_nvec(&t->mb.vec[0][luma_index[4 * i]], &t->mb.vec[0][luma_index[4 * i]], 2, 2, 4);
					t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 0].vec[0] =
                    t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 1].vec[0] =
                    t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 8].vec[0] =
                    t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 9].vec[0] = t->mb.vec[0][luma_index[4 * i]];
					break;
				case B_L0_8x8 - 100:
				case B_Bi_8x8 - 100:
                    vec1[0] = t->mb.vec[0][luma_index[4 * i]];
                    T264_predict_mv(t, 0, luma_index[4 * i], 2, &vec1[0]);
                    t->mb.vec[0][luma_index[4 * i]].x = eg_read_se(t->bs) + vec1[0].x;
                    t->mb.vec[0][luma_index[4 * i]].y = eg_read_se(t->bs) + vec1[0].y;

                    t->mb.vec[0][i / 2 * 8 + i % 2 * 2 + 1] = 
                    t->mb.vec[0][i / 2 * 8 + i % 2 * 2 + 4] = 
                    t->mb.vec[0][i / 2 * 8 + i % 2 * 2 + 5] = t->mb.vec[0][i / 2 * 8 + i % 2 * 2 + 0];
                    t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 0].vec[0] =
                    t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 1].vec[0] =
                    t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 8].vec[0] =
                    t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 9].vec[0] = t->mb.vec[0][i / 2 * 8 + i % 2 * 2 + 0];
					break;
				default :	//other mode are not supported
					assert(0);
					break;
				}
			}

			//for list1 mvd
			for(i = 0 ; i < 4 ; i ++)
            {
                switch(t->mb.submb_part[luma_index[4 * i]]) 
                {
				case B_DIRECT_8x8 - 100:	//will do at list0
				   t->mb.submb_part[luma_index[4 * i]] = MB_8x8;
					break;				
				case B_L0_8x8 - 100:
				   t->mb.submb_part[luma_index[4 * i]] = MB_8x8;
					INITINVALIDVEC(t->mb.vec[1][luma_index[4 * i]]);
					copy_nvec(&t->mb.vec[1][luma_index[4 * i]], &t->mb.vec[1][luma_index[4 * i]], 2, 2, 4);
					t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 0].vec[1] =
                    t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 1].vec[1] =
                    t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 8].vec[1] =
                    t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 9].vec[1] = t->mb.vec[1][luma_index[4 * i]];
					break;				
				case B_L1_8x8 - 100:
				case B_Bi_8x8 - 100:
	                t->mb.submb_part[luma_index[4 * i]] = MB_8x8;
                    vec1[1] = t->mb.vec[1][luma_index[4 * i]];
                    T264_predict_mv(t, 1, luma_index[4 * i], 2, &vec1[1]);
                    t->mb.vec[1][luma_index[4 * i]].x = eg_read_se(t->bs) + vec1[1].x;
                    t->mb.vec[1][luma_index[4 * i]].y = eg_read_se(t->bs) + vec1[1].y;

                    t->mb.vec[1][i / 2 * 8 + i % 2 * 2 + 1] = 
                    t->mb.vec[1][i / 2 * 8 + i % 2 * 2 + 4] = 
                    t->mb.vec[1][i / 2 * 8 + i % 2 * 2 + 5] = t->mb.vec[1][i / 2 * 8 + i % 2 * 2 + 0];
                    t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 0].vec[1] =
                    t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 1].vec[1] =
                    t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 8].vec[1] =
                    t->mb.vec_ref[VEC_LUMA + i / 2 * 16 + i % 2 * 2 + 9].vec[1] = t->mb.vec[1][i / 2 * 8 + i % 2 * 2 + 0];

					break;

				default :	//other mode are not supported
					assert(0);
					break;
				}
			}
			break;
        default:
            t->mb.mb_part = mb_type - 23;
            T264dec_mb_read_intra_cavlc(t);

            /* save ref */
            memset(t->mb.submb_part, -1, sizeof(t->mb.submb_part));
            t->mb.mb_part = -1;
            for(i = 0 ; i < 2 ; i ++)
            {
                for(j = 0 ; j < 16 ; j ++)
                {
                    INITINVALIDVEC(t->mb.vec[i][j]);
                }
            }
            break;
        }
    }
    else
    {
        t->mb.mb_part = mb_type;
        T264dec_mb_read_intra_cavlc(t);
    }

    if (t->mb.mb_mode != I_16x16 && t->mb.mb_mode != I_4x4)
    {
        int32_t cbp;

        cbp = inter_eg_to_cbp[eg_read_ue(t->bs)];
        t->mb.cbp_y = cbp % 16;
        t->mb.cbp_c = cbp / 16;

        if (cbp > 0)
        {
            t->mb.mb_qp_delta = eg_read_se(t->bs);

            for(i = 0 ; i < 16 ; i ++)
            {
                if (t->mb.cbp_y & (1 << (i / 4)))
                {
                    T264dec_mb_read_cavlc_residual(t, i, t->mb.dct_y_z[i], 16);
                }
            }
        }
    }
    if (t->mb.cbp_c != 0)
    {
        T264dec_mb_read_cavlc_residual(t, BLOCK_INDEX_CHROMA_DC, t->mb.dc2x2_z[0], 4);
        T264dec_mb_read_cavlc_residual(t, BLOCK_INDEX_CHROMA_DC, t->mb.dc2x2_z[1], 4);
        if (t->mb.cbp_c & 0x2)
        {
            for(i = 0 ; i < 4 ; i ++)
            {
                T264dec_mb_read_cavlc_residual(t, 16 + i, &(t->mb.dct_uv_z[0][i % 4][1]), 15);
                t->mb.dct_uv_z[0][i][0] = t->mb.dc2x2_z[0][i];
            }
            for(i = 0 ; i < 4 ; i ++)
            {
                T264dec_mb_read_cavlc_residual(t, 20 + i, &(t->mb.dct_uv_z[1][i % 4][1]), 15);
                t->mb.dct_uv_z[1][i][0] = t->mb.dc2x2_z[1][i];
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
}
