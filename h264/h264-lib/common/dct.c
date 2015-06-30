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

#include "stdio.h"
#include "T264.h"

#define Q_BITS          15
#define DQ_BITS         6
#define DQ_ROUND        (1<<(DQ_BITS-1))

//////////////////////////////////////////////////////
// static var
DECLARE_ALIGNED2_MATRIX_H(quant, 6, 4 * 4, int16_t, CACHE_SIZE) = 
{
    13107, 8066, 13107, 8066, 8066, 5243, 8066, 5243,
    13107, 8066, 13107, 8066, 8066, 5243, 8066, 5243,
    11916, 7490, 11916, 7490, 7490, 4660, 7490, 4660,
    11916, 7490, 11916, 7490, 7490, 4660, 7490, 4660,
    10082, 6554, 10082, 6554, 6554, 4194, 6554, 4194,
    10082, 6554, 10082, 6554, 6554, 4194, 6554, 4194,
     9362, 5825,  9362, 5825, 5825, 3647, 5825, 3647,
     9362, 5825,  9362, 5825, 5825, 3647, 5825, 3647,
     8192, 5243,  8192, 5243, 5243, 3355, 5243, 3355,
     8192, 5243,  8192, 5243, 5243, 3355, 5243, 3355,
     7282, 4559,  7282, 4559, 4559, 2893, 4559, 2893,
     7282, 4559,  7282, 4559, 4559, 2893, 4559, 2893
};

DECLARE_ALIGNED2_MATRIX_H(dequant, 6, 4 * 4, int16_t, CACHE_SIZE) = 
{
    10, 13, 10, 13, 13, 16, 13, 16, 10, 13, 10, 13, 13, 16, 13, 16,
    11, 14, 11, 14, 14, 18, 14, 18, 11, 14, 11, 14, 14, 18, 14, 18,
    13, 16, 13, 16, 16, 20, 16, 20, 13, 16, 13, 16, 16, 20, 16, 20,
    14, 18, 14, 18, 18, 23, 18, 23, 14, 18, 14, 18, 18, 23, 18, 23,
    16, 20, 16, 20, 20, 25, 20, 25, 16, 20, 16, 20, 20, 25, 20, 25,
    18, 23, 18, 23, 23, 29, 23, 29, 18, 23, 18, 23, 23, 29, 23, 29
};

//////////////////////////////////////////////////////
// DCT & IDCT
void 
dct4x4_c(int16_t* data)
{
    int32_t i;
    int16_t s[4];

    //
    // horizontal
    //
    for(i = 0 ; i < 4 ; i ++)
    {
        s[0] = data[i * 4 + 0] + data[i * 4 + 3];
        s[3] = data[i * 4 + 0] - data[i * 4 + 3];
        s[1] = data[i * 4 + 1] + data[i * 4 + 2];
        s[2] = data[i * 4 + 1] - data[i * 4 + 2];

        data[i * 4 + 0] = s[0] + s[1];
        data[i * 4 + 2] = s[0] - s[1];
        data[i * 4 + 1] = (s[3] << 1) + s[2];
        data[i * 4 + 3] = s[3] - (s[2] << 1);
    }

    //
    // vertical
    //
    for(i = 0 ; i < 4 ; i ++)
    {
        s[0] = data[0 * 4 + i] + data[3 * 4 + i];
        s[3] = data[0 * 4 + i] - data[3 * 4 + i];
        s[1] = data[1 * 4 + i] + data[2 * 4 + i];
        s[2] = data[1 * 4 + i] - data[2 * 4 + i];

        data[0 * 4 + i] = s[0] + s[1];
        data[2 * 4 + i] = s[0] - s[1];
        data[1 * 4 + i] = (s[3] << 1) + s[2];
        data[3 * 4 + i] = s[3] - (s[2] << 1);
    }
}

