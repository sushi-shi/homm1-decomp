; HoMM1 unscaled icon renderers: plain, mirrored, monochrome, mirrored
; monochrome, dimmed and mirrored dimmed. Retail links them as ONE object:
; the procedures are separated by a single 90h (EVEN) instead of the int3
; fill LINK puts between objects, they share the frame variables below, and
; VC4 LINK pulls the group from the BASE library at IconToBitmap's slot.

.386
.model flat
option casemap:none
option prologue:none
option epilogue:none

EXTERN _gDimPalette:BYTE

.data
PUBLIC _gIconXAdjust
PUBLIC _gIconYAdjust
PUBLIC _gIconDataBase
PUBLIC _gIconWidth
PUBLIC _gIconHeight
_gIconXAdjust DWORD 0
_gIconYAdjust DWORD 0
_gIconDataBase DWORD 0
_gIconWidth DWORD 0
_gIconHeight DWORD 0
; A sixth frame variable that no retail code reads (0x004a1498).
_gIconUnused DWORD 0

.code

; C++ equivalents (include/BASE/Icon2b.h, Iconm2b.h, Icond2b.h; all __cdecl).
; Every routine opens with the same frame setup; offsetMode is the last
; argument (IconDrawOffsetMode) and quarters the frame's x offset:
;
;   IconEntry* e = (IconEntry*)ic->m_data + frame;       // 12-byte entries
;   gIconDataBase = ic->m_data;
;   int xAdjust = e->x;
;   if (offsetMode) {
;       int half = xAdjust >> 1;                         // arithmetic shifts
;       xAdjust = half - ((xAdjust - half) >> 1);        // about x / 4
;   }
;   gIconXAdjust = xAdjust;  gIconYAdjust = e->y;
;   gIconWidth = (unsigned short)e->w;  gIconHeight = (unsigned short)e->h;
;   unsigned char* src = gIconDataBase + e->srcOffset;
;   x += xAdjust;  if (x < 0) x = 0;                    // Flip*: x -= xAdjust
;   y += gIconYAdjust;  if (y < 0) y = 0;
;
; The frame is run-length coded per row. A byte b means:
;   b == 0          next row: rowStart += pitch, out = rowStart
;   b & 0x80        skip (b & 0x7f) pixels; b == 0x80 ends the frame
;   otherwise       b pixels (colour icons: b literal bytes follow in src;
;                   Mono/Dim: no payload, the count alone is the shape)

; Draw one unscaled icon frame into a bitmap.
;
;   void IconToBitmap(icon* ic, bitmap* bmp, int x, int y, int frame, int offsetMode) {
;       /* frame setup */
;       if ((short)(x + gIconWidth) > bmp->m_width) return;
;       if ((short)(y + gIconHeight) > bmp->m_height) return;
;       unsigned char* rowStart = (unsigned char*)bmp->m_pixels + y * bmp->m_width + x;
;       unsigned char* out = rowStart;
;       for (;;) {
;           unsigned char b = *src++;
;           if (b & 0x80) { if (!(b & 0x7f)) return; out += b & 0x7f; }
;           else if (b == 0) out = rowStart += bmp->m_width;
;           else { memcpy(out, src, b); out += b; src += b; }   // DWORDs + tail
;       }
;   }
?IconToBitmap@@YAXPAVicon@@PAVbitmap@@HHHH@Z PROC NEAR
    push ebp
    mov ebp, esp
    push esi
    push edi
    mov eax, DWORD PTR [ebp+18h]
    mov ebx, 0ch
    mul ebx
    mov esi, DWORD PTR [ebp+8]
    mov esi, DWORD PTR [esi+10h]
    mov _gIconDataBase, esi
    add esi, eax
    movsx eax, WORD PTR [esi]
    test DWORD PTR [ebp+1ch], 0ffffffffh
    je icon_adjusted
    mov ebx, eax
    sar eax, 1
    sub ebx, eax
    sar ebx, 1
    sub eax, ebx
