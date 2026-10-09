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
_gIconUnused DWORD 0

.code

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
