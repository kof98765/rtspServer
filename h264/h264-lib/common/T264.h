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

#ifndef _T264_H_
#define _T264_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "portab.h"


/////////poly
#define DECLARE_ALIGNED_MATRIX_H(name,sizex,sizey,type,alignment) type name[(sizex)*(sizey)]
/////////


#define T264_MAJOR     ((uint32_t)0 << 16)
#define T264_MINOR     ((uint32_t)14)
#define T264_VER       (T264_MAJOR | T264_MINOR)
// flags field
#define USE_INTRA16x16 0x1      // use intra 16x16 pred
#define USE_INTRA4x4   0x2      // use intra 4x4   pred
#define USE_SUBBLOCK   0x4      // use inter 8x8   pred
#define USE_HALFPEL    0x8      // half pel search
#define USE_QUARTPEL   0x10     // quarter pel search
#define USE_FULLSEARCH 0x20     // full search
#define USE_DIAMONDSEACH        0x40
#define USE_FORCEBLOCKSIZE      0x80 // search specify block size
#define USE_FASTINTERPOLATE     0x100  // fast interpolate
#define USE_SAD                 0x10000  // use sad
#define USE_SATD                0x20000  // use satd
#define USE_INTRAININTER        0x40000 // inter will try intra mode
#define USE_EXTRASUBPELSEARCH   0x80000 // search 8 points vs 4 points (qulity vs. speed)
#define USE_SCENEDETECT         0x100000

// custom flag
#define CUSTOM_FASTINTERPOLATE (1 << 1)

#define SEARCH_16x16P 0x1
#define SEARCH_16x8P  0x2
#define SEARCH_8x16P  0x4
#define SEARCH_8x8P   0x8
#define SEARCH_8x4P   0x10
#define SEARCH_4x8P   0x20
#define SEARCH_4x4P   0x40

#define SEARCH_16x16B (0x1 << 16)
#define SEARCH_16x8B  (0x2 << 16)
#define SEARCH_8x16B  (0x4 << 16)
#define SEARCH_8x8B   (0x8 << 16)

// cpu ability
//#define T264_CPU_FORCE   0x1
//#define T264_CPU_MMX     0x10
//#define T264_CPU_SSE     0x1000
//#define T264_CPU_SSE2    0x10000
#define T264_CPU_FORCE   0x0
#define T264_CPU_MMX     0x00
#define T264_CPU_SSE     0x0000
#define T264_CPU_SSE2    0x00000

// mb_neighbour
#define MB_LEFT        0x1
#define MB_TOP         0x2
#define MB_TOPRIGHT    0x4

#define MAX_REFFRAMES  16
#define MAX_BREFNUMS   5
#define MAX_PLUGINS    5
#define CACHE_SIZE     16
#define EDGED_WIDTH    (32 + CACHE_SIZE)
#define EDGED_HEIGHT   EDGED_WIDTH

#define T264_MIN(a,b) ( (a)<(b) ? (a) : (b) )
#define T264_MAX(a,b) ( (a)>(b) ? (a) : (b) )

/* plugins state */
#define STATE_BEFORESEQ 0   /* before sequence */
#define STATE_BEFOREGOP 1   /* before gop header */
#define STATE_BEFOREPIC 2   /* before picture */
#define STATE_AFTERPIC  3   /* after picture encoded */

/* stat control */
#define DISPLAY_PSNR    1
#define DISPLAY_BLOCK   2

#define CHROMA_COEFF_COST 4 /* default chroma coeff cost */

#define IPM_LUMA 9          /* index i4x4 pred mode */
#define VEC_LUMA 9          /* index vector */
#define NNZ_LUMA 9          /* index non zero count */
#define NNZ_CHROMA0 14
#define NNZ_CHROMA1 38

enum
{
    NAL_SLICE_NOPART = 1,
    NAL_SLICE_PARTA,
    NAL_SLICE_PARTB,
    NAL_SLICE_PARTC,
    NAL_SLICE_IDR,
    NAL_SEI,
    NAL_SEQ_SET,
    NAL_PIC_SET,
    NAL_ACD,
    NAL_END_SEQ,
    NAL_END_STREAM,
    NAL_FILTER,
    NAL_CUSTOM_SET = 26
};

