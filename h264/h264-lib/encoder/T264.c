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

// T264.cpp : Defines the entry point for the console application.
//

//#define USE_DISPLAY
#include "config.h"

#ifdef USE_DISPLAY
#ifndef __GCC__
#include "win.h"
#else
#include "display.h"
#endif
#endif

//Ti_DSP Platform ported by YouXiaoquan,HFUT-Ti United Lab,China
//YouXiaoquan@126.com
#include "stdio.h"
#ifndef CHIP_DM642
#include "sys/timeb.h"
#endif
#include "time.h"

#include "stdlib.h"
#ifndef CHIP_DM642
#include "memory.h"
#endif
#include "math.h"
#include "T264.h"
#include "utility.h"
#include "string.h"


// parameters begin
int32_t total_no = 300;
char src_path[256];
char out_path[256];
char rec_path[256];
// for decoder PSNR
char ref_path[256];
static int  ref_skip;

// parameters end

#ifdef USE_DISPLAY

void
winDisplay(T264_t* t, T264_frame_t* f)
{
    uint8_t* p;
    unsigned char* buffer1, *buffer2, *buffer3, *Y, *U, *V;
    unsigned char *src[3];
    int32_t i;

    src[0] = Y = buffer1 = malloc(t->width*t->height*sizeof(char));
    p = f->Y[0];
    for (i = 0 ; i < t->height ; i++)
    {
        memcpy(buffer1, p, t->width);
        buffer1 += t->width;
        p += t->edged_stride;
    }

    src[2] = V = buffer2 = malloc((t->width*t->height*sizeof(char))>>2);
    p = f->V;
    for (i = 0 ; i < (t->height >> 1); i++)
    {
        memcpy(buffer2, p, t->width >> 1);
        buffer2 += (t->width >> 1);
        p += t->edged_stride_uv;
    }

    src[1] = U = buffer3 = malloc((t->width*t->height*sizeof(char))>>2);
    p = f->U;
    for (i = 0 ; i < (t->height >> 1); i++)
    {
        memcpy(buffer3, p, t->width >> 1);
        buffer3 += (t->width >> 1);
        p += t->edged_stride_uv;
    }
#ifndef __GCC__
    displayImage(Y,V,U);
#else
    dither(src);
    free(src[1]);
    free(src[2]);
    free(src[0]);
#endif
}

void
uninit_display()
{
#ifndef __GCC__
    closeDisplay ();
#else
    exit_display();
#endif
}
#endif

