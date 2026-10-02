; Draw one unscaled icon frame mirrored horizontally, clipped to the bitmap.
; Buka 2.1 iconf2bc.cpp supplies the identity; HoMM1 retail is hand-written.

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
EXTERN _gClipRowsLeft:DWORD
EXTERN _gClipLeftSkip:DWORD
EXTERN _gClipVisibleWidth:DWORD
EXTERN _gClipRowSkip:DWORD
EXTERN _gClipColumn:DWORD

.code

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
