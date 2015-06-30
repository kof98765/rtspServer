;/*****************************************************************************
; *
; *  T264 AVC CODEC
; *
; *  Copyright(C) 2004-2005 llcc <lcgate1@yahoo.com.cn>
; *               2004-2005 visionany <visionany@yahoo.com.cn>
; *		2005.1.13	CloudWu	Add function T264_pia_u_wxh_mmx/sse
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

section .rodata data align=16

align 16
    sse2_20 times 8 dw 20
    sse2_5n times 8 dw -5
    sse2_16 times 8 dw 16

section .text

;======================================================
;
; void
; interpolate_halfpel_h_sse2(uint8_t* src, int32_t src_stride, uint8_t* dst, int32_t dst_stride, int32_t width, int32_t height)
;
;======================================================
align 16

cglobal interpolate_halfpel_h_sse2
interpolate_halfpel_h_sse2
    
    push ebx
    push esi
    push edi
    push ebp
    
    mov esi, [esp + 16 + 4]     ; src
    mov eax, [esp + 16 + 8]     ; src_stride
    mov edi, [esp + 16 + 12]    ; dst
    mov ebx, [esp + 16 + 16]    ; dst_stride
    mov ebp, [esp + 16 + 20]    ; width
    
    pxor xmm7, xmm7
    xor edx, edx

.looprow

    xor ecx, ecx

.loopcol

    movq xmm4, [esi + ecx - 2]    ; src - 2
    movq xmm5, [esi + ecx - 1]    ; src - 1
    movq xmm0, [esi + ecx + 0]    ; src
    movq xmm1, [esi + ecx + 1]    ; src + 1
    movq xmm2, [esi + ecx + 2]    ; src + 2
    movq xmm3, [esi + ecx + 3]    ; src + 3
    
    punpcklbw xmm5, xmm7          ; src - 1
    punpcklbw xmm4, xmm7          ; src - 2
    punpcklbw xmm0, xmm7          ; src
    punpcklbw xmm1, xmm7          ; src + 1
    punpcklbw xmm2, xmm7          ; src + 2
    punpcklbw xmm3, xmm7          ; src + 3

    paddw  xmm4, [sse2_16]        ; (src - 2) + 16

    paddw  xmm5, xmm2             ; (src - 1) + (src + 2)
    paddw  xmm0, xmm1             ; (src + 0) + (src + 1)
    
    pmullw xmm5, [sse2_5n]        ; ((src - 1) + (src + 2)) * -5
    pmullw xmm0, [sse2_20]        ; ((src + 0) + (src + 1)) * 20

    paddw  xmm4, xmm3             ; (src - 2) + (src + 3)
    paddw  xmm4, xmm5             ;                       - 5 * (src - 1)
    paddw  xmm0, xmm4             ; all
    
    psraw  xmm0, 5
    
    ; pack
    packuswb xmm0, xmm0
    movq [edi + ecx], xmm0
    
    add ecx, 8
    cmp ecx, ebp
    jl .loopcol
    inc edx
    add esi, eax
    add edi, ebx
    cmp edx, [esp + 16 + 24]    ; height
    jl .looprow
    
    pop ebp
    pop edi
    pop esi
    pop ebx
    
    ret

;======================================================
;
; void
; interpolate_halfpel_v_sse2(uint8_t* src, int32_t src_stride, uint8_t* dst, int32_t dst_stride, int32_t width, int32_t height)
;
;======================================================

align 16

cglobal interpolate_halfpel_v_sse2
interpolate_halfpel_v_sse2
    
    push ebx
    push esi
    push edi
    push ebp
    
    mov esi, [esp + 16 + 4]     ; src
    mov eax, [esp + 16 + 8]     ; src_stride
    mov edi, [esp + 16 + 12]    ; dst
    mov ebp, [esp + 16 + 20]    ; width
    
    pxor xmm7, xmm7
    xor edx, edx

.looprow

    xor ebx, ebx

.loopcol
    
    lea  ecx, [esi + ebx]
    movq xmm0, [ecx]    ; src
    sub  ecx, eax
    movq xmm4, [ecx]    ; src - stride
    sub  ecx, eax
    movq xmm5, [ecx]    ; src - 2 * stride
    lea  ecx, [esi + eax]
    add  ecx, ebx
    movq xmm1, [ecx]    ; src + stride
    add  ecx, eax
    movq xmm2, [ecx]    ; src + 2 * stride
    add  ecx, eax
    movq xmm3, [ecx]    ; src + 3 * stride

    punpcklbw xmm0, xmm7          ; src
    punpcklbw xmm4, xmm7          ; src - 1
    punpcklbw xmm5, xmm7          ; src - 2
    punpcklbw xmm1, xmm7          ; src + 1
    punpcklbw xmm2, xmm7          ; src + 2
    punpcklbw xmm3, xmm7          ; src + 3

    paddw  xmm5, [sse2_16]        ; (src - 2) + 16

    paddw  xmm4, xmm2             ; (src - 1) + (src + 2)
    paddw  xmm0, xmm1             ; (src + 0) + (src + 1)
    
    pmullw xmm4, [sse2_5n]        ; ((src - 1) + (src + 2)) * -5
    pmullw xmm0, [sse2_20]        ; ((src + 0) + (src + 1)) * 20

    paddw  xmm5, xmm3             ; (src - 2) + (src + 3)
    paddw  xmm5, xmm4             ;                       - 5 * (src - 1)
    paddw  xmm0, xmm5             ; all
    
    psraw  xmm0, 5
    
    ; pack
    packuswb xmm0, xmm0
    movq [edi + ebx], xmm0
    
    add ebx, 8
    cmp ebx, ebp
    jl .loopcol
    inc edx
    add esi, eax
    add edi, [esp + 16 + 16]
    cmp edx, [esp + 16 + 24]    ; height
    jl .looprow
    
    pop ebp
    pop edi
    pop esi
    pop ebx
    
    ret

;======================================================
;
; void
; interpolate_halfpel_hv_sse2(uint8_t* src, int32_t src_stride, uint8_t* dst, int32_t dst_stride, int32_t width, int32_t height)
;
;======================================================

align 16

cglobal interpolate_halfpel_hv_sse2
interpolate_halfpel_hv_sse2
    
    ret
    

;=======================PIA=================================
%macro new_pia16b_sse2 0
    movdqu  xmm0, [eax] ; p1
    movdqu  xmm1, [eax + ebx] ;
    lea eax, [eax+2*ebx]
    
    movdqu  xmm5, [ecx] ; p2
    movdqu  xmm6, [ecx + edx];
    lea ecx, [ecx+2*edx]

    pavgb	xmm0, xmm5
    pavgb   xmm1, xmm6
    
    movdqa  [esi], xmm0
    movdqa  [esi + edi], xmm1
    
    lea esi, [esi+2*edi]
%endmacro

; %1 = src1, %2 = src2, %3 = src1_stride, %4 = src2_stride, %5 = dst, %6 = dst stride
%macro pia16b_sse 6
    movq	mm0, [%1]
    movq	mm1, [%2]
    pavgb	mm0,mm1
	movq	[%5],mm0

    movq	mm0, [%1 + 8]
    movq	mm1, [%2 + 8]
    pavgb	mm0,mm1
	movq	[%5 + 8],mm0

	add		%1,%3
	add		%2,%4
	add		%5,%6    
%endmacro

; %1 = src1, %2 = src2, %3 = src1_stride, %4 = src2_stride, %5 = dst, %6 = dst stride
%macro pia8b_sse 6
    movq	mm0, [%1]
    movq	mm1, [%2]
    pavgb	mm0,mm1
	movq	[%5],mm0
	add		%1,%3
	add		%2,%4
	add		%5,%6    
%endmacro

; %1 = src1, %2 = src2, %3 = src1_stride, %4 = src2_stride, %5 = dst, %6 = dst stride
%macro pia4b_mmx 6
    movd	mm0, [%1]
    movd	mm1, [%2]
    pavgb	mm0,mm1
	movd	[%5],mm0
	add		%1,%3
	add		%2,%4
	add		%5,%6    
%endmacro

;======================================================
;
; void 
; T264_pia_u_4x4_mmx(uint8_t* p1, uint8_t* p2, int32_t p1_stride, int32_t p2_stride, uint8_t* dst, int32_t dst_stride);
;
;======================================================

align 16

cglobal T264_pia_u_4x4_mmx
T264_pia_u_4x4_mmx    
    push ebx
    push esi
    push edi
   
    mov esi, [esp + 4 + 12]     ; p1 
    mov ebx, [esp + 8 + 12]		; p2
	mov	eax, [esp + 12 + 12]    ; 
	mov	edx, [esp + 16 + 12]    ; 
	mov	edi, [esp + 20 + 12]    ; 
	mov	ecx, [esp + 24 + 12]    ; 

	pia4b_mmx	esi,ebx,eax,edx,edi,ecx
	pia4b_mmx	esi,ebx,eax,edx,edi,ecx
	pia4b_mmx	esi,ebx,eax,edx,edi,ecx
	pia4b_mmx	esi,ebx,eax,edx,edi,ecx

	EMMS

    pop edi
    pop esi
    pop ebx

    ret

;======================================================
;
; void 
; T264_pia_u_4x8_mmx(uint8_t* p1, uint8_t* p2, int32_t p1_stride, int32_t p2_stride, uint8_t* dst, int32_t dst_stride);
;
;======================================================

align 16

cglobal T264_pia_u_4x8_mmx
T264_pia_u_4x8_mmx    
    push ebx
    push esi
    push edi
   
    mov esi, [esp + 4 + 12]     ; p1 
    mov ebx, [esp + 8 + 12]		; p2
	mov	eax, [esp + 12 + 12]    ; 
	mov	edx, [esp + 16 + 12]    ; 
	mov	edi, [esp + 20 + 12]    ; 
	mov	ecx, [esp + 24 + 12]    ; 

	pia4b_mmx	esi,ebx,eax,edx,edi,ecx
	pia4b_mmx	esi,ebx,eax,edx,edi,ecx
	pia4b_mmx	esi,ebx,eax,edx,edi,ecx
	pia4b_mmx	esi,ebx,eax,edx,edi,ecx
	pia4b_mmx	esi,ebx,eax,edx,edi,ecx
	pia4b_mmx	esi,ebx,eax,edx,edi,ecx
	pia4b_mmx	esi,ebx,eax,edx,edi,ecx
	pia4b_mmx	esi,ebx,eax,edx,edi,ecx

	EMMS

    pop edi
    pop esi
    pop ebx

    ret

;======================================================
;
; void 
; T264_pia_u_8x4_sse(uint8_t* p1, uint8_t* p2, int32_t p1_stride, int32_t p2_stride, uint8_t* dst, int32_t dst_stride);
;
;======================================================

align 16

cglobal T264_pia_u_8x4_sse
T264_pia_u_8x4_sse    
    push ebx
    push esi
    push edi
   
    mov esi, [esp + 4 + 12]     ; p1 
    mov ebx, [esp + 8 + 12]		; p2
	mov	eax, [esp + 12 + 12]    ; 
	mov	edx, [esp + 16 + 12]    ; 
	mov	edi, [esp + 20 + 12]    ; 
	mov	ecx, [esp + 24 + 12]    ; 

	pia8b_sse	esi , ebx , eax , edx , edi , ecx
	pia8b_sse	esi,ebx,eax,edx,edi,ecx
	pia8b_sse	esi,ebx,eax,edx,edi,ecx
	pia8b_sse	esi,ebx,eax,edx,edi,ecx

    pop edi
    pop esi
    pop ebx

    ret

;======================================================
;
; void 
; T264_pia_u_8x8_sse(uint8_t* p1, uint8_t* p2, int32_t p1_stride, int32_t p2_stride, uint8_t* dst, int32_t dst_stride);
;
;======================================================

align 16

cglobal T264_pia_u_8x8_sse
T264_pia_u_8x8_sse    
    push ebx
    push esi
    push edi
   
    mov esi, [esp + 4 + 12]     ; p1 
    mov ebx, [esp + 8 + 12]		; p2
	mov	eax, [esp + 12 + 12]    ; 
	mov	edx, [esp + 16 + 12]    ; 
	mov	edi, [esp + 20 + 12]    ; 
	mov	ecx, [esp + 24 + 12]    ; 

	pia8b_sse	esi,ebx,eax,edx,edi,ecx
	pia8b_sse	esi,ebx,eax,edx,edi,ecx
	pia8b_sse	esi,ebx,eax,edx,edi,ecx
	pia8b_sse	esi,ebx,eax,edx,edi,ecx
	pia8b_sse	esi,ebx,eax,edx,edi,ecx
	pia8b_sse	esi,ebx,eax,edx,edi,ecx
	pia8b_sse	esi,ebx,eax,edx,edi,ecx
	pia8b_sse	esi,ebx,eax,edx,edi,ecx

    pop edi
    pop esi
    pop ebx

    ret

;======================================================
;
; void 
; T264_pia_u_8x16_sse(uint8_t* p1, uint8_t* p2, int32_t p1_stride, int32_t p2_stride, uint8_t* dst, int32_t dst_stride);
;
;======================================================

align 16

cglobal T264_pia_u_8x16_sse
T264_pia_u_8x16_sse    
    push ebx
    push esi
    push edi
   
    mov esi, [esp + 4 + 12]     ; p1 
    mov ebx, [esp + 8 + 12]		; p2
	mov	eax, [esp + 12 + 12]    ; 
	mov	edx, [esp + 16 + 12]    ; 
	mov	edi, [esp + 20 + 12]    ; 
	mov	ecx, [esp + 24 + 12]    ; 

	pia8b_sse	esi,ebx,eax,edx,edi,ecx
	pia8b_sse	esi,ebx,eax,edx,edi,ecx
	pia8b_sse	esi,ebx,eax,edx,edi,ecx
	pia8b_sse	esi,ebx,eax,edx,edi,ecx
	pia8b_sse	esi,ebx,eax,edx,edi,ecx
	pia8b_sse	esi,ebx,eax,edx,edi,ecx
	pia8b_sse	esi,ebx,eax,edx,edi,ecx
	pia8b_sse	esi,ebx,eax,edx,edi,ecx
	pia8b_sse	esi,ebx,eax,edx,edi,ecx
	pia8b_sse	esi,ebx,eax,edx,edi,ecx
	pia8b_sse	esi,ebx,eax,edx,edi,ecx
	pia8b_sse	esi,ebx,eax,edx,edi,ecx
	pia8b_sse	esi,ebx,eax,edx,edi,ecx
	pia8b_sse	esi,ebx,eax,edx,edi,ecx
	pia8b_sse	esi,ebx,eax,edx,edi,ecx
	pia8b_sse	esi,ebx,eax,edx,edi,ecx

    pop edi
    pop esi
    pop ebx

    ret

;======================================================
;
; void 
; T264_pia_u_16x8_sse(uint8_t* p1, uint8_t* p2, int32_t p1_stride, int32_t p2_stride, uint8_t* dst, int32_t dst_stride);
;
;======================================================

align 16

cglobal T264_pia_u_16x8_sse
T264_pia_u_16x8_sse    
    push ebx
    push esi
    push edi
   
    mov esi, [esp + 4 + 12]     ; p1 
    mov ebx, [esp + 8 + 12]		; p2
	mov	eax, [esp + 12 + 12]    ; 
	mov	edx, [esp + 16 + 12]    ; 
	mov	edi, [esp + 20 + 12]    ; 
	mov	ecx, [esp + 24 + 12]    ; 

	pia16b_sse	esi,ebx,eax,edx,edi,ecx
	pia16b_sse	esi,ebx,eax,edx,edi,ecx
	pia16b_sse	esi,ebx,eax,edx,edi,ecx
	pia16b_sse	esi,ebx,eax,edx,edi,ecx
	pia16b_sse	esi,ebx,eax,edx,edi,ecx
	pia16b_sse	esi,ebx,eax,edx,edi,ecx
	pia16b_sse	esi,ebx,eax,edx,edi,ecx
	pia16b_sse	esi,ebx,eax,edx,edi,ecx

    pop edi
    pop esi
    pop ebx

    ret

;======================================================
;
; void 
; T264_pia_u_16x16_sse(uint8_t* p1, uint8_t* p2, int32_t p1_stride, int32_t p2_stride, uint8_t* dst, int32_t dst_stride);
;
;======================================================

align 16

cglobal T264_pia_u_16x16_sse
T264_pia_u_16x16_sse    
    push ebx
    push esi
    push edi
   
    mov esi, [esp + 4 + 12]     ; p1 
    mov ebx, [esp + 8 + 12]		; p2
	mov	eax, [esp + 12 + 12]    ; 
	mov	edx, [esp + 16 + 12]    ; 
	mov	edi, [esp + 20 + 12]    ; 
	mov	ecx, [esp + 24 + 12]    ; 

	pia16b_sse	esi,ebx,eax,edx,edi,ecx
	pia16b_sse	esi,ebx,eax,edx,edi,ecx
	pia16b_sse	esi,ebx,eax,edx,edi,ecx
	pia16b_sse	esi,ebx,eax,edx,edi,ecx
	pia16b_sse	esi,ebx,eax,edx,edi,ecx
	pia16b_sse	esi,ebx,eax,edx,edi,ecx
	pia16b_sse	esi,ebx,eax,edx,edi,ecx
	pia16b_sse	esi,ebx,eax,edx,edi,ecx
	pia16b_sse	esi,ebx,eax,edx,edi,ecx
	pia16b_sse	esi,ebx,eax,edx,edi,ecx
	pia16b_sse	esi,ebx,eax,edx,edi,ecx
	pia16b_sse	esi,ebx,eax,edx,edi,ecx
	pia16b_sse	esi,ebx,eax,edx,edi,ecx
	pia16b_sse	esi,ebx,eax,edx,edi,ecx
	pia16b_sse	esi,ebx,eax,edx,edi,ecx
	pia16b_sse	esi,ebx,eax,edx,edi,ecx

    pop edi
    pop esi
    pop ebx

    ret
    
;======================================================
;
; void 
; T264_pia_u_16x16_sse2(uint8_t* p1, uint8_t* p2, int32_t p1_stride, int32_t p2_stride, uint8_t* dst, int32_t dst_stride);
;
;======================================================

align 16

cglobal T264_pia_u_16x16_sse2
T264_pia_u_16x16_sse2

    push ebx
    push esi
    push edi
    
    mov eax, [esp + 4 + 12] ; p1
    mov ecx, [esp + 8 + 12] ; p2
    mov ebx, [esp + 12+ 12] ; p1_stride
    mov edx, [esp + 16+ 12] ; p2_stride
    mov esi, [esp + 20+ 12] ; dst
    mov edi, [esp + 24+ 12] ; dst_stride
    
    new_pia16b_sse2
    new_pia16b_sse2
    new_pia16b_sse2
    new_pia16b_sse2
    new_pia16b_sse2
    new_pia16b_sse2
    new_pia16b_sse2
    new_pia16b_sse2
    
    pop edi
    pop esi
    pop ebx
    
    ret    
    
;======================================================
;
; void 
; T264_pia_u_16x8_sse2(uint8_t* p1, uint8_t* p2, int32_t p1_stride, int32_t p2_stride, uint8_t* dst, int32_t dst_stride);
;
;======================================================

align 16

cglobal T264_pia_u_16x8_sse2
T264_pia_u_16x8_sse2

    push ebx
    push esi
    push edi
    
    mov eax, [esp + 4 + 12] ; p1
    mov ecx, [esp + 8 + 12] ; p2
    mov ebx, [esp + 12+ 12] ; p1_stride
    mov edx, [esp + 16+ 12] ; p2_stride
    mov esi, [esp + 20+ 12] ; dst
    mov edi, [esp + 24+ 12] ; dst_stride
    
    new_pia16b_sse2
    new_pia16b_sse2
    new_pia16b_sse2
    new_pia16b_sse2
    
    pop edi
    pop esi
    pop ebx
    
    ret    
