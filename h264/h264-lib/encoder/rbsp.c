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
#include "bitstream.h"
#include "portab.h"
#include "rbsp.h"
#include "cabac_engine.h"

void 
nal_unit_init(_RW T264_nal_t* nal, int32_t nal_ref_idc, int32_t nal_unit_type)
{
    nal->nal_ref_idc   = nal_ref_idc;
    nal->nal_unit_type = nal_unit_type;
}

void 
nal_unit_write(_R T264_t* t, _R T264_nal_t* nal)
{
    eg_write_direct(t->bs, 1, 32);
    eg_write_direct(t->bs, ((uint32_t)nal->nal_ref_idc) << 5 | nal->nal_unit_type, 8);
}

void 
seq_set_init(_R T264_t* t, _RW T264_seq_set_t* seq)
{
    //
    // NOTE: FIXME
    //
	//for CABAC
	if( t->param.cabac || t->param.b_num > 0 )
		seq->profile_idc               = 77;
	else
		seq->profile_idc      = 66;
  
    seq->level_idc                 = 30;
    seq->seq_id                    = 0;
    seq->log2_max_frame_num_minus4 = 12;
    seq->pic_order_cnt_type        = 0;
    seq->max_pic_order             = 12;
    seq->num_ref_frames            = t->param.ref_num;
    seq->pic_width_in_mbs_minus1   = t->width / 16 - 1;
    seq->pic_height_in_mbs_minus1  = t->height / 16 - 1;
    seq->frame_mbs_only_flag       = 1;
}

void 
seq_set_write(_R T264_t* t, _RW T264_seq_set_t* seq)
{
    eg_write_direct(t->bs, ((uint32_t)seq->profile_idc << 16) |seq->level_idc, 24);
    eg_write_ue(t->bs, seq->seq_id);
    eg_write_ue(t->bs, seq->log2_max_frame_num_minus4);
    eg_write_ue(t->bs, seq->pic_order_cnt_type);

    if (seq->pic_order_cnt_type == 0)
    {
        eg_write_ue(t->bs, seq->max_pic_order);
    }
    else
    {
        //
        // ...
        //
    }

    eg_write_ue(t->bs, seq->num_ref_frames);
    eg_write_direct1(t->bs, 0);
    eg_write_ue(t->bs, seq->pic_width_in_mbs_minus1);
    eg_write_ue(t->bs, seq->pic_height_in_mbs_minus1);
    eg_write_direct1(t->bs, seq->frame_mbs_only_flag);
    eg_write_direct1(t->bs, 0);
    eg_write_direct1(t->bs, 0);
    // vui_parameters_present_flag
    eg_write_direct1(t->bs, 1);
    // aspect_ratio_info_present_flag
    eg_write_direct1(t->bs, 1);
    eg_write_direct(t->bs, t->param.aspect_ratio, 8);
    // overscan_info_present_flag
    eg_write_direct1(t->bs, 0);
    // video_signal_type_present_flag
    eg_write_direct1(t->bs, 1);
    eg_write_direct(t->bs, t->param.video_format, 3);
    // video_full_range_flag
    eg_write_direct1(t->bs, 0);
    // colour_description_present_flag
    eg_write_direct1(t->bs, 0);
    // chroma_loc_info_present_flag
    eg_write_direct1(t->bs, 0);
    // timing_info_present_flag
    // nal_hrd_parameters_present_flag
    // vcl_hrd_parameters_present_flag
    // pic_struct_present_flag 
    // bitstream_restriction_flag
    eg_write_direct(t->bs, 0, 5);

    rbsp_trailing_bits(t);
}

void 
pic_set_init(_R T264_t* t, _RW T264_pic_set_t* pic)
{
    pic->seq_id = 0;
    pic->pic_id = 0;
	//for CABAC
	if(t->param.cabac == 0)
    	pic->entroy_coding_mode_flag = 0;
	else
		pic->entroy_coding_mode_flag = 1;

    pic->pic_order_present_flag = 0;
    pic->num_slice_groups_minus1 = 0;
    pic->num_ref_idx_l0_active_minus1 = t->param.ref_num - 1;
    pic->num_ref_idx_l1_active_minus1 = 0;
    pic->weighted_pred_flag = 0;
    pic->weighted_bipred_idc = 0;
    pic->pic_init_qp_minus26 = t->param.qp - 26;
    pic->pic_init_qs_minus26 = t->param.qp - 26;
    pic->chroma_qp_index_offset = 0;
    if (t->param.disable_filter)
        pic->deblocking_filter_control_present_flag = 1;
    else
        pic->deblocking_filter_control_present_flag = 0;
}

void 
pic_set_write(_R T264_t* t, _RW T264_pic_set_t* pic)
{
    eg_write_ue(t->bs, pic->pic_id);
    eg_write_ue(t->bs, pic->seq_id);
    eg_write_direct1(t->bs, pic->entroy_coding_mode_flag);
    eg_write_direct1(t->bs, pic->pic_order_present_flag);
    eg_write_ue(t->bs, pic->num_slice_groups_minus1);
    eg_write_ue(t->bs, pic->num_ref_idx_l0_active_minus1);
    eg_write_ue(t->bs, pic->num_ref_idx_l1_active_minus1);
    eg_write_direct1(t->bs, pic->weighted_pred_flag);
//  eg_write_direct1(t->bs, pic->weighted_bipred_idc); /*error bits*/
	eg_write_direct(t->bs, pic->weighted_bipred_idc, 2);
    eg_write_se(t->bs, pic->pic_init_qp_minus26);
    eg_write_se(t->bs, pic->pic_init_qs_minus26);
    eg_write_se(t->bs, pic->chroma_qp_index_offset);
    eg_write_direct1(t->bs, pic->deblocking_filter_control_present_flag);
    eg_write_direct1(t->bs, 0);
    eg_write_direct1(t->bs, 0);
    rbsp_trailing_bits(t);
}

