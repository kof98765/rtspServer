




/************************************************************************
 *
 *  win.c, display routines for Win32 for tmndecode (H.263 decoder)
 *
 ************************************************************************/

/* Disclaimer of Warranty
 * 
 * These software programs are available to the user without any license fee
 * or royalty on an "as is" basis. The University of British Columbia
 * disclaims any and all warranties, whether express, implied, or
 * statuary, including any implied warranties or merchantability or of
 * fitness for a particular purpose.  In no event shall the
 * copyright-holder be liable for any incidental, punitive, or
 * consequential damages of any kind whatsoever arising from the use of
 * these programs.
 * 
 * This disclaimer of warranty extends to the user of these programs and
 * user's customers, employees, agents, transferees, successors, and
 * assigns.
 * 
 * The University of British Columbia does not represent or warrant that the
 * programs furnished hereunder are free of infringement of any
 * third-party patents.
 * 
 * Commercial implementations of H.263, including shareware, are subject to
 * royalty fees to patent holders.  Many of these patents are general
 * enough such that they are unavoidable regardless of implementation
 * design.
 * 
 */



/* Copyright ¨ 1996 Intel Corporation All Rights Reserved
 * 
 * Permission is granted to use, copy and distribute the software in this
 * file for any purpose and without fee, provided, that the above
 * copyright notice and this statement appear in all copies.  Intel makes
 * no representations about the suitability of this software for any
 * purpose.  This software is provided "AS IS."
 * 
 * Intel specifically disclaims all warranties, express or implied, and all
 * liability, including consequential and other indirect damages, for the
 * use of this software, including liability for infringement of any
 * proprietary rights, and including the warranties of merchantability and
 * fitness for a particular purpose.  Intel does not assume any
 * responsibility for any errors which may appear in this software nor any
 * responsibility to update it.  */

/************************************************************************
 * These routines were written by Intel Architecture Labs (IAL)
 *
 * v1 : Michael Third
 * v2 : Karl (removed unnecessary code, collected routines in one file)
 *           (moved and changed parts of message loop)
 *           (changed YUVtoRGB routine)
 *           (added synchronization with events
 *           instead of empty while loop)
 ************************************************************************/

//#include "config.h"
//#include "tmndec.h"
//#include "global.h"
#include "win.h"

 

/* vdinit.c */
unsigned char *clp = NULL;
unsigned char *clp1 = NULL;

