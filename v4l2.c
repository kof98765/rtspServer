#include "v4l2.h"
#include "huffman.h"


int buffer_length;
int frame_length;
int g_width = 320;
int g_height = 240;
int g_index=0;
int g_capture_count = 1;
int g_rotate = 0;
int g_cap_fmt = V4L2_PIX_FMT_YUYV;
int fd_v4l;
#define BUF_CONUNT 4

typedef enum {

        IO_METHOD_READ,

        IO_METHOD_MMAP,

        IO_METHOD_USERPTR,

} io_method;

static io_method        io              = IO_METHOD_MMAP;// 视频采集方式，内存映射方式
struct framebuffer framebuf[BUF_CONUNT];
struct framebuffer buffers;

int xioctl(int fd, int request, void* argp)
{
        int r;
        do {
            r = ioctl(fd, request, argp);
        }
        while (-1 == r && EINTR == errno);

        return r;
}

/******************************************************************************
Description.: 
Input Value.: 
Return Value: 
******************************************************************************/
int memcpy_picture(unsigned char *out, unsigned char *buf, unsigned int size)
{
    unsigned char *ptdeb, *ptlimit, *ptcur = buf;
    int sizein, pos=0;

    if (!is_huffman(buf)) {
        ptdeb = ptcur = buf;
        ptlimit = buf + size;
        while ((((ptcur[0] << 8) | ptcur[1]) != 0xffc0) && (ptcur < ptlimit))
        ptcur++;
        if (ptcur >= ptlimit)
            return pos;
        sizein = ptcur - ptdeb;

        memcpy(out+pos, buf, sizein); pos += sizein;
        memcpy(out+pos, dht_data, sizeof(dht_data)); pos += sizeof(dht_data);
        memcpy(out+pos, ptcur, size - sizein); pos += size-sizein;
  } else
  {
        memcpy(out+pos, ptcur, size); pos += size;
  }
  return pos;
}

void list_videoInfo(int fd_v4l)
{
    struct v4l2_capability cap;
    xioctl(fd_v4l,VIDIOC_QUERYCAP,&cap);
    printf("Driver Name:%s\nCard Name:%s\nBus info:%s\nDriver Version:%u.%u.%u\n",
        cap.driver,
        cap.card,
        cap.bus_info,
        (cap.version>>16)&0XFF,
        (cap.version>>8)&0XFF,
        cap.version&0XFF);
}
void list_supportFormat(int fd_v4l)
{

    struct v4l2_fmtdesc fmtdesc;
    fmtdesc.index=0;
    fmtdesc.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;

    printf("Support format:\n");
    while(xioctl(fd_v4l, VIDIOC_ENUM_FMT, &fmtdesc) != -1)
    {
        printf("\t%d.%s\n",fmtdesc.index+1,fmtdesc.description);
        fmtdesc.index++;
    }
}
void list_currentDataFormat(int fd_v4l)
{
    struct v4l2_format fmt;
    fmt.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    
    xioctl(fd_v4l, VIDIOC_G_FMT, &fmt);
    printf("Current data format information:\n\twidth:%d\n\theight:%d\n",
    fmt.fmt.pix.width,fmt.fmt.pix.height);
    
    struct v4l2_fmtdesc fmtdesc;
    fmtdesc.index=0; 
    fmtdesc.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    
    while(xioctl(fd_v4l,VIDIOC_ENUM_FMT,&fmtdesc)!=-1)
    {
        if(fmtdesc.pixelformat & fmt.fmt.pix.pixelformat)
        {
            printf("\tformat:%s\n",fmtdesc.description);
            break;
        }
        fmtdesc.index++;
    }
}
int v4l_mmap_init(int fd_v4l)
{
        unsigned int i=0;
        struct v4l2_buffer buf;
        struct v4l2_requestbuffers req;
        memset(&req, 0, sizeof (req));
        req.count = BUF_CONUNT;
        req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        req.memory = V4L2_MEMORY_MMAP;

        if (xioctl(fd_v4l, VIDIOC_REQBUFS, &req) < 0)
        {
                perror("v4l_capture_setup: VIDIOC_REQBUFS failed\n");
                return -1;
        }
        for(i=0;i<BUF_CONUNT;i++)
        {
            memset(&buf, 0, sizeof (buf));
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            buf.index = i;

            if (xioctl(fd_v4l, VIDIOC_QUERYBUF, &buf) < 0)
            {
                printf("%d",i);
                perror("VIDIOC_QUERYBUF\n"); 
                return -1;
            }
            framebuf[i].length = buf.length;
            framebuf[i].offset = (size_t) buf.m.offset;
            framebuf[i].start = (unsigned char*)mmap (NULL, buf.length,PROT_READ | PROT_WRITE, MAP_SHARED, fd_v4l, buf.m.offset);

            printf("buffers.length = %d,buffers.offset = %d ,buffers.start[0] = %x\n",framebuf[i].length,framebuf[i].offset,(unsigned int)framebuf[i].start);

        }

    buffers.start=(unsigned char *)malloc(buf.length);


}
static void v4l_read_init (int fd)//初始化读取
{
        framebuf[0].length = frame_length;
        framebuf[0].start = malloc (frame_length);

        if (!framebuf[0].start) {
                fprintf (stderr, "Out of memory\n");
        }

}

