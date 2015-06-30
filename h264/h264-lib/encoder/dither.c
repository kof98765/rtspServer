/************************************************************************
 *
 *  dither.c, pseudo colour dithering for tmndecode (H.263 decoder)
 *  Copyright (C) 1995, 1996  Telenor R&D, Norway
 *
 *  Contacts:
 *  Robert Danielsen                  <Robert.Danielsen@nta.no>
 *
 *  Telenor Research and Development  http://www.nta.no/brukere/DVC/
 *  P.O.Box 83                        tel.:   +47 63 84 84 00
 *  N-2007 Kjeller, Norway            fax.:   +47 63 81 00 76
 *
 *  Copyright (C) 1997  University of BC, Canada
 *  Modified by: Michael Gallant <mikeg@ee.ubc.ca>
 *               Guy Cote <guyc@ee.ubc.ca>
 *               Berna Erol <bernae@ee.ubc.ca>
 *
 *  Contacts:
 *  Michael Gallant                   <mikeg@ee.ubc.ca>
 *
 *  UBC Image Processing Laboratory   http://www.ee.ubc.ca/image
 *  2356 Main Mall                    tel.: +1 604 822 4051
 *  Vancouver BC Canada V6T1Z4        fax.: +1 604 822 5949
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



/* based on mpeg2decode, (C) 1994, MPEG Software Simulation Group and
 * mpeg2play, (C) 1994 Stefan Eckart <stefan@lis.e-technik.tu-muenchen.de>
 * 
 */


#include <stdio.h>
#include <stdlib.h>
//#include <X11/Xlib.h>
//#include <X11/Xutil.h>

#include "display.h"

#ifdef USE_DISPLAY
extern unsigned char pixel[256];
EXTERN int horizontal_size, vertical_size, mb_width, mb_height;
EXTERN int coded_picture_width, coded_picture_height;
EXTERN int ref_coded_picture_width, ref_coded_picture_height;
EXTERN int chrom_width, chrom_height, blk_cnt;
EXTERN int ref_chrom_width, ref_chrom_height;
extern int expand;

static unsigned char ytab[16 * (256 + 16)];
static unsigned char uvtab[256 * 269 + 270];

/****************************************************************************

  4x4 ordered dither

  Threshold pattern:

     0  8  2 10
    12  4 14  6
     3 11  1  9
    15  7 13  5

 ****************************************************************************/
void
 ord4x4_dither_init (void)
{
  int i, j, v;
  unsigned char ctab[256 + 32];

  for (i = 0; i < 256 + 16; i++)
  {
    v = (i - 8) >> 4;
    if (v < 2)
      v = 2;
    else if (v > 14)
      v = 14;
    for (j = 0; j < 16; j++)
      ytab[16 * i + j] = pixel[(v << 4) + j];
  }

  for (i = 0; i < 256 + 32; i++)
  {
    v = (i + 48 - 128) >> 5;
    if (v < 0)
      v = 0;
    else if (v > 3)
      v = 3;
    ctab[i] = v;
  }

  for (i = 0; i < 255 + 15; i++)
    for (j = 0; j < 255 + 15; j++)
      uvtab[256 * i + j] = (ctab[i + 16] << 6) | (ctab[j + 16] << 4) | (ctab[i] << 2) | ctab[j];
}