icon_adjusted:
    mov _gIconXAdjust, eax
    movsx ebx, WORD PTR [esi+2]
    mov _gIconYAdjust, ebx
    movzx ecx, WORD PTR [esi+4]
    mov _gIconWidth, ecx
    movzx edx, WORD PTR [esi+6]
    mov _gIconHeight, edx
    mov esi, DWORD PTR [esi+8]
    add esi, _gIconDataBase
    add DWORD PTR [ebp+10h], eax
    jns icon_x_nonnegative
    mov DWORD PTR [ebp+10h], 0
icon_x_nonnegative:
    add DWORD PTR [ebp+14h], ebx
    jns icon_y_nonnegative
    mov DWORD PTR [ebp+14h], 0
icon_y_nonnegative:
    mov edi, DWORD PTR [ebp+0ch]
    mov eax, DWORD PTR [ebp+10h]
    add eax, ecx
    cmp ax, WORD PTR [edi+10h]
    jg icon_done
    mov eax, DWORD PTR [ebp+14h]
    add eax, edx
    cmp ax, WORD PTR [edi+12h]
    jg icon_done
    cld
    movzx eax, WORD PTR [edi+10h]
    mov ebx, eax
    mov ecx, DWORD PTR [ebp+14h]
    mul ecx
    mov edx, DWORD PTR [ebp+10h]
    add eax, edx
    mov edi, DWORD PTR [edi+14h]
    add edi, eax
    mov edx, edi
    xor eax, eax
    xor ecx, ecx
icon_next_run:
    lodsb
    or al, al
    js icon_skip_run
    je icon_next_row
    mov cl, al
    shr cl, 2
    rep movsd
    mov cl, al
    and cl, 3
    rep movsb
    jmp icon_next_run
icon_next_row:
    add edx, ebx
    mov edi, edx
    jmp icon_next_run
icon_skip_run:
    and al, 7fh
    je icon_done
    add edi, eax
    jmp icon_next_run
icon_done:
    pop edi
    pop esi
    pop ebp
    ret
?IconToBitmap@@YAXPAVicon@@PAVbitmap@@HHHH@Z ENDP

EVEN
; Draw one unscaled icon frame, mirrored horizontally: x is the frame's
; right edge, rows are written right to left.
;
;   void FlipIconToBitmap(icon* ic, bitmap* bmp, int x, int y, int frame, int offsetMode) {
;       /* frame setup, with x -= xAdjust */
;       if (x - gIconWidth + 1 < 0) return;
;       if ((short)(y + gIconHeight) > bmp->m_height) return;
;       unsigned char* rowStart = (unsigned char*)bmp->m_pixels + y * bmp->m_width + x;
;       unsigned char* out = rowStart;
;       for (;;) {
;           unsigned char b = *src++;
;           if (b & 0x80) { if (!(b & 0x7f)) return; out -= b & 0x7f; }
;           else if (b == 0) out = rowStart += bmp->m_width;
;           else while (b--) *out-- = *src++;
;       }
;   }
?FlipIconToBitmap@@YAXPAVicon@@PAVbitmap@@HHHH@Z PROC NEAR
    push ebp
    mov ebp, esp
    push esi
    push edi
    mov eax, DWORD PTR [ebp+18h]
    mov ebx, 0ch
    mul ebx
    mov esi, DWORD PTR [ebp+8]
    mov esi, DWORD PTR [esi+10h]
    mov _gIconDataBase, esi
    add esi, eax
    movsx eax, WORD PTR [esi]
    test DWORD PTR [ebp+1ch], 0ffffffffh
    je flip_icon_adjusted
    mov ebx, eax
    sar eax, 1
    sub ebx, eax
    sar ebx, 1
    sub eax, ebx
flip_icon_adjusted:
    mov _gIconXAdjust, eax
    movsx ebx, WORD PTR [esi+2]
    mov _gIconYAdjust, ebx
    movzx ecx, WORD PTR [esi+4]
    mov _gIconWidth, ecx
    movzx edx, WORD PTR [esi+6]
    mov _gIconHeight, edx
    mov esi, DWORD PTR [esi+8]
    add esi, _gIconDataBase
    sub DWORD PTR [ebp+10h], eax
    jns flip_icon_x_nonnegative
    mov DWORD PTR [ebp+10h], 0
