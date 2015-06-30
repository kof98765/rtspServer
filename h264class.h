#ifndef H264CLASS_H
#define H264CLASS_H
#include <stdint.h>
extern "C"
{
#include "convert.h"
#include "x264.h"

}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#define CLEAR(x) (memset((&x),0,sizeof(x)))
class h264Class
{
public:
    h264Class();
    ~h264Class();
     void initEncode(char *);
     void init_param(x264_param_t* param, const char* file);
     int encode(unsigned char *cam_yuv,unsigned char *dst,int i);
    int getFrameNum();
private:
    x264_t* m_t264;
    x264_param_t m_param;
    x264_picture_t m_pic;
    x264_nal_t * nal;
    char* m_pSrc;
    char* m_pDst;
    int m_lDstSize;
    char* m_pPoolData;
};

#endif // H264CLASS_H