int v4l_userp_init (int fd)
{
        int i=0;
        unsigned int page_size;
        page_size = getpagesize ();
        frame_length= (frame_length+ page_size - 1) & ~(page_size - 1);
        struct v4l2_requestbuffers req;

        memset(&req, 0, sizeof (req));
        req.count = BUF_CONUNT;
        req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        req.memory = V4L2_MEMORY_USERPTR;

        if (xioctl(fd, VIDIOC_REQBUFS, &req) < 0)
        {
                perror("v4l_capture_setup: VIDIOC_REQBUFS failed\n");
                return -1;
        }

        for (i = 0; i < BUF_CONUNT; i++) {

                framebuf[i].length = frame_length;
                /* boundary */ 
                framebuf[i].start = (unsigned char *)memalign (page_size, frame_length);
 
                if (!framebuf[i].start) {
                        fprintf (stderr, "Out of memory\n");
                       return -1;
                }
        }
}


int v4l_capture_stop(int fd)
{
        enum v4l2_buf_type type;
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (xioctl (fd, VIDIOC_STREAMOFF, &type) < 0) 
        {
                perror("VIDIOC_STREAMOFF error\n");
                return -1;
        }
}
 int v4l_capture_init(char *dev)
{    
        struct v4l2_format fmt;
        struct v4l2_control ctrl;
        struct v4l2_crop crop;
        struct v4l2_capability cap;
        struct v4l2_cropcap cropcap;
        int fd_v4l = 0;
        unsigned int min;
        
        if ((fd_v4l = open(dev, O_RDWR/*|O_NONBLOCK*/,0)) < 0)
        {
                fprintf(stderr,"Unable to open %s\n", dev);
                return 0;
        }

        if (-1 == ioctl (fd_v4l, VIDIOC_QUERYCAP, &cap)) {

                if (EINVAL == errno) {
                       fprintf (stderr, "%s is no V4L2 device\n",dev);
                } else 
                       perror ("VIDIOC_QUERYCAP");
         }

     if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
        fprintf (stderr, "%s is no video capture device\n", dev);
        

     switch (io) {

        case IO_METHOD_READ:

                if (!(cap.capabilities & V4L2_CAP_READWRITE)) {

                        fprintf (stderr, "%s does not support read i/o\n",dev);
                        exit (EXIT_FAILURE);
                }
                break;

        case IO_METHOD_MMAP:
        case IO_METHOD_USERPTR:

                if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
                        fprintf (stderr, "%s does not support streaming i/o\n",dev);
                        exit (EXIT_FAILURE);
                }
                break;

        }


        CLEAR (cropcap);

        cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (0 == ioctl (fd_v4l, VIDIOC_CROPCAP, &cropcap)) {

                crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

                crop.c = cropcap.defrect; /* reset to default */

                if (-1 == ioctl (fd_v4l, VIDIOC_S_CROP, &crop)) {
                     switch (errno) { 

                         case EINVAL:
                                /* Cropping not supported. */
                                perror("Cropping not supported!!\n");
                                break;
                         default:
                                /* Errors ignored. */
                                break;
                    }
                }
        } 

        //fmt.fmt.pix.pixelformat=V4L2_PIX_FMT_YUYV;

        list_videoInfo(fd_v4l);

        list_supportFormat(fd_v4l);

        list_currentDataFormat(fd_v4l);

        memset(&fmt,0,sizeof(fmt ));

        xioctl(fd_v4l, VIDIOC_S_FMT, &fmt);

        fmt.fmt.pix.width=g_width;
        fmt.fmt.pix.height=g_height;
        fmt.fmt.pix.priv = 1;
        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        fmt.fmt.pix.pixelformat = g_cap_fmt;
        fmt.fmt.pix.field = V4L2_FIELD_ANY;

        frame_length=fmt.fmt.pix.width*fmt.fmt.pix.height*3;
        if (xioctl(fd_v4l, VIDIOC_S_FMT, &fmt) < 0)
        {
                printf("set format failed\n");
                perror("not a jpeg format \n");
                fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
        } 
        if (xioctl(fd_v4l, VIDIOC_S_FMT, &fmt) < 0)
        {
                printf("set format failed\n");
                perror("not a jpeg format \n");
                fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB565;
        } 
        if (xioctl(fd_v4l, VIDIOC_S_FMT, &fmt) < 0)
        {
                printf("set format failed\n");
                perror("not a RGB565 format \n");
                perror("set format failed\n");
                return 0;
        } 


        ctrl.id = V4L2_CID_HUE;
        ctrl.id = V4L2_CID_BRIGHTNESS ;
        ctrl.value = g_rotate;
        
        if (ioctl(fd_v4l, VIDIOC_S_CTRL, &ctrl) < 0)
                perror("set ctrl failed\n");
                

    switch (io) {

        case IO_METHOD_READ:

                v4l_read_init(fd_v4l);
                break;

        case IO_METHOD_MMAP:

                v4l_mmap_init(fd_v4l);
                break;

        case IO_METHOD_USERPTR:
            
                v4l_userp_init(fd_v4l);
                break;

        }

        return fd_v4l;
}

 
int v4l_capture_start(int fd) //开始采集

