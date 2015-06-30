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
#include "T264.h"
#include "bitstream.h"
#include "utility.h"
#ifndef CHIP_DM642
#include "memory.h"
#endif
#include "assert.h"
#ifndef CHIP_DM642
#include "sse2.h"
#endif
#include "interpolate.h"
#include "math.h"
#include "dct.h"
#include "dec_cavlc.h"
#include "deblock.h"
#include "block.h"
//for dec CABAC
#include "cabac_engine.h"
#include "dec_cabac.h"
#include "inter.h"
#define NAL_BUFFER_LEN 1024 * 1024  /* big enough ? */

void
T264dec_load_ref(T264_t* t)
{
    int32_t i, j, k;

    t->rec = &t->refn[0];

    if (t->slice_type == SLICE_P)
    {
        for(i = 0, j = 0 ; i < t->refl0_num ; i ++)
        {
            if (t->refn[i + 1].poc >= 0)
            {
                t->ref[0][j ++] = &t->refn[i + 1];
            }
        }
    }
    else if (t->slice_type == SLICE_B)
    {
        for(i = 0, j = 0, k = 0 ; i < t->refl0_num + t->refl1_num; i ++)
        {
            if (t->refn[i + 1].poc < t->slice.pic_order_cnt_lsb)
            {
                if (t->refn[i + 1].poc >= 0)
                    t->ref[0][j ++] = &t->refn[i + 1];
            }
            else
            {
                // yes, t->refn[i].poc already > 0
                t->ref[1][k ++] = &t->refn[i + 1];
            }
        }
    }
}

void
T264dec_save_ref(T264_t* t)
{
    int32_t i;
    T264_frame_t tmp;
    /* deblock filter exec here */
    if (t->need_deblock)
        T264_deblock_frame(t, t->rec);
    /* current only del with i,p */
    T264_extend_border(t, t->rec);
    T264_interpolate_halfpel(t, t->rec);

    tmp = t->refn[t->ss.num_ref_frames];
    t->refn[0].poc = t->slice.pic_order_cnt_lsb;
    for(i = t->ss.num_ref_frames ; i >= 1 ; i --)
    {
        t->refn[i] = t->refn[i - 1];
    }

    t->refn[0] = tmp;
}

void
T264dec_mb_load_context(T264_t* t, int32_t mb_y, int32_t mb_x)
{
    int32_t i, j;

    /* nnz count will be set in read_cavlc, but in many cases, 
        nnz will equal to 0 because cbp == 0
     */
    memset(t->mb.nnz, 0, sizeof(t->mb.nnz));
    memset(t->mb.nnz_ref, 0, sizeof(t->mb.nnz_ref));
    memset(t->mb.mode_i4x4, Intra_4x4_DC, 16 * sizeof(uint8_t));
    memset(t->mb.submb_part, -1, sizeof(t->mb.submb_part));
    memset(t->mb.dct_y_z, 0, sizeof(t->mb.dct_y_z));
    memset(t->mb.dct_uv_z, 0, sizeof(t->mb.dct_uv_z));
    memset(t->mb.dc4x4_z, 0, sizeof(t->mb.dc4x4_z));
    memset(t->mb.dc2x2_z, 0, sizeof(t->mb.dc2x2_z));
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

    T264_mb_load_context(t, mb_y, mb_x);
    
    t->mb.src_y = t->mb.dst_y;
    t->mb.src_u = t->mb.dst_u;
    t->mb.src_v = t->mb.dst_v;
}

