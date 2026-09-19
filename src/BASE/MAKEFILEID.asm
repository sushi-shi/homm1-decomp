; HoMM1 resource-name hash used by resourceManager::MakeId.

.386
.model flat
option casemap:none
option prologue:none
option epilogue:none

.code

?MAKEFILEID@@YAKPAD@Z PROC
    push ebp
    mov ebp, esp
    push esi
    mov esi, DWORD PTR [ebp+8]
    sub ebx, ebx
    sub eax, eax
hash_next:
    mov bl, BYTE PTR [esi]
    or bl, bl
    jz hash_done
    and bl, 07fh
    cmp bl, 060h
    jb hash_folded
    sub bl, 020h
hash_folded:
    xchg al, ah
    rol ax, 1
    sub ax, bx
    inc esi
    jmp hash_next
hash_done:
    pop esi
    pop ebp
    ret
?MAKEFILEID@@YAKPAD@Z ENDP

END
