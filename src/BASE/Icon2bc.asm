; HoMM1 clipped icon renderers, plain and mirrored. Buka 2.1 icon2bc.cpp and
; iconf2bc.cpp supply the identities; HoMM1 retail is hand-written and links
; both as ONE object (a 90h EVEN pad separates them, not inter-object int3).

.386
.model flat
option casemap:none
option prologue:none
option epilogue:none

EXTERN _gIconXAdjust:DWORD
EXTERN _gIconYAdjust:DWORD
EXTERN _gIconDataBase:DWORD
EXTERN _gIconWidth:DWORD
EXTERN _gIconHeight:DWORD

.data
PUBLIC _gClipRowsLeft
PUBLIC _gClipLeftSkip
PUBLIC _gClipVisibleWidth
PUBLIC _gClipRowSkip
PUBLIC _gClipColumn
_gClipRowsLeft DWORD 0
_gClipLeftSkip DWORD 0
_gClipVisibleWidth DWORD 0
_gClipRowSkip DWORD 0
_gClipColumn DWORD 0

.code

; Draw one unscaled icon frame into a bitmap, clipped to the bitmap bounds.
; Buka 2.1 icon2bc.cpp supplies the identity; HoMM1 retail is hand-written.
;
; C++ equivalent (include/BASE/Icon2b.h, __cdecl). The frame setup and RLE
; are Icon2b.asm's. Rows above the bitmap are skipped by scanning the source
; for its row-end 0 byte (a 0 inside a literal run would end the row early).
; gClipColumn is not cleared on entry; every row end clears it. "visible" ==
; 0 means "no right clip". Column bookkeeping is retail's, quirks included:
; a skip run that straddles the left edge adds only its visible part.
;
;   void ClippedIconToBitmap(icon* ic, bitmap* bmp, int x, int y, int frame, int offsetMode) {
;       /* frame setup (Icon2b.asm) without the x/y clamps */
;       int w = gIconWidth, h = gIconHeight;
;       x += gIconXAdjust;
;       if (x < 0) { gClipLeftSkip = -x; w -= gClipLeftSkip; x = 0; }
;       else gClipLeftSkip = 0;
;       y += gIconYAdjust;
;       if (y < 0) {
;           if ((h += y) <= 0) return;
;           for (int rows = -y; rows; --rows) while (*src++ != 0) {}
;           y = 0;
;       }
;       if (bmp->m_height <= y) return;
;       gClipRowsLeft = (y + h > bmp->m_height) ? bmp->m_height - y : h;
;       if (bmp->m_width <= x) return;
;       gClipVisibleWidth = (x + w > bmp->m_width) ? bmp->m_width - x : 0;
;       unsigned char* rowStart = (unsigned char*)bmp->m_pixels + y * bmp->m_width + x;
;       unsigned char* out = rowStart;
;       gClipRowSkip = gClipLeftSkip;                    // left pixels still to drop
;       for (;;) {
;           int b = *src++;
;           if (b == 0) {                                // next row
;       next_row:
;               out = rowStart += bmp->m_width;
;               gClipColumn = 0;  gClipRowSkip = gClipLeftSkip;
;               if (--gClipRowsLeft == 0) return;
;           } else if (b & 0x80) {                       // transparent run
;               if (!(b &= 0x7f)) return;
;               if (gClipLeftSkip) {
;                   if (gClipRowSkip >= b) { gClipRowSkip -= b; gClipColumn += b; continue; }
;                   b -= gClipRowSkip;  gClipRowSkip = 0;
;                   out += b;  gClipColumn += b;  continue;
;               }
;               if (gClipVisibleWidth && gClipVisibleWidth - gClipColumn - b < 0) {
;                   while (*src++ != 0) {}  goto next_row;
;               }
;               out += b;  gClipColumn += b;
;           } else {                                     // b literal pixels
;               if (gClipLeftSkip) {
;                   if (gClipRowSkip >= b) {
;                       src += b; gClipColumn += b; gClipRowSkip -= b; continue;
;                   }
;                   src += gClipRowSkip; b -= gClipRowSkip;
;                   gClipColumn += gClipRowSkip; gClipRowSkip = 0;
;               }
;               if (gClipVisibleWidth) {
;                   int room = gClipVisibleWidth - gClipColumn;
;                   if (room == 0 || room - b < 0) {         // right edge reached
;                       if (room) { gClipColumn += room; memcpy(out, src, room); src += room; }
;                       while (*src++ != 0) {}  goto next_row;
;                   }
;               }
;               gClipColumn += b;
;               memcpy(out, src, b); out += b; src += b;     // DWORDs + tail
;           }
;       }
;   }
?ClippedIconToBitmap@@YAXPAVicon@@PAVbitmap@@HHHH@Z PROC NEAR
    push ebp
    mov ebp, esp
    push esi
    push edi
    mov eax, DWORD PTR [ebp+18h]
    mov ebx, 0ch
    mul ebx
    mov esi, DWORD PTR [ebp+8h]
    mov esi, DWORD PTR [esi+10h]
    mov DWORD PTR _gIconDataBase, esi
    add esi, eax
    movsx eax, WORD PTR [esi]
    test DWORD PTR [ebp+1ch], 0ffffffffh
    je clip_7ca77
    mov ebx, eax
    sar eax, 1
    sub ebx, eax
    sar ebx, 1
    sub eax, ebx