void
T264dec_mb_decode(T264_t* t)
{
    /* p skip decode as p mode */
    if(t->mb.mb_mode == P_MODE)
    {
        T264dec_mb_decode_interp_y(t);

        //
        // Chroma
        //
        T264dec_mb_decode_interp_uv(t);

        t->stat.p_block_num[t->mb.mb_part] ++;
    }
    else if (t->mb.mb_mode == B_MODE)
    {
        T264dec_mb_decode_interb_y(t);

        //
        // Chroma
        //
        T264dec_mb_decode_interb_uv(t);

        t->stat.b_block_num[t->mb.mb_part] ++;
    }
    else if (t->mb.mb_mode == I_4x4 || t->mb.mb_mode == I_16x16)
    {
        T264dec_mb_decode_intra_y(t);

        //
        // Chroma
        //
        T264dec_mb_decode_intra_uv(t);

        t->stat.i_block_num[t->mb.mb_mode] ++;
    }
}

void
T264dec_mb_save_context(T264_t* t, int32_t i, int32_t j)
{
    memcpy(t->mb.context, &t->mb, sizeof(*t->mb.context));
}

void
T264dec_parse_slice_header(T264_t* t)
{
    t->slice.first_mb_in_slice = eg_read_ue(t->bs); 
    assert(t->slice.first_mb_in_slice == 0);
    t->slice_type = t->slice.slice_type = eg_read_ue(t->bs);
    t->slice.pic_id = eg_read_ue(t->bs);    
    t->slice.frame_num = eg_read_direct(t->bs, t->ss.log2_max_frame_num_minus4 + 4);
    if (t->nal.nal_unit_type == NAL_SLICE_IDR)
    {
        t->slice.idr_pic_id = eg_read_ue(t->bs);
    }
    if (t->ss.pic_order_cnt_type == 0)
    {
        t->poc = t->slice.pic_order_cnt_lsb = eg_read_direct(t->bs, t->ss.max_pic_order + 4);
    }
    if (t->slice_type == SLICE_P)
    {
        t->refl1_num = 0;
        t->refl0_num = t->ps.num_ref_idx_l0_active_minus1 + 1;
        // num_ref_idx_active_override_flag
        if (eg_read_direct(t->bs, 1))
        {
            t->refl0_num = eg_read_ue(t->bs) + 1;
        }
    }
    else if (t->slice_type == SLICE_B)
    {
        // direct_spatial_mv_pred_flag
        t->param.direct_flag = t->slice.direct_spatial_mv_pred_flag = eg_read_direct(t->bs, 1);
        t->refl1_num = 1;
        t->refl0_num = t->ps.num_ref_idx_l0_active_minus1 + 1;
        // num_ref_idx_active_override_flag
        if (eg_read_direct(t->bs, 1))
        {
            t->refl0_num = eg_read_ue(t->bs) + 1;
            t->refl1_num = eg_read_ue(t->bs) + 1;
        }
    }

    /* ref_pic_list_reordering() */
    if(t->slice.slice_type != SLICE_I && t->slice.slice_type != SLICE_SI )
    {
        eg_read_skip(t->bs, 1);
        if (t->slice.slice_type == SLICE_B)
            eg_read_skip(t->bs, 1);
    }

    if (t->nal.nal_ref_idc != 0)
    {
        /* dec_ref_pic_marking() */
        if (t->nal.nal_unit_type == NAL_SLICE_IDR)
        {
            eg_read_direct(t->bs, 1);
            eg_read_direct(t->bs, 1);
        }
        else
        {
            eg_read_direct(t->bs, 1);
        }
    }
	//for dec CABAC
	if(t->ps.entroy_coding_mode_flag!=0 && t->slice_type!= SLICE_I)
	{
		t->slice.cabac_init_idc = eg_read_ue(t->bs);
	}
    t->qp_y = eg_read_se(t->bs) + t->ps.pic_init_qp_minus26 + 26;
    if (t->ps.deblocking_filter_control_present_flag)
    {
        t->need_deblock = !eg_read_ue(t->bs);
    }
}

static void __inline
get_output_frame(T264_t* t)
{
    /* we will reorder the output frame */
    if (t->slice_type != SLICE_B)
    {
        t->output.poc = -2;
        if (t->frame_num > 0)
        {
            //
            // refn[0] = current reconstruct frame
            // refn[1] = ref frame 1
            // refn[2] = ref frame 2
            //
            t->output = t->refn[1];
        }
    }
    else
    {
        // slice b always should send immediately
        t->output = t->refn[0];
    }
}

