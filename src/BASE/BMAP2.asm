; HoMM1 bitmap rectangle primitives.  These are the hand-written predecessors
; of the C++ routines retained by HoMM2's BASE/bmap2 module.

.386
.model flat
option casemap:none
option prologue:none
option epilogue:none

EXTERN _gDimPalette:BYTE

.data
_gBitmapSourceSkip DWORD 0
_gBitmapRowSkip DWORD 0

.code

?BlitBitmap@@YAXPAVbitmap@@HHHH0HH@Z PROC NEAR
    push ebp
    mov ebp, esp
    push esi
    push edi
    cld
    mov esi, DWORD PTR [ebp+8]
    movzx eax, WORD PTR [esi+10h]
    mov ebx, eax
    mov ecx, DWORD PTR [ebp+14h]
    sub eax, ecx
    js blit_done
    mov _gBitmapSourceSkip, eax
    mov eax, DWORD PTR [ebp+10h]
    mul ebx
    mov edx, DWORD PTR [ebp+0ch]
    add eax, edx
    mov esi, DWORD PTR [esi+14h]
    add esi, eax
    mov edi, DWORD PTR [ebp+1ch]
    movzx eax, WORD PTR [edi+10h]
    mov ebx, eax
    sub eax, ecx
    js blit_done
    mov _gBitmapRowSkip, eax
    mov eax, DWORD PTR [ebp+24h]
    mul ebx
    mov edx, DWORD PTR [ebp+20h]
    add eax, edx
    mov edi, DWORD PTR [edi+14h]
    add edi, eax
    mov eax, ecx
    mov ebx, _gBitmapSourceSkip
    mov edx, _gBitmapRowSkip
blit_row:
    mov ecx, eax
    shr ecx, 2
    rep movsd
    mov ecx, eax
    and ecx, 3
    rep movsb
    add esi, ebx
    add edi, edx
    dec DWORD PTR [ebp+18h]
    jne blit_row
blit_done:
    pop edi
    pop esi
    pop ebp
    ret
?BlitBitmap@@YAXPAVbitmap@@HHHH0HH@Z ENDP

?DimBitmapArea@@YAXPAVbitmap@@HHHH@Z PROC NEAR
    push ebp
    mov ebp, esp
    push esi
    push edi
    mov esi, DWORD PTR [ebp+8]
    movzx eax, WORD PTR [esi+10h]
    mov ebx, eax
    sub eax, DWORD PTR [ebp+14h]
    jb dim_done
    mov _gBitmapRowSkip, eax
    mov eax, ebx
    mov ecx, DWORD PTR [ebp+10h]
    mul ecx
    mov edx, DWORD PTR [ebp+0ch]
    add eax, edx
    mov edi, DWORD PTR [esi+14h]
    add edi, eax
    mov esi, edi
    mov ebx, OFFSET _gDimPalette
    mov edx, _gBitmapRowSkip
dim_row:
    mov ecx, DWORD PTR [ebp+14h]
dim_pixel:
    lodsb
    xlat
    stosb
    loop dim_pixel
    add edi, edx
    add esi, edx
    dec DWORD PTR [ebp+18h]
    jne dim_row
dim_done:
    pop edi
    pop esi
    pop ebp
    ret
?DimBitmapArea@@YAXPAVbitmap@@HHHH@Z ENDP

?FillBitmapArea@@YAXPAVbitmap@@HHHHH@Z PROC NEAR
    push ebp
    mov ebp, esp
    push esi
    push edi
    mov esi, DWORD PTR [ebp+8]
    movzx eax, WORD PTR [esi+10h]
    mov ebx, eax
    sub eax, DWORD PTR [ebp+14h]
    jb fill_done
    mov _gBitmapRowSkip, eax
    mov eax, ebx
    mov ecx, DWORD PTR [ebp+10h]
    mul ecx
    mov edx, DWORD PTR [ebp+0ch]
    add eax, edx
    mov edi, DWORD PTR [esi+14h]
    add edi, eax
    mov ebx, _gBitmapRowSkip
    mov edx, DWORD PTR [ebp+18h]
    mov eax, DWORD PTR [ebp+1ch]
fill_row:
    mov ecx, DWORD PTR [ebp+14h]
    rep stosb
    add edi, ebx
    dec edx
    jne fill_row
fill_done:
    pop edi
    pop esi
    pop ebp
    ret
?FillBitmapArea@@YAXPAVbitmap@@HHHHH@Z ENDP

END
