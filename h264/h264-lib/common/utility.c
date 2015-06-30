/*****************************************************************************
 *
 *  T264 AVC CODEC
 *
 *  Copyright(C) 2004-2005 llcc <lcgate1@yahoo.com.cn>
 *               2004-2005 visionany <visionany@yahoo.com.cn>
 *      2005.1.13 CloudWu modify PIA_u_wxh_c() function
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

#define Q_BITS          15
#define DQ_BITS         6
#define DQ_ROUND        (1<<(DQ_BITS-1))

#include "stdio.h"
#include "portab.h"
#ifndef CHIP_DM642
#include "malloc.h"
#endif
#include "utility.h"
#ifndef CHIP_DM642
#include "memory.h"
#endif
//
// from xvid
//
void* 
T264_malloc(int32_t size, int32_t alignment)
{
    uint8_t *mem_ptr;

    if (!alignment) {

        /* We have not to satisfy any alignment */
        if ((mem_ptr = (uint8_t *) malloc(size + 1)) != NULL) {

            /* Store (mem_ptr - "real allocated memory") in *(mem_ptr-1) */
            *mem_ptr = (uint8_t)1;

            /* Return the mem_ptr pointer */
            return ((void *)(mem_ptr+1));
        }
    } else {
        uint8_t *tmp;

        /* Allocate the required size memory + alignment so we
        * can realign the data if necessary */
        if ((tmp = (uint8_t *) malloc(size + alignment)) != NULL) {

            /* Align the tmp pointer */
            mem_ptr =
                (uint8_t *) ((uint32_t) (tmp + alignment - 1) &
                (~(uint32_t) (alignment - 1)));

            /* Special case where malloc have already satisfied the alignment
            * We must add alignment to mem_ptr because we must store
            * (mem_ptr - tmp) in *(mem_ptr-1)
            * If we do not add alignment to mem_ptr then *(mem_ptr-1) points
            * to a forbidden memory space */
            if (mem_ptr == tmp)
                mem_ptr += alignment;

            /* (mem_ptr - tmp) is stored in *(mem_ptr-1) so we are able to retrieve
            * the real malloc block allocated and free it in xvid_free */
            *(mem_ptr - 1) = (uint8_t) (mem_ptr - tmp);

            /* Return the aligned pointer */
            return ((void *)mem_ptr);
        }
    }

    return(NULL);
}

void 
T264_free(void* p)
{
    uint8_t *ptr;

    if (p == NULL)
        return;

    /* Aligned pointer */
    ptr = p;

    /* *(ptr - 1) holds the offset to the real allocated block
    * we sub that offset os we free the real pointer */
    ptr -= *(ptr - 1);

    /* Free the memory */
    free(ptr);
}

void
expand8to16_c(uint8_t* src, int32_t src_stride, int32_t quarter_width, int32_t quarter_height, int16_t* dst)
{
    int32_t i, j;

    for(i = 0 ; i < quarter_height * 4 ; i ++)
    {
        for(j = 0 ; j < quarter_width ; j ++)
        {
            dst[i * quarter_width * 4 + j * 4 + 0] = src[0 + j * 4];
            dst[i * quarter_width * 4 + j * 4 + 1] = src[1 + j * 4];
            dst[i * quarter_width * 4 + j * 4 + 2] = src[2 + j * 4];
            dst[i * quarter_width * 4 + j * 4 + 3] = src[3 + j * 4];
        }
        src += src_stride;
    }
}

void
expand8to16sub_c(uint8_t* pred, int32_t quarter_width, int32_t quarter_height, int16_t* dst, uint8_t* src, int32_t src_stride)
{
    int32_t i, j, k;
    uint8_t* start_p;
    uint8_t* start_s;
    
    for(i = 0 ; i < quarter_height ; i ++)
    {
        for(j = 0 ; j < quarter_width ; j ++)
        {
            start_p = pred + i * quarter_width * 4 * 4 + j * 4;
            start_s = src + i * src_stride * 4 + j * 4;
            for(k = 0 ; k < 4 ; k ++)
            {
                dst[0] = start_s[0] - start_p[0];
                dst[1] = start_s[1] - start_p[1];
                dst[2] = start_s[2] - start_p[2];
                dst[3] = start_s[3] - start_p[3];

                dst     += 4;
                start_p += 4 * quarter_width;
                start_s += src_stride;
            }
        }
    }
}