/* currently we _only_ support one slice */
void
T264dec_parse_slice(T264_t* t)
{
    int32_t i, j, skip, end_of_slice;
	//to be cleaned
	//extern int frame_cabac, mb_cabac, slice_type_cabac;
    T264dec_parse_slice_header(t);
    
    T264dec_load_ref(t);

	//for dec CABAC
	if( t->ps.entroy_coding_mode_flag == 1 )
	{
		/* alignment needed */
		BitstreamByteAlign( t->bs );

		/* init cabac */
		T264_cabac_context_init( &t->cabac, t->slice_type, t->ps.pic_init_qp_minus26+26+t->slice.slice_qp_delta, t->slice.cabac_init_idc );
		T264_cabac_decode_init ( &t->cabac, t->bs );
	}
    t->skip = -1;
    for(i = 0 ; i < t->mb_height ; i ++)
    {
        for(j = 0 ; j < t->mb_width ; j ++)
        {
            T264dec_mb_load_context(t, i, j);
            if( t->ps.entroy_coding_mode_flag == 1 )
			{
				//CABAC
				if(t->mb.mb_xy > 0)
				{
					end_of_slice = T264_cabac_decode_terminal(&t->cabac);
					assert(end_of_slice == 0);
				}
				skip = T264dec_mb_read_cabac(t);
			}
			else
			{
				//CAVLC
				T264dec_mb_read_cavlc(t);
			}	

            T264dec_mb_decode(t);
			if(t->ps.entroy_coding_mode_flag==1 && skip)
			{
				t->mb.mb_mode = (t->slice_type==SLICE_B)?B_SKIP:P_SKIP;
			}
            T264dec_mb_save_context(t, i, j);
        }
    }
	if( t->ps.entroy_coding_mode_flag == 1 )
	{
		/* end of slice */
		end_of_slice = T264_cabac_decode_terminal(&t->cabac);
		assert(end_of_slice == 1);
		BitstreamByteAlign(t->bs);
	}	
	//for dec CABAC
	if( t->ps.entroy_coding_mode_flag == 1 )
	{
		T264_cabac_model_update( &t->cabac, t->slice_type, t->ps.pic_init_qp_minus26+26 + t->slice.slice_qp_delta );
	}
    get_output_frame(t);

    if (t->slice_type != SLICE_B)
    {
        T264dec_save_ref(t);
    }
    else if (t->need_deblock)
    {
        T264_deblock_frame(t, t->rec);
    }
    t->emms();
}

void
T264dec_parse_pic_header(T264_t* t)
{
    t->ps.pic_id = eg_read_ue(t->bs);
    t->ps.seq_id = eg_read_ue(t->bs);
    assert(t->ps.pic_id == 0 && t->ps.seq_id == 0);
    t->ps.entroy_coding_mode_flag = eg_read_direct(t->bs, 1);
	//for dec CABAC, now support CABAC
    //assert(t->ps.entroy_coding_mode_flag == 0);
    t->ps.pic_order_present_flag = eg_read_direct(t->bs, 1);
    assert(t->ps.pic_order_present_flag == 0);
    t->ps.num_slice_groups_minus1 = eg_read_ue(t->bs);
    assert(t->ps.num_slice_groups_minus1 == 0);
    t->ps.num_ref_idx_l0_active_minus1 = eg_read_ue(t->bs);
    t->ps.num_ref_idx_l1_active_minus1 = eg_read_ue(t->bs);
    t->ps.weighted_pred_flag = eg_read_direct(t->bs, 1);
    t->ps.weighted_bipred_idc = eg_read_direct(t->bs, 2);
    t->ps.pic_init_qp_minus26 = eg_read_se(t->bs);
    t->ps.pic_init_qs_minus26 = eg_read_se(t->bs);
    t->ps.chroma_qp_index_offset = eg_read_se(t->bs);
    t->ps.deblocking_filter_control_present_flag = eg_read_direct(t->bs, 1);

    t->refl0_num = t->ps.num_ref_idx_l0_active_minus1 + 1;
    t->refl1_num = t->ps.num_ref_idx_l1_active_minus1 + 1;
    t->qp_y = t->ps.pic_init_qp_minus26 + 26;
}