clip_7ca77:
    mov _gIconXAdjust, eax
    movsx ebx, WORD PTR [esi+2h]
    mov DWORD PTR _gIconYAdjust, ebx
    movzx ecx, WORD PTR [esi+4h]
    mov DWORD PTR _gIconWidth, ecx
    movzx edx, WORD PTR [esi+6h]
    mov DWORD PTR _gIconHeight, edx
    mov esi, DWORD PTR [esi+8h]
    add esi, DWORD PTR _gIconDataBase
    mov edi, DWORD PTR [ebp+0ch]
    add eax, DWORD PTR [ebp+10h]
    jns clip_7cabd
    neg eax
    mov _gClipLeftSkip, eax
    sub ecx, eax
    mov DWORD PTR [ebp+10h], 0h
    jmp clip_7caca
clip_7cabd:
    mov DWORD PTR _gClipLeftSkip, 0h
    mov DWORD PTR [ebp+10h], eax
clip_7caca:
    add ebx, DWORD PTR [ebp+14h]
    jns clip_7cae2
    neg ebx
    sub edx, ebx
    jle clip_7cc9f
    cld
clip_7cada:
    lodsb
    test al, 0ffh
    jne clip_7cada
    dec ebx
    jne clip_7cada
clip_7cae2:
    movzx eax, WORD PTR [edi+12h]
    cmp eax, ebx
    jle clip_7cc9f
    mov DWORD PTR [ebp+14h], ebx
    add ebx, edx
    sub ebx, eax
    jle clip_7cb03
    neg ebx
    add ebx, edx
    mov DWORD PTR _gClipRowsLeft, ebx
    jmp clip_7cb09
clip_7cb03:
    mov DWORD PTR _gClipRowsLeft, edx
clip_7cb09:
    mov eax, DWORD PTR [ebp+10h]
    movzx ebx, WORD PTR [edi+10h]
    cmp ebx, eax
    jle clip_7cc9f
    add eax, ecx
    sub eax, ebx
    jle clip_7cb29
    neg eax
    add eax, ecx
    mov _gClipVisibleWidth, eax
    jmp clip_7cb33
clip_7cb29:
    mov DWORD PTR _gClipVisibleWidth, 0h
clip_7cb33:
    movzx eax, WORD PTR [edi+10h]
    mov ebx, eax
    mov ecx, DWORD PTR [ebp+14h]
    mul ecx
    mov edx, DWORD PTR [ebp+10h]
    add eax, edx
    mov edi, DWORD PTR [edi+14h]
    add edi, eax
    mov edx, edi
    mov eax, _gClipLeftSkip
    mov _gClipRowSkip, eax