{

        unsigned int i;
        enum v4l2_buf_type type;

        switch (io) {

        case IO_METHOD_READ:

                /* Nothing to do. */

                break;

        case IO_METHOD_MMAP:

                for (i = 0; i < BUF_CONUNT; i++) {

                        struct v4l2_buffer buf;
                        memset(&buf, 0, sizeof (buf));

                        buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                        buf.memory      = V4L2_MEMORY_MMAP;
                        buf.index       = i;

                        if (-1 == ioctl (fd, VIDIOC_QBUF, &buf))
                                perror ("VIDIOC_QBUF");

                   }

                type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

                if (-1 == ioctl (fd, VIDIOC_STREAMON, &type))
                {
                        perror ("VIDIOC_STREAMON");
                        return -1;
                }

                break;
                
        case IO_METHOD_USERPTR:

                for (i = 0; i < BUF_CONUNT; i++) {

                        struct v4l2_buffer buf;
                        memset(&buf, 0, sizeof (buf));

                        buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                        buf.memory      = V4L2_MEMORY_USERPTR;
                        buf.index       = i;
                        buf.m.userptr   = (unsigned long) framebuf[i].start;
                        buf.length      = framebuf[i].length;

                        if (-1 == ioctl (fd, VIDIOC_QBUF, &buf))
                                perror ("VIDIOC_QBUF");

                }

                type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                if (-1 == ioctl (fd, VIDIOC_STREAMON, &type))
                    perror("STREAMON");
                    
                break;

        }

}

struct framebuffer * v4l_get_frame(int fd)// 对图像帧进行读取
{


        struct v4l2_buffer buf;//这是视频接口V4L2中相关文件定义的，在videodev2.h中
        unsigned int i;
        int ret;

        switch (io){      

        case IO_METHOD_READ:
                ret= read (fd, framebuf[0].start, framebuf[0].length);
                if (ret==-1) {

                        switch (errno) {
                            case EAGAIN:
                                    return 0;
                             case EIO:
                                /* Could ignore EIO, see spec. */
                                /* fall through */
                            default:
                                perror ("read");
                        }
                }
                g_index=0;
                buffers.length=ret;
                memcpy(buffers.start, framebuf[0].start, framebuf[0].length);
                break;

        case IO_METHOD_MMAP://采用内存映射的方式

               memset(&buf, 0, sizeof (buf));//对缓冲区清零

                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

                buf.memory = V4L2_MEMORY_MMAP;
                
                if (xioctl (fd, VIDIOC_DQBUF, &buf)<0) {//VIDIOC_DQBUF控制参数作用是申请视频缓冲区
                              perror ("VIDIOC_DQBUF");
                }
                
                buffers.length=buf.bytesused;
                memcpy_picture(buffers.start,framebuf[buf.index].start,buffers.length);

                //memcpy(buffers.start,ptcur,buffers.length);
                printf("QBUF\n");
             if (-1 == ioctl (fd, VIDIOC_QBUF, &buf))
                        perror ("VIDIOC_QBUF");
             
             break;

        case IO_METHOD_USERPTR:

                memset(&buf, 0, sizeof (buf));
                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

                buf.memory = V4L2_MEMORY_USERPTR;

                if (-1 == ioctl (fd, VIDIOC_DQBUF, &buf)) {

                        switch (errno) {

                        case EAGAIN:
                                return 0;
                        case EIO:
                                /* Could ignore EIO, see spec. */
                                /* fall through */
                        default:
                                perror ("VIDIOC_DQBUF");
                        }

                }
                for (i = 0; i < BUF_CONUNT; i++)
                        if (buf.m.userptr == (unsigned long) framebuf[i].start
                            && buf.length == framebuf[i].length)
                                break;


                g_index=i;
                buffers.length=framebuf[i].length;
                memcpy(buffers.start,framebuf[i].start,buffers.length);
                if (-1 == ioctl (fd, VIDIOC_QBUF, &buf))
                        perror ("VIDIOC_QBUF");

                break;

        }

        return &buffers;

}



int v4l_capture_uninit(int fd)
{
  unsigned int i;

          for (i = 0; i < BUF_CONUNT; i++)
          {
              if (-1 == munmap (framebuf[i].start, framebuf[i].length))
                perror("munmap");
          }

  close(fd);
}