void 
slice_header_init(_R T264_t* t, _RW T264_slice_t* slice)
{
    slice->first_mb_in_slice = 0;
	slice->slice_type = t->slice_type;	/* ^^^ */
    slice->pic_id = 0;
    slice->idr_pic_id = t->idr_pic_id;
    slice->pic_order_cnt_lsb = t->poc % (1 << (t->ss.max_pic_order + 4));
    slice->frame_num = t->frame_num;
    slice->direct_spatial_mv_pred_flag = t->param.direct_flag;
    slice->ref_pic_list_reordering_flag_l0 = 0;
    slice->no_output_of_prior_pics_flag = 0;
    slice->long_term_reference_flag = 0;
    slice->adaptive_ref_pic_marking_mode_flag = 0;
    slice->slice_qp_delta = 0;

	//for CABAC
	/* get adapative cabac model if needed */
	if( t->param.cabac )
	{
		t->slice.cabac_init_idc = T264_cabac_model_get( &t->cabac, t->slice_type );
	}
}

void 
slice_header_write(_R T264_t* t, _RW T264_slice_t* slice)
{
    eg_write_ue(t->bs, slice->first_mb_in_slice);
    eg_write_ue(t->bs, slice->slice_type);
    eg_write_ue(t->bs, slice->pic_id);
    eg_write_direct(t->bs, slice->frame_num, t->ss.log2_max_frame_num_minus4 + 4);
    if (t->nal.nal_unit_type == NAL_SLICE_IDR)
    {
        eg_write_ue(t->bs, slice->idr_pic_id);
    }
    if (t->ss.pic_order_cnt_type == 0)
    {
        eg_write_direct(t->bs, slice->pic_order_cnt_lsb, t->ss.max_pic_order + 4);
    }
    if (t->slice_type == SLICE_P)
    {
        // num_ref_idx_active_override_flag
        if (t->refl0_num == t->param.ref_num)
        {
            eg_write_direct1(t->bs, 0);
        }
        else
        {
            eg_write_direct1(t->bs, 1);
            eg_write_ue(t->bs, t->refl0_num - 1);
        }
        t->ps.num_ref_idx_l0_active_minus1 = t->refl0_num - 1;
        t->ps.num_ref_idx_l1_active_minus1 = 0;
    }
    else if (t->slice_type == SLICE_B)
    {
        // direct_spatial_mv_pred_flag
        eg_write_direct1(t->bs, slice->direct_spatial_mv_pred_flag);
        // num_ref_idx_active_override_flag
        {
            eg_write_direct1(t->bs, 1);
            eg_write_ue(t->bs, t->refl0_num - 1);
            eg_write_ue(t->bs, t->refl1_num - 1);
        }
        t->ps.num_ref_idx_l0_active_minus1 = t->refl0_num - 1;
        t->ps.num_ref_idx_l1_active_minus1 = t->refl1_num - 1;
    }

    /* ref_pic_list_reordering() */
	/*
	 *	^^^ ccc
	 */
	if(slice->slice_type != SLICE_I && slice->slice_type != SLICE_SI )
	{
		eg_write_direct1(t->bs, slice->ref_pic_list_reordering_flag_l0);
        if (slice->slice_type == SLICE_B)
            eg_write_direct1(t->bs, slice->ref_pic_list_reordering_flag_l0);
	}

    if (t->nal.nal_ref_idc != 0)
    {
        /* dec_ref_pic_marking() */
        if (t->nal.nal_unit_type == NAL_SLICE_IDR)
        {
            eg_write_direct1(t->bs, slice->no_output_of_prior_pics_flag);
            eg_write_direct1(t->bs, slice->long_term_reference_flag);
        }
        else
        {
            eg_write_direct1(t->bs, slice->adaptive_ref_pic_marking_mode_flag);
        }
    }

	//for CABAC
	if(t->ps.entroy_coding_mode_flag!=0 && t->slice_type!= SLICE_I)
	{
		eg_write_ue(t->bs, t->slice.cabac_init_idc);
	}

    eg_write_se(t->bs, t->qp_y - t->param.qp);
    if (t->ps.deblocking_filter_control_present_flag)
    {
        eg_write_ue(t->bs, t->param.disable_filter);
    }
}

void 
custom_set_init(_R T264_t* t, _RW T264_custom_set_t* set)
{
    set->ver = T264_VER;
    set->flags = 0;
    if (t->flags & USE_FASTINTERPOLATE)
        set->flags |= CUSTOM_FASTINTERPOLATE;
}

void 
custom_set_write(_R T264_t* t, _RW T264_custom_set_t* set)
{
    eg_write_direct(t->bs, set->ver, 32);
    eg_write_direct(t->bs, set->flags, 32);
}

void rbsp_trailing_bits(_R T264_t* t)
{
    eg_write_direct1(t->bs, 1);
    eg_align(t->bs);
}
