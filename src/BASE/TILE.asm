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
fwd:
    mov ecx, eax
    rep movsd
    add edi, ebx
    dec dx
    jne fwd
    jmp epi
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