// Intra 16x16 MB predict
enum
{
	Intra_16x16_TOP = 0,
	Intra_16x16_LEFT,
	Intra_16x16_DC,
	Intra_16x16_PLANE,
	Intra_16x16_DCTOP,
	Intra_16x16_DCLEFT,
	Intra_16x16_DC128
};

// Intra chroma 8x8 MB predict
enum
{
    Intra_8x8_DC = 0,
    Intra_8x8_LEFT,
    Intra_8x8_TOP,
    Intra_8x8_PLANE,
    Intra_8x8_DCTOP,
    Intra_8x8_DCLEFT,
    Intra_8x8_DC128
};

// Intra 4x4 MB predict
enum
{
	Intra_4x4_TOP = 0,
	Intra_4x4_LEFT,
	Intra_4x4_DC,
	Intra_4x4_DIAGONAL_DOWNLEFT,	
	Intra_4x4_DIAGONAL_DOWNRIGHT,	
	Intra_4x4_VERTICAL_RIGHT,	
	Intra_4x4_HORIZONTAL_DOWN,	
	Intra_4x4_VERTICAL_LEFT,	
	Intra_4x4_HORIZONTAL_UP,	
	Intra_4x4_DCTOP = 9,
	Intra_4x4_DCLEFT,
	Intra_4x4_DC128
};

// Partition Size
enum
{
	MB_16x16 = 0,
	MB_16x8,
	MB_8x16,
	MB_8x8,
    MB_8x8ref0,
	MB_8x4,
	MB_4x8,
	MB_4x4,
	MB_2x2
};

enum
{
    B_DIRECT_16x16 = 10,
    B_L0_16x16,
    B_L1_16x16,
    B_Bi_16x16,
    B_L0_16x8,
    B_L1_16x8,
    B_Bi_16x8,
    B_L0_8x16,
    B_L1_8x16,
    B_Bi_8x16,
};

enum
{
    B_L0_L0_16x8 = 4,
    B_L0_L0_8x16,
    B_L1_L1_16x8,
    B_L1_L1_8x16,
    B_L0_L1_16x8,
    B_L0_L1_8x16,
    B_L1_L0_16x8,
    B_L1_L0_8x16,
    B_L0_Bi_16x8,
    B_L0_Bi_8x16,
    B_L1_Bi_16x8,
    B_L1_Bi_8x16,
    B_Bi_L0_16x8,
    B_Bi_L0_8x16,
    B_Bi_L1_16x8,
    B_Bi_L1_8x16,
    B_Bi_Bi_16x8,
    B_Bi_Bi_8x16,
    B_8x8
};

enum
{
    B_DIRECT_8x8 = 100,
    B_L0_8x8,
    B_L1_8x8,
    B_Bi_8x8,
    B_L0_8x4,
    B_L0_4x8,
    B_L1_8x4,
    B_L1_4x8,
    B_Bi_8x4,
    B_Bi_4x8,
    B_L0_4x4,
    B_L1_4x4,
    B_Bi_4x4
};

// MB encode mode
enum
{
    I_4x4,
	I_16x16,
    P_SKIP,
    P_MODE,
    B_SKIP,
    B_MODE,
};

// Slice type
enum
{
    SLICE_P = 0,
    SLICE_B,
    SLICE_I,
    SLICE_SP,
    SLICE_SI,
    SLICE_IDR
};

typedef enum 
{
    DEC_STATE_BUFFER = -1,
    DEC_STATE_OK,
    DEC_STATE_SEQ,
    DEC_STATE_PIC,
    DEC_STATE_SLICE,
    DEC_STATE_CUSTOM_SET,
    DEC_STATE_UNKOWN
} decoder_state_t;

//Inverse raster scan for 4x4 blocks in MB
/*
  from raster scan to lumaindex
  0    1   2   3 (orgin)
  4    5   6   7
  8    9  10  11
 12   13  14  15

  0    1   4   5
  2    3   6   7
  8    9  12  13
 10   11  14  15
*/
static const int8_t luma_index[] = 
{
    0, 1, 4, 5,
    2, 3, 6, 7,
    8, 9, 12, 13,
    10, 11, 14, 15
};

static const int8_t luma_inverse_x[] = 
{
    0, 1, 0, 1, 2, 3, 2, 3, 0, 1, 0, 1, 2, 3, 2, 3
};

