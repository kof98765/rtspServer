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

#ifndef _PORTAB_H_
#define _PORTAB_H_

#include "config.h"

#define _RW
#define _R
#define _W
#define _INPUT
#define _OUTPUT

#define TRUE 1
#define FALSE 0

#if defined(_MSC_VER)
#define int8_t   char
#define uint8_t  unsigned char
#define int16_t  short
#define uint16_t unsigned short
#define int32_t  int
#define uint32_t unsigned int
#define int64_t  __int64
#define uint64_t unsigned __int64
#define ptr_t uint32_t

#define BYTE   uint8_t
#define INT32  int32_t
#define INT16  int16_t
#define UINT16 uint16_t
#define UINT32 uint32_t

#if defined(ARCH_IS_IA32)
//#define BSWAP(a) __asm mov eax,a __asm bswap eax __asm mov a, eax
#define BSWAP(a) {unsigned int _temp0,_temp1,_temp2,_temp3,_temp4;_temp1=(a & 0xFF00FF00)>>8;_temp0=(a & 0x00FF00FF)<<8;_temp2=_temp0+_temp1;_temp3=(_temp2 & 0x0000FFFF)<<16;_temp4=(_temp2 & 0xFFFF0000)>>16;a=_temp3+_temp4;}
         static __inline uint64_t read_counter(void)
         {
             __asm {
                 rdtsc
             }
         }

#define SWAP(type, x, y) { type* _tmp_; _tmp_ = x; x = y ; y = _tmp_;}
#define CLIP1(x) (x & ~255) ? (-x >> 31) : x
#define ABS(x) ((x) > 0 ? (x) : -(x))

#else // ARCH_IS_IA32
#error Please port BSWAP to other platform!
#endif// ARCH_IS_IA32

// vc6 does not support __declspec(aligned(x)) instead we need vc6 + sp5(or more), here we do some test
#if _MSC_VER < 1200
#error We need vc6 or more higher version!
#else   // vc6 or higher
extern _declspec(align(16)) int32_t if_you_see_errors_here_please_install_vc6_sp5_or_higher[1];
#endif

#define DECLARE_ALIGNED_MATRIX(name,sizex,sizey,type,alignment) \
                __declspec(align(alignment)) type name[(sizex)*(sizey)]

#define DECLARE_ALIGNED_MATRIX_H(name,sizex,sizey,type,alignment) \
    __declspec(align(alignment)) type name[(sizex)*(sizey)]

#define DECLARE_ALIGNED2_MATRIX_H(name,sizex,sizey,type,alignment) \
    __declspec(align(alignment)) type name[(sizex)][(sizey)]

#endif // _MSC_VER

#ifdef __GCC__

#include <inttypes.h>

#define ptr_t uint32_t

#if defined(ARCH_IS_IA32)
//#define BSWAP(a) __asm__ ( "bswapl %0\n" : "=r" (a) : "0" (a) );
#define BSWAP(a) {unsigned int _temp0,_temp1,_temp2,_temp3,_temp4;_temp1=(a & 0xFF00FF00)>>8;_temp0=(a & 0x00FF00FF)<<8;_temp2=_temp0+_temp1;_temp3=(_temp2 & 0x0000FFFF)<<16;_temp4=(_temp2 & 0xFFFF0000)>>16;a=_temp3+_temp4;}

         static __inline int64_t read_counter(void)
         {
             int64_t ts;
             uint32_t ts1, ts2;
             __asm__ __volatile__("rdtsc\n\t":"=a"(ts1), "=d"(ts2));
             ts = ((uint64_t) ts2 << 32) | ((uint64_t) ts1);
             return ts;
         }

#define SWAP(type, x, y) { type* _tmp_; _tmp_ = x; x = y ; y = _tmp_;}
#define CLIP1(x) (x & ~255) ? (-x >> 31) : x
#define ABS(x) ((x) > 0 ? (x) : -(x))

#else // ARCH_IS_IA32
#error Please port BSWAP to other platform!
#endif// ARCH_IS_IA32

//#define DECLARE_ALIGNED_MATRIX(name,sizex,sizey,type,alignment) type name[(sizex)*(sizey)] __attribute__((aligned(alignment)))
#define DECLARE_ALIGNED_MATRIX(name,sizex,sizey,type,alignment) type name[(sizex)*(sizey)]
#define DECLARE_ALIGNED_MATRIX_H DECLARE_ALIGNED_MATRIX
//#define DECLARE_ALIGNED2_MATRIX_H(name,sizex,sizey,type,alignment) type name[(sizex)][(sizey)] __attribute__((aligned(alignment)))
#define DECLARE_ALIGNED2_MATRIX_H(name,sizex,sizey,type,alignment) type name[(sizex)][(sizey)]
#endif // __GCC__

//Ti_DSP Platform ported by YouXiaoquan,HFUT-Ti United Lab,China
//YouXiaoquan@126.com
#ifdef CHIP_DM642
#define int8_t   char
#define uint8_t  unsigned char
#define int16_t  short
#define uint16_t unsigned short
#define int32_t  int
#define uint32_t unsigned int
#define int64_t  long
#define uint64_t unsigned long
#define ptr_t uint32_t

#define BYTE   uint8_t
#define INT32  int32_t
#define INT16  int16_t
#define UINT16 uint16_t
#define UINT32 uint32_t

#define BSWAP(a) {unsigned int _temp0,_temp1,_temp2,_temp3,_temp4;_temp1=(a & 0xFF00FF00)>>8;_temp0=(a & 0x00FF00FF)<<8;_temp2=_temp0+_temp1;_temp3=(_temp2 & 0x0000FFFF)<<16;_temp4=(_temp2 & 0xFFFF0000)>>16;a=_temp3+_temp4;}
         
		 static __inline uint64_t read_counter(void)
         { 
          /*
             __asm {
                 rdtsc
             }
          *///Port to Ti_Platform
          return clock();
         }

#define SWAP(type, x, y) { type* _tmp_; _tmp_ = x; x = y ; y = _tmp_;}
#define CLIP1(x) (x & ~255) ? (-x >> 31) : x
#define ABS(x) ((x) > 0 ? (x) : -(x))

#define DECLARE_ALIGNED_MATRIX(name,sizex,sizey,type,alignment) \
                type name[(sizex)*(sizey)]
#define DECLARE_ALIGNED_MATRIX_H(name,sizex,sizey,type,alignment) \
                type name[(sizex)*(sizey)]
#define DECLARE_ALIGNED2_MATRIX_H(name,sizex,sizey,type,alignment) \
                type name[(sizex)][(sizey)] 
#endif//// ported to Ti_Dsp_DM642 By You_xiaoQuan HFUT


static __inline int32_t
clip3(int32_t a, int32_t low, int32_t high)
{
    if (a < low)
        return low;
    
    if (a > high)
        return high;

    return a;
}
#endif // _PORTAB_H_