void
init_param(T264_param_t* param, const char* file)
{
    FILE* fd; 
    char line[255];
    int32_t b;
    if (!(fd = fopen(file,"r")))
    {
        printf("Couldn't open parameter file %s.\n", file);
        exit(-1);
    }

    memset(param, 0, sizeof(*param));
    fgets(line, 254, fd); sscanf(line,"%d", &b);
    if (b != 4)
    {
        printf("wrong param file version, expect v4.0\n");
        exit(-1);
    }
    fgets(line, 254, fd); sscanf(line,"%d", &param->width);
    fgets(line, 254, fd); sscanf(line,"%d", &param->height);
    fgets(line, 254, fd); sscanf(line,"%d", &param->search_x);
    fgets(line, 254, fd); sscanf(line,"%d", &param->search_y);
    fgets(line, 254, fd); sscanf(line,"%d", &total_no);
    fgets(line, 254, fd); sscanf(line,"%d", &param->iframe);
    fgets(line, 254, fd); sscanf(line,"%d", &param->idrframe);
    fgets(line, 254, fd); sscanf(line,"%d", &param->b_num);
    fgets(line, 254, fd); sscanf(line,"%d", &param->ref_num);
    fgets(line, 254, fd); sscanf(line,"%d", &param->enable_rc);
    fgets(line, 254, fd); sscanf(line,"%d", &param->bitrate);
    fgets(line, 254, fd); sscanf(line,"%f", &param->framerate);
    fgets(line, 254, fd); sscanf(line,"%d", &param->qp);
    fgets(line, 254, fd); sscanf(line,"%d", &param->min_qp);
    fgets(line, 254, fd); sscanf(line,"%d", &param->max_qp);
    fgets(line, 254, fd); sscanf(line,"%d", &param->enable_stat);
    fgets(line, 254, fd); sscanf(line,"%d", &param->disable_filter);
    fgets(line, 254, fd); sscanf(line,"%d", &param->aspect_ratio);
    fgets(line, 254, fd); sscanf(line,"%d", &param->video_format);
    fgets(line, 254, fd); sscanf(line,"%d", &param->luma_coeff_cost);
    fgets(line, 254, fd); sscanf(line,"%d", &b);
    param->flags |= (USE_INTRA16x16) * (!!b);
    fgets(line, 254, fd); sscanf(line,"%d", &b);
    param->flags |= (USE_INTRA4x4) * (!!b);
    fgets(line, 254, fd); sscanf(line,"%d", &b);
    param->flags |= (USE_INTRAININTER) * (!!b);
    fgets(line, 254, fd); sscanf(line,"%d", &b);
    param->flags |= (USE_HALFPEL) * (!!b);
    fgets(line, 254, fd); sscanf(line,"%d", &b);
    param->flags |= (USE_QUARTPEL) * (!!b);
    fgets(line, 254, fd); sscanf(line,"%d", &b);
    param->flags |= (USE_SUBBLOCK) * (!!b);
    fgets(line, 254, fd); sscanf(line,"%d", &b);
    param->flags |= (USE_FULLSEARCH) * (!!b);
    fgets(line, 254, fd); sscanf(line,"%d", &b);
    param->flags |= (USE_DIAMONDSEACH) * (!!b);
    fgets(line, 254, fd); sscanf(line,"%d", &b);
    param->flags |= (USE_FORCEBLOCKSIZE) * (!!b);
    fgets(line, 254, fd); sscanf(line,"%d", &b);
    param->flags |= (USE_FASTINTERPOLATE) * (!!b);
    fgets(line, 254, fd); sscanf(line,"%d", &b);
    param->flags |= (USE_SAD) * (!!b);
    fgets(line, 254, fd); sscanf(line,"%d", &b);
    param->flags |= (USE_EXTRASUBPELSEARCH) * (!!b);
    fgets(line, 254, fd); sscanf(line,"%d", &b);
    param->flags |= (USE_SCENEDETECT) * (!!b);
    fgets(line, 254, fd); sscanf(line,"%d", &b);
    param->block_size |= (SEARCH_16x16P) * (!!b);
    fgets(line, 254, fd); sscanf(line,"%d", &b);
    param->block_size |= (SEARCH_16x8P) * (!!b);
    fgets(line, 254, fd); sscanf(line,"%d", &b);
    param->block_size |= (SEARCH_8x16P) * (!!b);
    fgets(line, 254, fd); sscanf(line,"%d", &b);
    param->block_size |= (SEARCH_8x8P) * (!!b);
    fgets(line, 254, fd); sscanf(line,"%d", &b);
    param->block_size |= (SEARCH_8x4P) * (!!b);
    fgets(line, 254, fd); sscanf(line,"%d", &b);
    param->block_size |= (SEARCH_4x8P) * (!!b);
    fgets(line, 254, fd); sscanf(line,"%d", &b);
    param->block_size |= (SEARCH_4x4P) * (!!b);
    fgets(line, 254, fd); sscanf(line,"%d", &b);
    param->block_size |= (SEARCH_16x16B) * (!!b);
    fgets(line, 254, fd); sscanf(line,"%d", &b);
    param->block_size |= (SEARCH_16x8B) * (!!b);
    fgets(line, 254, fd); sscanf(line,"%d", &b);
    param->block_size |= (SEARCH_8x16B) * (!!b);
    fgets(line, 254, fd); sscanf(line,"%d", &b);
    param->block_size |= (SEARCH_8x8B) * (!!b);
    fgets(line, 254, fd); sscanf(line,"%d", &param->cpu);
    fgets(line, 254, fd); sscanf(line, "%d", &param->cabac);

    fgets(line, 254, fd); sscanf(line,"%s", src_path);
    fgets(line, 254, fd); sscanf(line,"%s", out_path);
    fgets(line, 254, fd); sscanf(line,"%s", rec_path);
    param->rec_name = rec_path;

    fclose(fd);
}