void
contract16to8_c(int16_t* src, int32_t quarter_width, int32_t quarter_height, uint8_t* dst, int32_t dst_stride)
{
    int32_t i, j;

    for(i = 0 ; i < quarter_height * 4 ; i ++)
    {
        for(j = 0 ; j < quarter_width ; j ++)
        {
            int16_t tmp;
            tmp = src[i * quarter_width * 4 + j * 4 + 0];
            dst[0 + j * 4] = CLIP1(tmp);
            tmp = src[i * quarter_width * 4 + j * 4 + 1];
            dst[1 + j * 4] = CLIP1(tmp);
            tmp = src[i * quarter_width * 4 + j * 4 + 2];
            dst[2 + j * 4] = CLIP1(tmp);
            tmp = src[i * quarter_width * 4 + j * 4 + 3];
            dst[3 + j * 4] = CLIP1(tmp);
        }
        dst += dst_stride;
    }
}

void	//assigned
contract16to8add_c(int16_t* src, int32_t quarter_width, int32_t quarter_height, uint8_t* pred, uint8_t* dst, int32_t dst_stride)
{
    int32_t i, j, k;
    uint8_t* start_p;
    uint8_t* start_d;

    for(i = 0 ; i < quarter_height ; i ++)
    {
        for(j = 0 ; j < quarter_width ; j ++)
        {
            start_p = pred + i * quarter_width * 4 * 4 + j * 4;
            start_d = dst + i * dst_stride * 4 + j * 4;
            for(k = 0 ; k < 4 ; k ++)
            {
                int16_t tmp;

                tmp = src[0] + start_p[0];
                start_d[0] = CLIP1(tmp);
                tmp = src[1] + start_p[1];
                start_d[1] = CLIP1(tmp);
                tmp = src[2] + start_p[2];
                start_d[2] = CLIP1(tmp);
                tmp = src[3] + start_p[3];
                start_d[3] = CLIP1(tmp);

                //tmp = (src[0] + (start_p[0] << DQ_BITS) + DQ_ROUND) >> DQ_BITS;
                //start_d[0] = CLIP1(tmp);
                //tmp = (src[1] + (start_p[1] << DQ_BITS) + DQ_ROUND) >> DQ_BITS;
                //start_d[1] = CLIP1(tmp);
                //tmp = (src[2] + (start_p[2] << DQ_BITS) + DQ_ROUND) >> DQ_BITS;
                //start_d[2] = CLIP1(tmp);
                //tmp = (src[3] + (start_p[3] << DQ_BITS) + DQ_ROUND) >> DQ_BITS;
                //start_d[3] = CLIP1(tmp);

                src     += 4;
                start_p += 4 * quarter_width;
                start_d += dst_stride;
            }
        }
    }
}

void
memcpy_stride_u_c(void* src, int32_t width, int32_t height, int32_t src_stride, void* dst, int32_t dst_stride)
{
    int32_t i;
    uint8_t* s = src;
    uint8_t* d = dst;

    for(i = 0 ; i < height ; i ++)
    {
        memcpy(d, s, width);
        s += src_stride;
        d += dst_stride;
    }
}

static __inline uint32_t
T264_sad_u_c(uint8_t* src, int32_t src_stride, uint8_t* data, int32_t width, int32_t height, int32_t dst_stride)
{
    int32_t i, j;
    uint32_t sad;

    sad = 0;
    for(i = 0 ; i < height ; i ++)
    {
        for(j = 0 ; j < width ; j ++)
        {
            int32_t tmp = data[j] - src[j];
            sad += ABS(tmp);
        }
        src  += src_stride;
        data += dst_stride;
    }

    return sad;
}

