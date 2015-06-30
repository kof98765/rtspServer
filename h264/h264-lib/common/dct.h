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

#ifndef _DCT_H_
#define _DCT_H_

void dct4x4_c(int16_t* data);
void dct4x4dc_c(int16_t* data);
void dct2x2dc_c(int16_t* data);
void idct4x4_c(int16_t* data);
void idct4x4dc_c(int16_t* data);
void idct2x2dc_c(int16_t* data);

void quant4x4_c(int16_t* data, const int32_t Qp, int32_t is_intra);
void quant4x4dc_c(int16_t* data, const int32_t Qp);
void quant2x2dc_c(int16_t* data, const int32_t Qp, int32_t is_intra);
void iquant4x4_c(int16_t* data, const int32_t Qp);
void iquant4x4dc_c(int16_t* data, const int32_t Qp);
void iquant2x2dc_c(int16_t* data, const int32_t Qp);

#endif