flip_icon_x_nonnegative:
    add DWORD PTR [ebp+14h], ebx
    jns flip_icon_y_nonnegative
    mov DWORD PTR [ebp+14h], 0
flip_icon_y_nonnegative:
    mov edi, DWORD PTR [ebp+0ch]
    mov eax, DWORD PTR [ebp+10h]
    sub eax, ecx
    add eax, 1
    js flip_icon_done
    mov eax, DWORD PTR [ebp+14h]
    add eax, edx
    cmp ax, WORD PTR [edi+12h]
    jg flip_icon_done
    cld
    movzx eax, WORD PTR [edi+10h]
    mov ebx, eax
    mul DWORD PTR [ebp+14h]
    add eax, DWORD PTR [ebp+10h]
    mov edi, DWORD PTR [edi+14h]
    add edi, eax
    mov edx, edi
    xor eax, eax
    xor ecx, ecx
flip_icon_next_run:
    lodsb
    or al, al
    js flip_icon_skip_run
    je flip_icon_next_row
    mov cl, al
flip_icon_copy_pixel:
    lodsb
    mov BYTE PTR es:[edi], al
    dec edi
    loop flip_icon_copy_pixel
    jmp flip_icon_next_run
flip_icon_skip_run:
    and al, 7fh
    je flip_icon_done
    sub edi, eax
    jmp flip_icon_next_run
flip_icon_next_row:
    add edx, ebx
    mov edi, edx
    jmp flip_icon_next_run
flip_icon_done:
    pop edi
    pop esi
    pop ebp
    ret
?FlipIconToBitmap@@YAXPAVicon@@PAVbitmap@@HHHH@Z ENDP

EVEN
; Draw one unscaled icon frame using a single color.
;
;   void MonoIconToBitmap(icon* ic, bitmap* bmp, int x, int y, int frame,
;                         int color, int offsetMode) {
;       /* frame setup and the two bounds checks of IconToBitmap */
;       for (;;) {
;           unsigned char b = *src++;
;           if (b & 0x80) { if (!(b & 0x7f)) return; out += b & 0x7f; }
;           else if (b == 0) out = rowStart += bmp->m_width;
;           else { memset(out, (unsigned char)color, b); out += b; }
;       }
;   }
?MonoIconToBitmap@@YAXPAVicon@@PAVbitmap@@HHHHH@Z PROC NEAR
    push ebp
    mov ebp, esp
    push esi
    push edi
    mov eax, DWORD PTR [ebp+18h]
    mov ebx, 0ch
    mul ebx
    mov esi, DWORD PTR [ebp+8]
    mov esi, DWORD PTR [esi+10h]
    mov _gIconDataBase, esi
    add esi, eax
    movsx eax, WORD PTR [esi]
    test DWORD PTR [ebp+20h], 0ffffffffh
    je mono_icon_adjusted
    mov ebx, eax
    sar eax, 1
    sub ebx, eax
    sar ebx, 1
    sub eax, ebx
mono_icon_adjusted:
    mov _gIconXAdjust, eax
    movsx ebx, WORD PTR [esi+2]
    mov _gIconYAdjust, ebx
    movzx ecx, WORD PTR [esi+4]
    mov _gIconWidth, ecx
    movzx edx, WORD PTR [esi+6]
    mov _gIconHeight, edx
    mov esi, DWORD PTR [esi+8]
    add esi, _gIconDataBase
    add DWORD PTR [ebp+10h], eax
    jns mono_icon_x_nonnegative
    mov DWORD PTR [ebp+10h], 0
mono_icon_x_nonnegative:
    add DWORD PTR [ebp+14h], ebx
    jns mono_icon_y_nonnegative
    mov DWORD PTR [ebp+14h], 0
