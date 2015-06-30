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

%macro cextern 1
	%ifdef NOPREFIX
		extern %1
	%else
		extern _%1
		%define %1 _%1
	%endif
%endmacro

; input 0 1 2 3, output 3 4 1 0
%macro transpose 5
    movq       %5, %1
    punpckhwd  %5, %2  ; mm4 = 8 4 7 3
    punpcklwd  %1, %2  ; mm0 = 6 2 5 1

    movq       %2, %3
    punpckhwd  %2, %4  ; mm1 = 16 12 15 11
    punpcklwd  %3, %4  ; mm2 = 14 10 13 9

    movq       %4, %5
    punpckhdq  %4, %2  ; mm3 = 16 12 8 4
    punpckldq  %5, %2  ; mm4 = 15 11 7 3

    movq       %2, %1
    punpckhdq  %2, %3  ; mm1 = 14 10 6 2
    punpckldq  %1, %3  ; mm0 = 13 9 5 1
%endmacro

%macro addsub 5
    movq  %5, %1
    paddw %1, %4    ; %0 = s[0]
    psubw %5, %4    ; %4 = s[3]
    movq  %4, %2
    paddw %2, %3    ; %1 = s[1]
    psubw %4, %3    ; %3 = s[2]
%endmacro

%macro addsub2 5    
    movq  %5, %1   ; %5   = s[0]
    paddw %1, %2   ; d[0] = s[0] + s[1]
    psubw %5, %2   ; d[2] = tmp  - s[1]
    
    movq  %2, %4   ; %2   = s[3]
    paddw %2, %2   ; %2   = %2 + %2
    paddw %2, %3   ; d[1] = %2 + s[2]
    paddw %3, %3   ; s[2] = s[2]+ s[2]
    psubw %4, %3   ; d[3] = s[3]- s[2]
%endmacro

; output 0 4 1 2
%macro idct_addsub2 5    
    movq  %5, %1   ; %5   = d[0]
    paddw %1, %3   ; s[0] = d[0] + d[2]
    psubw %5, %3   ; s[1] = d[0] - d[2]
    
    movq  %3, %2   ; %3   = d[1]
    psraw %2, 1    ; %2   = %2 / 2
    psubw %2, %4   ; s[2] = %2 - d[3]
    psraw %4, 1    ; d[3] = d[3] / 2
    paddw %3, %4   ; s[3] = d[1] + d[3]
%endmacro

; %1 = mmx content, %2 = tmp mmx, %3 = zero mmx, %4 = xmm content, %5 = xmm tmp
%macro word2dw 5
    movq   %2, %1
    punpcklwd %2, %3  ;   dcba->0b0a
    punpckhwd %1, %3  ;   dcba->0d0c
    movq2dq %4, %1   ;   00 00 0d 0c
    pslldq %4, 8      ;   0d 0c 00 00
    movq2dq %5, %2   ;   00 00 0b 0a
    por %4, %5      ;   0d 0c 0b 0a
%endmacro

section .rodata data align=16

align 16
    sse2_neg1 dw -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
    sse2_1 dw 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
align 16
    mmx1 dw 1, 1, 1, 1
align 16
    mmx32 dw 32, 32, 32, 32
    
cextern quant
cextern dequant

align 16

section .text

;======================================================
;
; void 
; dct4x4_mmx(int16_t* data)
;
;======================================================

align 16

cglobal dct4x4_mmx
dct4x4_mmx
    
    mov  eax, [esp + 4]  ; data
    movq mm0, [eax + 0] 
    movq mm1, [eax + 8] 
    movq mm2, [eax + 16]
    movq mm3, [eax + 24]
    
    transpose mm0, mm1, mm2, mm3, mm4 ; input 0 1 2 3, output 0 1 4 3
    
    addsub mm0, mm1, mm4, mm3, mm2  ; input 0 1 2 3, output 0 1 3 4
    ; s[0] = mm0, s[1] = mm1, s[2] = mm3, s[3] = mm2

    addsub2 mm0, mm1, mm3, mm2, mm4  ; input 0 1 2 3, output 0 1 4 3

    transpose mm0, mm1, mm4, mm2, mm3 ; input 0 1 2 3, output 0 1 4 3
    
    addsub mm0, mm1, mm3, mm2, mm4  ; input 0 1 2 3, output 0 1 3 4
    ; s[0] = mm0, s[1] = mm1, s[2] = mm2, s[3] = mm4
    
    addsub2 mm0, mm1, mm2, mm4, mm3  ; input 0 1 2 3, output 0 1 4 3

    movq [eax + 0], mm0
    movq [eax + 8], mm1
    movq [eax +16], mm3
    movq [eax +24], mm4
 
    ret