int32_t 
encode(const char* paramfile)
{
    T264_param_t param;
    T264_t* t;
    uint8_t* buf, *dst;
    int32_t size;
    FILE* in_file, *out_file;
    uint32_t len;
    int32_t frame_no;
    int32_t bs_len = 0;

    uint8_t* rec;

    float total_time;
#ifdef _WIN32
    struct _timeb beg, end;
#endif

#ifdef CHIP_DM642 
	clock_t start_T=0,end_T=0,total_T=0;
#endif
    init_param(&param, paramfile);

    param.direct_flag = 1;
    // NOTE: currently we force p reference frame num = 1
    t = T264_open(&param);

    /* YV12p */
    size = param.height * param.width + (param.height * param.width >> 1);
    buf = T264_malloc(size, CACHE_SIZE);
    in_file = fopen(src_path, "rb");
    if (!in_file)
    {
        printf("cannot open input file.");
        return 0;
    }

    out_file = fopen(out_path, "wb");
    if (!out_file)
    {
        printf("cannot write 264 file.\n");
        return 0;
    }

    dst = T264_malloc(size, CACHE_SIZE);
    rec = T264_malloc(size, CACHE_SIZE);

#ifdef _WIN32
    _ftime(&beg);
#endif
    printf("frame poc length qp Y U V\n");
    for(frame_no = 0; frame_no < total_no; frame_no++)
    {        
        if (fread(buf, size, 1, in_file) <= 0)
            printf("cannot read from input file.");
#ifdef CHIP_DM642
		start_T=clock();
#endif
        len = T264_encode(t, buf, dst, size);
#ifdef CHIP_DM642 
		end_T=clock();
		total_T=total_T+end_T-start_T-192;
#endif    
        bs_len += len;

        if (fwrite(dst, len, 1, out_file) < 0)
            printf("cannot write 264 file.\n");
    }
#ifndef CHIP_DM642
#ifdef _WIN32
    _ftime(&end);
    total_time = (float)(end.time - beg.time) + (float)(end.millitm - beg.millitm) / 1000;
#endif
    printf("fps: %.2ffps, Length of Bitstream = %d Compact Ratio = %.2f\n", (float)(total_no) / total_time, bs_len, 1.0 * total_no * size / bs_len);
#endif
#ifdef CHIP_DM642
	printf("fps: %.2ffps with cpu 600MHz, Length of Bitstream = %d Compact Ratio = %.2f\n", ((float)total_no /((float)total_T/(float)600000000)),  bs_len, 1.0 * total_no * size / bs_len);
#endif
    fclose(in_file);
    fclose(out_file);
    T264_free(dst);
    T264_close(t);

    return 0;
}

void
write_frame(T264_t* t,T264_frame_t *frame,FILE *f_rec)
{
    int	i;
    uint8_t* p;

    if (f_rec)
    {
        p = frame->Y[0];
        for(i = 0 ; i < t->height ; i ++)
        {
            fwrite(p, t->width, 1, f_rec);
            p += t->edged_stride;
        }
        p = frame->U;
        for(i = 0 ; i < t->height >> 1 ; i ++)
        {
            fwrite(p, t->width >> 1, 1, f_rec);
            p += t->edged_stride_uv;
        }
        p = frame->V;
        for(i = 0 ; i < t->height >> 1 ; i ++)
        {
            fwrite(p, t->width >> 1, 1, f_rec);
            p += t->edged_stride_uv;
        }
    }
}
static float __inline
dec_psnr(uint8_t* p1, uint8_t* p2, int32_t width1, int32_t width2, int32_t height, int print)
{
	float sad = 0, psnr;
	int32_t i, j, size, ii, jj, diff, iii, jjj;
	uint8_t* p11, *p22;
	for(j=0; j<height; j+=16)
	{
		for (i = 0 ; i < width2 ; i +=16)
		{
			p11 = p1+i;
			p22 = p2+i;
			diff = 0;
			for(jj=0; jj<16; jj++)
			{
				for(ii=0; ii<16; ii++)
				{
					int32_t tmp;
					tmp = (p11[ii] - p22[ii]);
					sad += tmp * tmp;
					if(tmp != 0 && diff != 1)
					{
						if(print)
							printf("%2d != %2d ", p11[ii], p22[ii]);
						iii = ii;
						jjj = jj;
						diff = 1;
					}
				}
				p11 += width1;
				p22 += width2;
			}
			if(diff && print)
			{
				printf("%4d, %2d, %2d\n", (j)*(width2>>4) + (i), iii, jjj);
				print = 0;
			}
		}
		p1 += width1*16;
		p2 += width2*16;
	}
	size = width2 * height;
	if(sad < 1e-6)
		psnr = 0.0f;
	else
		psnr = (float)(10 * log10(65025.0f * size / sad));
	return psnr;
}

