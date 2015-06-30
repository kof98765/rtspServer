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
%macro new_sad16b 0
    movdqu  xmm0, [edx]
    movdqu  xmm1, [edx+ebx]
    lea edx,[edx+2*ebx]
    movdqa  xmm2, [eax]
    movdqa  xmm3, [eax+ecx]
    lea eax,[eax+2*ecx]
    psadbw  xmm0, xmm2
    paddusw xmm6,xmm0
    psadbw  xmm1, xmm3
    paddusw xmm6,xmm1
%endmacro

%macro new_sad8b 0
    movq mm0, [esi]
    movq mm1, [edi]
    psadbw mm0, mm1
    add    esi, ebx             ; src + src_stride
    add    edi, edx             ; dst + dst_stride
	paddusw mm2, mm0
%endmacro

%macro new_sad4b 0
    movq mm0, [esi]
    movq mm1, [edi]
    ; we only need low 4 bytes
    pand mm0, mm3
    pand mm1, mm3
    psadbw mm0, mm1
    add    esi, ebx             ; src + src_stride
    add    edi, edx             ; dst + dst_stride
	paddusw mm2,mm0
%endmacro

section .rodata data align=16

align 16
    mmx_mask01 db 0xff, 0xff, 0xff, 0xff, 0, 0, 0, 0

section .text

;======================================================
;
; uint32_t 
; T264_sad_u_16x16_sse2(uint8_t* src, int32_t src_stride, uint8_t* data, int32_t dst_stride);
;
;======================================================

align 16

cglobal T264_sad_u_16x16_sse2
T264_sad_u_16x16_sse2
   
    push ebx
    
    mov eax, [esp + 4 + 4]      ; src
    mov ecx, [esp + 8 + 4]      ; src_stride
    mov edx, [esp + 12 + 4]     ; data
    mov ebx, [esp + 16 + 4]     ; dst_stride
    
    pxor xmm6, xmm6

    new_sad16b
    new_sad16b
    new_sad16b
    new_sad16b
    new_sad16b
    new_sad16b
    new_sad16b
    new_sad16b
    
    pshufd  xmm5, xmm6, 00000010b
    paddusw xmm6, xmm5
    pextrw  eax, xmm6, 0
    
    pop ebx
    
    ret
    
;======================================================
;
; uint32_t 
; T264_sad_u_16x8_sse2(uint8_t* src, int32_t src_stride, uint8_t* data, int32_t dst_stride);
;
;======================================================

align 16

cglobal T264_sad_u_16x8_sse2
T264_sad_u_16x8_sse2
    
    push ebx
    
    mov eax, [esp + 4 + 4]      ; src
    mov ecx, [esp + 8 + 4]      ; src_stride
    mov edx, [esp + 12 + 4]     ; data
    mov ebx, [esp + 16 + 4]     ; dst_stride
    
    pxor xmm6, xmm6

    new_sad16b
    new_sad16b
    new_sad16b
    new_sad16b
    
    pshufd  xmm5, xmm6, 00000010b
    paddusw xmm6, xmm5
    pextrw  eax, xmm6, 0
    
    pop ebx

    ret
    
;======================================================
;
; uint32_t 
; T264_sad_u_8x16_sse(uint8_t* src, int32_t src_stride, uint8_t* data, int32_t dst_stride);
;
;======================================================

align 16

cglobal T264_sad_u_8x16_sse
T264_sad_u_8x16_sse
    
    push ebx
    push esi
    push edi
    
    mov esi, [esp + 4 + 12]      ; src
    mov ebx, [esp + 8 + 12]      ; src_stride
    mov edi, [esp + 12+ 12]      ; data
    mov edx, [esp + 16+ 12]      ; dst_stride
    
	pxor mm2, mm2;
    
    new_sad8b;
    new_sad8b;
    new_sad8b;
    new_sad8b;
    new_sad8b;
    new_sad8b;
    new_sad8b;
    new_sad8b;
    new_sad8b;
    new_sad8b;
    new_sad8b;
    new_sad8b;
    new_sad8b;
    new_sad8b;
    new_sad8b;
    new_sad8b;

	pextrw eax, mm2, 0

    pop edi
    pop esi
    pop ebx

    ret
    
;======================================================
;
; uint32_t 
; T264_sad_u_8x8_sse(uint8_t* src, int32_t src_stride, uint8_t* data, int32_t dst_stride);
;
;======================================================

align 16

cglobal T264_sad_u_8x8_sse
T264_sad_u_8x8_sse
    
    push ebx
    push esi
    push edi
    
    mov esi, [esp + 4 + 12]      ; src
    mov ebx, [esp + 8 + 12]      ; src_stride
    mov edi, [esp + 12+ 12]      ; data
    mov edx, [esp + 16+ 12]      ; dst_stride
    
	pxor mm2, mm2;
    
    new_sad8b;
    new_sad8b;
    new_sad8b;
    new_sad8b;
    new_sad8b;
    new_sad8b;
    new_sad8b;
    new_sad8b;

	pextrw eax, mm2, 0

    pop edi
    pop esi
    pop ebx

    ret
    
;======================================================
;
; uint32_t 
; T264_sad_u_8x4_sse(uint8_t* src, int32_t src_stride, uint8_t* data, int32_t dst_stride);
;
;======================================================

align 16

cglobal T264_sad_u_8x4_sse
T264_sad_u_8x4_sse
    
    push ebx
    push esi
    push edi
    
    mov esi, [esp + 4 + 12]      ; src
    mov ebx, [esp + 8 + 12]      ; src_stride
    mov edi, [esp + 12+ 12]      ; data
    mov edx, [esp + 16+ 12]      ; dst_stride
    
	pxor mm2, mm2;
    
    new_sad8b;
    new_sad8b;
    new_sad8b;
    new_sad8b;

	pextrw eax, mm2, 0

    pop edi
    pop esi
    pop ebx

    ret
    
;======================================================
;
; uint32_t 
; T264_sad_u_4x8_sse(uint8_t* src, int32_t src_stride, uint8_t* data, int32_t dst_stride);
;
;======================================================

align 16

cglobal T264_sad_u_4x8_sse
T264_sad_u_4x8_sse
    
    push ebx
    push esi
    push edi
    
    mov esi, [esp + 4 + 12]      ; src
    mov ebx, [esp + 8 + 12]      ; src_stride
    mov edi, [esp + 12+ 12]      ; data
    mov edx, [esp + 16+ 12]      ; dst_stride
    movq mm3, [mmx_mask01]
    
    pxor mm2, mm2
    
    new_sad4b;
    new_sad4b;
    new_sad4b;
    new_sad4b;
    new_sad4b;
    new_sad4b;
    new_sad4b;
    new_sad4b;

	pextrw eax, mm2, 0
	
    pop edi
    pop esi
    pop ebx

    ret
    
;======================================================
;
; uint32_t 
; T264_sad_u_4x4_sse(uint8_t* src, int32_t src_stride, uint8_t* data, int32_t dst_stride);
;
;======================================================

align 16

cglobal T264_sad_u_4x4_sse
T264_sad_u_4x4_sse
    
    push ebx
    push esi
    push edi
    
    mov esi, [esp + 4 + 12]      ; src
    mov ebx, [esp + 8 + 12]      ; src_stride
    mov edi, [esp + 12+ 12]      ; data
    mov edx, [esp + 16+ 12]      ; dst_stride
    movq mm3, [mmx_mask01]
    
    pxor mm2, mm2
    
    new_sad4b;
    new_sad4b;
    new_sad4b;
    new_sad4b;

	pextrw eax, mm2, 0

    pop edi
    pop esi
    pop ebx

    ret