static const int8_t luma_inverse_y[] = 
{
    0, 0, 1, 1, 0, 0, 1, 1, 2, 2, 3, 3, 2, 2, 3, 3
};

//! array used to find expencive coefficients
static const int8_t COEFF_COST[16] =
{
    3,2,2,1,1,1,0,0,0,0,0,0,0,0,0,0
};

typedef struct T264_t T264_t;
//typedef struct T264_search_context_t T264_search_context_t;

typedef struct  
{
    int32_t width, height;
    int32_t qp;
    int32_t bitrate;
    float   framerate;
    int32_t iframe;                     // I 帧间距
    int32_t idrframe;                   // idr 间距
    int32_t ref_num;                    // 参考帧数目
    int32_t b_num;                      // b frame num
    int32_t flags;
    int32_t cpu;
    int32_t search_x;
    int32_t search_y;
    int32_t block_size;
    int32_t disable_filter;
    int32_t aspect_ratio;
    int32_t video_format;
    int32_t luma_coeff_cost;
    int32_t min_qp;
    int32_t max_qp;
    int32_t enable_rc;
    int32_t enable_stat;
    int32_t direct_flag;
	//for CABAC
	int32_t cabac;
    void*   rec_name;
} T264_param_t;

typedef struct  
{
    int32_t i_block_num[2];
    int32_t p_block_num[8];
    int32_t b_block_num[2];
    int32_t skip_block_num;
} T264_stat_t;

//
// nal_unit
//
typedef struct
{
    int32_t nal_ref_idc;
    int32_t nal_unit_type;
    int32_t nal_size;
} T264_nal_t;

typedef struct
{
    int32_t profile_idc;
    int32_t level_idc;
    int32_t seq_id;
    int32_t log2_max_frame_num_minus4;
    int32_t pic_order_cnt_type;
    int32_t max_pic_order;
    int32_t num_ref_frames;
    int32_t pic_width_in_mbs_minus1;
    int32_t pic_height_in_mbs_minus1;
    int32_t frame_mbs_only_flag;
} T264_seq_set_t;

typedef struct
{
    int32_t pic_id;
    int32_t seq_id;
    int32_t entroy_coding_mode_flag;
    int32_t pic_order_present_flag;
    int32_t num_slice_groups_minus1;
    int32_t num_ref_idx_l0_active_minus1;
    int32_t num_ref_idx_l1_active_minus1;
    int32_t weighted_pred_flag;
    int32_t weighted_bipred_idc;
    int32_t pic_init_qp_minus26;
    int32_t pic_init_qs_minus26;
    int32_t chroma_qp_index_offset;
    int32_t deblocking_filter_control_present_flag;
} T264_pic_set_t;

typedef struct
{
    int32_t first_mb_in_slice;
    int32_t slice_type;
    int32_t pic_id;
    int32_t frame_num;
    int32_t idr_pic_id;
    /* pic_oder_cnt_type == 0*/
    int32_t pic_order_cnt_lsb;
    int32_t delta_pic_order_cnt_bottom;
    int32_t direct_spatial_mv_pred_flag;
    /*P frame*/
    int32_t num_ref_idx_active_override_flag;
    int32_t slice_qp_delta;
    int32_t ref_pic_list_reordering_flag_l0;
    int32_t no_output_of_prior_pics_flag;
    int32_t long_term_reference_flag;
    int32_t adaptive_ref_pic_marking_mode_flag;
	/* for CABAC */
	int32_t cabac_init_idc;
} T264_slice_t;

typedef struct
{
    int32_t ver;
    int32_t flags;
} T264_custom_set_t;

typedef struct
{
    void (*proc)(T264_t* t, void* data, int32_t state);
    void (*close)(T264_t* t, void* data);
    void* handle;
    int32_t ret;    // ret value, optional
} T264_plugin_t;

typedef struct
{
    int8_t  refno;
    int16_t x;
    int16_t y;
} T264_vector_t;

