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
#include "stdlib.h"
#include "T264.h"
#include "stat.h"
#include "math.h"
#ifndef CHIP_DM642
#include "memory.h"
#endif
#include "deblock.h"

typedef struct  
{
    T264_stat_t stat_prev;
    FILE* rec;
    uint8_t* buf;
    uint8_t* bufb;
    int32_t frame_no;
    float psnr_y;
    float psnr_u;
    float psnr_v;
} stat_t;

static float __inline
psnr(uint8_t* p1, uint8_t* p2, int32_t size)
{
    float sad = 0;
    int32_t i;

    for (i = 0 ; i < size ; i ++)
    {
        int32_t tmp;
        tmp = (p1[i] - p2[i]);
        sad += tmp * tmp;
    }

    return (float)(10 * log10(65025.0f * size / sad));
}

static void
stat_proc(T264_t* t, void* data, int32_t state)
{
    T264_frame_t* f;
    float y, u, v;
    int32_t i;
    uint8_t* d, *buf, *bufs;
    stat_t* stat = data;
    T264_stat_t* stat_prev = &stat->stat_prev;
    static const char type[] = {'P', 'B', 'I'};
    
    switch (state) 
    {
    case STATE_AFTERPIC:
        if (t->slice_type == SLICE_B)
        {			
            f = &t->refn[0];
            bufs = buf = stat->bufb;
            if (t->param.disable_filter == 0)
                T264_deblock_frame(t, f);
        }
        else
        {
            f = &t->refn[1];
            bufs = buf = stat->buf;
        }

        d = f->Y[0];
        for (i = 0 ; i < t->param.height ; i ++)
        {
            memcpy(buf, d, t->param.width);
            d += t->edged_stride;
            buf += t->stride;
        }
        y = psnr(t->cur.Y[0], bufs, t->width * t->height);
        d = f->U;
        for (i = 0 ; i < t->param.height >> 1 ; i ++)
        {
            memcpy(buf, d, t->param.width >> 1);
            d += t->edged_stride_uv;
            buf += t->stride_uv;
        }
        u = psnr(t->cur.U, bufs + t->width * t->height, t->width * t->height >> 2);
        d = f->V;
        for (i = 0 ; i < t->param.height >> 1 ; i ++)
        {
            memcpy(buf, d, t->param.width >> 1);
            d += t->edged_stride_uv;
            buf += t->stride_uv;
        }
        v = psnr(t->cur.V, bufs + t->width * t->height + (t->width * t->height >> 2), t->width * t->height >> 2);
        if (stat->rec)
        {
            if (t->slice_type == SLICE_B)
            {
                fwrite(stat->bufb, t->param.height * t->param.width + (t->param.height * t->param.width >> 1), 1, stat->rec);
                if (t->pending_bframes_num == 0)
                    fwrite(stat->buf, t->param.height * t->param.width + (t->param.height * t->param.width >> 1), 1, stat->rec);
            }
            else if ((t->slice_type == SLICE_I && t->param.b_num == 0) // no b slice we write it immediately
                || (t->slice_type == SLICE_I && (t->frame_id - 1) % t->param.idrframe == 0)) // if we have b slice we only write it when it is an idr frame
            {
                fwrite(stat->buf, t->param.height * t->param.width + (t->param.height * t->param.width >> 1), 1, stat->rec);
            }
            else if (t->param.b_num == 0)
            {
                fwrite(stat->buf, t->param.height * t->param.width + (t->param.height * t->param.width >> 1), 1, stat->rec);
            }
        }
        if (t->param.enable_stat & DISPLAY_PSNR)
            printf("%4d(%c) %4d %5d %d %.4f %.4f %.4f\n", 
                stat->frame_no ++,
                type[t->slice_type],
                t->cur.poc,
                t->frame_bits / 8,
                t->qp_y,
                y, u, v);
        if (t->param.enable_stat & DISPLAY_BLOCK)
            printf(" i4x4:%d, i16x16:%d, pskip:%d, p16x16:%d, p16x8:%d, p8x16:%d, p8x8:%d.\n",
                t->stat.i_block_num[I_4x4] - stat_prev->i_block_num[I_4x4], 
                t->stat.i_block_num[I_16x16] - stat_prev->i_block_num[I_16x16], 
                t->stat.skip_block_num - stat_prev->skip_block_num,
                t->stat.p_block_num[MB_16x16] - stat_prev->p_block_num[MB_16x16], 
                t->stat.p_block_num[MB_16x8] - stat_prev->p_block_num[MB_16x8], 
                t->stat.p_block_num[MB_8x16] - stat_prev->p_block_num[MB_8x16], 
                t->stat.p_block_num[MB_8x8] - stat_prev->p_block_num[MB_8x8]);

        stat->psnr_y += y;
        stat->psnr_u += u;
        stat->psnr_v += v;

        *stat_prev = t->stat;
        break;
    }
}

void 
stat_create(T264_t* t, T264_plugin_t* plugin)
{
    stat_t* s = malloc(sizeof(stat_t));
    s->rec = fopen(t->param.rec_name, "wb");
    memset(&s->stat_prev, 0, sizeof(s->stat_prev));
    s->buf = malloc(t->width * t->height + (t->width * t->height >> 1));
    s->bufb = malloc(t->width * t->height + (t->width * t->height >> 1));
    s->frame_no = 0;
    s->psnr_y = 0;
    s->psnr_u = 0;
    s->psnr_v = 0;
    plugin->handle = s;
    plugin->proc = stat_proc;
    plugin->close = stat_destroy;
    plugin->ret = 0;
}

void 
stat_destroy(T264_t* t, T264_plugin_t* plugin)
{
    stat_t* s = plugin->handle;

    printf("Average PSNR Y: %.2f U: %.2f V: %.2f.\n", s->psnr_y / s->frame_no, s->psnr_u / s->frame_no, s->psnr_v / s->frame_no);
    free(s->buf);
    free(s->bufb);
    if (s->rec)
    {
        fclose(s->rec);
    }
}
