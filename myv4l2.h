#ifndef MYV4L2_H
#define MYV4L2_H

 int v4l_capture_setup(void);
 int start_capturing(int fd_v4l);
 int v4l_capture_test(int fd_v4l, const char * file);
 int get_frame(int fd_v4l);
 #endif