mono_icon_y_nonnegative:
    mov edi, DWORD PTR [ebp+0ch]
    mov eax, DWORD PTR [ebp+10h]
    add eax, ecx
    cmp ax, WORD PTR [edi+10h]
    jg mono_icon_done
    mov eax, DWORD PTR [ebp+14h]
    add eax, edx
    cmp ax, WORD PTR [edi+12h]
    jg mono_icon_done
    cld
    movzx eax, WORD PTR [edi+10h]
    mov ebx, eax
    mov ecx, DWORD PTR [ebp+14h]
    mul ecx
    mov edx, DWORD PTR [ebp+10h]
    add eax, edx
    mov edi, DWORD PTR [edi+14h]
    add edi, eax
    mov edx, edi
    xor eax, eax
    xor ecx, ecx
    and DWORD PTR [ebp+1ch], 0ffh
mono_icon_next_run:
    lodsb
    or al, al
    js mono_icon_skip_run
    je mono_icon_next_row
    mov cl, al
    mov eax, DWORD PTR [ebp+1ch]
    rep stosb
    jmp mono_icon_next_run
mono_icon_next_row:
    add edx, ebx
    mov edi, edx
    jmp mono_icon_next_run
mono_icon_skip_run:
    and al, 7fh
    je mono_icon_done
    add edi, eax
    jmp mono_icon_next_run
mono_icon_done:
    pop edi
    pop esi
    pop ebp
    ret
?MonoIconToBitmap@@YAXPAVicon@@PAVbitmap@@HHHHH@Z ENDP

EVEN
; Draw one unscaled monochrome icon frame, mirrored horizontally.
;
;   void FlipMonoIconToBitmap(icon* ic, bitmap* bmp, int x, int y, int frame,
;                             int color, int offsetMode) {
;       /* frame setup, with x -= xAdjust */
;       if (x - gIconWidth < 0) return;                  // no +1, unlike FlipIcon
;       if ((short)(y + gIconHeight) > bmp->m_height) return;
;       for (;;) {                                       // run fills use STD
;           unsigned char b = *src++;
;           if (b & 0x80) { if (!(b & 0x7f)) return; out -= b & 0x7f; }
;           else if (b == 0) out = rowStart += bmp->m_width;
;           else while (b--) *out-- = (unsigned char)color;
;       }
;   }
?FlipMonoIconToBitmap@@YAXPAVicon@@PAVbitmap@@HHHHH@Z PROC NEAR
    push ebp
    mov ebp, esp
    push esi
    push edi
    mov eax, DWORD PTR [ebp+18h]
    mov ebx, 0ch
    mul ebx
    mov esi, DWORD PTR [ebp+8]
    mov esi, DWORD PTR [esi+10h]
    mov _gIconDataBase, esi
    add esi, eax
    movsx eax, WORD PTR [esi]
    test DWORD PTR [ebp+20h], 0ffffffffh
    je flip_mono_adjusted
    mov ebx, eax
    sar eax, 1
    sub ebx, eax
    sar ebx, 1
    sub eax, ebx
flip_mono_adjusted:
    mov _gIconXAdjust, eax
    movsx ebx, WORD PTR [esi+2]
    mov _gIconYAdjust, ebx
    movzx ecx, WORD PTR [esi+4]
    mov _gIconWidth, ecx
    movzx edx, WORD PTR [esi+6]
    mov _gIconHeight, edx
    mov esi, DWORD PTR [esi+8]
    add esi, _gIconDataBase
    sub DWORD PTR [ebp+10h], eax
    jns flip_mono_x_nonnegative
    mov DWORD PTR [ebp+10h], 0
flip_mono_x_nonnegative:
    add DWORD PTR [ebp+14h], ebx
    jns flip_mono_y_nonnegative
    mov DWORD PTR [ebp+14h], 0
flip_mono_y_nonnegative:
    mov edi, DWORD PTR [ebp+0ch]
    mov eax, DWORD PTR [ebp+10h]
    sub eax, ecx
    js flip_mono_done
    mov eax, DWORD PTR [ebp+14h]
    add eax, edx
    cmp ax, WORD PTR [edi+12h]
    jg flip_mono_done
    cld
    movzx eax, WORD PTR [edi+10h]
    mov ebx, eax
    mov ecx, DWORD PTR [ebp+14h]
    mul ecx
    mov edx, DWORD PTR [ebp+10h]
    add eax, edx
    mov edi, DWORD PTR [edi+14h]
    add edi, eax
    mov edx, edi
    xor eax, eax
    xor ecx, ecx
    and DWORD PTR [ebp+1ch], 0ffh