; Run loop: 0 = next row, 80h|n = n transparent (80h ends), n = n literal
; bytes; gClipRowSkip drops the left-clipped pixels of each row first.
clip_7cb54:
    xor eax, eax
    lodsb
    or al, al
    js clip_7cc22
    je clip_7cbf9
    test DWORD PTR _gClipLeftSkip, 0ffffffffh
    je clip_7cba3
    cmp DWORD PTR _gClipRowSkip, eax
    js clip_7cb89
    add esi, eax
    add DWORD PTR _gClipColumn, eax
    sub DWORD PTR _gClipRowSkip, eax
    jmp clip_7cb54
clip_7cb89:
    mov ecx, DWORD PTR _gClipRowSkip
    add esi, ecx
    sub eax, ecx
    add DWORD PTR _gClipColumn, ecx
    mov DWORD PTR _gClipRowSkip, 0h
clip_7cba3:
    test DWORD PTR _gClipVisibleWidth, 0ffffffffh
    je clip_7cbe0
    mov ecx, DWORD PTR _gClipVisibleWidth
    sub ecx, DWORD PTR _gClipColumn
    je clip_7cbd7
    sub ecx, eax
    jns clip_7cbde
    add eax, ecx
    add DWORD PTR _gClipColumn, eax
    mov ecx, eax
    shr ecx, 2h
    rep movsd
    mov ecx, eax
    and ecx, 3h
    rep movsb
clip_7cbd7:
    lodsb
    test al, 0ffh
    jne clip_7cbd7
    jmp clip_7cbf9
clip_7cbde:
    xor ecx, ecx
clip_7cbe0:
    add DWORD PTR _gClipColumn, eax
    mov ecx, eax
    shr ecx, 2h
    rep movsd
    mov ecx, eax
    and ecx, 3h
    rep movsb
    jmp clip_7cb54
clip_7cbf9:
    add edx, ebx
    mov edi, edx
    mov DWORD PTR _gClipColumn, 0h
    mov eax, _gClipLeftSkip
    mov _gClipRowSkip, eax
    dec DWORD PTR _gClipRowsLeft
    je clip_7cc9f
    jmp clip_7cb54
clip_7cc22:
    and al, 7fh
    je clip_7cc9f
    test DWORD PTR _gClipLeftSkip, 0ffffffffh
    je clip_7cc6a
    cmp DWORD PTR _gClipRowSkip, eax
    js clip_7cc4b
    sub DWORD PTR _gClipRowSkip, eax
    add DWORD PTR _gClipColumn, eax
    jmp clip_7cb54
clip_7cc4b:
    mov ecx, DWORD PTR _gClipRowSkip
    sub eax, ecx
    add edi, eax
    add DWORD PTR _gClipColumn, eax
    mov DWORD PTR _gClipRowSkip, 0h
    jmp clip_7cb54
clip_7cc6a:
    test DWORD PTR _gClipVisibleWidth, 0ffffffffh
    je clip_7cc92
    mov ecx, DWORD PTR _gClipVisibleWidth
    sub ecx, DWORD PTR _gClipColumn
    sub ecx, eax
    jns clip_7cc90
clip_7cc86:
    lodsb
    test al, 0ffh
    jne clip_7cc86
    jmp clip_7cbf9
clip_7cc90:
    xor ecx, ecx
clip_7cc92:
    add edi, eax
    add DWORD PTR _gClipColumn, eax
    jmp clip_7cb54
clip_7cc9f:
    pop edi
    pop esi
    pop ebp
    ret
?ClippedIconToBitmap@@YAXPAVicon@@PAVbitmap@@HHHH@Z ENDP