void
T264dec_parse_seq_header(T264_t* t)
{
    int32_t i;
    int32_t prev_ref_num = t->ss.num_ref_frames;

    t->ss.profile_idc = eg_read_direct(t->bs, 8);
    eg_read_skip(t->bs, 8);
    t->ss.level_idc = eg_read_direct(t->bs, 8);
    t->ss.seq_id = eg_read_ue(t->bs);
    assert(t->ss.seq_id == 0);
    t->ss.log2_max_frame_num_minus4 = eg_read_ue(t->bs);
    t->ss.pic_order_cnt_type = eg_read_ue(t->bs);
    assert(t->ss.pic_order_cnt_type == 0);
    if (t->ss.pic_order_cnt_type == 0)
    {
        t->ss.max_pic_order = eg_read_ue(t->bs);
    }

    t->ss.num_ref_frames = eg_read_ue(t->bs);
    eg_read_skip(t->bs, 1);
    t->ss.pic_width_in_mbs_minus1 = eg_read_ue(t->bs);
    t->ss.pic_height_in_mbs_minus1 = eg_read_ue(t->bs);
    t->ss.frame_mbs_only_flag = eg_read_direct(t->bs, 1);
    assert(t->ss.frame_mbs_only_flag == 1);
    eg_read_skip(t->bs, 2);
    // vui_parameters_present_flag
    if (eg_read_direct(t->bs, 1))
    {
        // aspect_ratio_info_present_flag
        if (eg_read_direct(t->bs, 1) == 1)
            t->aspect_ratio = eg_read_direct(t->bs, 8);
        // overscan_info_present_flag
        eg_read_skip(t->bs, 1);
        // video_signal_type_present_flag
        if (eg_read_direct(t->bs, 1) == 1)
            t->video_format = eg_read_direct(t->bs, 3);
        // others discard
    }
    t->mb_height = t->ss.pic_height_in_mbs_minus1 + 1;
    t->mb_width = t->ss.pic_width_in_mbs_minus1 + 1;
    t->width = t->mb_width * 16;
    t->height = t->mb_height * 16;
    t->edged_width = t->width + 2 * EDGED_WIDTH;
    t->edged_height = t->height + 2 * EDGED_HEIGHT;
    t->stride    = t->width;
    t->stride_uv = t->width >> 1;
    t->edged_stride = t->edged_width;
    t->edged_stride_uv = t->edged_width >> 1;
    t->mb_stride = t->mb_width;

    /* malloc ref frame buffer */
    /* first we malloc current decode buffer */
    if (t->refn[0].Y[0] == 0)
    {
        uint8_t* p = T264_malloc(t->edged_width * t->edged_height + (t->edged_width * t->edged_height >> 1), CACHE_SIZE);
        t->refn[0].Y[0] = p + EDGED_HEIGHT * t->edged_width + EDGED_WIDTH;
        t->refn[0].U = p + t->edged_width * t->edged_height + (t->edged_width * EDGED_HEIGHT >> 2) + (EDGED_WIDTH >> 1);
        t->refn[0].V = p + t->edged_width * t->edged_height + (t->edged_width * t->edged_height >> 2) + (t->edged_width * EDGED_HEIGHT >> 2) + (EDGED_WIDTH >> 1);
        t->refn[0].mb = T264_malloc(t->mb_height * t->mb_width * sizeof(T264_mb_context_t), CACHE_SIZE);
        p = T264_malloc(t->edged_width * t->edged_height * 3, CACHE_SIZE);
        t->refn[0].Y[1] = p + EDGED_HEIGHT * t->edged_width + EDGED_WIDTH;
        t->refn[0].Y[2] = t->refn[0].Y[1] + t->edged_width * t->edged_height;
        t->refn[0].Y[3] = t->refn[0].Y[2] + t->edged_width * t->edged_height;
    }
    for (i = prev_ref_num ; i < t->ss.num_ref_frames ; i ++)
    {
        uint8_t* p = T264_malloc(t->edged_width * t->edged_height + (t->edged_width * t->edged_height >> 1), CACHE_SIZE);
        t->refn[i + 1].Y[0] = p + EDGED_HEIGHT * t->edged_width + EDGED_WIDTH;
        t->refn[i + 1].U = p + t->edged_width * t->edged_height + (t->edged_width * EDGED_HEIGHT >> 2) + (EDGED_WIDTH >> 1);
        t->refn[i + 1].V = p + t->edged_width * t->edged_height + (t->edged_width * t->edged_height >> 2) + (t->edged_width * EDGED_HEIGHT >> 2) + (EDGED_WIDTH >> 1);
        t->refn[i + 1].mb = T264_malloc(t->mb_height * t->mb_width * sizeof(T264_mb_context_t), CACHE_SIZE);
        p = T264_malloc(t->edged_width * t->edged_height * 3, CACHE_SIZE);
        t->refn[i + 1].Y[1] = p + EDGED_HEIGHT * t->edged_width + EDGED_WIDTH;
        t->refn[i + 1].Y[2] = t->refn[i + 1].Y[1] + t->edged_width * t->edged_height;
        t->refn[i + 1].Y[3] = t->refn[i + 1].Y[2] + t->edged_width * t->edged_height;
    }
    t->refl0_num = 0;
    t->refl1_num = 0;
}

