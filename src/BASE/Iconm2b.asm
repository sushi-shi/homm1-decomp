; Draw one unscaled icon frame using a single color.

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

?MonoIconToBitmap@@YAXPAVicon@@PAVbitmap@@HHHHHHHHH@Z PROC NEAR
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
?MonoIconToBitmap@@YAXPAVicon@@PAVbitmap@@HHHHHHHHH@Z ENDP

END
