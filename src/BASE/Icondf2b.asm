; Dim the destination through a horizontally mirrored icon frame.

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
EXTERN _gDimPalette:BYTE

.code

?FlipDimIconToBitmap@@YAXPAVicon@@PAVbitmap@@HHHHHHHHH@Z PROC NEAR
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
?FlipDimIconToBitmap@@YAXPAVicon@@PAVbitmap@@HHHHHHHHH@Z ENDP

END
