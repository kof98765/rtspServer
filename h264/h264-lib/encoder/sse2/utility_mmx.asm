;/*****************************************************************************
; *
; *  T264 AVC CODEC
; *
; *  Copyright(C) 2004-2005 llcc <lcgate1@yahoo.com.cn>
; *               2004-2005 visionany <visionany@yahoo.com.cn>
; *				  2004-2005 Cloud Wu <sywu@sohu.com> 
; *		2004		CloudWu	create this file	
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

; %1 = src, %2 = dest MMX reg ,%3 MMX temp register
%macro expand8to16_4_mmx 3
    movd	%2,[%1]
	movq	%3,%2
	PXOR	%3,%2
	PUNPCKLBW %2,%3
%endmacro

; %1 = src, %2 = dst
%macro contract16to8_4_mmx 2
    movq	mm0,[%1]
    packuswb mm0, mm1
    movd	[%2],mm0
%endmacro

; %1 = src, %2 = dst
%macro contract16to8_8_mmx 2
    movq	mm0,[%1]
	movq	mm1,[%1 + 8]	;fetch next 4 int16
    packuswb mm0, mm1
    movq	[%2],mm0
    ;movd   %3, %4
%endmacro

; %1 = src, %2 = pred, %3 =dst
%macro contract16to8add_4_mmx 3
    movq	mm0,[%1]
	movd	mm2,[%2]
    packuswb mm0, mm1
	movq	mm1,[%1 + 8]	;fetch next 4 int16
    packuswb mm0, mm1
    movq	[%3],mm0
%endmacro


section .text

;======================================================
;
; uint32_t 
; contract16to8_4x4_mmx(uint16_t* src, int32_t src_stride, uint8_t* dst, int32_t dst_stride);
;
;======================================================

align 16

cglobal contract16to8_4x4_mmx
contract16to8_4x4_mmx    
    push ebx
    push esi
    push edi
    
    mov esi, [esp + 4 + 12]     ; src
    mov edi, [esp + 12 + 12]    ; dst
	contract16to8_8_mmx esi , edi 

	mov ebx, [esp + 8 + 12]		;src_stride 
	shl	ebx, 2					;as is int16_t
	add esi, ebx		;[esp + 8 + 12]		; src + src_stride * 2
	mov ebx, [esp + 16 + 12]	;dst_stride
	shl	ebx, 1					;as we process 8 elements
	add edi, ebx				;dst + dst_stride 
	contract16to8_8_mmx esi , edi 

    pop edi
    pop esi
    pop ebx

    ret

;======================================================
;
; void 
; contract16to8add_4x4_mmx(uint16_t* src,uint8_t* pred, int32_t pred_stride,uint8_t* dst, int32_t dst_stride);
;
;======================================================

align 16

cglobal contract16to8add_4x4_mmx
contract16to8add_4x4_mmx    
    push ebx
    push esi
    push edi
	push edx
	push ecx	
 	push eax	
   
    mov esi, [esp + 4 + 24]     ; src
    mov ebx, [esp + 8 + 24]		; pred
	mov	eax, [esp + 12 + 24]    ; pred_stride
	mov	edi, [esp + 16 + 24]    ; dst
	mov	edx, [esp + 20 + 24]    ; dst_stride
	
	mov	ecx,2

.Loop		
	expand8to16_4_mmx	ebx , mm0 , mm1	;now int16 pred is in mm0
	
	movq	mm2,[esi]
	PADDSW  mm2,mm0		;save 4 int16 result

	add	esi,8			;advance 4 int16
	add ebx,eax
	expand8to16_4_mmx	ebx , mm0 , mm1	;now int16 pred is in mm0

	movq	mm3,[esi]
	PADDSW  mm3,mm0		;save 4 int16 result
	
	PACKUSWB mm2,mm3	
	movd	[edi],mm2
	add		edi,edx		;add dst_stride
	psrlq	mm2,32		;right shift 32bits
	movd	[edi],mm2

	add		ebx,eax
	add		edi,edx		;add dst_stride	
	add		esi,8		;advance 4 int16

	sub		ecx,1
	jnz		.Loop

	pop	eax
	pop ecx	
	pop edx
    pop edi
    pop esi
    pop ebx

    retn 20

;======================================================
;
; void 
; expand8to16sub_4x4_mmx(uint8_t* pred,int32_t pred_stride,int8_t* src,int32_t src_stride,uint16_t* dst);
;
;======================================================

align 16

cglobal expand8to16sub_4x4_mmx
expand8to16sub_4x4_mmx    
    push ebx
    push esi
    push edi
	push edx
	push ecx	
 	push eax	
   
    mov esi, [esp + 4 + 24]     ; pred 
    mov ebx, [esp + 8 + 24]		; pred_stride
	mov	eax, [esp + 12 + 24]    ; src
	mov	edx, [esp + 16 + 24]    ; src_stride
	mov	edi, [esp + 20 + 24]    ; dst
	
	mov	ecx,4

.Loop		
	expand8to16_4_mmx	esi , mm0 , mm1	;now int16 pred is in mm0
	expand8to16_4_mmx	eax , mm2 , mm1	;now int16 src is in mm2	
	;movq	mm2,[esi]
	PSUBSW  mm2,mm0		;save 4 int16 result <src - pred>
	movq	[edi],mm2

	add	esi,ebx	;8			;	advance pred_stride
	add eax,edx				; + src_stride
	
	add		edi,8		;add 4 int16

	sub		ecx,1
	jnz		.Loop

	pop	eax
	pop ecx	
	pop edx
    pop edi
    pop esi
    pop ebx

    retn 20

	
;======================================================
;
; uint32_t 
; contract16to8_mmx(int16_t* src, int32_t quarter_width, int32_t quarter_height, uint8_t* dst, int32_t dst_stride);
;
;======================================================

align 16

cglobal contract16to8_mmx
contract16to8_mmx
    push esi
    push edi

    mov esi, [esp + 4 + 8]      ; src
    ;mov edx, [esp + 8 + 8]      ; quarter_width
    mov ecx, [esp + 12 + 8]     ; quarter_height
	mov edi, [esp + 16 + 8]     ; dst
	mov ebx, [esp + 20 + 8]     ; dst_stride
	mov	eax,edx
	shl	eax,2	;4 * quarter_width
	shl	ecx,2	;quarter_height * 4

.LoopFirst
	mov	edx,[esp + 8 + 8]
.LoopSecond
	contract16to8_4_mmx	esi,edi
	add	esi,8	; 4 int16
	add	edi,4	; 4 uint8
	sub	edx,1
	jnz	.LoopSecond
	sub	edi,eax	;4
	add edi,ebx			;dst+dst_stride

	sub	ecx,1
	jnz	.LoopFirst		;outer
	
	EMMS
    pop edi
    pop esi

    ret
 
;======================================================
;
; void 
; contract16to8add_mmx(int16_t* src, int32_t quarter_width, int32_t quarter_height, uint8_t* pred, uint8_t* dst, int32_t dst_stride)
;
;======================================================

align 16

cglobal contract16to8add_mmx
contract16to8add_mmx
    push esi
    push edi
	push ebp
	push edx
	push ecx	
	push ebx
 	push eax	
    
	mov esi, [esp + 4 + 28]      ; src
    mov eax, [esp + 8 + 28]      ; quarter_width
    mov ecx, [esp + 12 + 28]     ; quarter_height
	mov ebx, [esp + 16 + 28]     ; pred
	mov edi, [esp + 20 + 28]     ; dst

	mov ebp, [esp + 24 + 28]     ; dst_stride	

.LoopFirst
	mov	edx,[esp + 12 + 28]		;quarter_height

.LoopSecond
	push ebp					;dst_stride
	mov eax, [esp + 12 + 28]	;quarter_width
	push edi
	shl	eax,2				;quarter_width * 4
	push eax
	push ebx
	push esi
	call contract16to8add_4x4_mmx
	
	add	esi,32	; 4x4 int16
	add	ebx,4	; 4 uint8
	add	edi,4	; 4 uint8

	sub	edx,1	
	jnz	.LoopSecond
	
	mov eax, [esp + 8 + 28]
	shl eax,2	;4 * quaterwidth
	sub	edi,eax	;update dest
	mov eax, [esp + 24 + 28]
	shl eax,2	;4 * dst_stride
	add edi,eax	;add 4*dst_stride

	mov eax, [esp + 8 + 28]	;quaterwidth
	shl eax,2				;4 * quaterwidth
	sub ebx,eax
	shl eax,2
	add	ebx,eax	;update pred

	sub	ecx,1
	jnz	.LoopFirst		;outer
	
	EMMS

	pop	eax
	pop	ebx
	pop ecx	
	pop edx
	pop ebp
    pop edi
    pop esi
ret

;======================================================
;
; void 
; expand8to16sub_mmx(uint8_t* pred, int32_t quarter_width, int32_t quarter_height, uint8_t* dst , int16_t* src, int32_t src_stride)
;
;======================================================

align 16

cglobal expand8to16sub_mmx
expand8to16sub_mmx
    push esi
    push edi
	push ebp
	push edx
	push ecx	
	push ebx
 	push eax	
    
	mov esi, [esp + 20 + 28]      ; src
    mov eax, [esp + 8 + 28]      ; quarter_width
    mov ecx, [esp + 12 + 28]     ; quarter_height
	mov ebx, [esp + 4 + 28]     ; pred
	mov edi, [esp + 16 + 28]     ; dst

	mov ebp, [esp + 24 + 28]     ; src_stride	

.LoopFirst
	mov	edx,[esp + 12 + 28]		;quarter_height

.LoopSecond
	mov eax, [esp + 8 + 28]		;quarter_width	
	push edi					;dst
	push ebp					;src_stride
	push esi					;src
	shl	eax,2					;quarter_width * 4
	push eax					;pred-stride
	push ebx					;pred
	call expand8to16sub_4x4_mmx
	
	add	esi,4	;32	; 4x4 int16
	add	ebx,4	; 4 uint8
	add	edi,32	;4	; 16 int16

	sub	edx,1	
	jnz	.LoopSecond
	
	mov eax, [esp + 8 + 28]
	shl eax,2	;4 * quaterwidth
	sub	esi,eax	;update dest
	mov eax, [esp + 24 + 28]	;src_stride
	shl eax,2	;4 * src_stride
	add esi,eax	;add 4*src_stride

	mov eax, [esp + 8 + 28]	;quaterwidth
	shl eax,2				;4 * quaterwidth
	sub ebx,eax
	shl eax,2
	add	ebx,eax	;update pred

	sub	ecx,1
	jnz	.LoopFirst		;outer
	
	EMMS

	pop	eax
	pop	ebx
	pop ecx	
	pop edx
	pop ebp
    pop edi
    pop esi
ret