flip_mono_next_run:
    cld
    lodsb
    or al, al
    js flip_mono_skip_run
    je flip_mono_next_row
    mov cl, al
    mov eax, DWORD PTR [ebp+1ch]
    std
    rep stosb
    jmp flip_mono_next_run
flip_mono_next_row:
    add edx, ebx
    mov edi, edx
    jmp flip_mono_next_run
flip_mono_skip_run:
    and al, 7fh
    je flip_mono_done
    sub edi, eax
    jmp flip_mono_next_run
flip_mono_done:
    pop edi
    pop esi
    pop ebp
    ret
?FlipMonoIconToBitmap@@YAXPAVicon@@PAVbitmap@@HHHHH@Z ENDP

EVEN
; Dim the destination through the shape of one unscaled icon frame. The row
; step is the screen pitch 640 (280h), not bmp->m_width.
;
;   void DimIconToBitmap(icon* ic, bitmap* bmp, int x, int y, int frame, int offsetMode) {
;       /* frame setup and the two bounds checks of IconToBitmap */
;       for (;;) {
;           unsigned char b = *src++;
;           if (b & 0x80) { if (!(b & 0x7f)) return; out += b & 0x7f; }
;           else if (b == 0) out = rowStart += 640;
;           else for (; b; --b, ++out) *out = gDimPalette[*out];
;       }
;   }
?DimIconToBitmap@@YAXPAVicon@@PAVbitmap@@HHHH@Z PROC NEAR
    push ebp
    mov ebp, esp
    push esi
    push edi
    mov eax, DWORD PTR [ebp+18h]
    mov ebx, 0ch
    mul ebx
    mov esi, DWORD PTR [ebp+8]
    mov esi, DWORD PTR [esi+10h]
    mov _gIconDataBase, esi
    add esi, eax
    movsx eax, WORD PTR [esi]
    test DWORD PTR [ebp+1ch], 0ffffffffh
    je dim_icon_adjusted
    mov ebx, eax
    sar eax, 1
    sub ebx, eax
    sar ebx, 1
    sub eax, ebx
dim_icon_adjusted:
    mov _gIconXAdjust, eax
    movsx ebx, WORD PTR [esi+2]
    mov _gIconYAdjust, ebx
    movzx ecx, WORD PTR [esi+4]
    mov _gIconWidth, ecx
    movzx edx, WORD PTR [esi+6]
    mov _gIconHeight, edx
    mov esi, DWORD PTR [esi+8]
    add esi, _gIconDataBase
    add DWORD PTR [ebp+10h], eax
    jns dim_icon_x_nonnegative
    mov DWORD PTR [ebp+10h], 0
dim_icon_x_nonnegative:
    add DWORD PTR [ebp+14h], ebx
    jns dim_icon_y_nonnegative
    mov DWORD PTR [ebp+14h], 0
dim_icon_y_nonnegative:
    mov edi, DWORD PTR [ebp+0ch]
    mov eax, DWORD PTR [ebp+10h]
    add eax, ecx
    cmp ax, WORD PTR [edi+10h]
    jg dim_icon_done
    mov eax, DWORD PTR [ebp+14h]
    add eax, edx
    cmp ax, WORD PTR [edi+12h]
    jg dim_icon_done
    cld
    movzx eax, WORD PTR [edi+10h]
    mov ebx, eax
    mov ecx, DWORD PTR [ebp+14h]
    mul ecx
    mov edx, DWORD PTR [ebp+10h]
    add eax, edx
    mov edi, DWORD PTR [edi+14h]
    add edi, eax
    mov edx, edi
    xor eax, eax
    xor ecx, ecx
    mov ebx, OFFSET _gDimPalette
dim_icon_next_run:
    lodsb
    or al, al
    js dim_icon_skip_run
    je dim_icon_next_row
    movzx ecx, al
dim_icon_pixel:
    mov al, BYTE PTR [edi]
    xlat
    stosb
    loop dim_icon_pixel
    jmp dim_icon_next_run