void 
dct4x4dc_c(int16_t* data)
{
    int32_t i;
    int16_t s[4];

    for(i = 0 ; i < 4 ; i ++)
    {
        s[0] = data[i * 4 + 0] + data[i * 4 + 3];
        s[3] = data[i * 4 + 0] - data[i * 4 + 3];
        s[1] = data[i * 4 + 1] + data[i * 4 + 2];
        s[2] = data[i * 4 + 1] - data[i * 4 + 2];

        data[i * 4 + 0] = s[0] + s[1];
        data[i * 4 + 2] = s[0] - s[1];
        data[i * 4 + 1] = s[3] + s[2];
        data[i * 4 + 3] = s[3] - s[2];
    }

    for(i = 0 ; i < 4 ; i ++)
    {
        s[0] = data[0 * 4 + i] + data[3 * 4 + i];
        s[3] = data[0 * 4 + i] - data[3 * 4 + i];
        s[1] = data[1 * 4 + i] + data[2 * 4 + i];
        s[2] = data[1 * 4 + i] - data[2 * 4 + i];

        data[0 * 4 + i] = (s[0] + s[1] + 1) >> 1;
        data[2 * 4 + i] = (s[0] - s[1] + 1) >> 1;
        data[1 * 4 + i] = (s[3] + s[2] + 1) >> 1;
        data[3 * 4 + i] = (s[3] - s[2] + 1) >> 1;
    }
}

void 
dct2x2dc_c(int16_t* data)
{   
    int16_t s[4];

    s[0] = data[0];
    s[1] = data[1];
    s[2] = data[2];
    s[3] = data[3];

    data[0] = s[0] + s[2] + s[1] + s[3];
    data[1] = s[0] + s[2] - s[1] - s[3];
    data[2] = s[0] - s[2] + s[1] - s[3];
    data[3] = s[0] - s[2] - s[1] + s[3];
}

void
idct4x4_c(int16_t* data)
{
    int32_t i;
    int16_t s[4];

    for (i = 0; i < 4; i ++)
    {
        s[0] = data[i * 4 + 0] + data[i * 4 + 2];
        s[1] = data[i * 4 + 0] - data[i * 4 + 2];
        s[2] = (data[i * 4 + 1] >> 1) - data[i * 4 + 3];
        s[3] = data[i * 4 + 1] + (data[i * 4 + 3] >> 1);

        data[i * 4 + 0] = s[0] + s[3];
        data[i * 4 + 3] = s[0] - s[3];
        data[i * 4 + 1] = s[1] + s[2];
        data[i * 4 + 2] = s[1] - s[2];
    }

    for (i = 0; i < 4; i ++)
    {
        s[0] = data[0 * 4 + i] + data[2 * 4 + i];
        s[1] = data[0 * 4 + i] - data[2 * 4 + i];
        s[2] = (data[1 * 4 + i] >> 1) - data[3 * 4 + i];
        s[3] = data[1 * 4 + i] + (data[3 * 4 + i] >> 1);

        data[0 * 4 + i] = (s[0] + s[3] + 32) >> 6;
        data[3 * 4 + i] = (s[0] - s[3] + 32) >> 6;
        data[1 * 4 + i] = (s[1] + s[2] + 32) >> 6;
        data[2 * 4 + i] = (s[1] - s[2] + 32) >> 6;
    }
}

void 
idct4x4dc_c(int16_t* data)
{
    int32_t i;
    int16_t s[4];

    for (i = 0; i < 4; i ++)
    {
        s[0] = data[i * 4 + 0] + data[i * 4 + 2];
        s[1] = data[i * 4 + 0] - data[i * 4 + 2];
        s[2] = data[i * 4 + 1] - data[i * 4 + 3];
        s[3] = data[i * 4 + 1] + data[i * 4 + 3];

        data[i * 4 + 0] = s[0] + s[3];
        data[i * 4 + 3] = s[0] - s[3];
        data[i * 4 + 1] = s[1] + s[2];
        data[i * 4 + 2] = s[1] - s[2];
    }

    for (i = 0; i < 4; i ++)
    {
        s[0] = data[0 * 4 + i] + data[2 * 4 + i];
        s[1] = data[0 * 4 + i] - data[2 * 4 + i];
        s[2] = data[1 * 4 + i] - data[3 * 4 + i];
        s[3] = data[1 * 4 + i] + data[3 * 4 + i];

        data[0 * 4 + i] = s[0] + s[3];
        data[3 * 4 + i] = s[0] - s[3];
        data[1 * 4 + i] = s[1] + s[2];
        data[2 * 4 + i] = s[1] - s[2];
    }
}