void
T264dec_parse_custom_set(T264_t* t)
{
    int32_t encoder_ver = eg_read_direct(t->bs, 32);
    int32_t flag = eg_read_direct(t->bs, 32);

    if (flag & CUSTOM_FASTINTERPOLATE)
        t->flags |= USE_FASTINTERPOLATE;
}

void
T264dec_custom_buffer(T264_t* t, uint8_t* buf, int32_t stride)
{
}

decoder_state_t
T264dec_copy_nal(T264_t* t)
{
    uint8_t tmp;
    uint32_t shift;
    uint32_t shift1;

    shift = t->shift;
    shift1 = t->shift1;

    while (t->src_buf < t->src_end)
    {
        tmp = *t->src_buf ++;
        shift1 = (shift1 | tmp) << 8;

        if (shift1 == 0x00000300)
        {
            shift1 = 0xffffff00;    /* reset state */

            /* shift == x x 0 0 ==> shift == x x 0 ff
               here we need to prevent the next few words become a wrong start code */
            shift |= 0xff;

            continue;
        }

        shift = (shift << 8) | tmp;
        if (shift == 0x00000001)
        {
            t->shift1 = 0xffffff00;
            t->shift = -1;
            /* we do not copy the next start code */
            t->nal_len -= 3;
            return DEC_STATE_OK;
        }
        t->nal_buf[t->nal_len ++] = tmp;
    }

    t->shift = shift;
    t->shift1 = shift1;

    return DEC_STATE_BUFFER;
}

decoder_state_t
T264dec_find_nal(T264_t* t)
{
    int32_t find = 0;
    uint8_t tmp;
    uint32_t shift;

    shift = t->shift;	

    while (t->src_buf < t->src_end)
    {
        tmp = *t->src_buf ++;
        shift = (shift << 8) | tmp;
        
        if (shift == 0x00000001)
        {
            t->shift = -1;
            t->nal_len = 0;
            find = 1;
            break;
        }
    }

    if (find)
    {
        t->action = T264dec_copy_nal;
        return T264dec_parse(t);
    }

    t->shift = shift;	

    return DEC_STATE_BUFFER;
}