dim_icon_next_row:
    add edx, 280h
    mov edi, edx
    jmp dim_icon_next_run
dim_icon_skip_run:
    and al, 7fh
    je dim_icon_done
    add edi, eax
    jmp dim_icon_next_run
dim_icon_done:
    pop edi
    pop esi
    pop ebp
    ret
?DimIconToBitmap@@YAXPAVicon@@PAVbitmap@@HHHH@Z ENDP

EVEN
; Dim the destination through a horizontally mirrored icon frame (row step
; 640, like DimIconToBitmap).
;
;   void FlipDimIconToBitmap(icon* ic, bitmap* bmp, int x, int y, int frame, int offsetMode) {
;       /* frame setup, with x -= xAdjust; checks as FlipMonoIconToBitmap */
;       for (;;) {
;           unsigned char b = *src++;
;           if (b & 0x80) { if (!(b & 0x7f)) return; out -= b & 0x7f; }
;           else if (b == 0) out = rowStart += 640;
;           else for (; b; --b, --out) *out = gDimPalette[*out];
;       }
;   }
?FlipDimIconToBitmap@@YAXPAVicon@@PAVbitmap@@HHHH@Z PROC NEAR
    push ebp
    mov ebp, esp
    push esi
    push edi
    mov eax, DWORD PTR [ebp+18h]
    mov ebx, 0ch
    mul ebx
    mov esi, DWORD PTR [ebp+8]
    mov esi, DWORD PTR [esi+10h]
    mov _gIconDataBase, esi
    add esi, eax
    movsx eax, WORD PTR [esi]
    test DWORD PTR [ebp+1ch], 0ffffffffh
    je flip_dim_adjusted
    mov ebx, eax
    sar eax, 1
    sub ebx, eax
    sar ebx, 1
    sub eax, ebx
flip_dim_adjusted:
    mov _gIconXAdjust, eax
    movsx ebx, WORD PTR [esi+2]
    mov _gIconYAdjust, ebx
    movzx ecx, WORD PTR [esi+4]
    mov _gIconWidth, ecx
    movzx edx, WORD PTR [esi+6]
    mov _gIconHeight, edx
    mov esi, DWORD PTR [esi+8]
    add esi, _gIconDataBase
    sub DWORD PTR [ebp+10h], eax
    jns flip_dim_x_nonnegative
    mov DWORD PTR [ebp+10h], 0
flip_dim_x_nonnegative:
    add DWORD PTR [ebp+14h], ebx
    jns flip_dim_y_nonnegative
    mov DWORD PTR [ebp+14h], 0
flip_dim_y_nonnegative:
    mov edi, DWORD PTR [ebp+0ch]
    mov eax, DWORD PTR [ebp+10h]
    sub eax, ecx
    js flip_dim_done
    mov eax, DWORD PTR [ebp+14h]
    add eax, edx
    cmp ax, WORD PTR [edi+12h]
    jg flip_dim_done
    cld
    movzx eax, WORD PTR [edi+10h]
    mov ebx, eax
    mov ecx, DWORD PTR [ebp+14h]
    mul ecx
    mov edx, DWORD PTR [ebp+10h]
    add eax, edx
    mov edi, DWORD PTR [edi+14h]
    add edi, eax
    mov edx, edi
    xor eax, eax
    xor ecx, ecx
    mov ebx, OFFSET _gDimPalette
flip_dim_next_run:
    cld
    lodsb
    or al, al
    js flip_dim_skip_run
    je flip_dim_next_row
    mov cl, al
flip_dim_pixel:
    mov al, BYTE PTR [edi]
    xlat
    std
    stosb
    loop flip_dim_pixel
    jmp flip_dim_next_run
flip_dim_next_row:
    add edx, 280h
    mov edi, edx
    jmp flip_dim_next_run
flip_dim_skip_run:
    and al, 7fh
    je flip_dim_done
    sub edi, eax
    jmp flip_dim_next_run
flip_dim_done:
    pop edi
    pop esi
    pop ebp
    ret
?FlipDimIconToBitmap@@YAXPAVicon@@PAVbitmap@@HHHH@Z ENDP

END
