/*
 *  Copyright 2001 by Texas Instruments Incorporated.
 *  All rights reserved. Property of Texas Instruments Incorporated.
 *  Restricted rights to use, duplicate or disclose this code are
 *  granted through contract.
 */
/*
 *  ======== T264.cmd ========
 *
 */
-heap  0x6FFFFF/*0x9100;0xEF7B000;0xFFFFFF*/
-stack 0x1000/*0xC00;0x1330;0x97B00;0x15E00This Stack Is So Long We Will Make It short !*/
MEMORY 
{  
   ISRAM0       : origin = 0x0,         len = 0x400
   ISRAM1       : origin = 0x80000000,         len = 0x1FC00
   SDRAM       : origin = 0x80020000,         len = 0x7FFFFF/*0xFFFFFFF*/
}

SECTIONS
{
        .vectors > ISRAM0
        .text    > SDRAM
		.switch  > ISRAM1
		.bss     > ISRAM1
        .cinit   > ISRAM1
        .const   > ISRAM1
        .far     > ISRAM1
        .stack   > ISRAM1
        .cio     > ISRAM1
        .sysmem  > SDRAM
        
}

/*-priority*/
-priority
-l rtdx64xx.lib         /* RTDX support */
-l cslDM642.lib
-l rts6400.lib          /* C and C++ run-time library support */