// this struct should keep 16 bytes align
typedef struct
{
    // intra 4x4 mode
    uint8_t     mode_i4x4[16];
    // inter sub partition size(if no sub partition -1)
    uint8_t     submb_part[16];
    // inter 4x4 block mv
    T264_vector_t vec[2][4 * 4];
	// for CABAC, mv delta
	int16_t		mvd[2][4*4][2];
    // non zero count
    uint8_t     nnz[16 + 4 + 4];
	// sad
	uint32_t    sad;

    // I16x16, I4x4, P_MODE, ....
    uint8_t     mb_mode;
    uint8_t     mb_mode_uv;
    // intra 16x16 mode
    uint8_t     mode_i16x16;
    // inter
    uint8_t     mb_part;
    // rate control
    int8_t      mb_qp_delta; 

	// for CABAC, the following field should be saved in the context
	uint8_t     is_copy;
	uint16_t    cbp_y;
	uint16_t    cbp_c;
	uint16_t	cbp;
    // just pad to 16 bytes aligned
    uint8_t     pad[CACHE_SIZE - 7];
} T264_mb_context_t;

typedef struct
{
    uint8_t*  Y[4], *U, *V;
    int32_t   poc;
    T264_mb_context_t* mb;
    int32_t   qp;
} T264_frame_t;

typedef struct T264_search_context_t
{
    // all candidate
    T264_vector_t* vec;
    int32_t vec_num;
    T264_vector_t vec_best;
    int32_t height;
    int32_t width;
    int8_t limit_x;
    int8_t limit_y;
    int32_t offset;
    int32_t mb_part;
    int32_t list_index;
} T264_search_context_t;

//this is for CABAC

typedef struct
{
	/* model */
	struct
	{
		int i_model;
		int i_cost;
	} slice[3];

	/* context */
	struct
	{
		int i_state;
		int i_mps;
		int i_count;
	} ctxstate[399];

	/* state */
	int i_low;
	int i_range;

	int i_sym_cnt;

	/* bit stream */
	int b_first_bit;
	int i_bits_outstanding;
	void *s;

} T264_cabac_t;

#ifndef _PREDICT_H_1
#define _PREDICT_H_1
typedef void (*T264_predict_16x16_mode_t)(uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left);
typedef void (*T264_predict_4x4_mode_t)  (uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left);
typedef void (*T264_predict_8x8_mode_t)  (uint8_t* dst, int32_t dst_stride, uint8_t* top, uint8_t* left);
#endif

typedef void (*expand8to16_t)(uint8_t* src, int32_t src_stride, int32_t quarter_width, int32_t quarter_height, int16_t* dst);
typedef void (*expand8to16sub_t)(uint8_t* pred, int32_t quarter_width, int32_t quarter_height, int16_t* dst, uint8_t* src, int32_t src_stride);
typedef void (*contract16to8_t)(int16_t* src, int32_t quarter_width, int32_t quarter_height, uint8_t* dst, int32_t dst_stride);
typedef void (*contract16to8add_t)(int16_t* src, int32_t quarter_width, int32_t quarter_height, uint8_t* org, uint8_t* dst, int32_t dst_stride);
typedef void (*memcpy_stride_u_t)(void* src, int32_t width, int32_t height, int32_t src_stride, void* dst, int32_t dst_stride);
typedef uint32_t (*T264_cmp_t)(uint8_t* src, int32_t src_stride, uint8_t* data, int32_t dst_stride);
typedef void (*T264_pia_t)(uint8_t* p1, uint8_t* p2, int32_t p1_stride, int32_t p2_stride, uint8_t* dst, int32_t dst_stride);
typedef uint32_t (*T264_satd_i16x16_u_t)(uint8_t* src, int32_t src_stride, uint8_t* data, int32_t dst_stride);

typedef void (*T264_dct_t)(int16_t* data);
typedef void (*T264_quant4x4_t)(int16_t* data, const int32_t Qp, int32_t is_intra);
typedef void (*T264_quant4x4dc_t)(int16_t* data, const int32_t Qp);
typedef void (*T264_quant2x2dc_t)(int16_t* data, const int32_t Qp, int32_t is_intra);
typedef void (*T264_iquant_t)(int16_t* data, const int32_t Qp);

