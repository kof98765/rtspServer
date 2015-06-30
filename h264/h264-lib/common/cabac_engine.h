/*****************************************************************************
 * cabac.h: h264 encoder library
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

#ifndef _CABAC_ENGINE_H_
#define _CABAC_ENGINE_H_
#include "T264.h"
#include "bitstream.h"
/* encoder/decoder: init the contexts given i_slice_type, the quantif and the model */
void T264_cabac_context_init( T264_cabac_t *cb, int i_slice_type, int i_qp, int i_model );

/* decoder only: */
void T264_cabac_decode_init    ( T264_cabac_t *cb, bs_t *s );
int  T264_cabac_decode_decision( T264_cabac_t *cb, int i_ctx_idx );
int  T264_cabac_decode_bypass  ( T264_cabac_t *cb );
int  T264_cabac_decode_terminal( T264_cabac_t *cb );

/* encoder only: adaptive model init */
void T264_cabac_model_init( T264_cabac_t *cb );
int  T264_cabac_model_get ( T264_cabac_t *cb, int i_slice_type );
void T264_cabac_model_update( T264_cabac_t *cb, int i_slice_type, int i_qp );
/* encoder only: */
void T264_cabac_encode_init ( T264_cabac_t *cb, bs_t *s );
void T264_cabac_encode_decision( T264_cabac_t *cb, int i_ctx_idx, int b );
void T264_cabac_encode_bypass( T264_cabac_t *cb, int b );
void T264_cabac_encode_terminal( T264_cabac_t *cb, int b );
void T264_cabac_encode_flush( T264_cabac_t *cb );


#endif