EVEN
; Draw one unscaled icon frame mirrored horizontally, clipped to the bitmap.
; Buka 2.1 iconf2bc.cpp supplies the identity; HoMM1 retail is hand-written.
;
; C++ equivalent: ClippedIconToBitmap with the frame drawn right to left from
; x (its right edge). Only the horizontal setup and the write direction
; differ; the rows above the bitmap, gClipRowsLeft and the RLE/clip loop are
; the same, with "out += n" / literal copies replaced by "out -= n" /
; "*out-- = *src++". Pixels beyond the right edge play gClipLeftSkip's role:
;
;   x -= gIconXAdjust;
;   int W = bmp->m_width;
;   if (x < 0 || x >= W - 1) {                       // unsigned "x + 1 - W" carry test
;       gClipLeftSkip = x - (W - 1);  w -= gClipLeftSkip;  x = W - 1;
;   } else
;       gClipLeftSkip = 0;
;   /* y clip as ClippedIconToBitmap */
;   if (x < 0) return;
;   gClipVisibleWidth = (x - w + 1 < 0) ? x + 1 : 0;  // pixels left of x, if clipped
?FlipClippedIconToBitmap@@YAXPAVicon@@PAVbitmap@@HHHH@Z PROC NEAR
    push ebp
    mov ebp, esp
    push esi
    push edi
    mov eax, DWORD PTR [ebp+18h]
    mov ebx, 0ch
    mul ebx
    mov esi, DWORD PTR [ebp+8h]
    mov esi, DWORD PTR [esi+10h]
    mov DWORD PTR _gIconDataBase, esi
    add esi, eax
    movsx eax, WORD PTR [esi]
    test DWORD PTR [ebp+1ch], 0ffffffffh
    je flipclip_7ccd7
    mov ebx, eax
    sar eax, 1
    sub ebx, eax
    sar ebx, 1
    sub eax, ebx
flipclip_7ccd7:
    mov _gIconXAdjust, eax
    movsx ebx, WORD PTR [esi+2h]
    mov DWORD PTR _gIconYAdjust, ebx
    movzx ecx, WORD PTR [esi+4h]
    mov DWORD PTR _gIconWidth, ecx
    movzx edx, WORD PTR [esi+6h]
    mov DWORD PTR _gIconHeight, edx
    mov esi, DWORD PTR [esi+8h]
    add esi, DWORD PTR _gIconDataBase
    mov edi, DWORD PTR [ebp+0ch]
    sub DWORD PTR [ebp+10h], eax
    movzx eax, WORD PTR [edi+10h]
    neg eax
    inc eax
    push eax
    add eax, DWORD PTR [ebp+10h]
    ja flipclip_7cd25
    mov _gClipLeftSkip, eax
    sub ecx, eax
    pop eax
    neg eax
    mov DWORD PTR [ebp+10h], eax
    jmp flipclip_7cd30
flipclip_7cd25:
    pop eax
    mov DWORD PTR _gClipLeftSkip, 0h
flipclip_7cd30:
    add ebx, DWORD PTR [ebp+14h]
    jns flipclip_7cd48
    neg ebx
    sub edx, ebx
    jle flipclip_7cef8
    cld
flipclip_7cd40:
    lodsb
    test al, 0ffh
    jne flipclip_7cd40
    dec ebx
    jne flipclip_7cd40
flipclip_7cd48:
    movzx eax, WORD PTR [edi+12h]
    cmp eax, ebx
    jle flipclip_7cef8
    mov DWORD PTR [ebp+14h], ebx
    add ebx, edx
    sub ebx, eax
    jle flipclip_7cd69
    neg ebx
    add ebx, edx
    mov DWORD PTR _gClipRowsLeft, ebx
    jmp flipclip_7cd6f
flipclip_7cd69:
    mov DWORD PTR _gClipRowsLeft, edx
flipclip_7cd6f:
    mov eax, DWORD PTR [ebp+10h]
    movzx ebx, WORD PTR [edi+10h]
    and eax, eax
    js flipclip_7cef8
    sub eax, ecx
    inc eax
    jns flipclip_7cd8c
    add eax, ecx
    mov _gClipVisibleWidth, eax
    jmp flipclip_7cd96
flipclip_7cd8c:
    mov DWORD PTR _gClipVisibleWidth, 0h