int32_t 
decode(const char* filename)
{
    /* just for show how to drive the decoder */
#define BUFFER_SIZE 4096
    T264_t* t = T264dec_open();
    uint8_t buffer[BUFFER_SIZE + 4];
    int32_t run = 1,screen = 0;
    FILE* src_file = fopen(filename, "rb");
    size_t size;
    FILE* f_rec = 0;
	//for decoder PSNR
	int frame_num = 0;
	FILE* f_ref = 0;
	uint8_t *ref_buf = NULL;
    int32_t frame_nums = 0;

    float total_time;
#ifdef _WIN32
    struct _timeb beg, end;
#endif
	
    // xxx
    printf("Current fully support t264 encoder's bitstream.\n");

    if (!src_file)
    {
        printf("cannot open file %s.\n", filename);
        return -1;
    }
    if (rec_path[0])
    {
        f_rec = fopen(rec_path, "wb");
        if (!f_rec)
        {
            printf("cannot open rec file %s.\n", rec_path);
        }
    }
	//for decoder PSNR
	if(ref_path[0])
	{
		f_ref = fopen(ref_path, "rb");
		if(!f_ref)
		{
			printf("cannot open ref file %s.\n", ref_path);
		}
	}

#ifdef _WIN32
    _ftime(&beg);
#endif
    while (run) 
    {
        decoder_state_t state = T264dec_parse(t);
        switch(state) 
        {
        case DEC_STATE_BUFFER:
            /* read more data */
            size = fread(buffer, 1, BUFFER_SIZE, src_file);
            if (size > 0)
            {
                if (size != BUFFER_SIZE)
                {
                    buffer[size] = 0;
                    buffer[size + 1] = 0;
                    buffer[size + 2] = 0;
                    buffer[size + 3] = 1;
                    size += 4;
                }

                T264dec_buffer(t, buffer, size);
            }
            else
            {
                /* if all data has readed, here we will return */
                run = 0;
                /* NOTE: here we should get the last frame */
                write_frame(t, T264dec_flush_frame(t), f_rec);
#ifdef USE_DISPLAY
                winDisplay(t, T264dec_flush_frame(t));
#endif
                frame_nums ++;
            }
            break;
        case DEC_STATE_PIC:
            /* write one pic */
            break;
        case DEC_STATE_SEQ:

#ifdef USE_DISPLAY
            if (screen == 0)
            {
#ifndef __GCC__
                initDisplay(t->width, t->height);
#else
                init_display("",t->width, t->height);
#endif
                screen++;
            }

#endif
            if (t->frame_id > 0)
            {
                /* NOTE: here we should get the last frame */
                write_frame(t, T264dec_flush_frame(t), f_rec);
    #ifdef USE_DISPLAY
                winDisplay(t, T264dec_flush_frame(t));
    #endif
                frame_nums ++;
            }
            printf("ref frames num: %d.\n", t->ss.num_ref_frames);
            printf("width: %d.\n", (t->ss.pic_width_in_mbs_minus1 + 1) << 4);
            printf("height: %d.\n", (t->ss.pic_height_in_mbs_minus1 + 1) << 4);
            break;
        case DEC_STATE_SLICE:
            {
                if (t->output.poc >= 0)
                {
                    write_frame(t, &t->output, f_rec);
				//for decoder PSNR
				if(f_ref)
				{
					float psnr_y, psnr_u, psnr_v;
					int size;
					size = t->width*t->height;
					if(ref_buf == NULL)
					{
						ref_buf = (uint8_t *)malloc(size);
					}
					
					if(ref_buf != NULL)
					{
						uint8_t *p;
						p = t->output.Y[0];
						fread(ref_buf, 1, size, f_ref);
						psnr_y = dec_psnr(p, ref_buf, t->edged_stride, t->width, t->height, 0);

						size >>= 2;
						p = t->output.U;
						fread(ref_buf, 1, size, f_ref);
						psnr_u = dec_psnr(p, ref_buf, t->edged_stride_uv, t->width>>1, t->height>>1, 0);
						p = t->output.V;
						fread(ref_buf, 1, size, f_ref);
						psnr_v = dec_psnr(p, ref_buf, t->edged_stride_uv, t->width>>1, t->height>>1, 0);
						printf("%4d, %.2f, %.2f, %.2f\n", frame_num++, psnr_y, psnr_u, psnr_v);
						if(ref_skip > 0)
						{
							fseek(f_ref, ref_skip*size*6, SEEK_CUR);
						}
					}
				}
#ifdef USE_DISPLAY
                    winDisplay(t, &t->output);
#endif
                    frame_nums ++;                
                }
            };
            break;
        case DEC_STATE_CUSTOM_SET:
            {
                printf("used fast interpolate: %s.\n", t->flags & USE_FASTINTERPOLATE ? "yes" : "no");
            }
            break;
        default:
            /* do not care */
            break;
        }
    };

#ifndef CHIP_DM642
#ifdef _WIN32
    _ftime(&end);
    total_time = (float)(end.time - beg.time) + (float)(end.millitm - beg.millitm) / 1000;
#endif
    printf("fps: %.2ffps(total decode: %d frames).\n", (float)frame_nums / total_time, frame_nums);
#endif    
#ifdef CHIP_DM642
	printf("fps: %.2ffps(total decode: %d frames).\n", (float)frame_nums / total_time, frame_nums);
#endif
    T264dec_close(t);
    fclose(src_file);
    if (f_rec)
        fclose(f_rec);
	if (f_ref)
		fclose(f_ref);
	if(ref_buf != NULL)
		free(ref_buf);
#ifdef USE_DISPLAY
    uninit_display();
#endif

    return 0;
}

