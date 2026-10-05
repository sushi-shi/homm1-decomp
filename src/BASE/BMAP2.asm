; Bitmap rectangle primitives.

.386
.model flat
option casemap:none
option prologue:none
option epilogue:none

PUBLIC _gDimPalette

; Procedures start on even addresses: retail pads DimBitmapArea's odd end
; with one 90h before FillBitmapArea (in-module fill, not linker int3).

.data
; Dimming remap for 256 palette indices (retail 0x004a1aa0). It is the first
; datum of this module's .data: BMAP2's own skip words follow it at +100h.
_gDimPalette BYTE 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 00Ah, 010h, 011h, 012h, 013h, 014h
             BYTE 015h, 016h, 017h, 018h, 019h, 01Ah, 01Bh, 01Ch, 01Dh, 01Eh, 01Fh, 01Fh, 01Fh, 01Fh, 01Fh, 01Fh
             BYTE 025h, 026h, 027h, 028h, 029h, 02Ah, 02Bh, 02Ch, 02Dh, 02Eh, 02Fh, 030h, 031h, 032h, 033h, 034h
             BYTE 035h, 035h, 035h, 035h, 035h, 035h, 03Bh, 03Ch, 03Dh, 03Eh, 03Fh, 040h, 041h, 042h, 043h, 044h
             BYTE 045h, 046h, 047h, 048h, 049h, 04Ah, 04Bh, 04Ch, 04Dh, 04Dh, 04Dh, 04Dh, 04Dh, 04Dh, 053h, 054h
             BYTE 055h, 056h, 057h, 058h, 059h, 05Ah, 05Bh, 05Ch, 05Dh, 05Eh, 05Fh, 060h, 061h, 062h, 063h, 064h
             BYTE 065h, 065h, 065h, 065h, 065h, 065h, 06Bh, 06Ch, 06Dh, 06Eh, 06Fh, 070h, 071h, 072h, 073h, 074h
             BYTE 075h, 076h, 077h, 078h, 079h, 07Ah, 07Bh, 07Ch, 07Dh, 07Dh, 07Dh, 07Dh, 07Dh, 07Dh, 083h, 084h
             BYTE 085h, 086h, 087h, 088h, 089h, 08Ah, 08Bh, 08Ch, 08Dh, 08Eh, 08Fh, 090h, 091h, 092h, 093h, 094h
             BYTE 095h, 095h, 095h, 095h, 095h, 095h, 09Bh, 09Ch, 09Dh, 09Eh, 09Fh, 0A0h, 0A1h, 0A2h, 0A3h, 0A4h
             BYTE 0A5h, 0A6h, 0A7h, 0A8h, 0A9h, 0AAh, 0ABh, 0ACh, 0ADh, 0ADh, 0ADh, 0ADh, 0ADh, 0ADh, 0B3h, 0B4h
             BYTE 0B5h, 0B6h, 0B7h, 0B8h, 0B9h, 0BAh, 0BBh, 0BCh, 0BDh, 0BEh, 0BFh, 0C0h, 0C1h, 0C2h, 0C3h, 0C4h
             BYTE 0C5h, 0C5h, 0C5h, 0C5h, 0C5h, 0C5h, 0CBh, 0CCh, 0CDh, 0CEh, 0CFh, 0D0h, 0D1h, 0D2h, 0D3h, 0D4h
             BYTE 0D5h, 0D5h, 0D5h, 0D5h, 0D5h, 0D5h, 0BCh, 0BFh, 0C0h, 0C3h, 071h, 075h, 079h, 07Dh, 0E3h, 0E4h
             BYTE 0E5h, 0E5h, 0E5h, 0E5h, 0E5h, 0E5h, 047h, 048h, 049h, 04Ah, 04Bh, 03Fh, 042h, 03Dh, 045h, 049h
             BYTE 04Dh, 0F4h, 0F4h, 0F4h, 0F4h, 0F5h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
_gBitmapSourceSkip DWORD 0
_gBitmapRowSkip DWORD 0
; Twelve zero bytes that no retail code reads (0x004a15c0..0x004a15cb).
_gBitmapUnused DWORD 3 DUP (0)

.code

; All four are __cdecl C++ functions (include/BASE/bmap2.h). Row loops are
; do/while: callers pass height >= 1 (and width >= 1). A width wider than the
; bitmap's row returns without drawing.

; BlitBitmap in C++:
;
;   void BlitBitmap(bitmap* src, int sx, int sy, int w, int h,
;                   bitmap* dst, int dx, int dy) {
;       if ((gBitmapSourceSkip = src->m_width - w) < 0) return;
;       unsigned char* in = (unsigned char*)src->m_pixels + sy * src->m_width + sx;
;       if ((gBitmapRowSkip = dst->m_width - w) < 0) return;
;       unsigned char* out = (unsigned char*)dst->m_pixels + dy * dst->m_width + dx;
;       do {
;           memcpy(out, in, w);                          // w / 4 DWORDs, then w & 3 bytes
;           in += w + gBitmapSourceSkip;
;           out += w + gBitmapRowSkip;
;       } while (--h);
;   }
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