;======================================================
;
; void 
; dct4x4dc_mmx(int16_t* data)
;
;======================================================

align 16

cglobal dct4x4dc_mmx
dct4x4dc_mmx

    mov  eax, [esp + 4]  ; data
    movq mm0, [eax + 0] 
    movq mm1, [eax + 8] 
    movq mm2, [eax + 16]
    movq mm3, [eax + 24]
    
    transpose mm0, mm1, mm2, mm3, mm4 ; input 0 1 2 3, output 0 1 4 3
    
    addsub mm0, mm1, mm4, mm3, mm2  ; input 0 1 2 3, output 0 1 3 4
    ; s[0] = mm0, s[1] = mm1, s[2] = mm3, s[3] = mm2

    addsub mm0, mm2, mm3, mm1, mm4  ; input 0 1 2 3, output 0 1 4 3

    transpose mm0, mm2, mm4, mm1, mm3 ; input 0 1 2 3, output 0 1 4 3
    
    addsub mm0, mm2, mm3, mm1, mm4  ; input 0 1 2 3, output 0 1 3 4
    ; s[0] = mm0, s[1] = mm2, s[2] = mm1, s[3] = mm4
    
    addsub mm0, mm4, mm1, mm2, mm3  ; input 0 1 2 3, output 0 1 4 3

    movq mm1, [mmx1]
    
    paddw mm0, mm1
    paddw mm4, mm1
    paddw mm3, mm1
    paddw mm2, mm1
    
    psraw mm0, 1
    psraw mm4, 1
    psraw mm3, 1
    psraw mm2, 1
    
    movq [eax + 0], mm0
    movq [eax + 8], mm4
    movq [eax +16], mm3
    movq [eax +24], mm2

    ret

;======================================================
;
; void 
; idct4x4_mmx(int16_t* data)
;
;======================================================

align 16

cglobal idct4x4_mmx
idct4x4_mmx

    mov  eax, [esp + 4]  ; data
    movq mm0, [eax + 0] 
    movq mm1, [eax + 8] 
    movq mm2, [eax + 16]
    movq mm3, [eax + 24]
    
    transpose mm0, mm1, mm2, mm3, mm4 ; input 0 1 2 3, output 0 1 4 3
    
    idct_addsub2 mm0, mm1, mm4, mm3, mm2  ; input 0 1 2 3, output 0 4 1 2
    ; s[0] = mm0, s[1] = mm2, s[2] = mm1, s[3] = mm4

    addsub mm0, mm2, mm1, mm4, mm3  ; input 0 1 2 3, output 0 1 3 4

    transpose mm0, mm2, mm4, mm3, mm1 ; input 0 1 2 3, output 0 1 4 3
    
    idct_addsub2 mm0, mm2, mm1, mm3, mm4  ; input 0 1 2 3, output 0 4 1 2
    ; s[0] = mm0, s[1] = mm4, s[2] = mm2, s[3] = mm1

    addsub mm0, mm4, mm2, mm1, mm3  ; input 0 1 2 3, output 0 1 3 4

    movq mm2, [mmx32]
    
    paddw mm0, mm2
    paddw mm4, mm2
    paddw mm1, mm2
    paddw mm3, mm2
    
    psraw mm0, 6
    psraw mm4, 6
    psraw mm1, 6
    psraw mm3, 6
    
    movq [eax + 0], mm0
    movq [eax + 8], mm4
    movq [eax +16], mm1
    movq [eax +24], mm3
 
    ret

;======================================================
;
; void 
; idct4x4dc_mmx(int16_t* data)
;
;======================================================

align 16

