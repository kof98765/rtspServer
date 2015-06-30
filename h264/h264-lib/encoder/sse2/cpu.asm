;/*****************************************************************************
; *
; *  T264 AVC CODEC
; *
; *  Copyright(C) 2004-2005 llcc <lcgate1@yahoo.com.cn>
; *               2004-2005 visionany <visionany@yahoo.com.cn>
; *
; *  This program is free software ; you can redistribute it and/or modify
; *  it under the terms of the GNU General Public License as published by
; *  the Free Software Foundation ; either version 2 of the License, or
; *  (at your option) any later version.
; *
; *  This program is distributed in the hope that it will be useful,
; *  but WITHOUT ANY WARRANTY ; without even the implied warranty of
; *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; *  GNU General Public License for more details.
; *
; *  You should have received a copy of the GNU General Public License
; *  along with this program ; if not, write to the Free Software
; *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
; *
; ****************************************************************************/

bits 32

; ideal from xvid

; modify by Thomascatlee@163.com
; for GCC
%macro cglobal 1
	%ifdef NOPREFIX
		global %1
	%else
		global _%1
		%define %1 _%1
	%endif
%endmacro

; from xvid
%macro  CHECK_FEATURE         3
  mov ecx, %1
  and ecx, edx
  neg ecx
  sbb ecx, ecx
  and ecx, %2
  or %3, ecx
%endmacro

%define T264_CPU_MMX     0x10
%define T264_CPU_SSE     0x1000
%define T264_CPU_SSE2    0x10000

%define CPUID_MMX               0x00800000
%define CPUID_SSE               0x02000000
%define CPUID_SSE2              0x04000000

section .data

align 16

section .text

;======================================================
;
; int32_t
; T264_detect_cpu();
;
;======================================================

align 16

cglobal T264_detect_cpu
T264_detect_cpu
    
    push ebx
    push ebp
    pushfd

    xor ebp, ebp
    ; cpuid support ?    
    pushfd
    pop eax
    xor eax, 0x200000
    mov ecx, eax
    push eax
    popfd
    pushfd
    pop eax
    popfd
    cmp eax, ecx
    jnz .quit
    
    ; cpuid
    mov eax ,1
    cpuid
    
    ; MMX support ?
    CHECK_FEATURE CPUID_MMX, T264_CPU_MMX, ebp

    ; SSE support ?
    CHECK_FEATURE CPUID_SSE, T264_CPU_SSE, ebp

    ; SSE2 support?
    CHECK_FEATURE CPUID_SSE2, T264_CPU_SSE2, ebp
    
.quit
    mov eax, ebp
    pop ebp
    pop ebx
    ret

;======================================================
;
; void
; T264_emms_mmx();
;
;======================================================

cglobal T264_emms_mmx
T264_emms_mmx
    
    emms
    ret