//copied from JM,by cloud wu
static __inline 
uint32_t _satd_4x4_dif_c(int16_t* diff)
{
  int32_t k, satd = 0, m[16], dd, d[16];
    
    /*===== hadamard transform =====*/
    m[ 0] = diff[ 0] + diff[12];
    m[ 4] = diff[ 4] + diff[ 8];
    m[ 8] = diff[ 4] - diff[ 8];
    m[12] = diff[ 0] - diff[12];
    m[ 1] = diff[ 1] + diff[13];
    m[ 5] = diff[ 5] + diff[ 9];
    m[ 9] = diff[ 5] - diff[ 9];
    m[13] = diff[ 1] - diff[13];
    m[ 2] = diff[ 2] + diff[14];
    m[ 6] = diff[ 6] + diff[10];
    m[10] = diff[ 6] - diff[10];
    m[14] = diff[ 2] - diff[14];
    m[ 3] = diff[ 3] + diff[15];
    m[ 7] = diff[ 7] + diff[11];
    m[11] = diff[ 7] - diff[11];
    m[15] = diff[ 3] - diff[15];
    
    d[ 0] = m[ 0] + m[ 4];
    d[ 8] = m[ 0] - m[ 4];
    d[ 4] = m[ 8] + m[12];
    d[12] = m[12] - m[ 8];
    d[ 1] = m[ 1] + m[ 5];
    d[ 9] = m[ 1] - m[ 5];
    d[ 5] = m[ 9] + m[13];
    d[13] = m[13] - m[ 9];
    d[ 2] = m[ 2] + m[ 6];
    d[10] = m[ 2] - m[ 6];
    d[ 6] = m[10] + m[14];
    d[14] = m[14] - m[10];
    d[ 3] = m[ 3] + m[ 7];
    d[11] = m[ 3] - m[ 7];
    d[ 7] = m[11] + m[15];
    d[15] = m[15] - m[11];
    
    m[ 0] = d[ 0] + d[ 3];
    m[ 1] = d[ 1] + d[ 2];
    m[ 2] = d[ 1] - d[ 2];
    m[ 3] = d[ 0] - d[ 3];
    m[ 4] = d[ 4] + d[ 7];
    m[ 5] = d[ 5] + d[ 6];
    m[ 6] = d[ 5] - d[ 6];
    m[ 7] = d[ 4] - d[ 7];
    m[ 8] = d[ 8] + d[11];
    m[ 9] = d[ 9] + d[10];
    m[10] = d[ 9] - d[10];
    m[11] = d[ 8] - d[11];
    m[12] = d[12] + d[15];
    m[13] = d[13] + d[14];
    m[14] = d[13] - d[14];
    m[15] = d[12] - d[15];
    
    d[ 0] = m[ 0] + m[ 1];
    d[ 1] = m[ 0] - m[ 1];
    d[ 2] = m[ 2] + m[ 3];
    d[ 3] = m[ 3] - m[ 2];
    d[ 4] = m[ 4] + m[ 5];
    d[ 5] = m[ 4] - m[ 5];
    d[ 6] = m[ 6] + m[ 7];
    d[ 7] = m[ 7] - m[ 6];
    d[ 8] = m[ 8] + m[ 9];
    d[ 9] = m[ 8] - m[ 9];
    d[10] = m[10] + m[11];
    d[11] = m[11] - m[10];
    d[12] = m[12] + m[13];
    d[13] = m[12] - m[13];
    d[14] = m[14] + m[15];
    d[15] = m[15] - m[14];
    
    /*===== sum up =====*/
    for (dd=d[k=0]; k<16; dd=d[++k])
    {
      satd += (dd < 0 ? -dd : dd);
    }
    satd = ((satd+1)>>1);

    return satd;
}

static __inline uint32_t
T264_satd_u_c(uint8_t* src, int32_t src_stride, uint8_t* data, int32_t width, int32_t height, int32_t dst_stride)
{
    int32_t i, j, n, m;
    uint32_t sad;
    int16_t tmp[16];

    sad = 0;
    for(i = 0 ; i < height ; i += 4)
    {
        for(j = 0 ; j < width ; j += 4)
        {
            uint8_t* tmp_s = src + i * src_stride + j;
            uint8_t* tmp_d = data+ i * dst_stride + j;
            for(n = 0 ; n < 4 ; n ++)
            {
                for(m = 0 ; m < 4 ; m ++)
                    tmp[n * 4 + m] = tmp_d[m] - tmp_s[m];
                tmp_d += dst_stride;
                tmp_s += src_stride;
            }

            sad += _satd_4x4_dif_c(tmp);
        }
    }

    return sad;
}


#define SADFUNC(w, h, base) uint32_t T264_##base##_u_##w##x##h##_c(uint8_t* src, int32_t src_stride, uint8_t* data, int32_t dst_stride) {return T264_##base##_u_c(src, src_stride, data, w, h, dst_stride); }


SADFUNC(16, 16, sad)
SADFUNC(16, 8,  sad)
SADFUNC(8,  16, sad)
SADFUNC(8,  8,  sad)
SADFUNC(8,  4,  sad)
SADFUNC(4,  8,  sad)
SADFUNC(4,  4,  sad)

SADFUNC(16, 16, satd)
SADFUNC(16, 8,  satd)
SADFUNC(8,  16, satd)
SADFUNC(8,  8,  satd)
SADFUNC(8,  4,  satd)
SADFUNC(4,  8,  satd)
SADFUNC(4,  4,  satd)

/**********************************************************************************
 *
 *  Based on the FUNCTION T264_satd_u_c()
 *  use SATD for 16x16 Intra
 *  Thomascatlee@163.com
 *
 *********************************************************************************/