void
 ord4x4_dither_frame (unsigned char *src[], unsigned char *dst)
{
  int i, j;
  unsigned char *py = src[0];
  unsigned char *pu = src[1];
  unsigned char *pv = src[2];

  int width, height, cwidth;

  if (expand)
  {
    width = 2 * ref_coded_picture_width;
    height = 2 * ref_coded_picture_height;
    cwidth = 2 * ref_chrom_width;
  } else
  {
    width = ref_coded_picture_width;
    height = ref_coded_picture_height;
    cwidth = ref_chrom_width;
  }

  for (j = 0; j < height; j += 4)
  {
    register unsigned int uv;

    /* line j + 0 */
    for (i = 0; i < width; i += 8)
    {
      uv = uvtab[(*pu++ << 8) | *pv++];
      *dst++ = ytab[((*py++) << 4) | (uv & 15)];
      *dst++ = ytab[((*py++ + 8) << 4) | (uv >> 4)];
      uv = uvtab[((*pu++ << 8) | *pv++) + 1028];
      *dst++ = ytab[((*py++ + 2) << 4) | (uv & 15)];
      *dst++ = ytab[((*py++ + 10) << 4) | (uv >> 4)];
      uv = uvtab[(*pu++ << 8) | *pv++];
      *dst++ = ytab[((*py++) << 4) | (uv & 15)];
      *dst++ = ytab[((*py++ + 8) << 4) | (uv >> 4)];
      uv = uvtab[((*pu++ << 8) | *pv++) + 1028];
      *dst++ = ytab[((*py++ + 2) << 4) | (uv & 15)];
      *dst++ = ytab[((*py++ + 10) << 4) | (uv >> 4)];
    }

    pu -= cwidth;
    pv -= cwidth;

    /* line j + 1 */
    for (i = 0; i < width; i += 8)
    {
      uv = uvtab[((*pu++ << 8) | *pv++) + 2056];
      *dst++ = ytab[((*py++ + 12) << 4) | (uv >> 4)];
      *dst++ = ytab[((*py++ + 4) << 4) | (uv & 15)];
      uv = uvtab[((*pu++ << 8) | *pv++) + 3084];
      *dst++ = ytab[((*py++ + 14) << 4) | (uv >> 4)];
      *dst++ = ytab[((*py++ + 6) << 4) | (uv & 15)];
      uv = uvtab[((*pu++ << 8) | *pv++) + 2056];
      *dst++ = ytab[((*py++ + 12) << 4) | (uv >> 4)];
      *dst++ = ytab[((*py++ + 4) << 4) | (uv & 15)];
      uv = uvtab[((*pu++ << 8) | *pv++) + 3084];
      *dst++ = ytab[((*py++ + 14) << 4) | (uv >> 4)];
      *dst++ = ytab[((*py++ + 6) << 4) | (uv & 15)];
    }

    /* line j + 2 */
    for (i = 0; i < width; i += 8)
    {
      uv = uvtab[((*pu++ << 8) | *pv++) + 1542];
      *dst++ = ytab[((*py++ + 3) << 4) | (uv & 15)];
      *dst++ = ytab[((*py++ + 11) << 4) | (uv >> 4)];
      uv = uvtab[((*pu++ << 8) | *pv++) + 514];
      *dst++ = ytab[((*py++ + 1) << 4) | (uv & 15)];
      *dst++ = ytab[((*py++ + 9) << 4) | (uv >> 4)];
      uv = uvtab[((*pu++ << 8) | *pv++) + 1542];
      *dst++ = ytab[((*py++ + 3) << 4) | (uv & 15)];
      *dst++ = ytab[((*py++ + 11) << 4) | (uv >> 4)];
      uv = uvtab[((*pu++ << 8) | *pv++) + 514];
      *dst++ = ytab[((*py++ + 1) << 4) | (uv & 15)];
      *dst++ = ytab[((*py++ + 9) << 4) | (uv >> 4)];
    }

    pu -= cwidth;
    pv -= cwidth;

    /* line j + 3 */
    for (i = 0; i < width; i += 8)
    {
      uv = uvtab[((*pu++ << 8) | *pv++) + 3598];
      *dst++ = ytab[((*py++ + 15) << 4) | (uv >> 4)];
      *dst++ = ytab[((*py++ + 7) << 4) | (uv & 15)];
      uv = uvtab[((*pu++ << 8) | *pv++) + 2570];
      *dst++ = ytab[((*py++ + 13) << 4) | (uv >> 4)];
      *dst++ = ytab[((*py++ + 5) << 4) | (uv & 15)];
      uv = uvtab[((*pu++ << 8) | *pv++) + 3598];
      *dst++ = ytab[((*py++ + 15) << 4) | (uv >> 4)];
      *dst++ = ytab[((*py++ + 7) << 4) | (uv & 15)];
      uv = uvtab[((*pu++ << 8) | *pv++) + 2570];
      *dst++ = ytab[((*py++ + 13) << 4) | (uv >> 4)];
      *dst++ = ytab[((*py++ + 5) << 4) | (uv & 15)];
    }
  }
}
#endif