void
help()
{
    printf("Usage:\n"
        "\tt264 -d test.264 [rec_path] [reference_path] [skip_num](to decode a 264 file.)\n"
        "\tt264 -e enconfig.txt (to encode a 264 file.)\n");
}

int 
main(int argc, char* argv[])
{
    int32_t i;
    int32_t is_encode = 0;
    int32_t is_decode = 0;
    char* file_name;

    printf("T264 ver: %d.%02d\n", T264_MAJOR, T264_MINOR);
    if (argc < 3)
    {
        help();
        return 0;
    }

    for (i = 1 ; i < 3 ; i ++)
    {
        if (strcmp(argv[i], "-d") == 0)
        {
            is_decode = 1;
        }
        else if(strcmp(argv[i], "-e") == 0)
        {
            is_encode = 1;
        }
        else
        {
            file_name = argv[i];
        }
    }

    if (is_encode)
    {
        return encode(file_name);
    }

    if (is_decode)
    {
        if (argc <= 3)
            rec_path[0] = 0;
        else
		{
			strcpy(rec_path, argv[3]);
		}
		if(argc <= 4)
		{
			ref_path[0] = 0;
		}
		else
		{
			strcpy(ref_path, argv[4]);
		}
		if(argc > 5)
		{
			ref_skip = atoi(argv[5]);
		}
        return decode(file_name);
    }

    help();

    return -1;
}
