BITS 64

SECTION .text

;
; void  Adapt ( short* pM, const short* pAdapt, int nDirection, int nOrder )
;
        ;; pM -> rdi
        ;; pAdapt -> rsi
        ;; nDirection -> rdx
        ;; nOrder -> rcx

ALIGN 16

GLOBAL Adapt_x86_64_mmx
GLOBAL _Adapt_x86_64_mmx
        
Adapt_x86_64_mmx:
_Adapt_x86_64_mmx: 
        shr  ecx, 4

        cmp  edx, byte 0       ; nDirection
        jge  short AdaptSub

AdaptAddLoop:
        movq  mm0, [rdi]
        paddw mm0, [rsi]
        movq  [rdi], mm0
        movq  mm1, [rdi + 8]
        paddw mm1, [rsi + 8]
        movq  [rdi + 8], mm1
        movq  mm2, [rdi + 16]
        paddw mm2, [rsi + 16]
        movq  [rdi + 16], mm2
        movq  mm3, [rdi + 24]
        paddw mm3, [rsi + 24]
        movq  [rdi + 24], mm3
        add   rdi, byte 32
        add   rsi, byte 32
        dec   rcx
        jnz   AdaptAddLoop
        
        emms
        ret

ALIGN 16

AdaptSub:   je    short AdaptDone

AdaptSubLoop:
        movq  mm0, [rdi]
        psubw mm0, [rsi]
        movq  [rdi], mm0
        movq  mm1, [rdi + 8]
        psubw mm1, [rsi + 8]
        movq  [rdi + 8], mm1
        movq  mm2, [rdi + 16]
        psubw mm2, [rsi + 16]
        movq  [rdi + 16], mm2
        movq  mm3, [rdi + 24]
        psubw mm3, [rsi + 24]
        movq  [rdi + 24], mm3
        add   rdi, byte 32
        add   rsi, byte 32
        dec   ecx
        jnz   AdaptSubLoop

        emms
AdaptDone:
        ret

;
; int  CalculateDotProduct ( const short* pA, const short* pB, int nOrder )
;
        ;; pA -> %rdi
        ;; pB -> %rsi
        ;; nOrder -> %rdx

ALIGN 16

GLOBAL CalculateDotProduct_x86_64_mmx
GLOBAL _CalculateDotProduct_x86_64_mmx

CalculateDotProduct_x86_64_mmx:    
_CalculateDotProduct_x86_64_mmx:    

        shr     edx, 4
        pxor    mm7, mm7

loopDot:
        movq    mm0, [rdi]
        pmaddwd mm0, [rsi]
        paddd   mm7, mm0
        movq    mm1, [rdi +  8]
        pmaddwd mm1, [rsi +  8]
        paddd   mm7, mm1
        movq    mm2, [rdi + 16]
        pmaddwd mm2, [rsi + 16]
        paddd   mm7, mm2
        movq    mm3, [rdi + 24]
        pmaddwd mm3, [rsi + 24]
        add     rdi, byte 32
        add     rsi, byte 32
        paddd   mm7, mm3
        dec     edx
        jnz     loopDot

        movq    mm6, mm7
        psrlq   mm7, 32
        paddd   mm6, mm7
        movd    eax, mm6
        emms

        ret