cglobal idct4x4dc_mmx
idct4x4dc_mmx

    mov  eax, [esp + 4]  ; data
    movq mm0, [eax + 0] 
    movq mm1, [eax + 8] 
    movq mm2, [eax + 16]
    movq mm3, [eax + 24]
    
    transpose mm0, mm1, mm2, mm3, mm4 ; input 0 1 2 3, output 0 1 4 3
    
    addsub mm0, mm1, mm3, mm4, mm2  ; input 0 1 2 3, output 0 4 3 1
    ; s[0] = mm0, s[1] = mm2, s[2] = mm4, s[3] = mm1

    addsub mm0, mm2, mm4, mm1, mm3  ; input 0 1 2 3, output 0 1 3 4

    transpose mm0, mm2, mm1, mm3, mm4 ; input 0 1 2 3, output 0 1 4 3
    
    addsub mm0, mm2, mm3, mm4, mm1  ; input 0 1 2 3, output 0 4 3 1
    ; s[0] = mm0, s[1] = mm1, s[2] = mm4, s[3] = mm2
    
    addsub mm0, mm1, mm4, mm2, mm3  ; input 0 1 2 3, output 0 1 3 4

    movq [eax + 0], mm0
    movq [eax + 8], mm1
    movq [eax +16], mm2
    movq [eax +24], mm3

    ret

;======================================================
;
; void
; quant4x4_sse2(int16_t* data, const int32_t Qp, int32_t is_intra)
;
;======================================================

align 16