flipclip_7cd96:
    movzx eax, WORD PTR [edi+10h]
    mov ebx, eax
    mov ecx, DWORD PTR [ebp+14h]
    mul ecx
    mov edx, DWORD PTR [ebp+10h]
    add eax, edx
    mov edi, DWORD PTR [edi+14h]
    add edi, eax
    mov edx, edi
    mov eax, _gClipLeftSkip
    mov _gClipRowSkip, eax
; Run loop: as ClippedIconToBitmap's, writing leftwards.
flipclip_7cdb7:
    xor eax, eax
    lodsb
    or al, al
    js flipclip_7ce7b
    je flipclip_7ce52
    test DWORD PTR _gClipLeftSkip, 0ffffffffh
    je flipclip_7ce06
    cmp DWORD PTR _gClipRowSkip, eax
    js flipclip_7cdec
    add esi, eax
    add DWORD PTR _gClipColumn, eax
    sub DWORD PTR _gClipRowSkip, eax
    jmp flipclip_7cdb7
flipclip_7cdec:
    mov ecx, DWORD PTR _gClipRowSkip
    add esi, ecx
    sub eax, ecx
    add DWORD PTR _gClipColumn, ecx
    mov DWORD PTR _gClipRowSkip, 0h
flipclip_7ce06:
    test DWORD PTR _gClipVisibleWidth, 0ffffffffh
    je flipclip_7ce3e
    mov ecx, DWORD PTR _gClipVisibleWidth
    sub ecx, DWORD PTR _gClipColumn
    je flipclip_7ce35
    sub ecx, eax
    jns flipclip_7ce3c
    add eax, ecx
    add DWORD PTR _gClipColumn, eax
    mov ecx, eax
flipclip_7ce2e:
    lodsb
    mov BYTE PTR es:[edi], al
    dec edi
    loop flipclip_7ce2e
flipclip_7ce35:
    lodsb
    test al, 0ffh
    jne flipclip_7ce35
    jmp flipclip_7ce52
flipclip_7ce3c:
    xor ecx, ecx
flipclip_7ce3e:
    add DWORD PTR _gClipColumn, eax
    mov ecx, eax
flipclip_7ce46:
    lodsb
    mov BYTE PTR es:[edi], al
    dec edi
    loop flipclip_7ce46
    jmp flipclip_7cdb7
flipclip_7ce52:
    add edx, ebx
    mov edi, edx
    mov DWORD PTR _gClipColumn, 0h
    mov eax, _gClipLeftSkip
    mov _gClipRowSkip, eax
    dec DWORD PTR _gClipRowsLeft
    je flipclip_7cef8
    jmp flipclip_7cdb7
flipclip_7ce7b:
    and al, 7fh
    je flipclip_7cef8
    test DWORD PTR _gClipLeftSkip, 0ffffffffh
    je flipclip_7cec3
    cmp DWORD PTR _gClipRowSkip, eax
    js flipclip_7cea4
    sub DWORD PTR _gClipRowSkip, eax
    add DWORD PTR _gClipColumn, eax
    jmp flipclip_7cdb7
flipclip_7cea4:
    mov ecx, DWORD PTR _gClipRowSkip
    sub eax, ecx
    sub edi, eax
    add DWORD PTR _gClipColumn, eax
    mov DWORD PTR _gClipRowSkip, 0h
    jmp flipclip_7cdb7
flipclip_7cec3:
    test DWORD PTR _gClipVisibleWidth, 0ffffffffh
    je flipclip_7ceeb
    mov ecx, DWORD PTR _gClipVisibleWidth
    sub ecx, DWORD PTR _gClipColumn
    sub ecx, eax
    jns flipclip_7cee9
flipclip_7cedf:
    lodsb
    test al, 0ffh
    jne flipclip_7cedf
    jmp flipclip_7ce52
flipclip_7cee9:
    xor ecx, ecx
flipclip_7ceeb:
    sub edi, eax
    add DWORD PTR _gClipColumn, eax
    jmp flipclip_7cdb7
flipclip_7cef8:
    pop edi
    pop esi
    pop ebp
    ret
?FlipClippedIconToBitmap@@YAXPAVicon@@PAVbitmap@@HHHH@Z ENDP

END
