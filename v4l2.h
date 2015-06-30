#ifndef V4L2_H
#define V4L2_H
#include <assert.h>
#include <linux/videodev2.h>
#include <string.h>

#include <sys/stat.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <asm/types.h>
#include <sys/mman.h>

struct framebuffer
{
        unsigned char *start;
        size_t offset;
        unsigned int length;
};
#define CLEAR(x) memset(&(x), 0, sizeof(x))
void list_videoInfo(int fd_v4l);
void list_supportFormat(int fd_v4l);
void list_currentDataFormat(int fd_v4l);
int v4l_mmap_init(int fd_v4l);
int v4l_capture_init(char *dev);
int v4l_capture_uninit(int fd);
struct framebuffer *v4l_get_frame(int fd_v4l);
int v4l_capture_stop(int fd);
int v4l_capture_start(int fd);
int is_huffman(unsigned char *buf);

#endif