cglobal quant4x4_sse2
quant4x4_sse2

    push ebx
    push esi
    push edi
    push ebp
    
    mov  edi, [esp + 4 + 16]  ; data
    mov  eax, [esp + 8 + 16]  ; qp
    cdq
    mov  ebp, [esp + 12 + 16] ; is_intra
    mov  ebx, 6

    idiv ebx
    add  eax, 15         ; qbits(eax) = 15 + qp / 6, mf_index(edx) = qp % 6    
    mov  esi, edx
    shl  esi, 5
    add  esi, quant     ; esi = quant[mf_index]
    mov  ecx, eax        ; ecx = qbits
    
    neg  ebp
    sbb  ebp, ebp
    and  ebp, 0xfffffffd
    add  ebp, 6          ; is_intra(ecx) ? 3 : 6
    
    mov  eax, 1
    shl  eax, cl         ; 1 << qbits
    cdq
    idiv ebp             ; 1 << qbits / is_intra(ecx) ? 3 : 6
    
    ; eax = f, ecx = qbits, esi = quant[mf_index], edi = data
    
    movd      mm0, eax
    movd      mm1, ecx
    pshufw    mm0, mm0, 0x44
    movq2dq   xmm6, mm0
    movq2dq   xmm7, mm1
    pshufd    xmm6, xmm6, 0x44        ; f
    pxor      mm3, mm3
    
    movdqa    xmm0, [edi + 0]         ; data
    movdqa    xmm1, [esi + 0]         ; quant
    
    ; > 0
    pxor      xmm4, xmm4
    movdqa    xmm2, xmm0
    pcmpgtw   xmm0, xmm4
    movdqa    xmm4, xmm0
    pand      xmm0, xmm2
    movdqa    xmm3, xmm0
    pmullw    xmm0, xmm1              ; low part
    pmulhw    xmm3, xmm1              ; high part
    movdqa    xmm5, xmm0
    punpcklwd xmm0, xmm3              ; low 4 - 32 bits
    punpckhwd xmm5, xmm3              ; high 4 - 32 bits
    movdqa    xmm3, xmm4
    punpcklwd xmm4, xmm4
    pand      xmm4, xmm6
    paddd     xmm0, xmm4              ; data * quant + f
    psrad     xmm0, xmm7              ; data * quant + f >> qbits
    punpckhwd xmm3, xmm3
    pand      xmm3, xmm6
    paddd     xmm5, xmm3              ; data * quant + f
    psrad     xmm5, xmm7              ; data * quant + f >> qbits
    packssdw  xmm0, xmm5
    
    ; < 0
    pxor      xmm4, xmm4
    movdqa    xmm5, xmm2
    pcmpgtw   xmm4, xmm2
    pand      xmm5, xmm4
    pmullw    xmm5, [sse2_neg1]
    movdqa    xmm3, xmm5
    pmullw    xmm5, xmm1
    pmulhw    xmm3, xmm1
    movdqa    xmm1, xmm5
    punpcklwd xmm5, xmm3
    punpckhwd xmm1, xmm3
    movdqa    xmm3, xmm4
    punpcklwd xmm4, xmm4
    pand      xmm4, xmm6
    paddd     xmm5, xmm4            ; data * quant - f
    psrad     xmm5, xmm7
    punpckhwd xmm3, xmm3
    pand      xmm3, xmm6
    paddd     xmm1, xmm3
    psrad     xmm1, xmm7
    packssdw  xmm5, xmm1
    pmullw    xmm5, [sse2_neg1]
    
    por       xmm5, xmm0
    movdqa    [edi + 0], xmm5

    movdqa    xmm0, [edi + 16]         ; data
    movdqa    xmm1, [esi + 16]         ; quant
    
    ; > 0
    pxor      xmm4, xmm4
    movdqa    xmm2, xmm0
    pcmpgtw   xmm0, xmm4
    movdqa    xmm4, xmm0
    pand      xmm0, xmm2
    movdqa    xmm3, xmm0
    pmullw    xmm0, xmm1              ; low part
    pmulhw    xmm3, xmm1              ; high part
    movdqa    xmm5, xmm0
    punpcklwd xmm0, xmm3              ; low 4 - 32 bits
    punpckhwd xmm5, xmm3              ; high 4 - 32 bits
    movdqa    xmm3, xmm4
    punpcklwd xmm4, xmm4
    pand      xmm4, xmm6
    paddd     xmm0, xmm4              ; data * quant + f
    psrad     xmm0, xmm7              ; data * quant + f >> qbits
    punpckhwd xmm3, xmm3
    pand      xmm3, xmm6
    paddd     xmm5, xmm3              ; data * quant + f
    psrad     xmm5, xmm7              ; data * quant + f >> qbits
    packssdw  xmm0, xmm5
    
    ; < 0
    pxor      xmm4, xmm4
    movdqa    xmm5, xmm2
    pcmpgtw   xmm4, xmm2
    pand      xmm5, xmm4
    pmullw    xmm5, [sse2_neg1]
    movdqa    xmm3, xmm5
    pmullw    xmm5, xmm1
    pmulhw    xmm3, xmm1
    movdqa    xmm1, xmm5
    punpcklwd xmm5, xmm3
    punpckhwd xmm1, xmm3
    movdqa    xmm3, xmm4
    punpcklwd xmm4, xmm4
    pand      xmm4, xmm6
    paddd     xmm5, xmm4            ; data * quant - f
    psrad     xmm5, xmm7
    punpckhwd xmm3, xmm3
    pand      xmm3, xmm6
    paddd     xmm1, xmm3
    psrad     xmm1, xmm7
    packssdw  xmm5, xmm1
    pmullw    xmm5, [sse2_neg1]
    
    por       xmm5, xmm0
    movdqa    [edi + 16], xmm5

    pop ebp
    pop edi
    pop esi
    pop ebx
    ret

;======================================================
;
; void
; iquant4x4_sse2(int16_t* data, const int32_t Qp)
;
;======================================================

align 16

cglobal iquant4x4_sse2
iquant4x4_sse2

    mov  eax, [esp + 8]  ; qp
    cdq
    mov  ecx, 6

    idiv ecx             ; qbits(eax) = qp / 6, mf_index(edx) = qp % 6    
    mov  ecx, edx
    shl  ecx, 5
    add  ecx, dequant   ; ecx = quant[mf_index]
    mov  edx, [esp + 4]  ; data
    
    ; eax = qbits, ecx = quant[mf_index], edx = data
    
    movdqa xmm6, [sse2_1]
    movdqa xmm0, [edx + 0]
    movdqa xmm2, [edx + 16]
    movdqa xmm1, [ecx + 0]
    movdqa xmm3, [ecx + 16]

    pmullw  xmm0, xmm1
    pmullw  xmm2, xmm3

    movd    xmm7, eax
    psllw   xmm6, xmm7      ; << qbits

    pmullw xmm0, xmm6
    pmullw xmm2, xmm6

    movdqa [edx + 0], xmm0
    movdqa [edx + 16], xmm2

    ret    