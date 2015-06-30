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

#ifndef _ESTIMATION_
#define _ESTIMATION_

#include "T264.h"

uint32_t check_point();
uint32_t T264_search(T264_t* t, T264_search_context_t* context);
uint32_t small_diamond_search(T264_t* t, uint8_t* cur, uint8_t* ref_st, T264_search_context_t* context, int32_t stride_cur, int32_t stride_ref, uint32_t sad);
uint32_t square_diamond_search(T264_t* t, uint8_t* cur, uint8_t* ref_st, T264_search_context_t* context, int32_t stride_cur, int32_t stride_ref, uint32_t sad);
uint32_t diamond_search(T264_t* t, uint8_t* cur, uint8_t* ref_st, T264_search_context_t* context, int32_t stride_cur, int32_t stride_ref, uint32_t sad);

uint32_t T264_search_full(T264_t* t, T264_search_context_t* context);
uint32_t T264_spiral_search_full(T264_t* t, T264_search_context_t* context);

#endif
