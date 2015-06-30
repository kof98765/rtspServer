#include "h264class.h"

h264Class::h264Class()
{
    m_pDst=0;
    m_pPoolData=0;


}
h264Class::~h264Class()
{
    delete m_pDst;
    delete m_pPoolData;
    x264_picture_clean(&m_pic);
}
int h264Class::encode(unsigned char *cam_yuv,unsigned char *dst,int i)
{

    m_pic.i_type = X264_TYPE_AUTO;
    m_pic.i_qpplus1=0;
    m_pic.img.i_csp = X264_CSP_I420;

    m_pic.img.plane[0]=cam_yuv;
    m_pic.img.plane[1]=cam_yuv+m_param.i_width*m_param.i_height;
    m_pic.img.plane[2]=cam_yuv+m_param.i_width*m_param.i_height/4+m_param.i_width*m_param.i_height;
    int n_nal;
    int ret;
    x264_picture_t pic_out;
    x264_nal_t *my_nal;
    CLEAR(nal);
    m_pic.i_pts=i;

    if((ret=x264_encoder_encode(m_t264,&nal,&n_nal,&m_pic,&pic_out))<0)
    {
        std::cout<<"encode error"<<ret;
        exit(EXIT_FAILURE);
    }

    unsigned int length=0;

    for(my_nal=nal;my_nal<nal+n_nal;++my_nal){
        //  write(fd_write,my_nal->p_payload,my_nal->i_payload);
        memcpy(dst,my_nal->p_payload,my_nal->i_payload);
        dst+=my_nal->i_payload;
        length+=my_nal->i_payload;
    }



    return length;
}

int h264Class::getFrameNum()
{
    return 0;
}
void h264Class::initEncode(char *paramfile)
{

    //* 配置参数
    //* 使用默认参数，在这里因为我的是实时网络传输，所以我使用了zerolatency的选项，使用这个选项之后就不会有delayed_frames，如果你使用的不是这样的话，还需要在编码完成之后得到缓存的编码帧
    // x264_param_default(&m_param);
    x264_param_default_preset(&m_param, "veryfast", "zerolatency");
    m_param.i_width = 640;
    m_param.i_height = 480;

    m_param.i_fps_num = 4;
    m_param.i_fps_den = 1;

    m_param.rc.i_bitrate = 96;
    m_param.rc.i_rc_method = X264_RC_ABR;

    //m_param.i_frame_reference = 4; /* 参考帧的最大帧数 */

    m_param.b_annexb = true;

    m_param.i_keyint_max = 2;

    //* 编码需要的辅助变量
    nal=(x264_nal_t *)malloc(sizeof(x264_nal_t ));
    x264_picture_alloc(&m_pic, X264_CSP_I420, m_param.i_width, m_param.i_height );
    m_t264 = x264_encoder_open(&m_param);


    //	m_pSrc = (uint8_t*)T264_malloc(m_lDstSize, CACHE_SIZE);
    // if(m_pDst==NULL)
    //      m_pDst = (char *)malloc(m_lDstSize);
    //  if(m_pPoolData==NULL)
    //     m_pPoolData = (char *)malloc(m_param.width*m_param.height*3/2);
}
void h264Class::init_param(x264_param_t* param, const char* file)
{
    /*
    int total_no;
    FILE* fd;
    char line[255];
    int32_t b;
    if (!(fd = fopen(file,"r")))
    {
        std::cout<<"Couldn't open parameter file %s.\n"<<file;
        exit(-1);
    }

    memset(param, 0, sizeof(*param));
    fgets(line, 254, fd); sscanf(line,"%d", &b);
    if (b != 4)
    {
        std::cout<<"wrong param file version, expect v4.0\n";
        exit(-1);
    }
    fgets(line, 254, fd); sscanf(line,"%d", &param->i_width);
    fgets(line, 254, fd); sscanf(line,"%d", &param->i_height);
    fgets(line, 254, fd); sscanf(line,"%d", &total_no);
    fgets(line, 254, fd); sscanf(line,"%d", &param->i_bframe);
    fgets(line, 254, fd); sscanf(line,"%d", &param->);
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
    param->flags |= (USE_SAD) * b;
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

    // 	fgets(line, 254, fd); sscanf(line,"%s", src_path);
    // 	fgets(line, 254, fd); sscanf(line,"%s", out_path);
    // 	fgets(line, 254, fd); sscanf(line,"%s", rec_path);
    // 	param->rec_name = rec_path;

    fclose(fd);
    */
}
