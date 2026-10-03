; HoMM1 retail tile renderer.  HoMM2 retained the same MASM unit, but changed
; the bitmap/tileset layouts and unrolled the forward path.

.386
.model flat, C
option casemap:none
option prologue:none
option epilogue:none

TILE_INDEX_MASK              EQU 00fffh
TILE_FLIP_VERTICAL           EQU 04000h
TILE_FLIP_HORIZONTAL         EQU 08000h

.data
_gTileScratch DWORD 0
_gTileRows    WORD 0

.code

; extern "C" void __cdecl TileToBitmap(tileset* tiles, unsigned int tile,
;                                      bitmap* dest, int x, int y)   (BASE/TILE.h)
; tile = index | TILE_FLIP_VERTICAL | TILE_FLIP_HORIZONTAL. Tiles are stored
; consecutively, width * height bytes each. The bound check lets
; index == m_tileCount through. The flipped-vertical and both-flips paths
; take the row count from m_tileWidth (tiles are square); the copies move
; whole DWORDs or groups of 8 bytes (widths are multiples of 8). The global
; scratch words only hold the original tile word and the row counter.
;
;   void TileToBitmap(tileset* tiles, unsigned int tile, bitmap* dest, int x, int y) {
;       unsigned char* out = (unsigned char*)dest->m_pixels + y * dest->m_width + x;
;       gTileScratch = tile;
;       unsigned int index = tile & TILE_INDEX_MASK;
;       if ((int)(tiles->m_tileCount - index) < 0)
;           return;
;       int w = tiles->m_tileWidth, h = tiles->m_tileHeight;
;       int skip = dest->m_width - w;                    // to the next row's start
;       unsigned char* in = (unsigned char*)tiles->m_data + w * h * index;
;       if (gTileScratch & TILE_FLIP_HORIZONTAL) {
;           if (gTileScratch & TILE_FLIP_VERTICAL) {    // path_hv: rotate 180
;               in += w * w - 1;
;               for (gTileRows = w; gTileRows; --gTileRows, out += skip)
;                   for (int i = 0; i < w; ++i)
;                       *out++ = *in--;
;           } else {                                     // path_h: mirror rows
;               out += w - 1;
;               for (gTileRows = h; gTileRows; --gTileRows, out += skip + 2 * w)
;                   for (int i = 0; i < w; ++i)
;                       *out-- = *in++;
;           }
;       } else if (gTileScratch & TILE_FLIP_VERTICAL) { // path_v: rows bottom-up
;           in += (w - 1) * w;
;           for (int row = w; row; --row, out += skip, in -= 2 * w) {
;               memcpy(out, in, w); out += w; in += w;
;           }
;       } else {                                         // fwd: straight copy
;           for (int row = h; row; --row, out += skip) {
;               memcpy(out, in, w); out += w; in += w;
;           }
;       }
;   }
TileToBitmap PROC C
    push ebp
    mov ebp, esp
    push esi
    push edi
    mov edi, DWORD PTR [ebp+16]
    movzx ebx, WORD PTR [edi+010h]
    mov eax, DWORD PTR [ebp+24]
    mul ebx
    add eax, DWORD PTR [ebp+20]
    mov edi, DWORD PTR [edi+014h]
    add edi, eax
    mov eax, DWORD PTR [ebp+12]
    mov _gTileScratch, eax
    and eax, TILE_INDEX_MASK
    mov DWORD PTR [ebp+12], eax
    mov esi, DWORD PTR [ebp+8]
    movzx ecx, WORD PTR [esi+00eh]
    sub ecx, DWORD PTR [ebp+12]
    js epi
    movzx ecx, WORD PTR [esi+010h]
    sub ebx, ecx
    mov eax, ecx
    movzx edx, WORD PTR [esi+012h]
    mul edx
    mov edx, DWORD PTR [ebp+12]
    mul edx
    mov dx, WORD PTR [esi+012h]
    mov esi, DWORD PTR [esi+014h]
    add esi, eax
    mov eax, _gTileScratch
    and eax, TILE_FLIP_HORIZONTAL
    jne path_h
    mov eax, _gTileScratch
    and eax, TILE_FLIP_VERTICAL
    jne path_v
    mov eax, ecx
    shr eax, 2
; fwd: no flip - h rows of w / 4 DWORDs.
fwd:
    mov ecx, eax
    rep movsd
    add edi, ebx
    dec dx
    jne fwd
    jmp epi
; path_v: start at the tile's last row and step back two rows after each copy.
path_v:
    mov eax, ecx
    dec eax
    mul ecx
    add esi, eax
    mov eax, ecx
    mov dx, cx
v_loop:
    mov ecx, eax
    shr ecx, 2
    rep movsd
    add edi, ebx
    sub esi, eax
    sub esi, eax
    dec dx
    jne v_loop
epi:
    pop edi
    pop esi
    pop ebp
    ret
; path_h: horizontal flip only - write each row right to left (8 bytes per
; iteration); EBX becomes pitch + w to land on the next row's last byte.
path_h:
    mov eax, _gTileScratch
    and eax, TILE_FLIP_VERTICAL
    jne path_hv
    add edi, ecx
    dec edi
    mov _gTileRows, dx
    mov edx, ecx
    add ebx, edx
    add ebx, edx
    shr edx, 3
h_outer:
    mov ecx, edx
h_inner:
    REPT 8
    lodsb
    mov BYTE PTR [edi], al
    dec edi
    ENDM
    loop h_inner
    add edi, ebx
    dec WORD PTR _gTileRows
    jne h_outer
    jmp epi
; path_hv: both flips - read the tile backwards (STD) from its last byte.
path_hv:
    mov eax, ecx
    mul ecx
    dec eax
    add esi, eax
    std
    mov edx, ecx
    shr edx, 3
    mov _gTileRows, cx
hv_outer:
    mov ecx, edx
hv_inner:
    REPT 8
    lodsb
    mov BYTE PTR [edi], al
    inc edi
    ENDM
    loop hv_inner
    add edi, ebx
    dec WORD PTR _gTileRows
    jne hv_outer
    cld
    jmp epi
TileToBitmap ENDP

END