typedef void (*T264_eighth_pixel_mc_u_t)(uint8_t* src, int32_t src_stride, uint8_t* dst, int16_t mvx, int16_t mvy, int32_t width, int32_t height);
typedef void (*T264_interpolate_halfpel_t)(uint8_t* src, int32_t src_stride, uint8_t* dst, int32_t dst_stride, int32_t width, int32_t height);
typedef void (*T264_pixel_avg_t)(uint8_t* p1, uint8_t* p2, int32_t p1_stride, int32_t p2_stride, uint8_t* dst, int32_t dst_stride, int32_t w, int32_t h);
typedef uint32_t (*T264_search_t)(T264_t* t, T264_search_context_t* context);
typedef void (*T264_emms_t)();
typedef decoder_state_t (*action_t)(T264_t* t);

typedef struct
{
    // COPY BEGIN
    // NOTE: copied from T264_mb_cache_t except padding words!!!
    // intra 4x4 mode
    uint8_t     mode_i4x4[16];
    // inter sub partition size(if no sub partition -1)
    uint8_t     submb_part[16];
    // inter 4x4 block mv
    T264_vector_t vec[2][4 * 4];
	// for CABAC, mv delta
	int16_t		mvd[2][4*4][2];
    // non zero count
    uint8_t     nnz[16 + 4 + 4];
    // sad
    uint32_t sad;

    // I16x16, I4x4, P_MODE, ....
    uint8_t     mb_mode;
    uint8_t     mb_mode_uv;
    // intra 16x16 mode
    uint8_t     mode_i16x16;
    // inter
    uint8_t     mb_part;
    // rate control
    int8_t      mb_qp_delta; 
	// for CABAC, the following 4 fields should be saved in contexts
	uint8_t     is_copy;
	uint16_t    cbp_y;
	uint16_t    cbp_c;
	uint16_t	cbp;
    // COPY END

    T264_mb_context_t* context;
    uint8_t     mb_part2[2];    // b slice use

    int32_t     mb_neighbour;
    int16_t     mb_x;
    int16_t     mb_y;
    int16_t		mb_xy;
    int32_t     lambda;

    uint8_t*    src_y;
    uint8_t*    dst_y;	
    uint8_t*    src_u;
    uint8_t*    dst_u;
    uint8_t*    src_v;
    uint8_t*    dst_v;

    // save the predict value for intra encode
    DECLARE_ALIGNED_MATRIX_H(pred_i16x16, 16, 16, uint8_t, CACHE_SIZE);
    DECLARE_ALIGNED_MATRIX_H(pred_i8x8u, 8, 8, uint8_t, CACHE_SIZE);
    DECLARE_ALIGNED_MATRIX_H(pred_i8x8v, 8, 8, uint8_t, CACHE_SIZE);
    DECLARE_ALIGNED_MATRIX_H(pred_p16x16, 16, 16, uint8_t, CACHE_SIZE);
 
    int16_t     dct_y_z[16][4*4];               // 块进行Z扫描后的系数
    int16_t     dct_uv_z[2][4][4*4];
    int16_t     dc4x4_z[16];                    // Z扫描后的16个DC系数
    int16_t     dc2x2_z[2][4];                  // Z扫描后的4个DC系数
    // for CABAC, the following 4 fileds have been moved to the beginning
    uint8_t     sub_is_copy[4];

    int8_t i4x4_pred_mode_ref[5 * 8];   // see load_context for detail layout
    uint8_t nnz_ref[6 * 8];
	uint32_t sad_ref[3]; //left, top, top-right
    struct
    {
        T264_vector_t vec[2];
        uint8_t part;
        uint8_t subpart;
    } vec_ref[5 * 8];
	
	// for CABAC, mv delta
	int16_t mvd_ref[2][5 * 8][2];
} T264_mb_t;