uint32_t
T264_satd_i16x16_u_c(uint8_t* src, int32_t src_stride, uint8_t* data, int32_t dst_stride)
{
    return T264_satd_u_c(src,src_stride,  data, 16,16,dst_stride);    
/*    int32_t i, j, n, m, k;
    uint32_t sad;
    int16_t tmp[16];
    int16_t s_dc[16];
    int16_t s[4];

    sad = 0;
    k = 0;
    for(i = 0 ; i < 16 ; i += 4)
    {
        for(j = 0 ; j < 16 ; j += 4)
        {
            uint8_t* tmp_s = src + i * src_stride + j;
            uint8_t* tmp_d = data+ i * dst_stride + j;
            for(n = 0 ; n < 4 ; n ++)
            {
                for(m = 0 ; m < 4 ; m ++)
                    tmp[n * 4 + m] = tmp_d[m] - tmp_s[m];
                tmp_d += dst_stride;
                tmp_s += src_stride;
            }

            for(n = 0 ; n < 4 ; n ++)
            {
                s[0] = tmp[0 * 4 + n] + tmp[3 * 4 + n];
                s[3] = tmp[0 * 4 + n] - tmp[3 * 4 + n];
                s[1] = tmp[1 * 4 + n] + tmp[2 * 4 + n];
                s[2] = tmp[1 * 4 + n] - tmp[2 * 4 + n];

                tmp[0 * 4 + n] = s[0] + s[1];
                tmp[2 * 4 + n] = s[0] - s[1];
                tmp[1 * 4 + n] = s[3] + s[2];
                tmp[3 * 4 + n] = s[3] - s[2];
            }
            
            //  Add for get DC coeff
            n = 0;
            s[0] = tmp[n * 4 + 0] + tmp[n * 4 + 3];
            s[3] = tmp[n * 4 + 0] - tmp[n * 4 + 3];
            s[1] = tmp[n * 4 + 1] + tmp[n * 4 + 2];
            s[2] = tmp[n * 4 + 1] - tmp[n * 4 + 2];
            s_dc[k] = ((s[0] + s[1]) >> 2);
            sad += ABS(s[0] - s[1]);
            sad += ABS(s[3] + s[2]);
            sad += ABS(s[3] - s[2]);
            k++;
            
            for(n = 1 ; n < 4 ; n ++)
            {
                s[0] = tmp[n * 4 + 0] + tmp[n * 4 + 3];
                s[3] = tmp[n * 4 + 0] - tmp[n * 4 + 3];
                s[1] = tmp[n * 4 + 1] + tmp[n * 4 + 2];
                s[2] = tmp[n * 4 + 1] - tmp[n * 4 + 2];
                sad += ABS(s[0] + s[1]);
                sad += ABS(s[0] - s[1]);
                sad += ABS(s[3] + s[2]);
                sad += ABS(s[3] - s[2]);
             }

           

        }
    }

    //  Hadamard of DC coeff 
    for(n = 0 ; n < 4 ; n ++)
    {
      s[0] = s_dc[0 * 4 + n] + s_dc[3 * 4 + n];
      s[3] = s_dc[0 * 4 + n] - s_dc[3 * 4 + n];
      s[1] = s_dc[1 * 4 + n] + s_dc[2 * 4 + n];
      s[2] = s_dc[1 * 4 + n] - s_dc[2 * 4 + n];
      
      tmp[0 * 4 + n] = s[0] + s[1];
      tmp[2 * 4 + n] = s[0] - s[1];
      tmp[1 * 4 + n] = s[3] + s[2];
      tmp[3 * 4 + n] = s[3] - s[2];
    }
    for(n = 0 ; n < 4 ; n ++)
    {
      s[0] = tmp[n * 4 + 0] + tmp[n * 4 + 3];
      s[3] = tmp[n * 4 + 0] - tmp[n * 4 + 3];
      s[1] = tmp[n * 4 + 1] + tmp[n * 4 + 2];
      s[2] = tmp[n * 4 + 1] - tmp[n * 4 + 2];
      sad += ABS(s[0] + s[1]);
      sad += ABS(s[0] - s[1]);
      sad += ABS(s[3] + s[2]);
      sad += ABS(s[3] - s[2]);
    }

    return sad >> 1;
*/
}

//calculate non-zero counts for an array v[i_count]
int32_t 
array_non_zero_count(int16_t *v, int32_t i_count)
{
    int32_t i;
    int32_t i_nz;

    for( i = 0, i_nz = 0; i < i_count; i++ )
    {
        if( v[i] )
        {
            i_nz++;
        }
    }
    return i_nz;
}
