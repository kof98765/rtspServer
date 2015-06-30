/************************************************************************
 *
 *  win.h, display routines for Win32 for tmndecode (H.263 decoder)
 *
 ************************************************************************/

/*
 * Disclaimer of Warranty
 *
 * These software programs are available to the user without any
 * license fee or royalty on an "as is" basis. The University of
 * British Columbia disclaims any and all warranties, whether
 * express, implied, or statuary, including any implied warranties
 * or merchantability or of fitness for a particular purpose.  In no
 * event shall the copyright-holder be liable for any incidental,
 * punitive, or consequential damages of any kind whatsoever arising
 * from the use of these programs.
 *
 * This disclaimer of warranty extends to the user of these programs
 * and user's customers, employees, agents, transferees, successors,
 * and assigns.
 *
 * The University of British Columbia does not represent or warrant
 * that the programs furnished hereunder are free of infringement of
 * any third-party patents.
 *
 * Commercial implementations of H.263, including shareware, are
 * subject to royalty fees to patent holders.  Many of these patents
 * are general enough such that they are unavoidable regardless of
 * implementation design.
 *
*/




/* Copyright ?1996 Intel Corporation All Rights Reserved

Permission is granted to use, copy and distribute the software in this
file for any purpose and without fee, provided, that the above
copyright notice and this statement appear in all copies.  Intel makes
no representations about the suitability of this software for any
purpose.  This software is provided "AS IS."

Intel specifically disclaims all warranties, express or implied, and
all liability, including consequential and other indirect damages, for
the use of this software, including liability for infringement of any
proprietary rights, and including the warranties of merchantability
and fitness for a particular purpose.  Intel does not assume any
responsibility for any errors which may appear in this software nor
any responsibility to update it.  */

#include <windows.h>
#include <process.h>
#include <vfw.h>
#include <memory.h>


typedef struct
{
  HANDLE hThread;
  HANDLE hReadyEvt;       //Add for initDisplay()
  HANDLE hReadyQuitEvt;   //Add for closeDisplay()
  HANDLE hEvent;
  HWND hWnd;
  MSG msg;
  WNDCLASS wc;
  HDRAWDIB hDrawDib;
  HDC hDC;
  BITMAPINFOHEADER biHeader;
  char lpszAppName[15];
  DWORD dwThreadID;
  BOOL imageIsReady;
  unsigned char *bufRGB;
  RECT rect;
  unsigned char *src[3];

  int width, height;
  int zoom, oldzoom;
  int windowDismissed;
	
} T_VDWINDOW;


#define VIDEO_BEGIN			    (WM_USER + 0)
#define VIDEO_DRAW_FRAME	  (WM_USER + 1)
#define VIDEO_REDRAW_FRAME	(WM_USER + 2)
#define VIDEO_END			      (WM_USER + 3)


#ifdef __cplusplus
extern "C"
{
#endif


int initDisplay (int pels, int lines);
int displayImage (unsigned char *lum, unsigned char *Cr, unsigned char *Cb);
int closeDisplay ();

void DisplayWinMain (void *);
LONG APIENTRY MainWndProc (HWND, UINT, UINT, LONG);
int DrawDIB ();
void init_dither_tab();
void ConvertYUVtoRGB(
  unsigned char *src0,
  unsigned char *src1,
  unsigned char *src2,
  unsigned char *dst_ori,
  int width,
  int height
);
int InitDisplayWindowThread (int,int);

#pragma comment(lib, "vfw32.lib")

#ifdef __cplusplus
}
#endif