decoder_state_t
T264dec_decode_nal(T264_t* t)
{
    eg_init(t->bs, t->nal_buf, t->nal_len);

    /* ready for next nal */
    t->action = T264dec_copy_nal;
    t->nal_len = 0;

    eg_read_skip(t->bs, 1);  /* discard forbidden + nal_ref_idc */
    t->nal.nal_ref_idc = eg_read_direct(t->bs, 2);
    t->nal.nal_unit_type = eg_read_direct(t->bs, 5);

    switch (t->nal.nal_unit_type)
    {
    case NAL_SLICE_NOPART:
        T264dec_parse_slice(t);
        t->frame_num ++;
        t->frame_id ++;
        return DEC_STATE_SLICE;
    case NAL_PIC_SET:
        T264dec_parse_pic_header(t);
        return DEC_STATE_PIC;
    case NAL_SLICE_IDR:
        T264dec_parse_slice(t);
        t->frame_num ++;
        t->frame_id ++;
        return DEC_STATE_SLICE;
    case NAL_SEQ_SET:
        T264dec_parse_seq_header(t);
        t->frame_num = 0;
        return DEC_STATE_SEQ;
    case NAL_CUSTOM_SET:
        T264dec_parse_custom_set(t);
        return DEC_STATE_CUSTOM_SET;
    default:
        assert(0);
        break;
    }

    return DEC_STATE_UNKOWN;
}

T264_t*
T264dec_open()
{
    T264_t* t = T264_malloc(sizeof(T264_t), CACHE_SIZE);
	int i, j;
    memset(t, 0, sizeof(T264_t));

    t->nal_buf = T264_malloc(NAL_BUFFER_LEN, CACHE_SIZE);
    t->bs = T264_malloc(sizeof(bs_t), CACHE_SIZE);

	//for cabac
	for(j=0; j<4; j++)
	{
		for(i=5; i<8; i++)
		{
			t->mb.vec_ref[(j<<3)+i].vec[0].refno = -2;
			t->mb.vec_ref[(j<<3)+i].vec[0].x = 0;
			t->mb.vec_ref[(j<<3)+i].vec[0].y = 0;
			t->mb.vec_ref[(j<<3)+i].vec[1].refno = -2;
			t->mb.vec_ref[(j<<3)+i].vec[1].x = 0;
			t->mb.vec_ref[(j<<3)+i].vec[1].y = 0;
		}
	}
    T264_init_cpu(t);

    t->action = T264dec_find_nal;
    t->shift = -1;
    t->shift1 = 0xffffff00;

    t->need_deblock = 1;
    t->flags = USE_HALFPEL| USE_QUARTPEL;   /* interpolate func needs this flag */

    return t;
}

void
T264dec_close(T264_t* t)
{
    int32_t i;

    for(i = 0 ; i < t->ss.num_ref_frames + 1 ; i ++)
    {
        T264_free(t->refn[i].Y[0] - (EDGED_HEIGHT * t->edged_width + EDGED_WIDTH));
        T264_free(t->refn[i].mb);
        T264_free(t->refn[i].Y[1] - (EDGED_HEIGHT * t->edged_width + EDGED_WIDTH));
    }
    T264_free(t->bs);
    T264_free(t->nal_buf);
    T264_free(t);
}

void
T264dec_buffer(T264_t* t, uint8_t* buf, int32_t len)
{
    t->src_buf = buf;
    t->src_end = buf + len;
}

decoder_state_t
T264dec_parse(T264_t* t)
{
    if (t->action(t) == DEC_STATE_BUFFER)
    {
        return DEC_STATE_BUFFER;
    }

    return T264dec_decode_nal(t);
}

T264_frame_t* 
T264dec_flush_frame(T264_t* t)
{
    /* the last i/p frame */
    return &t->refn[1];
}
