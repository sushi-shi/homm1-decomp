; Draw one unscaled icon frame into a bitmap.

.386
.model flat
option casemap:none
option prologue:none
option epilogue:none

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

.code

?IconToBitmap@@YAXPAVicon@@PAVbitmap@@HHHHHHHHH@Z PROC NEAR
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
?IconToBitmap@@YAXPAVicon@@PAVbitmap@@HHHHHHHHH@Z ENDP

END