struct T264_t
{
    T264_frame_t refn[MAX_REFFRAMES];
    T264_frame_t* ref[2][MAX_REFFRAMES];
    int32_t refl0_num;
    int32_t refl1_num;
    T264_frame_t cur;
    T264_frame_t* rec;
    int32_t     width;
    int32_t     height;
    int32_t     stride;
    int32_t     stride_uv;
    int32_t     edged_stride;
    int32_t     edged_stride_uv;
    int32_t     edged_width;
    int32_t     edged_height;
    int32_t     qp_y, qp_uv;
    void*        bs;
    uint8_t*    bs_buf;
    uint32_t    flags;
    int32_t     mb_width, mb_height;
	int32_t		mb_stride;
    uint32_t    idr_pic_id;
    /* the frame unique id in the whole encoding session, for statistic(rc uses it too) */
    uint32_t    frame_id;
    /* the frame_num in the bitstream semantic, for generating bitstream only */
    uint32_t    frame_num;
    /* the frame unique id in the current encoding gop, for deciding the slice type usage */
    uint32_t    frame_no;
    /* the frame unique id that the last key frame id */
    uint32_t    last_i_frame_id;
    uint32_t    poc;
    uint32_t    slice_type;
    int32_t     skip;
    uint32_t    sad_all;
    T264_frame_t pending_bframes[MAX_BREFNUMS];
    int32_t     pending_bframes_num;
    int32_t     header_bits;
    int32_t     frame_bits;

    /* ++ decoder section ++ */
    /* source section */
    uint8_t* src_buf;   /* source buffer start ptr */ 
    uint8_t* src_end;
    uint8_t* nal_buf;   /* buffer one whole nal unit */
    int32_t nal_len;    /* one nal unit length */

    uint32_t shift;     /* nal decode use */
    uint32_t shift1;

    action_t action;

    int32_t need_deblock;
    T264_frame_t* cur_frame;
    /* frame rate info */
    int32_t aspect_ratio;
    int32_t video_format;
    T264_frame_t output;
    /* -- decoder section -- */

    T264_param_t   param;
    T264_nal_t     nal;
    T264_seq_set_t ss;
    T264_pic_set_t ps;
    T264_slice_t   slice;
    T264_stat_t    stat;

    T264_mb_t      mb;
    int16_t        subpel_pts;
    T264_plugin_t  plugins[MAX_PLUGINS];

	//for CABAC
	T264_cabac_t cabac;

    int32_t        plug_num;
    T264_predict_16x16_mode_t pred16x16[4 + 3];
	T264_predict_8x8_mode_t   pred8x8[4 + 3];
	T264_predict_4x4_mode_t   pred4x4[9 + 3];
	T264_cmp_t cmp[8];
    T264_cmp_t sad[8];
    T264_pia_t pia[9];  //for pixel avearage func

    T264_dct_t fdct4x4;
    T264_dct_t fdct4x4dc;
    T264_dct_t fdct2x2dc;
    T264_dct_t idct4x4;
    T264_dct_t idct4x4dc;
    T264_dct_t idct2x2dc;

    T264_quant4x4_t   quant4x4;
    T264_quant4x4dc_t quant4x4dc;
    T264_quant2x2dc_t quant2x2dc;
    T264_iquant_t     iquant4x4;
    T264_iquant_t     iquant4x4dc;
    T264_iquant_t     iquant2x2dc;

    expand8to16_t     expand8to16;
    contract16to8_t   contract16to8;
    expand8to16sub_t  expand8to16sub;
    contract16to8add_t contract16to8add;
    memcpy_stride_u_t   memcpy_stride_u;
    T264_eighth_pixel_mc_u_t eighth_pixel_mc_u;
    T264_interpolate_halfpel_t interpolate_halfpel_h;
    T264_interpolate_halfpel_t interpolate_halfpel_v;
    T264_interpolate_halfpel_t interpolate_halfpel_hv;
    T264_pixel_avg_t pixel_avg;
    T264_satd_i16x16_u_t T264_satd_16x16_u;
    T264_search_t   search;
    T264_emms_t emms;
};

/* private func(for encoder & decoder share) */
void T264_init_cpu(T264_t* t);
void T264_mb_load_context(T264_t* t, int32_t mb_y, int32_t mb_x);
void T264_extend_border(T264_t* t, T264_frame_t* f);
void T264_interpolate_halfpel(T264_t* t, T264_frame_t* f);

/* extern api */
T264_t* T264_open(T264_param_t* para);
void T264_close(T264_t* t);
int32_t T264_encode(T264_t* t, uint8_t* src, uint8_t* dst, int32_t dst_size);

T264_t* T264dec_open();
void T264dec_close(T264_t* t);
void T264dec_buffer(T264_t* t, uint8_t* buf, int32_t len);
decoder_state_t T264dec_parse(T264_t* t);
T264_frame_t* T264dec_flush_frame(T264_t* t);

#ifdef __cplusplus
};
#endif

#endif