void 
idct2x2dc_c(int16_t* data)
{
    int16_t s[4];

    s[0] = data[0];
    s[1] = data[1];
    s[2] = data[2];
    s[3] = data[3];

    data[0] = s[0] + s[2] + s[1] + s[3];
    data[1] = s[0] + s[2] - s[1] - s[3];
    data[2] = s[0] - s[2] + s[1] - s[3];
    data[3] = s[0] - s[2] - s[1] + s[3];
}

///////////////////////////////////////////////////////////
// Quant & IQuant
void
quant4x4_c(int16_t* data, const int32_t Qp, int32_t is_intra)
{
    const int32_t qbits    = 15 + Qp / 6;
    const int32_t mf_index = Qp % 6;
    int32_t i;
    const int32_t f = (1 << qbits) / (is_intra ? 3 : 6);

    for(i = 0 ; i < 16 ; i ++)
    {
        if (data[i] > 0)
            data[i] = (data[i] * quant[mf_index][i] + f) >> qbits;
        else
            data[i] = -((-(data[i] * quant[mf_index][i]) + f) >> qbits);
    }
}

void
quant4x4dc_c(int16_t* data, const int32_t Qp)
{
    const int32_t qbits    = 15 + Qp / 6;
    const int32_t mf_index = Qp % 6;
    const int32_t mf00 = quant[mf_index][0];
    const int32_t f2 = (2 << qbits) / 3;	// Only 16x16 intra mode
    int32_t i;

    for(i = 0 ; i < 16 ; i ++)
    {
        if (data[i] > 0)
            data[i] = (data[i] * mf00 + f2) >> (qbits + 1);
        else
            data[i] = -((-(data[i] * mf00) + f2) >> (qbits + 1));
    }
}

void
quant2x2dc_c(int16_t* data, const int32_t Qp, int32_t is_intra)
{
    const int32_t qbits    = 15 + Qp / 6;
    const int32_t mf_index = Qp % 6;
    const int32_t mf00 = quant[mf_index][0];
    const int32_t f2 = (2 << qbits) / (is_intra ? 3 : 6);
    int32_t i;

    for(i = 0 ; i < 4 ; i ++)
    {
        if (data[i] > 0)
            data[i] = (data[i] * mf00 + f2) >> (qbits + 1);
        else
            data[i] = -((-(data[i] * mf00) + f2) >> (qbits + 1));
    }
}

void
iquant4x4_c(int16_t* data, const int32_t Qp)
{
    const int32_t qbits = Qp / 6;
    const int32_t index_mf = Qp % 6;
    int32_t i;

    for(i = 0 ; i < 16 ; i ++)
    {
        data[i] = (data[i] * dequant[index_mf][i]) << qbits;
    }    
}

void
iquant4x4dc_c(int16_t* data, const int32_t Qp)
{
    const int32_t qbits = Qp / 6 - 2;
    int32_t i;

    if (qbits >= 0)
    {
        const int32_t mf_index = Qp % 6;
        const int32_t dmf = dequant[mf_index][0] << qbits;
        for(i = 0 ; i < 16 ; i ++)
        {
            data[i] = data[i] * dmf;
        }
    }
    else
    {
        const int32_t dmf = dequant[Qp % 6][0];
        const int32_t t2  = 1 << (1 + qbits);
        for(i = 0 ; i < 16 ; i ++)
        {
            data[i] = (data[i] * dmf + t2) >> (-qbits);
        }
    }
}

void
iquant2x2dc_c(int16_t* data, const int32_t Qp)
{
    const int32_t qbits = Qp / 6 - 1;

    if (qbits >= 0 )
    {
        const int32_t dmf = dequant[Qp % 6][0] << qbits;

        data[0] = data[0] * dmf;
        data[1] = data[1] * dmf;
        data[2] = data[2] * dmf;
        data[3] = data[3] * dmf;
    }
    else
    {
        const int32_t dmf = dequant[Qp % 6][0];

        data[0] = (data[0] * dmf) >> 1;
        data[1] = (data[1] * dmf) >> 1;
        data[2] = (data[2] * dmf) >> 1;
        data[3] = (data[3] * dmf) >> 1;
    }
}
