/*****************************************************************************
 *
 *  T264 AVC CODEC
 *
 *  Copyright(C) 2004-2005 llcc <lcgate1@yahoo.com.cn>
 *               2004-2005 visionany <visionany@yahoo.com.cn>
 *  Ported to TI DSP Platform By YouXiaoQuan, HFUT-TI UNITED LAB,China      
 *               2004-2005 You Xiaoquan <YouXiaoquan@126.com>   
 *
 *  This program is free software ; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation ; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY ; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program ; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 ****************************************************************************/

#ifndef _CONFIG_H_
#define _CONFIG_H_

#ifdef _MSC_VER

#ifdef _M_IX86
#define ARCH_IS_IA32
#define ENABLE_PROFILE
#endif //_M_IX86

#endif // _MSC_VER

#ifdef __GCC__

//#ifdef _M_IX86
#define ARCH_IS_IA32
#define ENABLE_PROFILE
//#endif //_M_IX86

#endif // __GCC__


#if defined (_TI_DM642)
#define CHIP_DM642
//Ti_DSP Platform ported by YouXiaoquan,HFUT-Ti United Lab,China
//YouXiaoquan@126.com
#endif///////// CHIP_DM642


#endif