EVEN
; Overlap-safe copy inside one bitmap (no header declares it; no retail caller
; is known). Direction: rows moving down copy bottom-up and right to left.
; When the rows are equal, retail compares sx against dy (not dx).
;
;   void MoveBitmapArea(bitmap* bmp, int sx, int sy, int w, int h, int dx, int dy) {
;       int skip = bmp->m_width - w;
;       if (skip < 0) return;
;       gBitmapSourceSkip = skip;
;       bool backward = sy < dy || (sy == dy && sx < dy);
;       if (sy == dy && sx == dy) return;
;       unsigned char* base = (unsigned char*)bmp->m_pixels;
;       if (!backward) {
;           unsigned char* in = base + sy * bmp->m_width + sx;
;           unsigned char* out = base + dy * bmp->m_width + dx;
;           do {                                         // same row: byte copy
;               memcpy(out, in, w);                      // else DWORDs + tail
;               in += w + skip; out += w + skip;
;           } while (--h);
;       } else {                                         // STD
;           unsigned char* in = base + (sy + h - 1) * bmp->m_width + sx + w - 1;
;           unsigned char* out = base + (dy + h - 1) * bmp->m_width + dx + w - 1;
;           do {
;               for (int i = 0; i < w; ++i) *out-- = *in--;
;               in -= skip; out -= skip;
;           } while (--h);
;       }
;   }
;
; The backward different-row path moves w / 4 DWORDs first with ESI/EDI on
; the row's LAST byte, so for w >= 4 each DWORD reaches 3 bytes past the row
; end; the byte loop above is the w < 4 / same-row behaviour.
; @dead-code
; Zero-ref: no incoming call, jump or relocated reference in retail.
?MoveBitmapArea@@YAXPAVbitmap@@HHHHHH@Z PROC NEAR
    push ebp
    mov ebp, esp
    push esi
    push edi
    mov esi, DWORD PTR [ebp+8]
    movzx eax, WORD PTR [esi+10h]
    mov ebx, eax
    mov ecx, DWORD PTR [ebp+14h]
    sub eax, ecx
    js move_done
    mov _gBitmapSourceSkip, eax
    mov eax, DWORD PTR [ebp+10h]
    cmp eax, DWORD PTR [ebp+20h]
    jl move_backward
    jne move_forward
    mov eax, DWORD PTR [ebp+0ch]
    cmp eax, DWORD PTR [ebp+20h]
    jl move_backward
    je move_done
move_forward:
    mul ebx
    add eax, DWORD PTR [ebp+0ch]
    mov esi, DWORD PTR [esi+14h]
    mov edi, esi
    add esi, eax
    mov eax, DWORD PTR [ebp+20h]
    mul ebx
    add eax, DWORD PTR [ebp+1ch]
    add edi, eax
    mov eax, ecx
    mov ebx, _gBitmapSourceSkip
    mov edx, DWORD PTR [ebp+18h]
    cld
    mov ecx, DWORD PTR [ebp+10h]
    cmp ecx, DWORD PTR [ebp+20h]
    je forward_same_row
forward_row:
    mov ecx, eax
    shr ecx, 2
    rep movsd
    mov ecx, eax
    and ecx, 3
    rep movsb
    add esi, ebx
    add edi, ebx
    dec edx
    jne forward_row
move_done:
    pop edi
    pop esi
    pop ebp
    ret
forward_same_row:
    mov ecx, eax
    rep movsb
    add esi, ebx
    add edi, ebx
    dec edx
    jne forward_same_row
    pop edi
    pop esi
    pop ebp
    ret
move_backward:
    add eax, DWORD PTR [ebp+18h]
    dec eax
    mul ebx
    add eax, DWORD PTR [ebp+0ch]
    add eax, DWORD PTR [ebp+14h]
    dec eax
    mov esi, DWORD PTR [esi+14h]
    mov edi, esi
    add esi, eax
    mov eax, DWORD PTR [ebp+20h]
    add eax, DWORD PTR [ebp+18h]
    dec eax
    mul ebx
    add eax, DWORD PTR [ebp+1ch]
    add eax, DWORD PTR [ebp+14h]
    dec eax
    add edi, eax
    mov eax, ecx
    mov ebx, _gBitmapSourceSkip
    mov edx, DWORD PTR [ebp+18h]
    std
    mov ecx, DWORD PTR [ebp+10h]
    cmp ecx, DWORD PTR [ebp+20h]
    je backward_same_row
backward_row:
    mov ecx, eax
    shr ecx, 2
    rep movsd
    mov ecx, eax
    and ecx, 3
    rep movsb
    sub esi, ebx
    sub edi, ebx
    dec DWORD PTR [ebp+18h]
    jne backward_row
    cld
    pop edi
    pop esi
    pop ebp
    ret
backward_same_row:
    mov ecx, eax
    rep movsb
    sub esi, ebx
    sub edi, ebx
    dec edx
    jne backward_same_row
    cld
    pop edi
    pop esi
    pop ebp
    ret
?MoveBitmapArea@@YAXPAVbitmap@@HHHHHH@Z ENDP

EVEN
; DimBitmapArea in C++; the width check is unsigned (jb).
;
;   void DimBitmapArea(bitmap* bmp, int x, int y, int w, int h) {
;       if ((unsigned)bmp->m_width < (unsigned)w) return;
;       gBitmapRowSkip = bmp->m_width - w;
;       unsigned char* p = (unsigned char*)bmp->m_pixels + y * bmp->m_width + x;
;       do {
;           for (int i = 0; i < w; ++i, ++p)             // lodsb / xlat / stosb
;               *p = gDimPalette[*p];
;           p += gBitmapRowSkip;
;       } while (--h);
;   }
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

EVEN
; FillBitmapArea in C++:
;
;   void FillBitmapArea(bitmap* bmp, int x, int y, int w, int h, int color) {
;       if ((unsigned)bmp->m_width < (unsigned)w) return;
;       gBitmapRowSkip = bmp->m_width - w;
;       unsigned char* p = (unsigned char*)bmp->m_pixels + y * bmp->m_width + x;
;       do {
;           memset(p, (unsigned char)color, w);          // rep stosb
;           p += w + gBitmapRowSkip;
;       } while (--h);
;   }
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
