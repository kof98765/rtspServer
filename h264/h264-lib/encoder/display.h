#ifndef __Linux_Display__
#define __Linux_Display__

#ifndef GLOBAL
#define EXTERN extern
#else
#define EXTERN
#endif

#ifdef NON_ANSI_COMPILER
#define _ANSI_ARGS_(x) ()
#else
#define _ANSI_ARGS_(x) x
#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif

/* Some macros */  
#define mmax(a, b)        ((a) > (b) ? (a) : (b))
#define mmin(a, b)        ((a) < (b) ? (a) : (b))
#define mnint(a)        ((a) < 0 ? (int)(a - 0.5) : (int)(a + 0.5))
#define sign(a)         ((a) < 0 ? -1 : 1)
                     
//#define USE_DISPLAY
#ifdef USE_DISPLAY
/* display.c */
void init_display _ANSI_ARGS_ ((char *name, int width, int height));
void exit_display _ANSI_ARGS_ ((void));
void dither _ANSI_ARGS_ ((unsigned char *src[]));
void init_dither _ANSI_ARGS_ ((void));

/* dither.c */
void ord4x4_dither_init _ANSI_ARGS_ ((void));
void ord4x4_dither_frame _ANSI_ARGS_ ((unsigned char *[], unsigned char *));

/* yuv2rgb.c */
void Color16DitherImage _ANSI_ARGS_ ((unsigned char *[], unsigned char *));
void Color32DitherImage _ANSI_ARGS_ ((unsigned char *[], unsigned char *));
void InitColorDither _ANSI_ARGS_ ((int));
void init_dither_tab _ANSI_ARGS_ ((void));

/* yuvrgb24.c */
void ConvertYUVtoRGB (
                       unsigned char *src0,
                       unsigned char *src1,
                       unsigned char *src2,
                       unsigned char *dst_ori,
                       int width,
                       int height
);
#endif

#endif