#ifdef __cplusplus
extern "C"
{
#endif


T_VDWINDOW vdWindow;



int initDisplay (int pels, int lines)
{
  int errFlag = 0;
  DWORD dwRetVal;

  init_dither_tab ();
  errFlag |= InitDisplayWindowThread (pels, lines);
  dwRetVal = WaitForSingleObject (vdWindow.hReadyEvt, INFINITE);
  return errFlag;
}


int InitDisplayWindowThread (int width, int height)
{
  int errFlag = 0;

  /* now modify the couple that need it */
  vdWindow.width = width;
  vdWindow.height = height;
  vdWindow.biHeader.biWidth = vdWindow.width;
  vdWindow.biHeader.biHeight = vdWindow.height;
  vdWindow.biHeader.biSize = sizeof (BITMAPINFOHEADER);
  vdWindow.biHeader.biCompression = BI_RGB;
  vdWindow.biHeader.biPlanes = 1;
  vdWindow.biHeader.biBitCount = 24;


  vdWindow.biHeader.biSizeImage = 3 * vdWindow.width * vdWindow.height;
  vdWindow.imageIsReady = FALSE;

  /* allocate the memory needed to hold the RGB and visualization
   * information */
  vdWindow.bufRGB = NULL;

  vdWindow.bufRGB = (unsigned char *)malloc(sizeof(unsigned char)
    *(3 * vdWindow.width * vdWindow.height));

  /* Create synchronization event */
  vdWindow.hEvent = CreateEvent (NULL, FALSE, FALSE, NULL);
  vdWindow.hReadyEvt = CreateEvent (NULL, FALSE, FALSE, NULL);
  vdWindow.hReadyQuitEvt = CreateEvent (NULL, FALSE, FALSE, NULL);
  
  vdWindow.hThread =
    CreateThread (
                  NULL,
                  0,
                  (LPTHREAD_START_ROUTINE) DisplayWinMain,
                  (LPVOID) NULL,
                  0,
                  &(vdWindow.dwThreadID)
    );

  if (vdWindow.hThread == NULL)
  {
    errFlag = 1;
    return errFlag;
  }
  return errFlag;
}


/* vddraw.c */

int displayImage (unsigned char *lum, unsigned char *Cr, unsigned char *Cb)
{
  int errFlag = 0;
  DWORD dwRetVal;


  vdWindow.src[0] = lum;
  vdWindow.src[1] = Cb;
  vdWindow.src[2] = Cr;

  /* wait until we have finished drawing the last frame */
  if (vdWindow.windowDismissed == FALSE)
  {
    vdWindow.imageIsReady = TRUE;
    /* Post message to drawing thread's window to draw frame */
    PostMessage (vdWindow.hWnd, VIDEO_DRAW_FRAME, (WPARAM) NULL, (LPARAM) NULL);

    /* wait until the frame has been drawn */
    dwRetVal = WaitForSingleObject (vdWindow.hEvent, INFINITE);
    free(vdWindow.src[0]);
    free(vdWindow.src[1]);
    free(vdWindow.src[2]);
  }
  else
  {
    free(vdWindow.src[0]);
    free(vdWindow.src[1]);
    free(vdWindow.src[2]);
  }
  return errFlag;
}


int DrawDIB ()
{
  int errFlag = 0;

  errFlag |=
    DrawDibDraw (
                 vdWindow.hDrawDib,
                 vdWindow.hDC,
                 0,
                 0,
                 vdWindow.zoom * vdWindow.width,
                 vdWindow.zoom * vdWindow.height,
                 &vdWindow.biHeader,
                 vdWindow.bufRGB,
                 0,
                 0,
                 vdWindow.width,
                 vdWindow.height,
                 DDF_SAME_DRAW
    );


  return errFlag;
}




/* vdwinman.c */

void DisplayWinMain (void *dummy)
{
  int errFlag = 0;
  DWORD dwStyle;

  vdWindow.wc.style = CS_BYTEALIGNWINDOW;
  vdWindow.wc.lpfnWndProc = MainWndProc;
  vdWindow.wc.cbClsExtra = 0;
  vdWindow.wc.cbWndExtra = 0;
  vdWindow.wc.hInstance = 0;
  vdWindow.wc.hIcon = LoadIcon (NULL, IDI_APPLICATION);
  vdWindow.wc.hCursor = LoadCursor (NULL, IDC_ARROW);
  vdWindow.wc.hbrBackground = (HBRUSH)(GetStockObject (WHITE_BRUSH));
  vdWindow.wc.lpszMenuName = NULL;
  vdWindow.zoom = 1;
  strcpy (vdWindow.lpszAppName, "H.264 Display");
  vdWindow.wc.lpszClassName = vdWindow.lpszAppName;

  RegisterClass (&vdWindow.wc);


  dwStyle = WS_DLGFRAME | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;

  vdWindow.hWnd =
    CreateWindow (vdWindow.lpszAppName,
                  vdWindow.lpszAppName,
                  dwStyle,
                  CW_USEDEFAULT,
                  CW_USEDEFAULT,
                  vdWindow.width + 6,
                  vdWindow.height + 25,
                  NULL,
                  NULL,
                  0,
                  NULL
    );

  if (vdWindow.hWnd == NULL)
    ExitThread (errFlag = 1);

  ShowWindow (vdWindow.hWnd, SW_SHOWNOACTIVATE);
  UpdateWindow (vdWindow.hWnd);

  /* Message loop for display window's thread */
  while (GetMessage (&(vdWindow.msg), NULL, 0, 0))
  {
    TranslateMessage (&(vdWindow.msg));
    DispatchMessage (&(vdWindow.msg));
  }

  ExitThread (0);
}


LRESULT APIENTRY MainWndProc (HWND hWnd, UINT msg, UINT wParam, LONG lParam)
{
  LPMINMAXINFO lpmmi;

  switch (msg)
  {
    case VIDEO_BEGIN:
      vdWindow.hDC = GetDC (vdWindow.hWnd);
      vdWindow.hDrawDib = DrawDibOpen ();
      vdWindow.zoom = 1;
      vdWindow.oldzoom = 0;
      DrawDibBegin (
                    vdWindow.hDrawDib,
                    vdWindow.hDC,
                    2 * vdWindow.width,
                    2 * vdWindow.height,
                    &vdWindow.biHeader,
                    vdWindow.width,
                    vdWindow.height,
                    0
        );
      SetEvent (vdWindow.hReadyEvt);
      vdWindow.windowDismissed = FALSE;
      ReleaseDC (vdWindow.hWnd, vdWindow.hDC);
      break;
    case VIDEO_DRAW_FRAME:
      vdWindow.hDC = GetDC (vdWindow.hWnd);
      ConvertYUVtoRGB (
                       vdWindow.src[0],
                       vdWindow.src[1],
                       vdWindow.src[2],
                       vdWindow.bufRGB,
                       vdWindow.width,
                       vdWindow.height
        );
      /* draw the picture onto the screen */
      DrawDIB ();
      SetEvent (vdWindow.hEvent);
      ReleaseDC (vdWindow.hWnd, vdWindow.hDC);
      break;
    case VIDEO_END:
      /* Window has been closed.  The following lines handle the cleanup. */
      vdWindow.hDC = GetDC (vdWindow.hWnd);
      DrawDibEnd (vdWindow.hDrawDib);
      DrawDibClose (vdWindow.hDrawDib);
      ReleaseDC (vdWindow.hWnd, vdWindow.hDC);
      SetEvent (vdWindow.hReadyQuitEvt);
      vdWindow.windowDismissed = TRUE;
      PostQuitMessage (0);
      break;

    case WM_CREATE:
      PostMessage (hWnd, VIDEO_BEGIN, 0, 0);
      break;
    case WM_SIZE:
      switch (wParam)
      {
        case SIZE_MAXIMIZED:
          vdWindow.zoom = 2;
          break;
        case SIZE_MINIMIZED:
          vdWindow.oldzoom = vdWindow.zoom;
          break;
        case SIZE_RESTORED:
          if (vdWindow.oldzoom)
          {
            vdWindow.zoom = vdWindow.oldzoom;
            vdWindow.oldzoom = 0;
          } else
            vdWindow.zoom = 1;
          break;
        case SIZE_MAXHIDE:
          break;
        case SIZE_MAXSHOW:
          break;
      }
      PostMessage (hWnd, WM_PAINT, 0, 0);
      break;
    case WM_GETMINMAXINFO:
      lpmmi = (LPMINMAXINFO) lParam;

      GetWindowRect (hWnd, &vdWindow.rect);
      lpmmi->ptMaxPosition.x = vdWindow.rect.left;
      lpmmi->ptMaxPosition.y = vdWindow.rect.top;

      lpmmi->ptMaxSize.x = 2 * (vdWindow.width) + 6;
      lpmmi->ptMaxSize.y = 2 * (vdWindow.height) + 25;
      break;
    case WM_DESTROY:
      /* Window has been closed.  The following lines handle the cleanup. */
      DrawDibEnd (vdWindow.hDrawDib);
      ReleaseDC (vdWindow.hWnd, vdWindow.hDC);
      DrawDibClose (vdWindow.hDrawDib);

      vdWindow.windowDismissed = TRUE;
      PostQuitMessage (0);
      break;
    case WM_PAINT:
      if (vdWindow.imageIsReady)
      {
        vdWindow.hDC = GetDC (vdWindow.hWnd);
        DrawDIB ();
        ReleaseDC (vdWindow.hWnd, vdWindow.hDC);
      }
      break;

  }
  return DefWindowProc (hWnd, msg, wParam, lParam);
}



/* vdclose.c */

int closeDisplay ()
{
  int errFlag = 0;
  DWORD dwRetVal;
  if (vdWindow.hWnd)
  {
    PostMessage (vdWindow.hWnd, VIDEO_END, (WPARAM) NULL, (LPARAM) NULL);
    dwRetVal = WaitForSingleObject (vdWindow.hReadyQuitEvt, INFINITE);
  }
  if (vdWindow.hEvent)
    CloseHandle (vdWindow.hEvent);
  if (vdWindow.hReadyEvt)
    CloseHandle(vdWindow.hReadyEvt);
  if (vdWindow.hReadyEvt)
    CloseHandle(vdWindow.hReadyQuitEvt);
  if (vdWindow.hThread)
    CloseHandle (vdWindow.hThread);
  
  
  if(vdWindow.bufRGB != NULL)
    free(vdWindow.bufRGB);
  
  if(clp1 != NULL)
    free(clp1);
  return errFlag;
}



#ifdef __cplusplus
}
#endif