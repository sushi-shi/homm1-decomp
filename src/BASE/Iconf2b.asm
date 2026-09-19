; Draw one unscaled icon frame, mirrored horizontally.

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

.code

?FlipIconToBitmap@@YAXPAVicon@@PAVbitmap@@HHHHHHHHH@Z PROC NEAR
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
?FlipIconToBitmap@@YAXPAVicon@@PAVbitmap@@HHHHHHHHH@Z ENDP

END
